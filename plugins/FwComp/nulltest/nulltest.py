"""Null tests of Flatwater Comp: the null scripts in order, the numbers NULLTESTS.md publish.

    python -I nulltest.py [the arguments of co_ref.py]

co_ref.py gets the arguments, then null_presets.py and null_sidechain.py run with none.
    python -I nulltest.py --report     NULLTESTS.md again from measurements/FwComp/nulltests.json, rendering nothing
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

STEPS = (('co_ref.py', sys.argv[1:] + ['--save']), ('null_presets.py', ['--save']), ('null_sidechain.py', ['--save']))


def main():
    if '--report' in sys.argv:
        import report_lib as RL
        print('wrote', os.path.relpath(RL.write('FwComp')))
        return
    if '--vst3' in sys.argv:
        sys.exit(vst3_check())
    failed = 0
    for script, args in STEPS:
        print(f'== {script} {" ".join(args)}', flush=True)
        failed += subprocess.run([sys.executable, '-I', os.path.join(HERE, script), *args], cwd=HERE).returncode != 0
    sys.exit(1 if failed else 0)


NOTES = ["1/64T", "1/64", "1/64D", "1/32T", "1/32", "1/32D", "1/16T", "1/16", "1/16D", "1/8T", "1/8", "1/8D", "1/4T", "1/4",
         "1/4D", "1/2T", "1/2", "1/2D", "1 Bar", "2 Bars"]
BEATS = [1 / 24, 1 / 16, 3 / 32, 1 / 12, 1 / 8, 3 / 16, 1 / 6, 1 / 4, 3 / 8, 1 / 3, 1 / 2, 3 / 4, 2 / 3, 1, 1.5, 4 / 3, 2, 3, 4, 8]


def synced_ms(note, bpm):
    """the release of a note value at a tempo, as the plug-in limits it (the next shorter note above the longest release)"""
    while True:
        ms = BEATS[note] * 60000 / bpm
        if note <= 2 or ms <= (3960 if note == 19 else 3400): return ms
        note = 18 if note == 19 else 17 if note == 18 else note - (2 if note % 3 == 1 else 1)


def vst3_check():
    """--vst3: the built VST3 against the renderer, the Analog stage off: five signals at five settings, the
    release synced to a tempo (BPM mode, Host mode with and without a host tempo), then a key on the side-chain bus
    (a stereo key, a key on one channel, a silent key); the exit code"""
    import numpy as np, refhost as R, co_ref as N

    def host(thr, rat, att, rel, pu, mix, out, lim, sync=2, note=5, bpm=120.0):
        # choices by normalised value ('Name:'), the rest by text
        return {'Sync:': sync / 2.0, 'Release Note:': note / 19.0, 'Limiter:': int(lim == 'On'), 'BPM': bpm, 'Threshold': thr,
                'Ratio': rat, 'Attack': att, 'Release': rel, 'Punch': pu, 'Mix': mix, 'Output': out}

    def cases(fs):
        S = N.signals(fs, quick=True)
        names = list(S)[:5]
        out = []
        for name in names:
            x2 = np.stack([S[name], 0.7 * S[name][::-1]]).astype(np.float32)
            for st in N.SETTINGS[:3] + N.SETTINGS[6:8]:
                out.append(('%s, thr %g ratio %g out %g lim %s' % (name, st[0], st[1], st[6], st[7]), x2, host(*st), (),
                            lambda x, fs, st=st: N.ours(x, fs, *st)))
        x2 = np.stack([S[names[0]], S[names[0]]]).astype(np.float32)
        st = (-30.0, 4.0, 1.0, 100.0, 0.0, 100.0, -6.0, 'Off')
        for sync, note, bpm, tempo in ((0, 7, 90.0, 0.0), (0, 19, 120.0, 0.0), (1, 13, 120.0, 140.0), (1, 10, 120.0, 0.0), (0, 18, 30.0, 0.0)):
            ms = synced_ms(note, bpm if sync == 0 else (tempo or 120.0))
            out.append(('sync %s %s, BPM %g, host tempo %g: release %.1f ms' % (['BPM', 'Host'][sync], NOTES[note], bpm, tempo, ms), x2,
                        host(*st, sync=sync, note=note, bpm=bpm), ('bpm=%g' % tempo,) if tempo else (),
                        lambda x, fs, ms=ms: N.ours(x, fs, *st[:3], ms, *st[4:])))
        # a key on the side-chain bus: RefRender enables the bus only when it feeds it
        loud, low = S[names[0]], 0.3 * S[names[1]][::-1]
        zero = np.zeros_like(loud)
        for label, key in (('a stereo key', np.stack([loud, low])), ('a key on the left channel alone', np.stack([loud, zero])),
                           ('a silent key', np.stack([zero, zero]))):
            for ks in (N.SETTINGS[1], N.SETTINGS[7]):          # (st stays the sync cases' setting: their renders read it)
                key = key.astype(np.float32)
                out.append(('%s, thr %g ratio %g out %g lim %s' % (label, ks[0], ks[1], ks[6], ks[7]), x2, host(*ks), (),
                            lambda x, fs, ks=ks, key=key: N.ours(x, fs, *ks, key=key), key))
        return out
    return R.vst3_main(N, sys.argv[1:], cases)


if __name__ == '__main__':
    main()
