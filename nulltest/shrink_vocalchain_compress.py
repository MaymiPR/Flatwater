"""How much of the learned Compress correction to keep, per colour and fader step: on the validation set
(render_trainset_vocalchain.py val: held-out vocals at peaks -12, -6 and 0 dBFS) the net's correction scaled by alpha
0 .. 1 (exp (alpha c)), each null floored at -50 dB as in training, and per step the alpha with the best mean change
plus 3 x the mean amount any vocal got worse. The downstream is the training's linear stand-in (the de-esser's gain
from the uncorrected output). Runs in the PyTorch environment.

    python shrink_vocalchain_compress.py [tag ...]      (default _r2; writes build/vocalchainnet/shrink<tag>.json)
"""
import json
import os
import sys
import numpy as np
import torch

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import prep_vocalchain_compress as Pp
import baseline_vocalchain_compress as Bl
import train_vocalchain_compress as Tr

torch.backends.cudnn.allow_tf32 = False
torch.backends.cuda.matmul.allow_tf32 = False
ALPHAS = np.linspace(0, 1, 9)
FLOOR = -50.0
dev = Tr.dev


def nulls_scaled(u, c, g, y, h, hb):
    """the nulls (dB) of exp(alpha c) u through H and the frozen de-esser, for every alpha"""
    a = torch.as_tensor(ALPHAS, device=dev, dtype=torch.float64)[:, None]
    w = u[None] * torch.exp(a * c[None])
    n = w.shape[1]; L = 1 << int(np.ceil(np.log2(n + len(h))))
    z = torch.fft.irfft(torch.fft.rfft(w, L) * torch.fft.rfft(h, L), L)[:, :n]
    k = len(hb)
    zb = torch.nn.functional.conv1d(z[:, None], hb[None, None], padding=k // 2)[:, 0]
    out = z - (1 - g[None]) * zb
    s = Pp.FS
    return (10 * torch.log10(((y[None, s:] - out[:, s:]) ** 2).sum(1) / (y[s:] ** 2).sum())).cpu().numpy()


def main(tags):
    for tag in tags:
        table = {}
        for c in (1, 2, 3):
            ck = torch.load(os.path.join(Tr.OUTD, 'c%d%s.pt' % (c, tag)))
            net = Tr.Net(ck['hidden'], ck['mean'], ck['std']).to(dev).double(); net.load_state_dict(ck['state']); net.eval()
            h = torch.as_tensor(Pp.h_taps(c), device=dev); hb = torch.as_tensor(Pp.band_taps(), device=dev)
            alphas = []
            for v in range(-10, 11):
                d = np.load(os.path.join(Pp.TRAIN, 'val_c%d_%+03d.npz' % (c, v)))
                D = []
                for x, y in zip(d['x'].astype(np.float64), d['y'].astype(np.float64)):
                    x = np.concatenate([np.zeros(Pp.FS), x]); y = np.concatenate([np.zeros(Pp.FS), y])
                    u, x0, det, red, lred = Pp.compress(x, Bl.params(c, v))
                    _, _, g = Pp.downstream(c, u)
                    f = torch.as_tensor(Pp.features(x0, det, red, lred), dtype=torch.float64)[None].to(dev)
                    with torch.no_grad(): cc = net(f, torch.tensor([float(v)], device=dev, dtype=torch.float64))[0]
                    cf = torch.zeros(len(u), device=dev, dtype=torch.float64); cf[:len(cc)] = cc
                    nl = np.maximum(nulls_scaled(torch.as_tensor(u, device=dev), cf, torch.as_tensor(g, device=dev),
                                                 torch.as_tensor(y, device=dev), h, hb), FLOOR)
                    D.append(nl - nl[0])
                D = np.array(D)
                score = D.mean(0) + 3 * np.maximum(D, 0).mean(0)
                k = int(np.argmin(score))
                alphas.append(float(ALPHAS[k]))
                print('%s %-5s %+3d | alpha 1: mean %+.2f worst %+.2f | best alpha %.3f: mean %+.2f worst %+.2f' % (
                    tag, Tr.NAMES[c], v, D[:, -1].mean(), D[:, -1].max(), ALPHAS[k], D[:, k].mean(), D[:, k].max()), flush=True)
            table[c] = alphas
        json.dump(table, open(os.path.join(Tr.OUTD, 'shrink%s.json' % tag), 'w'))


if __name__ == '__main__':
    main(sys.argv[1:] or ['_r2'])
