"""Flatwater Pitch Bend, Phase 3: a modulated Pitch, the reference against ours (MEASUREMENTS.md, Phase 3).
  timing [block] [sine|vowel] [sync]   when a stepped Pitch lands: M1 a sequence alternating 0 / +6 st (a vowel: +3.1),
                                       250 ms steps (synced: 1 beat at 120 bpm); the time each output's frequency (from
                                       zero crossings, a vowel's fundamental low-passed both ways) crosses the steps'
                                       midpoint after the step, at every place of the step in the host block
  latch                                after a +3 st step among 0 steps: each side's level against the input and the
                                       residual against the reference's pass-through, on a vowel
  small                                the level at small shifts (every knob at 0): M1 a sequence of held values at
                                       depth 4 (+-0.96 st) on a vowel
  silence [fs]                         the pass-through's delay before and after a run of zeros in a vowel (lengths
                                       about the reset's), and per burst of a vowel with 0.2 s gaps, both sides

    python -I measure_modpitch.py timing 512 vowel
"""
import sys, os, subprocess, tempfile
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import numpy as np
from scipy.signal import butter, sosfiltfilt
import vb as H
import editor_state as S
import measure_mods as MM
import measure_shifter as MS

FS = 48000


def ours(x, keys, tables):
    d = tempfile.mkdtemp(prefix='pbm_', dir=H.TMP)
    fi, fo = os.path.join(d, 'in.f32'), os.path.join(d, 'out.f32')
    x.T.astype(np.float32).tofile(fi)
    args = [H.RENDER, fi, fo, '2', str(FS), 'preroll=1'] + ['%s=%s' % (k, v) for k, v in keys.items()]
    for k, t in tables.items():
        f = os.path.join(d, 'table%d.f32' % k)
        np.asarray(t, '<f4').tofile(f)
        args.append('table=%d:%s' % (k, f))
    subprocess.run(args, check=True)
    return np.fromfile(fo, np.float32).reshape(-1, 2).T


def vowel(secs, f0=150.0):
    v = MS.vowel(secs, f0, -18.0)
    return np.tile(np.asarray(v[0] if isinstance(v, tuple) else v, np.float32), (2, 1))


def seq(values):
    """a 16-step sequence in the 512-value table layout (the LFO's 256 points, then the steps)"""
    return np.concatenate([np.zeros(256), values, np.zeros(256 - len(values))])


def both(x, depth, table, extra_ref=None, extra_ours=None, tag='modpitch', block=512):
    ch = {S.SEMITONES: 0, **S.slot('pitch', 0, S.M1, depth), S.M_SMOOTH: 1.0, S.M_TYPE: 1, S.M_STEPS: 16, **(extra_ref or {})}
    keys = {'slot': 'pitch:0:1:%g' % depth, 'm1Smooth': 1, 'fine': 1, 'm1Type': 1, 'm1Steps': 16, 'block': block, **(extra_ours or {})}
    a = S.render(x, FS, ch, tag=tag, shapes={0: table}, extra=('block=%d' % block,))
    return a, ours(x, keys, {0: table})


def timing(block=512, sig='sine', sync=False):
    f0, depth = (220.0, 25) if sig == 'sine' else (150.0, 13)
    x = (np.tile((0.25 * np.sin(2 * np.pi * 220 * np.arange(int(8.4 * FS)) / FS)).astype(np.float32), (2, 1))
         if sig == 'sine' else vowel(8.4))
    step, delay = (24000, 0.0001) if sync else (12000, 0.0081)
    rate_ref = {S.M_NOTE: 13} if sync else MM.FREE(4.0)
    rate_ours = {'m1RateSynced': 10, 'bpm': 120} if sync else {'m1Sync': 1, 'm1Rate': 4.0}
    a, b = both(x, depth, seq([0.0, 1.0] * 8), rate_ref, rate_ours, 'mpt%d%s%s' % (block, sig, 'S' if sync else ''), block)
    lp = butter(8, 215, fs=FS, output='sos')

    def curve(y):
        if sig != 'sine': y = sosfiltfilt(lp, y)
        z = np.where((y[:-1] <= 0) & (y[1:] > 0))[0]
        t = z + (-y[z]) / (y[z + 1] - y[z])
        g = np.arange(int(0.2 * FS), int(t[-1]) - 100, 4)
        return g, 12 * np.log2(np.interp(g, 0.5 * (t[1:] + t[:-1]), FS / np.diff(t)) / f0)

    g, ca = curve(a[0]); _, cb = curve(b[0])
    n = min(len(ca), len(cb)); g, ca, cb = g[:n], ca[:n], cb[:n]
    ra, rb = [], []
    for k in range(1, x.shape[1] // step):
        t0 = delay * FS + k * step
        m = (g > t0 - 0.02 * FS) & (g < t0 + 0.03 * FS)
        if m.sum() < 50: continue
        lo, hi = np.median(ca[m][:40]), np.median(ca[m][-40:])
        s = np.sign(hi - lo); mid = 0.5 * (lo + hi)
        ra.append((g[m][np.argmax(s * (ca[m] - mid) > 0)] - t0) / FS * 1000)
        rb.append((g[m][np.argmax(s * (cb[m] - mid) > 0)] - t0) / FS * 1000)
    print('%s, block %d%s: the reference lands %.2f ms after the step (sd %.2f), ours %.2f (sd %.2f), over %d steps' % (
        sig, block, ', synced' if sync else '', np.mean(ra), np.std(ra), np.mean(rb), np.std(rb), len(ra)))


def latch():
    x = vowel(6.0)
    plain = S.render(x, FS, {S.SEMITONES: 0}, tag='mpl_plain')
    a, b = both(x, 25, seq([0.0] * 3 + [0.5] + [0.0] * 12), MM.FREE(2.0), {'m1Sync': 1, 'm1Rate': 2.0}, 'mpl_step')
    s0 = int(1.2 * FS)                                            # (the +3 step is at 0.5 .. 1.0 s)
    res = lambda y: 10 * np.log10(np.sum((y[:, s0:] - plain[:, s0:]) ** 2) / np.sum(plain[:, s0:] ** 2))
    print('after the step, at 0: the reference %+.2f dB against the input (residual against its pass-through %.1f dB), '
          'ours %+.2f dB' % (H.rms_db(a[:, s0:]) - H.rms_db(x[:, s0:]), res(a), H.rms_db(b[:, s0:]) - H.rms_db(x[:, s0:])))


def small():
    vals = [0.0, 1.0, 0.0, 0.02, 0.05, 0.1, 0.25, 0.5, 1.0, -0.02, -0.05, -0.1, -0.25, -0.5, -1.0, 0.0]
    x = vowel(9.0)
    a, b = both(x, 4, seq(vals), MM.FREE(2.0), {'m1Sync': 1, 'm1Rate': 2.0}, 'mps')
    for k, u in enumerate(vals):                                  # (the cycle runs from 1 s before the input)
        s0, s1 = int((k * 0.5 - 1 + 0.0081 + 0.15) * FS), int((k * 0.5 - 1 + 0.0081 + 0.48) * FS)
        if s0 < 0: continue
        print('%+6.3f st: the reference %+5.2f dB, ours %+5.2f dB' % (u * 0.96, H.rms_db(a[:, s0:s1]) - H.rms_db(x[:, s0:s1]),
                                                                    H.rms_db(b[:, s0:s1]) - H.rms_db(x[:, s0:s1])))


def frac_delay_of(y, x, a, b, top=600):
    """the delay of y against x over a .. b, to a fraction of a sample (cross-correlation peak, parabola)"""
    best = max(range(1, top), key=lambda d: np.dot(y[a + d:b + d], x[a:b]))
    c = [np.dot(y[a + d:b + d], x[a:b]) for d in (best - 1, best, best + 1)]
    return best + 0.5 * (c[0] - c[2]) / (c[0] - 2 * c[1] + c[2])


def silence(fs=48000):
    v = np.asarray(MS.vowel(4, 150, fs=fs)[0], np.float32)
    runs = {48000: ((1029, 0), (1030, 0), (1029, 3)), 44100: ((975, 0), (980, 0)), 96000: ((2100, 0), (2400, 0))}[fs]
    for n, off in runs:
        x = v.copy(); x[2 * fs + off:2 * fs + off + n] = 0
        x2 = np.tile(x, (2, 1))
        a = H.render_ref(x2, fs, {'Pitch:': 0.5})
        d = tempfile.mkdtemp(prefix='pbm_', dir=H.TMP); fi, fo = os.path.join(d, 'in.f32'), os.path.join(d, 'out.f32')
        x2.T.tofile(fi); subprocess.run([H.RENDER, fi, fo, '2', str(fs), 'preroll=1'], check=True)
        b = np.fromfile(fo, np.float32).reshape(-1, 2).T
        print('%d Hz, %d zeros from %d mod 8 of the input: the reference %.2f -> %.2f samples, ours %.2f -> %.2f' % (
            fs, n, off, frac_delay_of(a[0], x, fs, fs + 6000), frac_delay_of(a[0], x, 3 * fs, 3 * fs + 6000),
            frac_delay_of(b[0], x, fs, fs + 6000), frac_delay_of(b[0], x, 3 * fs, 3 * fs + 6000)))


def vibrato(f0, settings):
    v, _ = MS.vowel(4.0, f0, vib=100.0)
    x = np.tile(v.astype(np.float32), (2, 1))

    def track(y, fc):
        t, f, ap = MS.yin(y, hop=48, fmin=fc / 1.5, fmax=fc * 1.8, win=int(2.0 * FS / fc))
        return t * FS, 1200 * np.log2(f / f0), ap
    tx, cx, _ = track(v, f0)
    truth = np.interp(np.arange(len(v)), tx, cx)

    def fit(y, P):
        t, c, ap = track(y.mean(0), f0 * 2 ** (P / 12))
        c = c - 100 * P
        ok = np.isfinite(c) & (ap < 0.2) & (t > 0.3 * FS) & (t < 3.8 * FS)
        best = None
        for lag in np.arange(-5, 30.01, 0.25):
            tr = np.interp(t - lag * 48, np.arange(len(truth)), truth)
            e = np.sqrt(np.mean((c[ok] - np.median(c[ok] - tr[ok]) - tr[ok]) ** 2))
            if best is None or e < best[1]: best = (lag, e)
        tr = np.interp(t - best[0] * 48, np.arange(len(truth)), truth)
        return best[0], best[1], np.std(c[ok]) / np.std(tr[ok])
    for pf in settings:
        P, F = [float(q) for q in pf.split(':')]
        a = S.render(x, FS, {S.SEMITONES: 0, S.PITCH: P * 100, S.FORMANT: F * 100}, tag='mpv%g_%g_%g' % (f0, P, F))
        b = ours(x, {'fine': 1, 'pitch': P, 'formant': F}, {})
        print('%g Hz, P %+g F %+g: the reference %.2f ms (rms %.1f c, swing %.2f), ours %.2f ms (rms %.1f c, swing %.2f)'
              % ((f0, P, F) + fit(a, P) + fit(b, P)), flush=True)


if __name__ == '__main__':
    S.base()
    cmd = sys.argv[1] if len(sys.argv) > 1 else 'timing'
    if cmd == 'timing':
        timing(int(sys.argv[2]) if len(sys.argv) > 2 else 512, sys.argv[3] if len(sys.argv) > 3 else 'sine',
               len(sys.argv) > 4 and sys.argv[4] == 'sync')
    elif cmd == 'latch': latch()
    elif cmd == 'small': small()
    elif cmd == 'silence': silence(int(sys.argv[2]) if len(sys.argv) > 2 else 48000)
    elif cmd == 'vibrato': vibrato(float(sys.argv[2]), sys.argv[3:])
    else: raise SystemExit(__doc__)
