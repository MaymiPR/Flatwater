"""Wall's remaining regressions (steps -7, -6 and 0 in every training): per validation vocal, the null before and after
the plug-in's pair of nets (the average of their corrections, through the training's linear stand-in), and what the
de-esser does on it (its mean reduction over the loud part, dB) and the compressor's mean reduction, so the next round
knows whether the de-esser's gain is the input the nets lack. Runs in the PyTorch environment.

    python check_wall.py [tag+tag] [step ...]     (default _r2+_r3, steps -8 -7 -6 -5 -1 0 1)
    python check_wall.py val tag+tag ...          (each pair over all 21 steps of the validation set: the mean
                                                            change and the worst single vocal, each null floored at
                                                            -40 dB, as MEASUREMENTS.md's round table)
"""
import os, sys
import numpy as np
import torch
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.append(HERE)
import prep_compress as Pp
import null_compress_baseline as Bl
import train_compress as Tr
import fit_compress_shrink as Sh

torch.backends.cudnn.allow_tf32 = False
dev = Tr.dev
C = 3


def nets(tags):
    out = []
    for t in tags.split('+'):
        ck = torch.load(os.path.join(Tr.OUTD, 'c%d%s.pt' % (C, t)))
        n = Tr.Net(ck['hidden'], ck['mean'], ck['std']).to(dev).double(); n.load_state_dict(ck['state']); n.eval()
        out.append(n)
    return out


def main(tags, steps):
    ns = nets(tags)
    h = torch.as_tensor(Pp.h_taps(C), device=dev); hb = torch.as_tensor(Pp.band_taps(), device=dev)
    for v in steps:
        d = np.load(os.path.join(Pp.TRAIN, 'val_c%d_%+03d.npz' % (C, v)))
        rows = []
        for x, y in zip(d['x'].astype(np.float64), d['y'].astype(np.float64)):
            x = np.concatenate([np.zeros(Pp.FS), x]); y = np.concatenate([np.zeros(Pp.FS), y])
            u, x0, det, red, lred = Pp.compress(x, Bl.params(C, v))
            _, _, g = Pp.downstream(C, u)
            f = torch.as_tensor(Pp.features(x0, det, red, lred), dtype=torch.float64)[None].to(dev)
            with torch.no_grad():
                cc = sum(n(f, torch.tensor([float(v)], device=dev, dtype=torch.float64))[0] for n in ns) / len(ns)
            cf = torch.zeros(len(u), device=dev, dtype=torch.float64); cf[:len(cc)] = cc
            nl = Sh.nulls_scaled(torch.as_tensor(u, device=dev), cf, torch.as_tensor(g, device=dev), torch.as_tensor(y, device=dev), h, hb)
            loud = np.abs(x) > 10 ** (-30 / 20)
            ds = float(np.mean(20 * np.log10(np.maximum(g[loud], 1e-6)))) if loud.any() else 0.0
            cr = float(np.mean(np.asarray(red)[::1][:len(red)])) if np.ndim(red) else 0.0
            pk = 20 * np.log10(np.abs(x).max())
            rows.append((pk, nl[0], nl[-1], ds, cr))
        for pk, b, a, ds, cr in rows:
            flag = '  <-- worse' if max(a, -40) - max(b, -40) > 0.5 else ''
            print('Wall %+3d peak %5.1f dBFS | %6.1f -> %6.1f dB | de-esser mean %6.2f dB | comp reduction %6.2f%s'
                  % (v, pk, b, a, ds, cr, flag), flush=True)


def val(pairs, floor=-40.0):
    h = torch.as_tensor(Pp.h_taps(C), device=dev); hb = torch.as_tensor(Pp.band_taps(), device=dev)
    data = {}
    for v in range(-10, 11):
        d = np.load(os.path.join(Pp.TRAIN, 'val_c%d_%+03d.npz' % (C, v)))
        L = []
        for x, y in zip(d['x'].astype(np.float64), d['y'].astype(np.float64)):
            x = np.concatenate([np.zeros(Pp.FS), x]); y = np.concatenate([np.zeros(Pp.FS), y])
            u, x0, det, red, lred = Pp.compress(x, Bl.params(C, v))
            _, _, g = Pp.downstream(C, u)
            f = torch.as_tensor(Pp.features(x0, det, red, lred), dtype=torch.float64)[None].to(dev)
            L.append((u, g, y, f))
        data[v] = L
    for tags in pairs:
        ns = nets(tags)
        D, per = [], {}
        for v, L in data.items():
            for u, g, y, f in L:
                with torch.no_grad():
                    cc = sum(n(f, torch.tensor([float(v)], device=dev, dtype=torch.float64))[0] for n in ns) / len(ns)
                cf = torch.zeros(len(u), device=dev, dtype=torch.float64); cf[:len(cc)] = cc
                nl = Sh.nulls_scaled(torch.as_tensor(u, device=dev), cf, torch.as_tensor(g, device=dev), torch.as_tensor(y, device=dev), h, hb)
                dd = max(nl[-1], floor) - max(nl[0], floor)
                D.append(dd); per.setdefault(v, []).append(dd)
        steps = [float(np.mean(per[v])) for v in range(-10, 11)]
        print('%-12s mean over the steps %+.2f, worst single vocal %+.2f; worst at -7 / -6 / 0: %+.2f / %+.2f / %+.2f; vocals worse > 0.5 dB: %d'
              % (tags, np.mean(steps), max(D), max(per[-7]), max(per[-6]), max(per[0]), sum(d > 0.5 for d in D)), flush=True)


if __name__ == '__main__':
    a = sys.argv[1:]
    if a and a[0] == 'val':
        val(a[1:]); sys.exit()
    tags = a[0] if a and not a[0].lstrip('-+').isdigit() else '_r2+_r3'
    steps = [int(s) for s in a[1 if a and tags == a[0] else 0:]] or [-8, -7, -6, -5, -1, 0, 1]
    main(tags, steps)
