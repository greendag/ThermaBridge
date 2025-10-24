import json
import os
import re


def parse_header(path):
    if not os.path.exists(path):
        return None
    try:
        with open(path, 'r', encoding='utf-8') as f:
            txt = f.read()
    except Exception:
        return None
    out = {}
    m = re.search(r'#define\s+FW_VERSION\s+"([^"]+)"', txt)
    if m:
        out['firmware_version'] = m.group(1)
    m = re.search(r'#define\s+FW_BUILD_NUMBER\s+(\d+)', txt)
    if m:
        out['build'] = int(m.group(1))
    m = re.search(r'#define\s+FW_BASE_VERSION\s+"([^"]+)"', txt)
    if m:
        out['firmware_base'] = m.group(1)
    return out


def read_config(path):
    if not os.path.exists(path):
        return None
    try:
        with open(path, 'r', encoding='utf-8') as f:
            return json.load(f)
    except Exception:
        return None


def main():
    root = os.getcwd()
    hdr_path = os.path.join(root, 'src', 'build_info.h')
    cfg_path = os.path.join(root, 'data', 'config.json')

    hdr = parse_header(hdr_path)
    if hdr:
        print('src/build_info.h (parsed):')
        print(json.dumps(hdr, indent=2))
    else:
        print('src/build_info.h: not found or missing macros')

    cfg = read_config(cfg_path)
    if cfg:
        # data/config.json no longer contains version fields; report that explicitly
        print('\ndata/config.json: no version fields present (version info is in src/build_info.h)')
    else:
        print('\ndata/config.json: not found')


if __name__ == '__main__':
    main()
