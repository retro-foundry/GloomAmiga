#!/usr/bin/env python3
"""Compare one fixed software/OpenGL gameplay frame dump.

This is an opt-in visual parity helper. It intentionally does not run under the
default dummy-video selftest harness because SDL's dummy driver cannot prove
that the OpenGL/WebGL renderer initializes.
"""

from __future__ import annotations

import argparse
import os
import platform
import struct
import subprocess
import sys
import tempfile
from pathlib import Path


def find_executable(build_dir: Path, config: str | None) -> Path:
    exe_name = "gloom_pc.exe" if platform.system() == "Windows" else "gloom_pc"
    candidates: list[Path] = []

    if config:
        candidates.append(build_dir / config / exe_name)
    candidates.append(build_dir / exe_name)

    for candidate in candidates:
        if candidate.is_file():
            return candidate

    matches = sorted(build_dir.rglob(exe_name))
    if matches:
        return matches[0]

    raise FileNotFoundError(f"could not find {exe_name} under {build_dir}")


def read_bmp_rgb(path: Path) -> tuple[int, int, list[list[tuple[int, int, int]]]]:
    data = path.read_bytes()
    if data[:2] != b"BM":
        raise ValueError(f"{path} is not a BMP file")

    pixel_offset = struct.unpack_from("<I", data, 10)[0]
    width = struct.unpack_from("<i", data, 18)[0]
    height_raw = struct.unpack_from("<i", data, 22)[0]
    bpp = struct.unpack_from("<H", data, 28)[0]
    if width <= 0 or height_raw == 0 or bpp not in (24, 32):
        raise ValueError(f"{path} has unsupported BMP layout {width}x{height_raw}x{bpp}")

    height = abs(height_raw)
    top_down = height_raw < 0
    stride = ((width * bpp + 31) // 32) * 4
    step = bpp // 8
    rows: list[list[tuple[int, int, int]]] = []

    for y in range(height):
        source_y = y if top_down else height - 1 - y
        row = data[pixel_offset + source_y * stride : pixel_offset + source_y * stride + width * step]
        rows.append([tuple(row[x * step : x * step + 3]) for x in range(width)])

    return width, height, rows


def compare_frames(software_path: Path, opengl_path: Path) -> dict[str, float]:
    sw_width, sw_height, sw_pixels = read_bmp_rgb(software_path)
    gl_width, gl_height, gl_pixels = read_bmp_rgb(opengl_path)
    if (sw_width, sw_height) != (gl_width, gl_height):
        raise ValueError(
            f"frame dimensions differ: software={sw_width}x{sw_height} opengl={gl_width}x{gl_height}"
        )

    changed_pixels = 0
    total_abs = 0
    max_abs = 0
    over_50 = 0

    for y in range(sw_height):
        for x in range(sw_width):
            diff = sum(abs(int(sw_pixels[y][x][channel]) - int(gl_pixels[y][x][channel])) for channel in range(3))
            if diff != 0:
                changed_pixels += 1
                total_abs += diff
                max_abs = max(max_abs, diff)
                if diff > 50:
                    over_50 += 1

    total_pixels = sw_width * sw_height
    return {
        "width": float(sw_width),
        "height": float(sw_height),
        "total_pixels": float(total_pixels),
        "changed_pixels": float(changed_pixels),
        "changed_percent": (changed_pixels * 100.0) / total_pixels,
        "total_abs": float(total_abs),
        "mean_abs": total_abs / total_pixels,
        "mean_changed_abs": total_abs / changed_pixels if changed_pixels else 0.0,
        "max_abs": float(max_abs),
        "pixels_over_50": float(over_50),
    }


def run_frame_dump(exe: Path, renderer: str, output_path: Path, resolution: str, env: dict[str, str]) -> None:
    command = [
        str(exe),
        "--renderer",
        renderer,
        "--resolution",
        resolution,
        "--frame-dump",
        str(output_path),
    ]
    subprocess.run(command, cwd=exe.parent, env=env, check=True)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", default="build", type=Path)
    parser.add_argument("--config", default=None)
    parser.add_argument("--resolution", default="320x200")
    parser.add_argument("--keep-dumps", action="store_true")
    parser.add_argument("--max-changed-pixels", type=int, default=1200)
    parser.add_argument("--max-changed-percent", type=float, default=2.0)
    parser.add_argument("--max-total-abs", type=int, default=120000)
    parser.add_argument("--max-mean-abs", type=float, default=2.0)
    parser.add_argument("--max-pixels-over-50", type=int, default=900)
    args = parser.parse_args()

    exe = find_executable(args.build_dir.resolve(), args.config)
    env = os.environ.copy()
    env.setdefault("SDL_AUDIODRIVER", "dummy")
    env.pop("SDL_VIDEODRIVER", None)

    with tempfile.TemporaryDirectory(prefix="gloom-renderer-compare-") as temp_dir:
        temp_path = Path(temp_dir)
        software_path = temp_path / "software.bmp"
        opengl_path = temp_path / "opengl.bmp"

        run_frame_dump(exe, "software", software_path, args.resolution, env)
        run_frame_dump(exe, "opengl", opengl_path, args.resolution, env)
        metrics = compare_frames(software_path, opengl_path)

        print(
            "Renderer frame compare: "
            f"{int(metrics['width'])}x{int(metrics['height'])} "
            f"changed={int(metrics['changed_pixels'])}/{int(metrics['total_pixels'])} "
            f"({metrics['changed_percent']:.2f}%) "
            f"total_abs={int(metrics['total_abs'])} "
            f"mean_abs={metrics['mean_abs']:.3f} "
            f"mean_changed_abs={metrics['mean_changed_abs']:.3f} "
            f"max_abs={int(metrics['max_abs'])} "
            f"pixels_over_50={int(metrics['pixels_over_50'])}",
            flush=True,
        )

        failures = []
        if metrics["changed_pixels"] > args.max_changed_pixels:
            failures.append(f"changed pixels {int(metrics['changed_pixels'])} > {args.max_changed_pixels}")
        if metrics["changed_percent"] > args.max_changed_percent:
            failures.append(f"changed percent {metrics['changed_percent']:.2f} > {args.max_changed_percent:.2f}")
        if metrics["total_abs"] > args.max_total_abs:
            failures.append(f"total absolute diff {int(metrics['total_abs'])} > {args.max_total_abs}")
        if metrics["mean_abs"] > args.max_mean_abs:
            failures.append(f"mean absolute diff {metrics['mean_abs']:.3f} > {args.max_mean_abs:.3f}")
        if metrics["pixels_over_50"] > args.max_pixels_over_50:
            failures.append(f"pixels over 50 {int(metrics['pixels_over_50'])} > {args.max_pixels_over_50}")

        if args.keep_dumps:
            keep_dir = exe.parent / "renderer-compare"
            keep_dir.mkdir(exist_ok=True)
            software_keep = keep_dir / "software.bmp"
            opengl_keep = keep_dir / "opengl.bmp"
            software_keep.write_bytes(software_path.read_bytes())
            opengl_keep.write_bytes(opengl_path.read_bytes())
            print(f"Kept dumps: {software_keep} {opengl_keep}", flush=True)

        if failures:
            for failure in failures:
                print(f"Renderer frame compare failed: {failure}", file=sys.stderr)
            return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
