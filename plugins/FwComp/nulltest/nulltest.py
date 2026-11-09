"""Null tests of Flatwater Comp: the null scripts in order, the numbers NULLTESTS.md and NULLTESTS-presets.md publish.

    python -I nulltest.py [the arguments of null_fwcomp.py]

null_fwcomp.py gets the arguments, then null_presets.py run with none.
"""
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))

STEPS = (('null_fwcomp.py', sys.argv[1:]), ('null_presets.py', []))


def main():
    failed = 0
    for script, args in STEPS:
        print(f'== {script} {" ".join(args)}', flush=True)
        failed += subprocess.run([sys.executable, '-I', os.path.join(HERE, script), *args], cwd=HERE).returncode != 0
    sys.exit(1 if failed else 0)


if __name__ == '__main__':
    main()
