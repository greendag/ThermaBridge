#!/usr/bin/env python3
"""Validate that src/build_info.h exists and contains the required version macros.

This script is used by CI to ensure `src/build_info.h` is present and contains
`FW_VERSION` and `FW_BUILD_NUMBER`. The canonical version information lives in the
header; we no longer rely on `data/config.json` for version fields.
"""
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(__file__))
CONFIG = os.path.join(ROOT, 'data', 'config.json')
OUT = os.path.join(ROOT, 'src', 'build_info.h')


def read_header():
    if not os.path.exists(OUT):
        print(f'Error: {OUT} not found')
        return None
    try:
        with open(OUT, 'r', encoding='utf-8') as f:
            return f.read()
    except Exception as e:
        print(f'Error reading {OUT}: {e}')
        return None


def main():
    hdr = read_header()
    if hdr is None:
        sys.exit(2)
    # basic checks
    if 'FW_VERSION' not in hdr or 'FW_BUILD_NUMBER' not in hdr:
        print('Error: src/build_info.h does not contain FW_VERSION or FW_BUILD_NUMBER')
        sys.exit(2)
    print('OK: src/build_info.h is present and contains FW_VERSION and FW_BUILD_NUMBER')


if __name__ == '__main__':
    main()
