"""The doubler at other sample rates: a click train through the reference's Pitch stage alone (fader +10, every other
colour clear, the output less the output with Pitch clear) and through ours, and per click the voice's delay in each
(the wet click's peak against the dry click's, parabolic, in samples of that rate), so what differs shows as a
constant (the start delay or the LFO's lead), a slope (the drift) or a cycle (the swing or rate).

    python -I measure_doubler_rates.py COLOUR FS [SECONDS]      prints per click: time, reference, ours, difference
    python -I measure_doubler_rates.py summary COLOUR FS [SECONDS]
"""
import os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.append(HERE)
import numpy as np
import ch_ref as C
import nulltest as N

CLEAR = dict(N.CLEAR)


def clicks(fs, secs, spacing_s=0.025):
    x = np.zeros(int(secs * fs)); P = int(spacing_s * fs)
    x[P::P] = 0.5
    return x, P


def wet_ref(x, fs, colour):
    kw = dict(CLEAR, pitch_color=colour, pitch=10)
    return C.render(x, fs, {C.NOISE: 0}, C.host(**kw), preroll=1.0) - C.render(x, fs, {C.NOISE: 0}, C.host(**CLEAR), preroll=1.0)


def wet_ours(x, fs, colour):
    # (the reference's renders start with a second of silence: ours gets the same, so both clocks agree)
    pre = np.zeros(fs)
    xx = np.concatenate([pre, x])
    kw = dict(CLEAR, pitch_color=colour, pitch=10)
    return (N.render_ours(xx, kw, fs) - N.render_ours(xx, CLEAR, fs))[:, fs:]


def delays(w, x, P, maxlag):
    """per click (one every P samples), the wet's peak position after the click (parabolic)"""
    out = []
    for s in range(P, len(x) - maxlag - 2, P):
        seg = np.abs(w[s:s + maxlag])
        k = int(np.argmax(seg))
        if 0 < k < len(seg) - 1:
            a, b, c = seg[k - 1], seg[k], seg[k + 1]
            k = k + 0.5 * (a - c) / (a - 2 * b + c)
        out.append((s, k))
    return np.array(out)


def main(colour, fs, secs=8.0):
    x, P = clicks(fs, secs)
    maxlag = min(P - 4, int(0.045 * fs))
    r = wet_ref(x, fs, colour); o = wet_ours(x, fs, colour)
    res = []
    for side in (0, 1):
        dr = delays(r[side], x, P, maxlag); do = delays(o[side], x, P, maxlag)
        res.append((dr, do))
    return res, P


if __name__ == '__main__':
    a = sys.argv[1:]
    summ = a[0] == 'summary'
    if summ: a = a[1:]
    colour, fs = int(a[0]), int(a[1]); secs = float(a[2]) if len(a) > 2 else 8.0
    res, P = main(colour, fs, secs)
    for side, (dr, do) in enumerate(res):
        d = do[:, 1] - dr[:, 1]
        print('side %d: ours - reference (samples): median %+.3f, rms %.3f, first %+.3f, last %+.3f; reference first %.2f'
              % (side, np.median(d), np.sqrt(np.mean(d ** 2)), d[0], d[-1], dr[0, 1]))
        if not summ:
            for (s, a1), (_, b1) in zip(dr, do):
                print('  %7.3f s  ref %8.3f  ours %8.3f  %+7.3f' % (s / fs, a1, b1, b1 - a1))
