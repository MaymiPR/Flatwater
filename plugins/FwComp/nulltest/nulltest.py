"""Null tests of Flatwater Comp: the null scripts in order, the numbers NULLTESTS.md publish.

    python -I nulltest.py [the arguments of co_ref.py]

co_ref.py gets the arguments, then null_presets.py run with none.
    python -I nulltest.py --report     NULLTESTS.md again from measurements/FwComp/nulltests.json, rendering nothing
    python -I nulltest.py --vst3       our built VST3 against our renderer (the check before a number is published)
"""
import os
import subprocess
import sys

sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'scripts', 'nulltest'))   # scripts/nulltest/
HERE = os.path.dirname(os.path.abspath(__file__))

STEPS = (('co_ref.py', sys.argv[1:] + ['--save']), ('null_presets.py', ['--save']))


def main():
    if '--report' in sys.argv:
        import report_lib as RL
        print('wrote', os.path.relpath(RL.write('FwComp')))
        return
    if '--vst3' in sys.argv:
        sys.exit(0 if vst3_check() is not False else 1)
    failed = 0
    for script, args in STEPS:
        print(f'== {script} {" ".join(args)}', flush=True)
        failed += subprocess.run([sys.executable, '-I', os.path.join(HERE, script), *args], cwd=HERE).returncode != 0
    sys.exit(1 if failed else 0)


def vst3_check():
    """--vst3: The built Flatwater Comp.vst3, hosted through tools/RefRender (a JUCE host, latency removed), against the offline DSP renderer"""
    sys.path.append(os.path.join(HERE, '..', '..', 'FwVocalComp', 'nulltest'))
    import numpy as np, refhost as R, co_ref as N, vc_ref as NV

    FS = 48000
    VST = N.VST3
    NOTES = ["1/64T", "1/64", "1/64D", "1/32T", "1/32", "1/32D", "1/16T", "1/16", "1/16D", "1/8T", "1/8", "1/8D", "1/4T", "1/4",
             "1/4D", "1/2T", "1/2", "1/2D", "1 Bar", "2 Bars"]


    def plug(x, thr, rat, att, rel, pu, mix, out, lim, sync=2, note=5, bpm=120.0, host=0.0):
        import subprocess, tempfile
        d = tempfile.mkdtemp(prefix="wcp_")
        fin, fout = os.path.join(d, "in.f32"), os.path.join(d, "out.f32")
        np.asarray(x, np.float32).T.tofile(fin)
        cmd = [R.REFRENDER, VST, "Flatwater Comp", fin, fout, "fs=%d" % FS, "block=512"] + (["bpm=%g" % host] if host else [])
        # choices by normalised value (name:=v), the rest by text
        cmd += ["Sync:=%g" % (sync / 2.0), "Release Note:=%g" % (note / 19.0), "Analog:=0", "Limiter:=%d" % (lim == 'On'), "BPM=%g" % bpm,
                "Threshold=%g" % thr, "Ratio=%g" % rat, "Attack=%g" % att, "Release=%g" % rel, "Punch=%g" % pu, "Mix=%g" % mix,
                "Output=%g" % out]
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
        if not os.path.exists(fout): raise RuntimeError(r.stdout + r.stderr)
        return np.fromfile(fout, np.float32).reshape(-1, 2).T

    S = N.signals(FS, quick=True)
    names = list(S)[:5]
    for name in names:
        x = S[name]; x2 = np.stack([x, 0.7 * x[::-1]]).astype(np.float32)
        for st in N.SETTINGS[:3] + N.SETTINGS[6:8]:
            y = plug(x2, *st)
            m = N.ours(x2, FS, *st)
            print('%-26s thr %5g ratio %4g lim %-3s  plug-in vs offline %7.1f dB' % (name, st[0], st[1], st[7], NV.residual_db(y, m, int(0.1 * FS))), flush=True)
    # tempo sync: BPM mode at 90 BPM, Host mode at 140 BPM and with no host tempo (120)
    x = S[names[0]]; x2 = np.stack([x, x]).astype(np.float32)
    beats = [1 / 24, 1 / 16, 3 / 32, 1 / 12, 1 / 8, 3 / 16, 1 / 6, 1 / 4, 3 / 8, 1 / 3, 1 / 2, 3 / 4, 2 / 3, 1, 1.5, 4 / 3, 2, 3, 4, 8]
    def synced(note, bpm):
        while True:
            ms = beats[note] * 60000 / bpm
            if note <= 2 or ms <= (3960 if note == 19 else 3400): return ms
            note = 18 if note == 19 else 17 if note == 18 else note - (2 if note % 3 == 1 else 1)
    st = (-30.0, 4.0, 1.0, 100.0, 0.0, 100.0, -6.0, 'Off')
    for sync, note, bpm, host in ((0, 7, 90.0, 0.0), (0, 19, 120.0, 0.0), (1, 13, 120.0, 140.0), (1, 10, 120.0, 0.0), (0, 18, 30.0, 0.0)):
        y = plug(x2, *st[:3], 100.0, *st[4:], sync=sync, note=note, bpm=bpm, host=host)
        ms = synced(note, (bpm if sync == 0 else (host or 120.0)))
        m = N.ours(x2, FS, *st[:3], ms, *st[4:])
        print('sync %s %-6s bpm %g host %g -> release %.1f ms: plug-in vs offline %7.1f dB' % (['BPM', 'Host'][sync], NOTES[note], bpm, host, ms,
              NV.residual_db(y, m, int(0.1 * FS))), flush=True)


if __name__ == '__main__':
    main()
