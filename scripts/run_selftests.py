#!/usr/bin/env python3
"""Run the native Gloom PC self-tests from a CMake build tree."""

from __future__ import annotations

import argparse
import os
import platform
import subprocess
import sys
import tempfile
from pathlib import Path


SELFTESTS = [
    "--input-selftest",
    "--combat-selftest",
    "--player-death-selftest",
    "--sfx-selftest",
    "--pickup-selftest",
    "--hud-selftest",
    "--menu-selftest",
    "--completion-selftest",
    "--texture-source-selftest",
    "--renderer-selftest",
    "--autosave-selftest",
    "--wall-selftest",
    "--switch-selftest",
    "--teleport-selftest",
    "--level-transition-selftest",
]


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


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", default="build", type=Path)
    parser.add_argument("--config", default=None)
    args = parser.parse_args()

    exe = find_executable(args.build_dir.resolve(), args.config)
    env = os.environ.copy()
    env.setdefault("SDL_AUDIODRIVER", "dummy")
    env.setdefault("SDL_VIDEODRIVER", "dummy")

    print(f"Running self-tests with {exe}", flush=True)
    for test_arg in SELFTESTS:
        print(f"::group::{test_arg}", flush=True)
        if test_arg == "--autosave-selftest":
            with tempfile.TemporaryDirectory(prefix="gloom-autosave-selftest-") as temp_dir:
                result = subprocess.run([str(exe), test_arg], cwd=temp_dir, env=env, check=False)
        else:
            result = subprocess.run([str(exe), test_arg], cwd=exe.parent, env=env, check=False)
        print("::endgroup::", flush=True)
        if result.returncode != 0:
            return result.returncode

    return 0


if __name__ == "__main__":
    sys.exit(main())
