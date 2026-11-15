"""Null tests of Flatwater Bass Lift: the null scripts in order, the numbers NULLTESTS.md publish.

    python -I nulltest.py [the arguments of bf_ref.py]

bf_ref.py gets the arguments.
    python -I nulltest.py --report     NULLTESTS.md again from measurements/FwBassLift/nulltests.json, rendering nothing
    python -I nulltest.py --vst3 [--fs rate] [--only text] [--threshold dB]
                                       our built VST3 against our renderer, without the reference: exit code 0
                                       equal (below -120 dB), 1 a case differs, 2 it could not run
"""
import os
import subprocess
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'scripts', 'nulltest'))   # scripts/nulltest/
HERE = os.path.dirname(os.path.abspath(__file__))

STEPS = (('bf_ref.py', sys.argv[1:] + ['--save']),)


def main():
    if '--report' in sys.argv:
        import report_lib as RL
        print('wrote', os.path.relpath(RL.write('FwBassLift')))
        return
    if '--vst3' in sys.argv:
        sys.exit(vst3_check())
    failed = 0
    for script, args in STEPS:
        print(f'== {script} {" ".join(args)}', flush=True)
        failed += subprocess.run([sys.executable, '-I', os.path.join(HERE, script), *args], cwd=HERE).returncode != 0
    sys.exit(1 if failed else 0)


def vst3_check():
    """--vst3: our built VST3 against our renderer at the defaults, on noise, a low tone and drums; the exit code"""
    import refhost as R, bf_ref as h
    return R.vst3_main(h, sys.argv[1:])


if __name__ == '__main__':
    main()
