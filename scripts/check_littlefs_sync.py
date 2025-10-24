#!/usr/bin/env python3
"""
check_littlefs_sync.py

Compute a deterministic SHA256 of all files under the repository's `data/` folder
and compare to a recorded hash file `.littlefs_hash` in the repo root. Exit code
is 0 when the hashes match, non-zero when they differ.

Usage:
  python scripts/check_littlefs_sync.py        # prints status
  python scripts/check_littlefs_sync.py --update  # update .littlefs_hash to current state
  python scripts/check_littlefs_sync.py --upload  # run `platformio run -t uploadfs` when out of sync

This is a lightweight helper to avoid accidentally leaving LittleFS data out of the
device image. It only reads files in `data/` and ignores metadata like timestamps.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DATA_DIR = ROOT / "data"
HASH_FILE = ROOT / ".littlefs_hash"


def compute_hash(directory: Path) -> str:
    """Compute SHA256 of files under `directory` in a deterministic order."""
    h = hashlib.sha256()
    files = []
    for root, dirs, filenames in os.walk(directory):
        for fn in filenames:
            rel = Path(root, fn).relative_to(directory)
            files.append(rel.as_posix())
    files.sort()
    for rel_path in files:
        p = directory / rel_path
        # include the filename and file size to catch renames/empty changes deterministically
        h.update(rel_path.encode("utf-8"))
        h.update(b"\0")
        h.update(str(p.stat().st_size).encode("utf-8"))
        h.update(b"\0")
        with p.open("rb") as fh:
            while True:
                chunk = fh.read(8192)
                if not chunk:
                    break
                h.update(chunk)
        h.update(b"\0\0")
    return h.hexdigest()


def read_hash_file(path: Path) -> str | None:
    if not path.exists():
        return None
    try:
        text = path.read_text(encoding="utf-8").strip()
    except Exception:
        return None
    if not text:
        return None
    return text.splitlines()[0].strip()


def write_hash_file(path: Path, value: str) -> None:
    path.write_text(value + "\n", encoding="utf-8")


def run_uploadfs() -> int:
    # run platformio uploadfs from repo root
    cmd = [sys.executable, "-m", "platformio", "run", "-t", "uploadfs"]
    print("Running:", " ".join(cmd))
    return subprocess.call(cmd, cwd=str(ROOT))


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--update", action="store_true", help="Update .littlefs_hash with current state")
    p.add_argument("--upload", action="store_true", help="If out-of-sync, run platformio uploadfs")
    args = p.parse_args(argv)

    if not DATA_DIR.exists() or not DATA_DIR.is_dir():
        print(f"Data directory not found: {DATA_DIR}")
        return 2

    cur = compute_hash(DATA_DIR)
    on_disk = read_hash_file(HASH_FILE)

    print(f"Computed hash: {cur}")
    if on_disk:
        print(f"Recorded hash: {on_disk}")
    else:
        print("No recorded .littlefs_hash file found.")

    if args.update:
        write_hash_file(HASH_FILE, cur)
        print(f"Updated {HASH_FILE}")
        return 0

    if on_disk == cur:
        print("LittleFS data is in sync. No upload needed.")
        return 0

    print("LittleFS data appears OUT OF SYNC.")
    if args.upload:
        code = run_uploadfs()
        return code
    else:
        print("Run with --upload to automatically upload data/ to device using platformio uploadfs, or --update to accept current state.")
        return 3


if __name__ == "__main__":
    raise SystemExit(main())
