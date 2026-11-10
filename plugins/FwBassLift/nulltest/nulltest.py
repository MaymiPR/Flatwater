"""Null tests of Flatwater Bass Lift: the null scripts in order, the numbers NULLTESTS.md publish.

    python -I nulltest.py [the arguments of bf_ref.py]

bf_ref.py gets the arguments.
    python -I nulltest.py --report     NULLTESTS.md again from measurements/FwBassLift/nulltests.json, rendering nothing
    python -I nulltest.py --vst3       our built VST3 against our renderer (the check before a number is published)
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
        sys.exit(0 if vst3_check() is not False else 1)
    failed = 0
    for script, args in STEPS:
        print(f'== {script} {" ".join(args)}', flush=True)
        failed += subprocess.run([sys.executable, '-I', os.path.join(HERE, script), *args], cwd=HERE).returncode != 0
    sys.exit(1 if failed else 0)


def vst3_check():
    """--vst3: our built VST3 against our renderer at the defaults, on noise, a low tone and drums"""
    import refhost as R, nulltest_lib as nl, bf_ref as h
    S = {'noise -18 dBFS': nl.noise(3.0, -18.0, seed=7), 'tone 100 Hz -12 dBFS': nl.tone(100.0, 3.0, -12.0),
         'drums': nl.drums(3.0, -6.0)}
    return R.vst3_equals_renderer(h, S)


if __name__ == '__main__':
    main()
