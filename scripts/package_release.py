#!/usr/bin/env python3
"""Create a user-facing release archive from a native CMake build output."""

from __future__ import annotations

import argparse
import os
import shutil
import stat
import tarfile
import zipfile
from pathlib import Path


PLATFORM_ARCHIVES = {
    "linux": "tar.gz",
    "macos": "zip",
    "windows": "zip",
}


def find_executable(build_dir: Path, platform_name: str, config: str | None) -> Path:
    exe_name = "gloom_pc.exe" if platform_name == "windows" else "gloom_pc"
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


def copy_file_if_exists(source: Path, destination: Path) -> None:
    if source.is_file():
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)


def copy_payload(repo_root: Path, runtime_dir: Path, stage_dir: Path, exe: Path, platform_name: str) -> None:
    stage_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(exe, stage_dir / exe.name)

    if platform_name != "windows":
        executable = stage_dir / exe.name
        executable.chmod(executable.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)

    copy_file_if_exists(runtime_dir / "README.TXT", stage_dir / "README.TXT")
    copy_file_if_exists(runtime_dir / "GLOOM.INI", stage_dir / "GLOOM.INI")

    if not (stage_dir / "README.TXT").is_file():
        copy_file_if_exists(repo_root / "package" / "README.TXT", stage_dir / "README.TXT")
    if not (stage_dir / "GLOOM.INI").is_file():
        copy_file_if_exists(repo_root / "package" / "GLOOM.INI", stage_dir / "GLOOM.INI")
    copy_file_if_exists(repo_root / "VERIFY_RELEASE.md", stage_dir / "VERIFY_RELEASE.md")

    source_assets = runtime_dir / "amiga"
    if not source_assets.is_dir():
        raise FileNotFoundError(f"runtime assets were not copied to {source_assets}")
    shutil.copytree(source_assets, stage_dir / "amiga", dirs_exist_ok=True)


def write_zip(stage_dir: Path, archive_path: Path) -> None:
    with zipfile.ZipFile(archive_path, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        for path in sorted(stage_dir.rglob("*")):
            archive.write(path, path.relative_to(stage_dir.parent))


def write_tar_gz(stage_dir: Path, archive_path: Path) -> None:
    with tarfile.open(archive_path, "w:gz") as archive:
        archive.add(stage_dir, arcname=stage_dir.name)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--platform", choices=sorted(PLATFORM_ARCHIVES), required=True)
    parser.add_argument("--build-dir", default="build", type=Path)
    parser.add_argument("--config", default=None)
    parser.add_argument("--version", required=True)
    parser.add_argument("--output-dir", default="dist", type=Path)
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parents[1]
    build_dir = args.build_dir.resolve()
    output_dir = args.output_dir.resolve()
    archive_kind = PLATFORM_ARCHIVES[args.platform]
    package_name = f"gloom-pc-{args.version}-{args.platform}-x64"
    stage_root = output_dir / "_staging"
    stage_dir = stage_root / package_name
    archive_path = output_dir / f"{package_name}.{archive_kind}"

    exe = find_executable(build_dir, args.platform, args.config)
    runtime_dir = exe.parent

    if stage_dir.exists():
        shutil.rmtree(stage_dir)
    archive_path.unlink(missing_ok=True)
    output_dir.mkdir(parents=True, exist_ok=True)

    copy_payload(repo_root, runtime_dir, stage_dir, exe, args.platform)

    if archive_kind == "zip":
        write_zip(stage_dir, archive_path)
    else:
        write_tar_gz(stage_dir, archive_path)

    shutil.rmtree(stage_root)
    print(archive_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
