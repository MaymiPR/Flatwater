"""Learned Compress correction for Flatwater Vocal Chain (gray-box, the learned-block procedure): the fitted
compressor (Compress.c, the plug-in's Compressor) stays exact; a small GRU per colour scales its output.

    out = D(H(u * exp(c)))      c = CMAX tanh(GRU), one step per 4 samples at 48 kHz (12 kHz), each sample taking the
                                last two finished steps' values, interpolated (causal, as the plug-in can run it)

Inputs per step (render_compress_cache.py): the detector level, the compressor's and the limiter's reduction, the
peak and the power of the compressor's input (dB / 20), and the fader position / 10, each normalised. Loss: the null
residual of each piece (10 log10 of the residual over the reference, the first quarter second skipped), averaged,
plus the amount any piece is worse than the fitted model alone (so what nulls already, the tone ladders and the light
settings, is left alone). Runs in the PyTorch environment (local.yaml's gpu_python).

    python train_compress.py train <colour> [steps] [hidden]    (writes build/vocalchainnet/c<colour><TAG>.pt)
    python train_compress.py exact <colour>                     (the held-out vocals and the fit cases, whole,
                                                                          through the exact downstream)

The nets in MEASUREMENTS.md ("Flatwater Vocal Chain: Compress, a learned correction"): SUP=2000 FLOOR=-50 LAMBDA=3, 8000 steps,
hidden 32 (on the null alone Push learned nothing: its error is a smooth gain wander with no bias, which the
supervised start finds).

Settings from the environment: VMIN (train only on pieces at fader positions from VMIN up; default -10), FLOOR (a floor
on each piece's residual in the loss, dB under its reference; default none), LAMBDA (the weight of the no-worse term;
default 1), SUP (the first SUP steps supervised on the reference's gain: per 2 ms block the least-squares gain of
the fitted model's output against the reference, smoothed over 10 ms, weighted by the block's energy), SEL (keep
the checkpoint with the best held-out mean plus SEL x the mean amount any held-out piece is worse, both floored at
FLOOR; default 0, the mean alone), TAG (appended to the checkpoint's name), CACHE (the cache's suffix: _wallx for
render_compress_cache.py's WALLX=1 cache).
"""
import json
import math
import os
import sys
import time
import numpy as np
import torch

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.append(HERE)
TRAIN = os.path.join(HERE, '..', '..', '..', 'reference', 'fwvocalchain', 'train')
OUTD = os.path.join(HERE, '..', '..', '..', 'build', 'vocalchainnet')
FS = 48000
D = 4
SKIP = FS // 4
CMAX = math.log(10 ** (2.0 / 20))                         # the correction's bound: +/- 2 dB
NAMES = {1: 'Push', 2: 'Spank', 3: 'Wall'}
dev = torch.device('cuda')
VMIN = float(os.environ.get('VMIN', '-10'))
FLOOR = os.environ.get('FLOOR')
LAMBDA = float(os.environ.get('LAMBDA', '1'))
TAG = os.environ.get('TAG', '')
CACHE = os.environ.get('CACHE', '')
SUP = int(os.environ.get('SUP', '0'))
SEL = float(os.environ.get('SEL', '0'))
BLK = 96                                                    # the gain target's blocks (2 ms)


class Net(torch.nn.Module):
    def __init__(self, hidden, mean, std):
        super().__init__()
        self.register_buffer('mean', torch.as_tensor(mean, dtype=torch.float32))
        self.register_buffer('std', torch.as_tensor(std, dtype=torch.float32))
        self.gru = torch.nn.GRU(6, hidden, batch_first=True)
        self.out = torch.nn.Linear(hidden, 1)
        torch.nn.init.zeros_(self.out.weight); torch.nn.init.zeros_(self.out.bias)   # (starts as the fitted model)

    def forward(self, f, v):
        """f (batch, steps, 5), v (batch,): the correction per sample (batch, steps * D), in nepers"""
        x = torch.cat([f, (v / 10.0)[:, None, None].expand(-1, f.shape[1], 1)], 2)
        x = ((x - self.mean) / self.std).contiguous()
        hs, st = [], None
        for a in range(0, x.shape[1], 32768):                                     # (cuDNN's longest sequence)
            h, st = self.gru(x[:, a:a + 32768], st); hs.append(h)
        c = CMAX * torch.tanh(self.out(torch.cat(hs, 1))[..., 0])
        c1 = torch.nn.functional.pad(c, (1, 0))[:, :-1]                            # the last finished step
        c2 = torch.nn.functional.pad(c, (2, 0))[:, :-2]                            # the one before it
        w = torch.arange(1, D + 1, device=c.device, dtype=c.dtype) / D
        return (c2[..., None] + (c1 - c2)[..., None] * w).reshape(c.shape[0], -1)


def load(c, split):
    d = np.load(os.path.join(TRAIN, 'cache_c%d_%s%s.npz' % (c, split, CACHE)))
    T = {k: torch.as_tensor(d[k]).to(dev) for k in ('u', 'g', 'y', 'f', 'v', 'n', 'base', 'kind')}
    T['h'] = torch.as_tensor(d['h']).to(dev); T['hb'] = torch.as_tensor(d['hb']).to(dev)
    return T


def output(net, T, idx):
    """the corrected output: H as an FFT convolution, then the de-esser with its gain frozen"""
    w = T['u'][idx] * torch.exp(net(T['f'][idx], T['v'][idx]))
    n = w.shape[1]; L = 1 << int(math.ceil(math.log2(n + len(T['h']))))
    z = torch.fft.irfft(torch.fft.rfft(w, L) * torch.fft.rfft(T['h'], L), L)[:, :n]
    k = len(T['hb'])
    hb = torch.nn.functional.conv1d(z[:, None], T['hb'][None, None], padding=k // 2)[:, 0]
    return z - (1 - T['g'][idx]) * hb


def nulls(net, T, idx, floor=None):
    out = output(net, T, idx)
    y = T['y'][idx]
    t = torch.arange(y.shape[1], device=dev)
    m = ((t >= SKIP) & (t < T['n'][idx][:, None])).float()
    eps = 1e-30 if floor is None else 10 ** (float(floor) / 10)
    return 10 * torch.log10(((y - out) ** 2 * m).sum(1) / ((y ** 2 * m).sum(1) + 1e-30) + eps)


def gain_targets(net, T, batch=24):
    """per step: the log gain the fitted model's output needs per 2 ms block (smoothed over 10 ms) and its weight"""
    tg, wt = [], []
    with torch.no_grad():
        for a in range(0, len(T['v']), batch):
            idx = torch.arange(a, min(a + batch, len(T['v'])), device=dev)
            out = output(net, T, idx); y = T['y'][idx]
            n = out.shape[1] // BLK * BLK
            ob, yb = out[:, :n].reshape(len(idx), -1, BLK), y[:, :n].reshape(len(idx), -1, BLK)
            e = (ob * ob).sum(2)
            g = torch.log(((yb * ob).sum(2) / e.clamp_min(1e-30)).clamp(0.5, 2.0))
            act = (e > 1e-12).float()
            t = torch.arange(n // BLK, device=dev) * BLK
            act = act * ((t >= SKIP) & (t < T['n'][idx][:, None] - BLK)).float()
            sm = lambda s: torch.nn.functional.avg_pool1d(s[:, None], 5, 1, 2, count_include_pad=False)[:, 0]
            g = sm(g * e * act) / sm(e * act).clamp_min(1e-30)
            w = e * act / ((e * act).sum(1, keepdim=True) + 1e-30) * act.sum(1, keepdim=True)
            tg.append(g.clamp(-CMAX, CMAX).repeat_interleave(BLK // D, 1)); wt.append(w.repeat_interleave(BLK // D, 1))
    return torch.cat(tg), torch.cat(wt)


def evaluate(net, T, batch=24):
    with torch.no_grad():
        return torch.cat([nulls(net, T, torch.arange(a, min(a + batch, len(T['v'])), device=dev))
                          for a in range(0, len(T['v']), batch)]).cpu().numpy()


def report(T, nl):
    v = T['v'].cpu().numpy(); base = T['base'].cpu().numpy(); kind = T['kind'].cpu().numpy()
    hi = v >= 5
    s = 'mean %.2f (base %.2f), +5..+10 median %.2f (base %.2f), worst change %+.2f' % (
        nl.mean(), base.mean(), np.median(nl[hi]), np.median(base[hi]), (nl - base).max())
    if (kind == 1).any(): s += ', fit cases worst change %+.2f' % (nl - base)[kind == 1].max()
    return s


def train(c, steps=4000, hidden=32, batch=24, lr=3e-3):
    os.makedirs(OUTD, exist_ok=True)
    T = load(c, 'train'); H = load(c, 'held')
    f = T['f'].reshape(-1, 5)
    fv = torch.cat([f, (T['v'] / 10.0).repeat_interleave(T['f'].shape[1])[:, None]], 1)
    mean, std = fv.mean(0).cpu().numpy(), fv.std(0).clamp_min(1e-3).cpu().numpy()
    del f, fv
    torch.manual_seed(c)
    net = Net(hidden, mean, std).to(dev)
    opt = torch.optim.Adam(net.parameters(), lr=lr)
    sched = torch.optim.lr_scheduler.OneCycleLR(opt, max_lr=lr, total_steps=steps, pct_start=0.05)
    pick = torch.nonzero(T['v'] >= VMIN)[:, 0]
    P = len(pick); base = T['base']
    if FLOOR is not None: base = torch.maximum(base, torch.tensor(float(FLOOR), device=dev))
    print('%s: %d training pieces, %d held-out; held-out %s' % (NAMES[c], P, len(H['v']), report(H, H['base'].cpu().numpy())), flush=True)
    best, hist, t0 = 1e9, [], time.time()
    if SUP: tg, wt = gain_targets(net, T)
    for it in range(1, steps + 1):
        idx = pick[torch.randint(0, P, (batch,), device=dev)]
        if it <= SUP:
            c_ = net(T['f'][idx], T['v'][idx])[:, ::D][:, :tg.shape[1]]
            loss = 100 * (wt[idx] * (c_ - tg[idx]) ** 2).mean()
        else:
            nl = nulls(net, T, idx, FLOOR)
            loss = nl.mean() + LAMBDA * torch.relu(nl - base[idx]).mean()
        if not torch.isfinite(loss): continue
        opt.zero_grad(); loss.backward()
        torch.nn.utils.clip_grad_norm_(net.parameters(), 1.0)
        opt.step(); sched.step()
        if it % 250 == 0 or it == steps:
            hn = evaluate(net, H)
            score = float(hn.mean())
            if SEL:
                hb, hf = H['base'].cpu().numpy(), hn
                if FLOOR is not None: hb, hf = np.maximum(hb, float(FLOOR)), np.maximum(hn, float(FLOOR))
                score = float(hf.mean() + SEL * np.maximum(hf - hb, 0).mean())
            hist.append(dict(step=it, loss=loss.item(), held=score))
            print('%5d  %.0f s  loss %.2f  held-out %s' % (it, time.time() - t0, loss.item(), report(H, hn)), flush=True)
            if score < best:
                best = score
                torch.save(dict(state=net.state_dict(), hidden=hidden, mean=mean.tolist(), std=std.tolist(), colour=c, step=it), os.path.join(OUTD, 'c%d%s.pt' % (c, TAG)))
    net.load_state_dict(torch.load(os.path.join(OUTD, 'c%d%s.pt' % (c, TAG)))['state'])
    tn = evaluate(net, T)
    print('best (held-out mean %.2f); training pieces %s' % (best, report(T, tn)), flush=True)
    json.dump(hist, open(os.path.join(OUTD, 'c%d%s_history.json' % (c, TAG)), 'w'))


def exact(c):
    """whole examples through the exact chain: Compress.c, the correction, the colour's filter, the fixed filter and the
    de-esser computed on the corrected signal"""
    import render_compress_cache as Pp
    import null_compress_baseline as Bl
    import fit_compress as F
    torch.backends.cudnn.allow_tf32 = False                      # (full float32, as the plug-in)
    ck = torch.load(os.path.join(OUTD, 'c%d%s.pt' % (c, TAG)))
    net = Net(ck['hidden'], ck['mean'], ck['std']).to(dev); net.load_state_dict(ck['state']); net.eval()
    rows = []
    def run(v, x, y, name):
        u, x0, det, red, lred = Pp.compress(x, Bl.params(c, v))
        f = torch.as_tensor(Pp.features(x0, det, red, lred))[None].to(dev)
        with torch.no_grad(): corr = net(f, torch.tensor([float(v)], device=dev))[0].cpu().numpy().astype(np.float64)
        m = len(corr)
        w = u.copy(); w[:m] *= np.exp(corr)
        before = Pp.downstream(c, u)[0]; after = Pp.downstream(c, w)[0]
        rows.append((name, v, Pp.null(y[SKIP:], before[SKIP:]), Pp.null(y[SKIP:], after[SKIP:])))
    for v in range(-10, 11):
        d = np.load(os.path.join(TRAIN, 'held_c%d_%+03d.npz' % (c, v)))
        for i, (x, y) in enumerate(zip(d['x'].astype(np.float64), d['y'].astype(np.float64))): run(v, x, y, 'held %d' % i)
        for i, (x, y) in enumerate(F.load_data(c, v)): run(v, x[0], y[0], 'fit %d' % i)
    print('%s, exact: step | held-out vocals median before -> after | fit cases mean before -> after, worst change' % NAMES[c])
    for v in range(-10, 11):
        h = [(b, a) for n, vv, b, a in rows if vv == v and n.startswith('held')]
        f = [(b, a) for n, vv, b, a in rows if vv == v and n.startswith('fit')]
        print('%+3d | %6.1f -> %6.1f | %6.1f -> %6.1f, %+.2f' % (v, np.median([b for b, a in h]), np.median([a for b, a in h]),
              np.mean([b for b, a in f]), np.mean([a for b, a in f]), max(a - b for b, a in f)))
    json.dump(rows, open(os.path.join(OUTD, 'c%d%s_exact.json' % (c, TAG)), 'w'))


if __name__ == '__main__':
    mode, c = sys.argv[1], int(sys.argv[2])
    if mode == 'train':
        train(c, *(int(a) for a in sys.argv[3:5]))
    else:
        exact(c)
