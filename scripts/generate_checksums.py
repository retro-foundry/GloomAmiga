#!/usr/bin/env python3
"""Generate SHA256SUMS.txt for release archives."""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path


def is_release_archive(path: Path) -> bool:
    return path.is_file() and (path.name.endswith(".zip") or path.name.endswith(".tar.gz"))


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as file:
        for chunk in iter(lambda: file.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("asset_dir", type=Path)
    parser.add_argument("--output", default=None, type=Path)
    args = parser.parse_args()

    asset_dir = args.asset_dir.resolve()
    output = args.output or asset_dir / "SHA256SUMS.txt"
    archives = sorted(path for path in asset_dir.rglob("*") if is_release_archive(path))

    if not archives:
        raise FileNotFoundError(f"no release archives found under {asset_dir}")

    lines = []
    for archive in archives:
        relative = archive.relative_to(asset_dir).as_posix()
        lines.append(f"{sha256(archive)}  {relative}\n")

    output.write_text("".join(lines), encoding="utf-8")
    print(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
