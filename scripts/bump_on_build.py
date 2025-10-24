import sys
import os
import time
import subprocess

LOG_PATH = os.path.join(os.getcwd(), '.bump.log')

def log(msg: str):
    ts = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime())
    line = f"[{ts}] {msg}\n"
    try:
        with open(LOG_PATH, 'a', encoding='utf-8') as f:
            f.write(line)
    except Exception:
        pass
    print(line, end='')

try:
    from SCons.Script import COMMAND_LINE_TARGETS
    targets = [str(t) for t in COMMAND_LINE_TARGETS]
except Exception:
    targets = []

# Always run the bump script when this post-build hook executes. This keeps bump
# behavior consistent whether the user runs a normal build or an upload (upload
# triggers build+upload in PlatformIO).

# Determine project root and bump script path
cwd = os.getcwd()
script = os.path.join(cwd, 'scripts', 'bump_build.py')
log(f"bump_on_build: cwd={cwd}")
log(f"bump_on_build: checking bump script at {script}")
if not os.path.exists(script):
    log(f"bump_on_build: script not found: {script} (skipping)")
    sys.exit(0)

log(f"bump_on_build: invoking bump script (python {script})")
try:
    # Run the bump script with cwd=project root and capture output
    proc = subprocess.run([sys.executable, script], cwd=cwd, capture_output=True, text=True)
    log(f"bump_on_build: returncode={proc.returncode}")
    if proc.stdout:
        log(f"bump_on_build: stdout:\n{proc.stdout}")
        # Try to extract generated FW_VERSION and build number for a short console line
        try:
            # Look for a generated line like: Generated ... with FW_VERSION=v1.0.17
            for line in proc.stdout.splitlines():
                if 'Generated' in line and 'FW_VERSION=' in line:
                    # extract the portion after FW_VERSION=
                    part = line.split('FW_VERSION=')[-1].strip()
                    # Print a short confirmation to stdout for visibility
                    print(f"Bumped firmware: {part}")
                    break
        except Exception:
            pass
    if proc.stderr:
        log(f"bump_on_build: stderr:\n{proc.stderr}")
    if proc.returncode != 0:
        log(f"bump_on_build: bump script failed with return code {proc.returncode}")
except Exception as e:
    log(f"bump_on_build: failed to run bump script: {e}")


