import sys
import os

try:
    # When run as an extra_script by PlatformIO, SCons symbols are available
    from SCons.Script import COMMAND_LINE_TARGETS
except Exception:
    # Not running under SCons (manual run). Exit without doing anything.
    print("bump_on_upload: not running under SCons; skipping bump")
    sys.exit(0)

targets = [str(t) for t in COMMAND_LINE_TARGETS]
if 'upload' in targets:
    # Run bump_build.py from project root
    script = os.path.join(os.path.dirname(__file__), 'bump_build.py')
    print(f"bump_on_upload: upload detected in targets {targets}, running {script}")
    try:
        # Execute the bump script in the project root context
        cwd = os.getcwd()
        rc = os.system(f'"{sys.executable}" "{script}"')
        if rc != 0:
            print(f"bump_on_upload: bump script exited with code {rc}")
    except Exception as e:
        print(f"bump_on_upload: failed to run bump script: {e}")
else:
    print(f"bump_on_upload: upload not in targets ({targets}); skipping bump")
