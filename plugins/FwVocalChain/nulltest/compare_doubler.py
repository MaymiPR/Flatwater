"""The doubler's behaviour against the reference (the Pitch stage's Wide and Spreader colours pick their splice points
from the signal, so they are compared in behaviour, not sample for sample; Stereo too, for scale). Pitch at +10, every
other colour clear, the wet signal = the output less the output with Pitch clear, after 6 s of silence. Per colour,
input (white noise at -20 dBFS rms, a held-out vocal at -6 dBFS peaks, 20 s) and side:
- level: the wet energy over the dry's (dB);
- spectrum: the wet's third-octave levels, 100 Hz .. 12 kHz, ours less the reference's (rms and largest, dB);
- delay: the voice's delay tracked as the cross-correlation peak of the wet against the dry in 2048-sample windows
  (noise only): its 5th / 50th / 95th percentiles (ms) and the modulation's period (the track's strongest cycle, s);
- L / R: the correlation of the two wet sides.
Writes measurements/fwvocalchain/doubler.json (NULLTESTS.md takes its table from there).

    python -I compare_doubler.py
"""
import json, os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.append(HERE)
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'scripts', 'nulltest'))   # scripts/nulltest/
import numpy as np
import cv as C
import vocal_corpus as V
import nulltest as N

FS = 48000
OUT = os.path.join(HERE, '..', '..', '..', 'measurements', 'fwvocalchain', 'doubler.json')
CLEAR = dict(N.CLEAR)


def renders(x, colour):
    pad = 6 * FS
    x = np.concatenate([np.zeros(pad), x]).astype(np.float32)
    kw = dict(CLEAR, pitch_color=colour, pitch=10)
    ry = C.render(x, FS, {C.NOISE: 0}, C.host(**kw), preroll=0.0)[:, pad:]
    rd = C.render(x, FS, {C.NOISE: 0}, C.host(**CLEAR), preroll=0.0)[:, pad:]
    oy = N.render_ours(x, kw)[:, pad:]
    od = N.render_ours(x, CLEAR)[:, pad:]
    return (ry - rd, rd), (oy - od, od)


def third_octaves(w):
    from scipy.signal import welch
    f, p = welch(w, FS, nperseg=8192)
    cs = 1000 * 2 ** (np.arange(-10, 12) / 3)
    out = []
    for c in cs:
        m = (f >= c * 2 ** (-1 / 6)) & (f < c * 2 ** (1 / 6))
        out.append(10 * np.log10(np.sum(p[m]) + 1e-30))
    return np.array(out)


def delay_track(wet, dry, maxlag=2400, win=2048, hop=1024):
    from scipy.signal import fftconvolve
    lags = []
    for s in range(maxlag, len(wet) - win, hop):
        a = wet[s:s + win]; b = dry[s - maxlag:s + win]
        c = fftconvolve(a[::-1], b, mode='valid')[::-1]          # c[k]: wet at s against dry k samples earlier
        lags.append(int(np.argmax(np.abs(c))))
    return np.array(lags) / FS * 1000


def period(track, hop=1024):
    t = track - np.mean(track)
    sp = np.abs(np.fft.rfft(t * np.hanning(len(t)), 8 * len(t)))
    f = np.fft.rfftfreq(8 * len(t), hop / FS)
    m = f > 0.03
    return float(1 / f[m][np.argmax(sp[m])])


def measure(wet, dry, track):
    d = dict(level=[round(float(10 * np.log10(np.sum(wet[c] ** 2) / np.sum(dry[c] ** 2))), 2) for c in (0, 1)],
             lr=round(float(np.corrcoef(wet[0], wet[1])[0, 1]), 3), bands=[third_octaves(wet[c]).tolist() for c in (0, 1)])
    if track:
        d['delay'] = []
        for c in (0, 1):
            tr = delay_track(wet[c], dry[c])
            d['delay'].append(dict(p5=round(float(np.percentile(tr, 5)), 2), p50=round(float(np.percentile(tr, 50)), 2),
                                   p95=round(float(np.percentile(tr, 95)), 2), period=round(period(tr), 2)))
    return d


def main():
    rng = np.random.default_rng(31)
    noise = rng.standard_normal(20 * FS) * 10 ** (-20 / 20)
    v = V.part(31)[0][:20 * FS]; v = v / np.abs(v).max() * 10 ** (-6 / 20)
    res = []
    for colour, name in ((1, 'Stereo'), (2, 'Wide'), (3, 'Spreader')):
        for sig, x in (('noise', noise), ('vocal', v)):
            (rw, rd), (ow, od) = renders(x, colour)
            r, o = measure(rw, rd, sig == 'noise'), measure(ow, od, sig == 'noise')
            diff = [np.array(o['bands'][c]) - np.array(r['bands'][c]) for c in (0, 1)]
            row = dict(colour=name, signal=sig, ref=r, ours=o,
                       level_diff=[round(o['level'][c] - r['level'][c], 2) for c in (0, 1)],
                       band_rms=[round(float(np.sqrt(np.mean(d ** 2))), 2) for d in diff],
                       band_max=[round(float(np.max(np.abs(d))), 2) for d in diff],
                       null=[round(float(10 * np.log10(np.sum((ow[c] - rw[c]) ** 2) / np.sum(rw[c] ** 2))), 2) for c in (0, 1)])
            for k in ('bands',):
                r.pop(k); o.pop(k)
            res.append(row)
            print(json.dumps(row), flush=True)
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    json.dump(res, open(OUT, 'w', encoding='utf-8', newline='\n'), indent=1)


if __name__ == '__main__':
    main()
