#!/usr/bin/env python3
"""Compare deterministic software/OpenGL gameplay and screen frame dumps.

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


SCENARIOS: dict[str, dict[str, object]] = {
    "base": {"mode": "gameplay", "args": []},
    "red": {"mode": "gameplay", "args": ["--force-red-palette"]},
    "pixelate": {"mode": "gameplay", "args": ["--force-pixelate", "6"]},
    "blood": {"mode": "gameplay", "args": ["--force-blood"]},
    "two_player": {"mode": "gameplay", "args": ["--two-player"]},
    "combat": {"mode": "gameplay", "args": ["--combat"]},
    "animated_walls": {"mode": "gameplay", "args": ["--frame-dump-ticks", "90"]},
    "door": {
        "mode": "gameplay",
        "args": ["--force-first-door", "--frame-dump-ticks", "32", "--frame-dump-look-at-first-door"],
    },
    "transparent_wall": {
        "mode": "gameplay",
        "map": "amiga/maps/com1_2",
        "args": ["--frame-dump-look-at-transparent-wall"],
    },
    "level_transition": {"mode": "gameplay", "args": ["--force-level-transition"]},
    "menu": {"mode": "screen", "screen": "start_menu"},
    "combat_menu": {"mode": "screen", "screen": "combat_menu"},
    "pause": {"mode": "screen", "screen": "pause"},
    "script_intro": {"mode": "screen", "screen": "script_intro"},
    "completion": {"mode": "screen", "screen": "completion"},
}

DEFAULT_LIMITS: dict[str, dict[str, float]] = {
    "base": {
        "max_changed_pixels": 1200,
        "max_changed_percent": 2.0,
        "max_total_abs": 120000,
        "max_mean_abs": 2.0,
        "max_pixels_over_50": 900,
    },
    "blood": {
        "max_changed_pixels": 1200,
        "max_changed_percent": 2.0,
        "max_total_abs": 120000,
        "max_mean_abs": 2.0,
        "max_pixels_over_50": 900,
    },
    "pixelate": {
        "max_changed_pixels": 1400,
        "max_changed_percent": 2.5,
        "max_total_abs": 120000,
        "max_mean_abs": 2.0,
        "max_pixels_over_50": 1000,
    },
    "red": {
        "max_changed_pixels": 52000,
        "max_changed_percent": 80.0,
        "max_total_abs": 120000,
        "max_mean_abs": 2.0,
        "max_pixels_over_50": 900,
    },
    "two_player": {
        "max_changed_pixels": 2400,
        "max_changed_percent": 4.0,
        "max_total_abs": 260000,
        "max_mean_abs": 4.0,
        "max_pixels_over_50": 2200,
    },
    "combat": {
        "max_changed_pixels": 2400,
        "max_changed_percent": 4.0,
        "max_total_abs": 260000,
        "max_mean_abs": 4.0,
        "max_pixels_over_50": 2200,
    },
    "animated_walls": {
        "max_changed_pixels": 1200,
        "max_changed_percent": 2.0,
        "max_total_abs": 120000,
        "max_mean_abs": 2.0,
        "max_pixels_over_50": 900,
    },
    "door": {
        "max_changed_pixels": 1200,
        "max_changed_percent": 2.0,
        "max_total_abs": 120000,
        "max_mean_abs": 2.0,
        "max_pixels_over_50": 900,
    },
    "transparent_wall": {
        "max_changed_pixels": 1200,
        "max_changed_percent": 2.0,
        "max_total_abs": 120000,
        "max_mean_abs": 2.0,
        "max_pixels_over_50": 900,
    },
    "level_transition": {
        "max_changed_pixels": 1200,
        "max_changed_percent": 2.0,
        "max_total_abs": 120000,
        "max_mean_abs": 2.0,
        "max_pixels_over_50": 900,
    },
    "menu": {
        "max_changed_pixels": 3000,
        "max_changed_percent": 4.0,
        "max_total_abs": 180000,
        "max_mean_abs": 3.0,
        "max_pixels_over_50": 1500,
    },
    "combat_menu": {
        "max_changed_pixels": 2000,
        "max_changed_percent": 4.0,
        "max_total_abs": 180000,
        "max_mean_abs": 3.0,
        "max_pixels_over_50": 1500,
    },
    "pause": {
        "max_changed_pixels": 1000,
        "max_changed_percent": 2.0,
        "max_total_abs": 90000,
        "max_mean_abs": 1.5,
        "max_pixels_over_50": 750,
    },
    "script_intro": {
        "max_changed_pixels": 2000,
        "max_changed_percent": 4.0,
        "max_total_abs": 180000,
        "max_mean_abs": 3.0,
        "max_pixels_over_50": 1500,
    },
    "completion": {
        "max_changed_pixels": 2000,
        "max_changed_percent": 4.0,
        "max_total_abs": 180000,
        "max_mean_abs": 3.0,
        "max_pixels_over_50": 1500,
    },
}


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


def run_gameplay_frame_dump(
    exe: Path,
    renderer: str,
    output_path: Path,
    resolution: str,
    env: dict[str, str],
    hd_art: Path | None,
    extra_args: list[str],
    map_path: str | None,
) -> None:
    command = [
        str(exe),
        "--renderer",
        renderer,
        "--resolution",
        resolution,
        "--skip-menu",
        "--frame-dump",
        str(output_path),
    ]
    command.extend(extra_args)
    if hd_art is not None:
        command.extend(["--hd-art", str(hd_art)])
    if map_path is not None:
        command.append(map_path)
    subprocess.run(command, cwd=exe.parent, env=env, check=True)


def run_screen_frame_dump(
    exe: Path,
    renderer: str,
    screen: str,
    output_path: Path,
    resolution: str,
    env: dict[str, str],
    hd_art: Path | None,
) -> None:
    command = [
        str(exe),
        "--screen-frame-dump",
        screen,
        str(output_path),
        "--resolution",
        resolution,
        "--renderer",
        renderer,
    ]
    if hd_art is not None:
        command.extend(["--hd-art", str(hd_art)])
    subprocess.run(command, cwd=exe.parent, env=env, check=True)


def run_scenario_dump(
    exe: Path,
    renderer: str,
    scenario: dict[str, object],
    output_path: Path,
    resolution: str,
    env: dict[str, str],
    hd_art: Path | None,
) -> None:
    mode = scenario["mode"]
    if mode == "gameplay":
        run_gameplay_frame_dump(
            exe,
            renderer,
            output_path,
            resolution,
            env,
            hd_art,
            list(scenario.get("args", [])),
            str(scenario["map"]) if "map" in scenario else None,
        )
    elif mode == "screen":
        run_screen_frame_dump(
            exe,
            renderer,
            str(scenario["screen"]),
            output_path,
            resolution,
            env,
            hd_art,
        )
    else:
        raise ValueError(f"unknown scenario mode {mode!r}")


def metric_limit(scenario_name: str, args: argparse.Namespace, key: str) -> float:
    override = getattr(args, key)
    if override is not None:
        return float(override)
    return DEFAULT_LIMITS[scenario_name][key]


def check_metrics(scenario_name: str, metrics: dict[str, float], args: argparse.Namespace) -> list[str]:
    failures = []
    max_changed_pixels = metric_limit(scenario_name, args, "max_changed_pixels")
    max_changed_percent = metric_limit(scenario_name, args, "max_changed_percent")
    max_total_abs = metric_limit(scenario_name, args, "max_total_abs")
    max_mean_abs = metric_limit(scenario_name, args, "max_mean_abs")
    max_pixels_over_50 = metric_limit(scenario_name, args, "max_pixels_over_50")

    if metrics["changed_pixels"] > max_changed_pixels:
        failures.append(f"changed pixels {int(metrics['changed_pixels'])} > {int(max_changed_pixels)}")
    if metrics["changed_percent"] > max_changed_percent:
        failures.append(f"changed percent {metrics['changed_percent']:.2f} > {max_changed_percent:.2f}")
    if metrics["total_abs"] > max_total_abs:
        failures.append(f"total absolute diff {int(metrics['total_abs'])} > {int(max_total_abs)}")
    if metrics["mean_abs"] > max_mean_abs:
        failures.append(f"mean absolute diff {metrics['mean_abs']:.3f} > {max_mean_abs:.3f}")
    if metrics["pixels_over_50"] > max_pixels_over_50:
        failures.append(f"pixels over 50 {int(metrics['pixels_over_50'])} > {int(max_pixels_over_50)}")
    return failures


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", default="build", type=Path)
    parser.add_argument("--config", default=None)
    parser.add_argument("--resolution", default="320x200")
    parser.add_argument("--hd-art", default=None, type=Path)
    parser.add_argument(
        "--scenario",
        action="append",
        choices=sorted(SCENARIOS),
        help="Scenario to compare; may be repeated. Defaults to all renderer scenarios.",
    )
    parser.add_argument("--list-scenarios", action="store_true")
    parser.add_argument("--keep-dumps", action="store_true")
    parser.add_argument("--max-changed-pixels", type=int, default=None)
    parser.add_argument("--max-changed-percent", type=float, default=None)
    parser.add_argument("--max-total-abs", type=int, default=None)
    parser.add_argument("--max-mean-abs", type=float, default=None)
    parser.add_argument("--max-pixels-over-50", type=int, default=None)
    args = parser.parse_args()

    if args.list_scenarios:
        for name in sorted(SCENARIOS):
            print(name)
        return 0

    exe = find_executable(args.build_dir.resolve(), args.config)
    env = os.environ.copy()
    env.setdefault("SDL_AUDIODRIVER", "dummy")
    env.pop("SDL_VIDEODRIVER", None)

    scenario_names = args.scenario if args.scenario is not None else sorted(SCENARIOS)
    all_failures: list[str] = []

    with tempfile.TemporaryDirectory(prefix="gloom-renderer-compare-") as temp_dir:
        temp_path = Path(temp_dir)
        hd_art = args.hd_art.resolve() if args.hd_art is not None else None

        for scenario_name in scenario_names:
            scenario = SCENARIOS[scenario_name]
            software_path = temp_path / f"{scenario_name}_software.bmp"
            opengl_path = temp_path / f"{scenario_name}_opengl.bmp"

            run_scenario_dump(exe, "software", scenario, software_path, args.resolution, env, hd_art)
            run_scenario_dump(exe, "opengl", scenario, opengl_path, args.resolution, env, hd_art)
            metrics = compare_frames(software_path, opengl_path)

            print(
                "Renderer frame compare "
                f"[{scenario_name}]: "
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

            if args.keep_dumps:
                keep_dir = exe.parent / "renderer-compare"
                keep_dir.mkdir(exist_ok=True)
                software_keep = keep_dir / f"{scenario_name}_software.bmp"
                opengl_keep = keep_dir / f"{scenario_name}_opengl.bmp"
                software_keep.write_bytes(software_path.read_bytes())
                opengl_keep.write_bytes(opengl_path.read_bytes())
                print(f"Kept dumps: {software_keep} {opengl_keep}", flush=True)

            all_failures.extend(f"{scenario_name}: {failure}" for failure in check_metrics(scenario_name, metrics, args))

        if all_failures:
            for failure in all_failures:
                print(f"Renderer frame compare failed: {failure}", file=sys.stderr)
            return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
