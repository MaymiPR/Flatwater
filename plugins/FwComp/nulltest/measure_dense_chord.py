"""Measurement: Flatwater Comp's weak case, a sustained chord at a heavy ratio and a short Release (MEASUREMENTS.md,
Null-test results). The chord is the pad of null_sidechain.py (220, 277.2 and 329.6 Hz, peak 0.75) at Threshold -12,
Ratio 50, Attack 1, Release 30, Punch 10; the reference renders through tools/RefRender. Prints three tables;
nothing is saved.

    python -I measure_dense_chord.py

  1. The residual by the chord's level, with the learned correction on and off, and the gain both sides apply
     (lowest, mean and highest over 2 to 5 s, read where the input is above 0.05).
  2. The residual with one control moved from the weak setting.
  3. The residual with the release memory's floor (the tuning constant memoryFloorMs) at other values.
"""
import os, sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'FwVocalComp', 'nulltest'))   # plugins/FwVocalComp/nulltest/
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'scripts', 'nulltest'))   # scripts/nulltest/
import subprocess
import numpy as np
import vc_ref as NV
import co_ref as N
import null_sidechain as NS

FS = 48000
WEAK = (-12.0, 50.0, 1.0, 30.0, 10.0, 100.0, 0.0, 'Off')


def chord(secs=6.0):
    t = np.arange(int(secs * FS)) / FS
    return (0.25 * (np.sin(2 * np.pi * 220.0 * t) + np.sin(2 * np.pi * 277.2 * t) + np.sin(2 * np.pi * 329.6 * t))).astype(np.float32)


def ours(x, st, learned=1, tune=()):
    thr, rat, att, rel, pu, mix, out, lim = st
    fi, fo = os.path.join(N.TMP, 'dc_in%d.f32' % os.getpid()), os.path.join(N.TMP, 'dc_out%d.f32' % os.getpid())
    np.stack([x, x]).T.astype(np.float32).tofile(fi)
    args = [N.EXE, fi, fo, '2', str(FS), 'threshold=%g' % thr, 'ratio=%g' % rat, 'attack=%g' % att, 'release=%g' % rel, 'punch=%g' % pu,
            'mix=%g' % mix, 'output=%g' % out, 'limiter=%d' % (lim == 'On'), 'analog=0', 'learned=%d' % learned] + ['tune:' + t for t in tune]
    subprocess.run(args, check=True, capture_output=True)
    return np.fromfile(fo, np.float32).reshape(-1, 2).T


def residual(a, b):
    return NV.residual_db(a[0], b[0], int(0.2 * FS))


def gain(y, x):
    """(lowest, mean, highest) of the gain in dB over 2 to 5 s, where the input is above 0.05"""
    s = slice(2 * FS, 5 * FS); m = np.abs(x[s]) > 0.05
    g = 20 * np.log10(np.abs(y[0][s][m]) / np.abs(x[s][m]) + 1e-12)
    return g.min(), g.mean(), g.max()


def main():
    x0 = chord()
    two = lambda x: np.stack([x, x]).astype(np.float32)
    print('1. By the chord\'s level: residual (dB), learned correction on / off; the gain (dB): lowest, mean, highest')
    refs = {}
    for db in (0, -6, -12, -18):
        x = x0 * 10 ** (db / 20)
        b = NS.theirs(two(x), FS, WEAK); refs[db] = b
        a, a0 = ours(x, WEAK), ours(x, WEAK, learned=0)
        print('   %+3d dB: %6.1f / %6.1f   the reference %6.2f %6.2f %6.2f   ours %6.2f %6.2f %6.2f'
              % (db, residual(a, b), residual(a0, b), *gain(b, x), *gain(a, x)), flush=True)
    print('2. One control moved (the chord at full level): residual (dB)')
    for label, st in (('Ratio 4', (-12.0, 4.0, 1.0, 30.0, 10.0, 100.0, 0.0, 'Off')), ('Punch 0', (-12.0, 50.0, 1.0, 30.0, 0.0, 100.0, 0.0, 'Off')),
                      ('Punch 1', (-12.0, 50.0, 1.0, 30.0, 1.0, 100.0, 0.0, 'Off')), ('Release 100', (-12.0, 50.0, 1.0, 100.0, 10.0, 100.0, 0.0, 'Off')),
                      ('Attack 7', (-12.0, 50.0, 7.0, 30.0, 10.0, 100.0, 0.0, 'Off')), ('Threshold -24', (-24.0, 50.0, 1.0, 30.0, 10.0, 100.0, 0.0, 'Off')),
                      ('Output -12', (-12.0, 50.0, 1.0, 30.0, 10.0, 100.0, -12.0, 'Off'))):
        print('   %-14s %6.1f' % (label, residual(ours(x0, st), NS.theirs(two(x0), FS, st))), flush=True)
    print('3. The release memory\'s floor (the chord at full level): residual (dB)')
    for ms in (16.7561366, 50.0, 100.0, 300.0, 1000.0):
        print('   memoryFloorMs %-9g %6.1f' % (ms, residual(ours(x0, WEAK, tune=('memoryFloorMs=%g' % ms,)), refs[0])), flush=True)


if __name__ == '__main__':
    main()
