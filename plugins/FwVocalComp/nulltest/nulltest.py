"""Null tests of Flatwater Vocal Comp: the null scripts in order, the numbers NULLTESTS.md publish.

    python -I nulltest.py [the arguments of vc_ref.py]

vc_ref.py gets the arguments and --extra --save (the published table holds the other rates and the stereo cases), then
null_grid.py runs with --save.
    python -I nulltest.py --report     NULLTESTS.md again from measurements/FwVocalComp/nulltests.json, rendering nothing
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

STEPS = (('vc_ref.py', sys.argv[1:] + ['--extra', '--save']), ('null_grid.py', ['--save']))


def main():
    if '--report' in sys.argv:
        import report_lib as RL
        print('wrote', os.path.relpath(RL.write('FwVocalComp')))
        return
    if '--vst3' in sys.argv:
        sys.exit(vst3_check())
    failed = 0
    for script, args in STEPS:
        print(f'== {script} {" ".join(args)}', flush=True)
        failed += subprocess.run([sys.executable, '-I', os.path.join(HERE, script), *args], cwd=HERE).returncode != 0
    sys.exit(1 if failed else 0)


def vst3_check():
    """--vst3: the built VST3 against the renderer on the harness's quick signals at two settings; the exit code"""
    import refhost as R, vc_ref as N

    def cases(fs):
        out = []
        for name, x in N.signals(fs, quick=True).items():
            for comp, gate, gain in ((-24, None, 0), (-12, -30, -3)):
                out.append(('%s, Comp %d, Gate %s, Gain %d' % (name, comp, 'off' if gate is None else gate, gain), x,
                            {'Comp': comp, 'Gate': -80.0 if gate is None else gate, 'Gain': gain}, (),
                            lambda x, fs, s=(comp, gate, gain): N.ours(x, fs, *s)))
        return out
    return R.vst3_main(N, sys.argv[1:], cases)


if __name__ == '__main__':
    main()
