"""The learned Compress correction's training cache (train_vocalchain_compress.py): every example of
render_trainset_vocalchain.py, and the Phase 4 fit cases (fit_vocalchain_compress.py: the tone ladders, bursts, loud vocals
and noise ladder, so the correction learns to leave alone what the fitted model already nulls), run through the fitted
model (baseline_vocalchain_compress.params: the plug-in's parameters at the position).

The correction multiplies the compressor's output (after its limiter and output gain, before the colour's filter):
    out = D(H(u * exp(c)))      u: comp.c's output; H: the colour's filter then the fixed filter (an FIR of its
                                impulse response, 8192 taps, -115 dB tail); D: the main de-esser, z - (1 - g) * band(z)
so the compressor's own state never sees it, and with the de-esser's gain g taken from the model's output, everything
after u is linear. Per example the cache keeps u, g, the reference y and the network's inputs per step of 4 samples
(12 kHz): the detector level, the compressor's and the limiter's reduction, the peak and the power of the compressor's
input (dB / 20), cut into 4 s pieces (the loss skips each piece's first quarter second).

    python -I prep_vocalchain_compress.py [colour ...]   (writes reference/fwvocalchain/train/cache_c<c>_<train|held>.npz)

WALLX=1 in the environment adds render_trainset_vocalchain.py's wallx_c3_<v>.npz (Wall's extra vocals) to the training
pieces and writes cache_c<c>_<train|held>_wallx.npz instead, leaving the round 2 .. 4 caches alone.
"""
import ctypes
import json
import os
import sys
import numpy as np
from multiprocessing import Pool
from scipy.signal import lfilter, sosfilt

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
TRAIN = os.path.join(HERE, '..', 'reference', 'fwvocalchain', 'train')
FS = 48000
D = 4                                                       # samples per network step at 48 kHz
N = 4 * FS                                                  # samples per piece
SKIP = FS // 4
NH = 8192                                                   # taps of H
NF = 5                                                      # inputs per step (and the fader position)
WALLX = os.environ.get('WALLX') == '1'

_lib = None
_lin = None


def lib():
    global _lib, _lin
    if _lib is None:
        _lib = ctypes.CDLL(os.path.join(HERE, 'comp.dll'))
        P = ctypes.POINTER(ctypes.c_double)
        _lib.comp_dump.argtypes = [P, P, P, P, ctypes.c_int, ctypes.c_double, P, P, P, P]
        _lin = json.load(open(os.path.join(HERE, '..', 'reference', 'fwvocalchain', 'meas', 'compress_linear.json')))
    return _lib


def compress(x, p):
    """comp.c on the clear chain's shaped input: (u, the shaped input, detector level, compressor and limiter
    reductions)"""
    import fit_vocalchain_compress as F
    P = ctypes.POINTER(ctypes.c_double)
    x0 = np.ascontiguousarray(x + F.PRE[0] * x ** 2 + F.PRE[1] * x ** 3, np.float64)
    n = len(x0)
    u = np.zeros(n); ur = np.zeros(n); det = np.zeros(n); red = np.zeros(n); lred = np.zeros(n)
    pp = np.ascontiguousarray(p, np.float64)
    lib().comp_dump(x0.ctypes.data_as(P), x0.ctypes.data_as(P), u.ctypes.data_as(P), ur.ctypes.data_as(P), n, float(FS),
                    pp.ctypes.data_as(P), det.ctypes.data_as(P), red.ctypes.data_as(P), lred.ctypes.data_as(P))
    return u, x0, det, red, lred


def downstream(c, w):
    """H then the de-esser (exact, its gain from w itself): (output, H's output, the de-esser's gain)"""
    import fit_vocalchain_compress as F
    import fit_vocalchain_deess as Fd
    lib()
    lin = _lin['48000 %d' % c]
    z = sosfilt(Fd.fixed_sos(), lfilter(lin['b'], lin['a'], w))
    zz = np.stack([z, z])
    js = json.load(open(Fd.PARAMS))
    g = Fd.gain(zz, np.array(js['p']))
    la = js['la']
    if la > 0: g = np.concatenate([g[la:], np.full(la, g[-1])])
    hu = Fd.band(zz, js['h'])[0]
    return z - (1 - g) * hu, z, g


def h_taps(c):
    import fit_vocalchain_deess as Fd
    lib()
    lin = _lin['48000 %d' % c]
    imp = np.zeros(NH); imp[0] = 1
    return sosfilt(Fd.fixed_sos(), lfilter(lin['b'], lin['a'], imp))


def band_taps():
    import fit_vocalchain_deess as Fd
    h = np.asarray(json.load(open(Fd.PARAMS))['h'])
    return np.concatenate([h[:0:-1], h])                     # (zero-phase: centred)


def features(x0, det, red, lred):
    m = len(x0) // D * D
    blk = lambda a: a[:m].reshape(-1, D)
    f = np.stack([np.maximum(blk(det).max(1), -120),
                  blk(red).mean(1),
                  blk(lred).max(1),
                  np.maximum(20 * np.log10(np.abs(blk(x0)).max(1) + 1e-6), -120),
                  np.maximum(10 * np.log10((blk(x0) ** 2).mean(1) + 1e-12), -120)], 1) / 20.0
    return f.astype(np.float32)


def null(y, z):
    return 10 * np.log10(np.sum((y - z) ** 2) / max(np.sum(y ** 2), 1e-30) + 1e-30)


def pieces(c, v, x, y):
    """the example run through the model, cut into pieces of N samples (the last padded): a list of dicts"""
    import baseline_vocalchain_compress as Bl
    p = Bl.params(c, v)
    u, x0, det, red, lred = compress(x, p)
    out, _, g = downstream(c, u)
    f = features(x0, det, red, lred)
    res = []
    for a in range(0, max(len(x) - SKIP, 1), N):
        b = min(a + N, len(x))
        if b - a <= SKIP + FS // 4 or np.sum(y[a + SKIP:b] ** 2) < 1e-12: continue
        pad = lambda s: np.concatenate([s[a:b], np.zeros(N - (b - a))]).astype(np.float32)
        ff = np.zeros((N // D, NF), np.float32); fa = f[a // D:b // D]; ff[:len(fa)] = fa
        res.append(dict(u=pad(u), g=pad(g), y=pad(y), f=ff, v=np.float32(v), n=np.int32(b - a),
                        base=np.float32(null(y[a + SKIP:b], out[a + SKIP:b]))))
    return res


def _job(job):
    import fit_vocalchain_compress as F
    import render_trainset_vocalchain as T
    c, split, v = job
    out = []
    if split in ('train', 'held', 'frac', 'wallx'):
        prefix = {'held': 'held_', 'frac': 'frac_', 'wallx': 'wallx_'}.get(split, '')
        d = np.load(os.path.join(TRAIN, ('%sc%d_%+05.1f.npz' if split == 'frac' else '%sc%d_%+03d.npz') % (prefix, c, v)))
        for x, y in zip(d['x'].astype(np.float64), d['y'].astype(np.float64)): out += pieces(c, v, x, y)
    else:                                                   # the fit cases (both sides equal)
        for x, y in F.load_data(c, v): out += pieces(c, v, x[0], y[0])
    for o in out: o['kind'] = {'train': 0, 'frac': 0, 'wallx': 0, 'fit': 1, 'held': 2}[split]
    return out


def check(c):
    """the linear stand-in (H as taps, the de-esser's gain frozen) against the exact downstream, on a loud vocal"""
    d = np.load(os.path.join(TRAIN, 'held_c%d_+10.npz' % c))
    import baseline_vocalchain_compress as Bl
    u = compress(d['x'][3].astype(np.float64), Bl.params(c, 10))[0]
    out, _, g = downstream(c, u)
    z = np.convolve(u, h_taps(c))[:len(u)]
    hb = np.convolve(z, band_taps(), 'same')
    print('colour %d: the linear stand-in against the exact downstream: %.1f dB' % (c, null(out, z - (1 - g) * hb)))


def main(colours):
    import render_trainset_vocalchain as T
    for c in colours:
        check(c)
        jobs = [(c, s, v) for s in ('train', 'fit', 'held') for v in range(-10, 11)] + [(c, 'frac', v) for v in T.FRAC]
        if WALLX and c == 3: jobs += [(c, 'wallx', v) for v in T.WALLX]
        got = {'train': [], 'held': []}
        with Pool(12) as pool:
            for res in pool.imap_unordered(_job, jobs):
                for o in res: got['held' if o['kind'] == 2 else 'train'].append(o)
        for split, L in got.items():
            L.sort(key=lambda o: (o['kind'], float(o['v'])))
            np.savez(os.path.join(TRAIN, 'cache_c%d_%s%s.npz' % (c, split, '_wallx' if WALLX else '')),
                     **{k: np.stack([o[k] for o in L]) for k in ('u', 'g', 'y', 'f', 'v', 'n', 'base', 'kind')},
                     h=h_taps(c).astype(np.float32), hb=band_taps().astype(np.float32))
            print('colour %d %s: %d pieces, base null median %.1f dB' % (c, split, len(L), np.median([o['base'] for o in L])), flush=True)


if __name__ == '__main__':
    main([int(a) for a in sys.argv[1:]] or [1, 2, 3])
