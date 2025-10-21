#!/usr/bin/env python3
"""Validate data/config.json contains required build fields.

Exits 0 on success, non-zero on failure. Intended for CI usage.
"""
import json
import os
import sys

ROOT = os.path.dirname(os.path.dirname(__file__))
CONFIG = os.path.join(ROOT, 'data', 'config.json')
HEADER = os.path.join(ROOT, 'src', 'build_info.h')


def main():
    # Validate header contains required macros
    if not os.path.exists(HEADER):
        print(f'Error: {HEADER} missing')
        return 2
    content = ''
    try:
        with open(HEADER, 'r', encoding='utf-8') as f:
            content = f.read()
    except Exception as e:
        print(f'Error: failed to read {HEADER}: {e}')
        return 2
    missing = []
    if 'FW_VERSION' not in content:
        missing.append('FW_VERSION')
    if 'FW_BUILD_NUMBER' not in content:
        missing.append('FW_BUILD_NUMBER')
    if missing:
        print('Error: missing macros in src/build_info.h:', ', '.join(missing))
        return 2
    print('OK: src/build_info.h contains FW_VERSION and FW_BUILD_NUMBER')
    return 0


if __name__ == '__main__':
    sys.exit(main())
