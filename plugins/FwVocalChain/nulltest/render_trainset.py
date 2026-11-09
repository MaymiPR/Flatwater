"""Training renders for Flatwater Vocal Chain's learned Compress correction (the learned-block procedure, docs/IMPROVEMENTS.md):
the reference with every colour clear but Compress, at each colour (Push, Spank, Wall) and fader step -10 .. +10.

Each example is a 4 s mono signal (the same on both sides): vocal phrases from the corpus' training parts (0 .. 23) at
levels that wander, vocals over a noise bed, band-limited noise with and without an envelope (crest factors a sine
does not have: the fifth pass found the remaining error depends on it), tone mixtures, level steps and synthetic
voices, 48 per step (at first 16 at -10 .. 0: the net then cost light settings 1 .. 2.5 dB on unseen vocals). The
held-out set is the corpus' held-out parts (24 .. 31, their first 8 s at the de-esser fit's peaks, -3 .. +6 dBFS);
the validation set the same parts' next 8 s at peaks -12, -6 and 0 (the levels where the first nets cost). Writes
reference/fwvocalchain/train/c<c>_<v>.npz, held_c<c>_<v>.npz and val_c<c>_<v>.npz (git-ignored): x (examples, samples)
and y, the reference's output (one side; the two are equal). Parts 32 .. 35 stay unused (null_compress_grid.py).

Two more sets check the fader between steps: frac_c<c>_<v>.npz, the same kinds of examples at positions between the
steps (12 per position), and moves_c<c>.npz, 8 s vocals (training parts) while the fader holds, ramps and jumps,
rendered in 512-sample blocks with the fader set before each (as a host automates it); f holds its position per block.

A Wall round (2026-11-01) adds wallx_c3_<v>.npz: 48 more vocals (with and without a noise bed, peaks -12 .. +6 dBFS)
at the steps where Wall's nets regressed on light vocals (-7, -6, 0) and beside them (WALLX).

    python -I render_trainset.py [train|held|val|frac|moves|wallx|all]
"""
import os
import sys
import numpy as np
from multiprocessing import Pool
from scipy.signal import butter, sosfilt

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.append(HERE)
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'scripts', 'nulltest'))   # scripts/nulltest/
OUT = os.path.join(HERE, '..', '..', '..', 'reference', 'fwvocalchain', 'train')
FS = 48000
SECS = 4.0
CLEAR = dict(bass_color=0, treble_color=0, compress_color=0, reverb_color=0, delay_color=0, pitch_color=0)
HELD = ((24, 0), (25, 3), (26, -3), (27, 6), (28, 0), (29, 3), (30, 0), (31, 6))
FRAC = (-7.5, -2.5, -0.5, 0.5, 1.5, 2.5, 3.5, 4.5, 5.5, 6.5, 7.5, 8.5, 9.5, 9.8)
BLOCK = 512
WALLX = (-8, -7, -6, -5, -1, 0, 1)
KINDS = ('vocal', 'vocal', 'vocal', 'vocal', 'vocalbed', 'noise', 'noise', 'tones', 'steps', 'voice')

_parts = {}


def db(v): return 10 ** (v / 20)


def part(k):
    import vocal_corpus as V
    if k not in _parts: _parts[k] = V.part(k)[0].astype(np.float64)
    return _parts[k]


def wander(rng, n, depth):
    """a slow level wander (dB, linear between knots every 0.3 .. 1.5 s)"""
    knots = [0]
    while knots[-1] < n: knots.append(knots[-1] + int(rng.uniform(0.3, 1.5) * FS))
    return db(np.interp(np.arange(n), knots, rng.uniform(-depth, depth, len(knots))))


def envelope(rng, n):
    """syllable-like bursts with attacks, decays and gaps"""
    e = np.zeros(n); pos = int(rng.uniform(0, 0.1) * FS)
    while pos < n:
        on = int(rng.uniform(0.05, 0.6) * FS); gap = int(rng.uniform(0, 0.3) * FS)
        t = np.arange(on) / FS
        att, rel = rng.uniform(0.002, 0.06), rng.uniform(0.02, 0.3)
        seg = np.maximum(np.minimum(1, t / att) * np.minimum(1, (on / FS - t) / rel), 0) * db(rng.uniform(-18, 0))
        m = min(on, n - pos); e[pos:pos + m] = seg[:m]; pos += on + gap
    return e


def bandnoise(rng, n):
    lo = 10 ** rng.uniform(np.log10(60), np.log10(1500)); hi = min(10 ** rng.uniform(np.log10(2000), np.log10(16000)), 20000)
    z = sosfilt(butter(2, [lo, hi], 'bandpass', fs=FS, output='sos'), rng.standard_normal(n))
    return z / np.sqrt(np.mean(z ** 2))


def vocal(rng, n):
    x = part(int(rng.integers(0, 24)))
    a = int(rng.integers(0, len(x) - n)); s = x[a:a + n].copy()
    for _ in range(4):                                        # (a window with something in it)
        if np.abs(s).max() > 0.05: break
        a = int(rng.integers(0, len(x) - n)); s = x[a:a + n].copy()
    return s / max(np.abs(s).max(), 1e-9) * wander(rng, n, 3)


def voice(rng, n):
    t = np.arange(n) / FS
    f0 = rng.uniform(80, 400) * (1 + 0.02 * np.sin(2 * np.pi * rng.uniform(4, 7) * t))
    ph = 2 * np.pi * np.cumsum(f0) / FS
    tilt = rng.uniform(0.7, 1.5)
    x = sum(np.sin(k * ph) / k ** tilt for k in range(1, 40) if k * f0.max() < 16000)
    if rng.random() < 0.6:
        x = sosfilt(butter(2, sorted([rng.uniform(300, 900), rng.uniform(1200, 3500)]), 'bandpass', fs=FS, output='sos'), x) * 3
    return x * envelope(rng, n)


def make(rng):
    """one example, its peak set to -24 .. +6 dBFS"""
    n = int(SECS * FS); t = np.arange(n) / FS
    kind = str(rng.choice(KINDS))
    if kind == 'vocal':
        x = vocal(rng, n)
    elif kind == 'vocalbed':
        x = vocal(rng, n); x = x + bandnoise(rng, n) * np.sqrt(np.mean(x ** 2)) * db(rng.uniform(-24, -6))
    elif kind == 'noise':
        x = bandnoise(rng, n)
        r = rng.random()
        if r < 0.4: x = x * envelope(rng, n)
        elif r < 0.7: x = x * (1 + rng.uniform(0.2, 0.9) * np.sin(2 * np.pi * 10 ** rng.uniform(-0.5, 1.3) * t))
        x = x * wander(rng, n, 6)
    elif kind == 'tones':
        x = sum(db(rng.uniform(-12, 0)) * np.sin(2 * np.pi * 10 ** rng.uniform(np.log10(60), np.log10(5000)) * t + rng.uniform(0, 6.3))
                for _ in range(int(rng.integers(1, 4))))
        x = x * wander(rng, n, 9)
    elif kind == 'steps':
        carrier = voice(rng, n) if rng.random() < 0.3 else (bandnoise(rng, n) if rng.random() < 0.5
                                                            else np.sin(2 * np.pi * 10 ** rng.uniform(np.log10(80), np.log10(4000)) * t))
        if np.abs(carrier).max() > 0: carrier = carrier / np.abs(carrier).max()
        e = np.zeros(n); pos = 0
        while pos < n:
            m = int(rng.uniform(0.03, 0.8) * FS); e[pos:pos + m] = db(rng.uniform(-40, 0)) * (rng.random() > 0.1); pos += m
        x = carrier * e
    else:
        x = voice(rng, n)
    pk = np.abs(x).max()
    if pk <= 0: return np.zeros(n, np.float32)
    return (x / pk * db(rng.uniform(-24, 6))).astype(np.float32)


def examples(c, v):
    rng = np.random.default_rng(7919 * c + 101 * (v + 10) + 13)
    return [make(rng) for _ in range(48)]


def wallx_examples(v):
    rng = np.random.default_rng(104729 + 101 * (v + 10))
    out = []
    n = int(SECS * FS)
    while len(out) < 48:
        x = vocal(rng, n)
        if rng.random() < 0.3: x = x + bandnoise(rng, n) * np.sqrt(np.mean(x ** 2)) * db(rng.uniform(-24, -6))
        pk = np.abs(x).max()
        if pk > 0: out.append((x / pk * db(rng.uniform(-12, 6))).astype(np.float32))
    return out


def held_examples():
    out = []
    for k, pk in HELD:
        x = part(k)[:FS * 8]; out.append((x / np.abs(x).max() * db(pk)).astype(np.float32))
    return out


def val_examples():
    out = []
    for k in range(24, 32):
        x = part(k)[FS * 8:FS * 16]
        for pk in (-12, -6, 0): out.append((x / np.abs(x).max() * db(pk)).astype(np.float32))
    return out


def frac_examples(c, v):
    rng = np.random.default_rng(7919 * c + int(round(100 * (v + 10))) + 50000)
    return [make(rng) for _ in range(12)]


def moves(c):
    """eight 8 s vocals (training parts, peaks -12 .. +6 dBFS) and the fader's position per block: holds of 0.3 .. 2 s,
    ramps of 0.1 .. 1.5 s and jumps between positions -4 .. +10"""
    rng = np.random.default_rng(31 * c + 77)
    n = 8 * FS; nb = n // BLOCK
    X, F = [], []
    for _ in range(8):
        x = part(int(rng.integers(0, 24)))
        a = int(rng.integers(0, len(x) - n)); s = x[a:a + n]
        X.append((s / max(np.abs(s).max(), 1e-9) * db(rng.uniform(-12, 6))).astype(np.float32))
        f = np.zeros(nb); pos = 0; cur = rng.uniform(-4, 10)
        while pos < nb:
            hold = int(rng.uniform(0.3, 2) * FS / BLOCK); f[pos:pos + hold] = cur; pos += hold
            new = rng.uniform(-4, 10)
            ramp = 0 if rng.random() < 0.3 else int(rng.uniform(0.1, 1.5) * FS / BLOCK)
            f[pos:pos + ramp] = np.linspace(cur, new, ramp + 2)[1:-1][:max(0, min(ramp, nb - pos))]; pos += ramp; cur = new
        F.append(np.round(f, 3))
    return X, F


def render_moving(x, f, c):
    """the reference with the Compress fader set before each block (f: its position per block)"""
    import cv as C
    import refstate as W
    p = C.plugin()
    vals = list(W.get_values(C._base)); vals[C.NOISE] = 0
    W.apply_values(p, vals, C._base); p.reset()
    hp = C.host(**dict(CLEAR, compress_color=c, compress=float(f[0])))
    for k in C.FADERS + C.COLOURS + ('balance',):
        p.parameters[k].raw_value = hp.get(k, 0.5 if k in C.FADERS or k == 'balance' else 1.0 / 3.0)
    pre = 94 * BLOCK                                        # (about a second of silence first, whole blocks)
    xin = np.concatenate([np.zeros((2, pre), np.float32), np.stack([x, x]), np.zeros((2, 2 * BLOCK), np.float32)], 1)
    out = []
    for a in range(0, xin.shape[1], BLOCK):
        b = (a - pre) // BLOCK                              # (the signal's own blocks)
        if 0 <= b < len(f): p.parameters['compress'].raw_value = (float(f[b]) + 10.0) / 20.0
        out.append(p.process(xin[:, a:a + BLOCK], FS, buffer_size=BLOCK, reset=False))
    y = np.concatenate(out, 1)                              # (pedalboard takes the latency out across the calls too)
    return y[:, pre:pre + len(x)]


def _render_moves(c):
    path = os.path.join(OUT, 'moves_c%d.npz' % c)
    if os.path.exists(path): return path, 0.0
    X, F = moves(c)
    Y = [render_moving(x, f, c) for x, f in zip(X, F)]
    np.savez(path, x=np.stack(X), y=np.stack([y[0] for y in Y]).astype(np.float32), f=np.stack(F))
    return path, max(float(np.abs(y[0] - y[1]).max()) for y in Y)


def _render(job):
    import cv as C
    split, c, v = job
    if split == 'moves': return _render_moves(c)
    prefix = {'held': 'held_', 'val': 'val_', 'frac': 'frac_', 'wallx': 'wallx_'}.get(split, '')
    path = os.path.join(OUT, ('%sc%d_%+05.1f.npz' if split == 'frac' else '%sc%d_%+03d.npz') % (prefix, c, v))
    if os.path.exists(path): return path, 0.0
    X = (held_examples() if split == 'held' else val_examples() if split == 'val' else frac_examples(c, v) if split == 'frac'
         else wallx_examples(v) if split == 'wallx' else examples(c, v))
    Y = []; side = 0.0
    for x in X:
        y = C.render(x, FS, {C.NOISE: 0}, C.host(**dict(CLEAR, compress_color=c, compress=v)))
        side = max(side, float(np.abs(y[0] - y[1]).max())); Y.append(y[0])
    np.savez(path, x=np.stack(X), y=np.stack(Y).astype(np.float32))
    return path, side


def main(which):
    os.makedirs(OUT, exist_ok=True)
    splits = ('train', 'held', 'val', 'frac', 'moves') if which == 'all' else (which,)
    jobs = [(s, c, v) for s in splits for c in ((3,) if s == 'wallx' else (1, 2, 3))
            for v in ((0,) if s == 'moves' else FRAC if s == 'frac' else WALLX if s == 'wallx' else range(10, -11, -1))]
    worst = 0.0
    with Pool(12) as pool:
        for i, (p, side) in enumerate(pool.imap_unordered(_render, jobs)):
            worst = max(worst, side)
            print('%d / %d %s' % (i + 1, len(jobs), os.path.basename(p)), flush=True)
    print('rendered %d positions; largest left / right difference %.3g' % (len(jobs), worst))


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else 'all')
