"""The plug-in's learned Compress correction against PyTorch's: ChRender (every colour clear but Compress, the
Noise off) against train_vocalchain_compress.py's net run through the exact model (prep_vocalchain_compress.py), on a
held-out vocal after a second of silence, per colour at a few fader positions. Optionally an older renderer (built
without the net) against the model alone. PyTorch runs in full float32 (cuDNN's TF32 alone differs from the plug-in
by -65 .. -89 dB); the net's steps start where the plug-in's compressor starts, so the model is tried 0 .. 3 samples
later and the best kept (at 48 kHz the chain's delay before the compressor plus its look-ahead puts it 1 later).
Runs in the PyTorch environment.

    python check_vocalchain_compressnet.py [old ChRender.exe]
"""
import os
import subprocess
import sys
import tempfile
import numpy as np
import torch

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import prep_vocalchain_compress as Pp
import baseline_vocalchain_compress as Bl
import train_vocalchain_compress as Tr

FS = 48000
RENDER = os.path.join(HERE, '..', 'build', 'plugins', 'FwVocalChain', 'ChRender_artefacts', 'Release', 'ChRender.exe')
TAGS = os.environ.get('TAG', '_r2').split(',')       # one tag, or Push,Spank,Wall as exported (a+b: averaged)
TAGS = dict(zip((1, 2, 3), [t.split('+') for t in (TAGS * 3 if len(TAGS) == 1 else TAGS)]))
torch.backends.cudnn.allow_tf32 = False
torch.backends.cuda.matmul.allow_tf32 = False


def render(exe, x, c, v):
    with tempfile.TemporaryDirectory() as d:
        a, b = os.path.join(d, 'in.f32'), os.path.join(d, 'out.f32')
        x.astype(np.float32).tofile(a)
        args = [exe, a, b, '1', str(FS), 'noise=0', 'compressColor=%d' % c, 'compress=%g' % v] + \
               ['%sColor=0' % k for k in ('bass', 'treble', 'reverb', 'delay', 'pitch')]
        subprocess.run(args, check=True)
        return np.fromfile(b, np.float32).reshape(-1, 2).T.astype(np.float64)


def model(x, c, v, nets=()):
    u, x0, det, red, lred = Pp.compress(x, Bl.params(c, v))
    if nets:
        f = torch.as_tensor(Pp.features(x0, det, red, lred))[None].to(Tr.dev)
        with torch.no_grad():
            corr = sum(n(f, torch.tensor([float(v)], device=Tr.dev))[0].cpu().numpy().astype(np.float64) for n in nets) / len(nets)
        u = u.copy(); u[:len(corr)] *= np.exp(corr)
    return Pp.downstream(c, u)[0]


def main(old=None):
    d = np.load(os.path.join(Pp.TRAIN, 'held_c1_+10.npz'))
    x = np.concatenate([np.zeros(FS), d['x'][3].astype(np.float64)])
    for c in (1, 2, 3):
        net = []
        for t in TAGS[c]:
            ck = torch.load(os.path.join(Tr.OUTD, 'c%d%s.pt' % (c, t)))
            n = Tr.Net(ck['hidden'], ck['mean'], ck['std']).to(Tr.dev); n.load_state_dict(ck['state']); n.eval(); net.append(n)
        for v in (-10, 0, 5, 10, 7.5):
            y = render(RENDER, x, c, v)
            best = min(((Pp.null(m[FS:], y[0, FS:]), k, m) for k in range(4)
                        for m in [model(np.concatenate([np.zeros(k), x]), c, v, net)[k:]]), key=lambda r: r[0])
            m = best[2]
            s = '%-5s %+5.1f: plug-in against PyTorch %6.1f dB (left), %6.1f (right; the steps %d later)' % (
                Tr.NAMES[c], v, Pp.null(m[FS:], y[0, FS:]), Pp.null(m[FS:], y[1, FS:]), best[1])
            if old:
                y0 = render(old, x, c, v); m0 = model(x, c, v)
                s += '; without the net: old plug-in against the model %6.1f; the correction moves the output %6.1f' % (
                    Pp.null(m0[FS:], y0[0, FS:]), Pp.null(y0[0, FS:], y[0, FS:]))
            print(s, flush=True)


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else None)
