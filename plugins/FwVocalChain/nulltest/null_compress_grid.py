"""Compress alone (every other colour clear, the Noise off) through the reference and our renderer, on vocals no fit or
training has used (vocal_corpus parts 32 .. 35, 8 s after 1 s of silence) at input peaks -12, -6 and 0 dBFS, per colour
at the even steps -10 .. +10: where the learned correction helps and where it costs. With an older renderer (WVC_RENDER_OLD)
both are nulled against the same reference renders.

    python -I null_compress_grid.py         (writes reference/fwvocalchain/train/grid.json)
    python -I null_compress_grid.py rates   (parts 32 and 33 at peaks -6 and 0, steps -4, +4, +10, at 44.1 and 96 kHz)
"""
import json
import os
import sys
import numpy as np
from multiprocessing import Pool

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.append(HERE)
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'scripts', 'nulltest'))   # scripts/nulltest/
FS = 48000
PARTS = (32, 33, 34, 35)
PEAKS = (-12, -6, 0)
STEPS = range(-10, 11, 2)
OUT = os.path.join(HERE, '..', '..', '..', 'reference', 'fwvocalchain', 'train', 'grid.json')


def job(args):
    import ch_ref as C
    import nulltest as N
    import vocal_corpus_lib as V
    from scipy.signal import resample_poly
    c, v, fs, parts, peaks = args
    old = os.environ.get('WVC_RENDER_OLD')
    rows = []
    for part in parts:
        s = V.part(part)[0][:FS * 8].astype(np.float64)
        if fs != FS: s = resample_poly(s, {44100: 147, 96000: 2}[fs], {44100: 160, 96000: 1}[fs])
        for pk in peaks:
            x = np.concatenate([np.zeros(fs), s / np.abs(s).max() * 10 ** (pk / 20)]).astype(np.float32)
            kw = dict(N.CLEAR, compress_color=c, compress=v)
            y = C.render(x, fs, {C.NOISE: 0}, C.host(**kw), preroll=0.0)[:, fs:]
            res = []
            for exe in ([old] if old else []) + [N.RENDER]:
                N.RENDER = exe
                z = N.render_ours(x, kw, fs)[:, fs:]
                res.append(float(10 * np.log10(np.sum((y - z) ** 2) / np.sum(y ** 2))))
            rows.append(dict(c=c, v=v, fs=fs, part=part, peak=pk, nulls=res))
    return rows


def rates():
    jobs = [(c, v, fs, (32, 33), (-6, 0)) for fs in (44100, 96000) for c in (1, 2, 3) for v in (-4, 4, 10)]
    rows = []
    with Pool(12) as pool:
        for r in pool.imap_unordered(job, jobs): rows += r
    for fs in (44100, 96000):
        for c, name in ((1, 'Push'), (2, 'Spank'), (3, 'Wall')):
            for v in (-4, 4, 10):
                r = np.array([q['nulls'] for q in rows if q['fs'] == fs and q['c'] == c and q['v'] == v])
                print('%5d Hz %-5s %+3d | ' % (fs, name, v) + ('median %6.1f -> %6.1f, worst change %+.1f' % (
                    np.median(r[:, 0]), np.median(r[:, 1]), (r[:, 1] - r[:, 0]).max()) if r.shape[1] == 2 else 'median %6.1f' % np.median(r[:, 0])))


def main():
    jobs = [(c, v, FS, PARTS, PEAKS) for c in (1, 2, 3) for v in STEPS]
    rows = []
    with Pool(12) as pool:
        for r in pool.imap_unordered(job, jobs): rows += r
    json.dump(rows, open(OUT, 'w'))
    two = len(rows[0]['nulls']) == 2
    print('colour step | median null per input peak (%s)' % ('before -> after' if two else 'dB'))
    for c, name in ((1, 'Push'), (2, 'Spank'), (3, 'Wall')):
        for v in STEPS:
            cells = []
            for pk in PEAKS:
                r = np.array([q['nulls'] for q in rows if q['c'] == c and q['v'] == v and q['peak'] == pk])
                cells.append('%6.1f -> %6.1f (worst %+.1f)' % (np.median(r[:, 0]), np.median(r[:, 1]), (r[:, 1] - r[:, 0]).max())
                             if two else '%6.1f' % np.median(r[:, 0]))
            print('%-5s %+3d | %s' % (name, v, ' | '.join(cells)))


if __name__ == '__main__':
    rates() if 'rates' in sys.argv[1:] else main()
