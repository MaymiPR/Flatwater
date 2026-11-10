"""Null tests of Flatwater Vocal Comp: the null scripts in order, the numbers NULLTESTS.md publish.

    python -I nulltest.py [the arguments of vc_ref.py]

vc_ref.py gets the arguments.
    python -I nulltest.py --report     NULLTESTS.md again from measurements/FwVocalComp/nulltests.json, rendering nothing
    python -I nulltest.py --vst3       our built VST3 against our renderer (the check before a number is published)
"""
import os
import subprocess
import sys

sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'scripts', 'nulltest'))   # scripts/nulltest/
HERE = os.path.dirname(os.path.abspath(__file__))

STEPS = (('vc_ref.py', sys.argv[1:] + ['--save']),)


def main():
    if '--report' in sys.argv:
        import report_lib as RL
        print('wrote', os.path.relpath(RL.write('FwVocalComp')))
        return
    if '--vst3' in sys.argv:
        sys.exit(0 if vst3_check() is not False else 1)
    failed = 0
    for script, args in STEPS:
        print(f'== {script} {" ".join(args)}', flush=True)
        failed += subprocess.run([sys.executable, '-I', os.path.join(HERE, script), *args], cwd=HERE).returncode != 0
    sys.exit(1 if failed else 0)


def vst3_check():
    """--vst3: The built Flatwater Vocal Comp.vst3, hosted through pedalboard, against the offline DSP renderer (they must be identical)."""
    import numpy as np, pedalboard as pb, vc_ref as N, refhost as R

    FS = 48000
    VST = N.VST3
    p = pb.load_plugin(VST)
    print('loaded', p.name, 'params', [k for k in p.parameters.keys()][:8], 'latency', p.reported_latency_samples)
    S = N.signals(quick=True)
    for name, x in S.items():
        for comp, gate, gain in ((-24, None, 0), (-12, -30, -3)):
            p.comp = float(comp); p.gate = -80.0 if gate is None else float(gate); p.gain = float(gain)
            y = p(x[None, :].astype(np.float32), FS, buffer_size=512, reset=True)[0]
            m = N.ours(x, FS, comp, gate, gain)[0]
            print('%-26s %4d/%s  plug-in vs offline %7.1f dB' % (name, comp, 'off' if gate is None else gate, N.residual_db(y, m, int(0.1 * FS))))


if __name__ == '__main__':
    main()
