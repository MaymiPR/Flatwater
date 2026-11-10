"""The learned Compress correction's starting point: on render_trainset.py's examples, the fitted model's null
(fit_compress.py's model: the clear chain's shaper, Compress.c, the colour's filter, the fixed filter and the main
de-esser) per colour and fader step, and the ceiling a gain correction could reach: the model's output scaled per
block by its least-squares gain against the reference (2 ms and 10 ms blocks; an estimate, since a correction inside
the plug-in acts before the filters and the de-esser). Positions between the steps (the frac_ set) take the plug-in's
parameters there: linear between the two steps, or the nearer step where their structures differ (plugins/FwVocalChain/Source/dsp/Engine.cpp).

    python -I null_compress_baseline.py          (writes reference/fwvocalchain/train/baseline.json)
"""
import json
import os
import sys
import numpy as np
from multiprocessing import Pool

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.append(HERE)
TRAIN = os.path.join(HERE, '..', '..', '..', 'reference', 'fwvocalchain', 'train')
FS = 48000
SKIP = FS // 4                                              # (the first quarter second: the model and reference settle)


def null(y, z):
    return 10 * np.log10(np.sum((y - z) ** 2) / max(np.sum(y ** 2), 1e-30) + 1e-30)


def blockgain(y, z, n):
    m = len(y) // n * n
    yb, zb = y[:m].reshape(-1, n), z[:m].reshape(-1, n)
    g = np.sum(yb * zb, 1) / np.maximum(np.sum(zb * zb, 1), 1e-30)
    g = np.where(np.sum(zb * zb, 1) > 1e-20, np.clip(g, 0.5, 2.0), 1.0)
    out = z.copy(); out[:m] = (zb * g[:, None]).reshape(-1)
    return out


def params(c, v):
    """the plug-in's parameters at fader position v"""
    import fit_compress as F
    js = json.load(open(F.PARAMS))
    x = v + 10.0; k0 = min(19, int(x)); t = x - k0
    a = np.array(js['%d %+d' % (c, k0 - 10)]['p'], float); b = np.array(js['%d %+d' % (c, k0 - 9)]['p'], float)
    if a[37] == b[37] and a[38] == b[38]: return a + (b - a) * t
    return a if t < 0.5 else b


def one(job):
    import fit_compress as F
    split, c, v = job
    prefix = {'held': 'held_', 'frac': 'frac_'}.get(split, '')
    d = np.load(os.path.join(TRAIN, ('%sc%d_%+05.1f.npz' if split == 'frac' else '%sc%d_%+03d.npz') % (prefix, c, v)))
    p = params(c, v)
    F.COLOUR[0] = c; F.DEESS[0] = True
    res = dict(model=[], b2=[], b10=[])
    for x, y in zip(d['x'].astype(np.float64), d['y'].astype(np.float64)):
        z = F.model(np.stack([x, x]), p)[0]
        y, z = y[SKIP:], z[SKIP:]
        if np.sum(y ** 2) < 1e-12: continue
        res['model'].append(null(y, z))
        res['b2'].append(null(y, blockgain(y, z, 96)))
        res['b10'].append(null(y, blockgain(y, z, 480)))
    return split, c, v, res


def main():
    import render_trainset as T
    jobs = [(s, c, v) for s in ('train', 'held') for c in (1, 2, 3) for v in range(-10, 11)]
    jobs += [('frac', c, v) for c in (1, 2, 3) for v in T.FRAC]
    out = {}
    with Pool(12) as pool:
        for split, c, v, res in pool.imap_unordered(one, jobs):
            out['%s %d %+g' % (split, c, v)] = res
    json.dump(out, open(os.path.join(TRAIN, 'baseline.json'), 'w'))
    print('colour step | train: model  2 ms  10 ms | held: model  2 ms  10 ms   (median null, dB)')
    for c, name in ((1, 'Push'), (2, 'Spank'), (3, 'Wall')):
        for v in range(-10, 11):
            r = [out['%s %d %+d' % (s, c, v)] for s in ('train', 'held')]
            print('%-5s %+3d   |  %6.1f %6.1f %6.1f |  %6.1f %6.1f %6.1f' % ((name, v) + tuple(
                np.median(rr[k]) for rr in r for k in ('model', 'b2', 'b10'))))
    print("between the steps (frac): colour position | model  2 ms  10 ms | the two steps' train medians (model)")
    for c, name in ((1, 'Push'), (2, 'Spank'), (3, 'Wall')):
        for v in T.FRAC:
            r = out['frac %d %+g' % (c, v)]; k0 = int(np.floor(v))
            print('%-5s %+5.1f | %6.1f %6.1f %6.1f | %6.1f %6.1f' % (name, v, np.median(r['model']), np.median(r['b2']), np.median(r['b10']),
                  np.median(out['train %d %+d' % (c, k0)]['model']), np.median(out['train %d %+d' % (c, k0 + 1)]['model'])))


if __name__ == '__main__':
    main()
