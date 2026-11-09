"""Flatwater Pitch Bend, Phase 1: the reference's modulators measured through its Mix (MEASUREMENTS.md, Flatwater Pitch
Bend). Mix is an exact crossfade, out = dry + m (wet - dry), and the wet path does not depend on it; so with Pitch at
+12 semitones (wet far from dry) and a modulator on Mix, the Mix the reference applied is read back sample by sample:
m = <out - dry, wet - dry> / |wet - dry|^2 over a short window (both channels). Every curve is the modulator's own
output through the Mix slot's depth and the Mix range's clip.

    python -I measure_mods.py <experiment> [...]       experiments: see EXPERIMENTS below; 'all' runs them
Results: measurements/fwpitchbend/mods/<experiment>.json
"""
import sys, os, json, math
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import numpy as np
import vb as H
import editor_state as S

FS = 48000
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'measurements', 'fwpitchbend', 'mods')
WIN = 16
_wet = {}


def mix_curve(x, changes, shapes=None, tag='mod', bpm=120, extra=()):
    """the Mix (0 .. 1) the reference applied, per sample (a WIN-sample centred average), for input x and the state
    changes (Pitch +12 added; Mix and its slots as given)"""
    key = (x.shape[1], float(np.sum(x[0, :1000])), bpm, tuple(extra))
    if key not in _wet:
        _wet[key] = S.render(x, FS, {S.PITCH: 1200}, tag='wet', bpm=bpm, extra=extra)
    wet = _wet[key]
    out = S.render(x, FS, {S.PITCH: 1200, **changes}, tag=tag, bpm=bpm, extra=extra, shapes=shapes)
    d = (wet - x).astype(np.float64); e = (out - x).astype(np.float64)
    k = np.ones(WIN)
    num = sum(np.convolve(d[c] * e[c], k, 'same') for c in range(d.shape[0]))
    den = sum(np.convolve(d[c] * d[c], k, 'same') for c in range(d.shape[0]))
    return num / np.maximum(den, 1e-20)


def noise(secs):
    return H.noise(secs, -12.0, seed=11)


def save(name, data):
    os.makedirs(OUT, exist_ok=True)
    with open(os.path.join(OUT, name + '.json'), 'w') as f:
        json.dump(data, f, indent=1)


def sample(m, step=0.01):
    """the curve every `step` seconds (for the JSON)"""
    return [round(float(v), 5) for v in m[::int(step * FS)]]


def lfo_mix(base, depth, source=S.M1, extra_changes=None, secs=4.0, shapes=None, tag='lfo', bpm=120):
    ch = {S.MIX: base, **S.slot('mix', 0, source, depth), **(extra_changes or {})}
    return mix_curve(noise(secs), ch, shapes, tag, bpm)


# ---- experiments -------------------------------------------------------------------------------------------------
def exp_depth():
    """how a slot's depth scales a modulator on Mix: base 50, depth +-25 / 50 / 100, M1 at its default (a sine table,
    1 bar at 120 bpm = 0.5 Hz)"""
    res = {}
    for depth in (25, 50, 100, -50):
        m = lfo_mix(50.0, depth, tag='depth%d' % depth)
        q = m[FS // 10:]
        res[str(depth)] = dict(min=float(q.min()), max=float(q.max()), curve=sample(m))
        print('depth %4d: Mix %.4f .. %.4f' % (depth, q.min(), q.max()))
    save('depth', res)


def exp_shape():
    """the M1 curve against its table (512 values) over time, at base 50 depth 50 (no clip)"""
    p, raw = S.base()
    t0, t1 = S.tables(raw)
    m = lfo_mix(50.0, 50, secs=4.0, tag='shape')
    t = np.arange(len(m)) / FS
    res = dict(table0_first=[round(float(v), 5) for v in t0[:8]], curve=sample(m, 0.005))
    # compare with the table read as one cycle (first 256 or all 512 values), 0.5 Hz
    for n in (256, 512):
        ph = (t * 0.5) % 1.0
        pred = 0.5 + 0.5 * np.interp(ph * n, np.arange(n + 1), np.append(t0[:n], t0[0]))
        err = m[FS // 10:] - pred[FS // 10:]
        res['rms_err_table%d' % n] = float(np.sqrt(np.mean(err ** 2)))
        print('table read as %d per cycle: rms error %.5f' % (n, res['rms_err_table%d' % n]))
    save('shape', res)


def cycle_freq(m, lo=0.03, hi=40.0):
    """the curve's fundamental (Hz): the peak of its spectrum, refined by a parabola"""
    q = m[FS // 10:] - np.mean(m[FS // 10:])
    n = 1 << int(math.ceil(math.log2(len(q) * 8)))
    sp = np.abs(np.fft.rfft(q * np.hanning(len(q)), n)); f = np.fft.rfftfreq(n, 1 / FS)
    band = (f > lo) & (f < hi)
    i = np.argmax(np.where(band, sp, 0))
    a, b, c = np.log(sp[i - 1:i + 2] + 1e-30)
    return float(f[i] + 0.5 * (a - c) / (a - 2 * b + c) * (f[1] - f[0]))


def exp_rate():
    """Rate in Hz (Sync off: the state's Hz value) and synced (note lengths at 90 and 120 bpm)"""
    res = {'hz': {}, 'synced': {}}
    for hz in (0.25, 1.0, 3.7):
        m = lfo_mix(50.0, 50, extra_changes={S.M_SYNC: 1, S.M_HZ: hz}, secs=max(4.0, 3.0 / hz), tag='hz')
        res['hz'][str(hz)] = cycle_freq(m)
        print('Hz %.3f -> measured %.4f Hz' % (hz, res['hz'][str(hz)]))
    for note in (4, 8, 10, 13, 16, 18, 19, 21):
        for bpm in (90, 120):
            try:
                m = lfo_mix(50.0, 50, extra_changes={S.M_NOTE: note}, secs=8.0, tag='note', bpm=bpm)
            except RuntimeError as e:
                print('note %d: %s' % (note, e)); res['synced']['%d@%d' % (note, bpm)] = str(e); continue
            f = cycle_freq(m)
            res['synced']['%d@%d' % (note, bpm)] = f
            print('note %d at %d bpm -> %.4f Hz (%.4f beats per cycle)' % (note, bpm, f, bpm / 60.0 / f))
    save('rate', res)


FREE = lambda hz, k=0: {288 + k: 1, S.M_SYNC + k: 1, S.M_HZ + k: hz}     # a free rate in Hz (both flags; see RESEARCH.md)
RAMP = np.concatenate([np.linspace(-1.0, 1.0, 256, endpoint=False), np.zeros(256)]).astype(np.float32)
SQUARE = np.concatenate([np.ones(128), -np.ones(128), np.zeros(256)]).astype(np.float32)


def exp_phase():
    """Phase: the ramp's place in its cycle at the start of the render (1 s pre-roll before it), by Phase 0 / 0.25 / 0.5,
    free 1 Hz and synced (1 beat at 120 and 90 bpm)"""
    res = {}
    for name, ch, bpm in (('free1Hz', FREE(1.0), 120), ('beat120', {S.M_NOTE: 13}, 120), ('beat90', {S.M_NOTE: 13}, 90)):
        for ph in (0.0, 0.25, 0.5):
            m = lfo_mix(50.0, 50, extra_changes={**ch, S.M_PHASE: ph, S.M_SMOOTH: 1.0}, secs=3.0, shapes={0: RAMP}, tag='phase', bpm=bpm)
            res['%s_phase%g' % (name, ph)] = dict(start=sample(m[:FS // 5], 0.001), mid=sample(m[FS:FS + FS // 2], 0.002))
            print('%s phase %.2f: m at 0, 0.1 s, 1 s = %.4f %.4f %.4f' % (name, ph, m[WIN], m[FS // 10], m[FS]))
    save('phase', res)


def exp_warp():
    """Warp: the ramp's phase mapping within a cycle at Warp 0.1 / 0.5 / 1 / 2 / 10 (free 1 Hz, Smooth 1)"""
    res = {}
    for w in (1.0, 0.1, 0.5, 2.0, 10.0):
        m = lfo_mix(50.0, 50, extra_changes={**FREE(1.0), S.M_WARP: w, S.M_SMOOTH: 1.0}, secs=3.0, shapes={0: RAMP}, tag='warp')
        res[str(w)] = sample(m[FS:2 * FS], 0.002)
        print('warp %g: m at 0.1 .. 0.9 of a second: %s' % (w, ' '.join('%.3f' % m[FS + int(k * FS / 10)] for k in range(1, 10))))
    save('warp', res)


def exp_smooth():
    """Smooth: a square table (free 0.5 Hz) through Smooth 1 / 10 / 100 / 1000"""
    res = {}
    for sm in (1.0, 10.0, 100.0, 1000.0):
        m = lfo_mix(50.0, 50, extra_changes={**FREE(0.5), S.M_SMOOTH: sm}, secs=5.0, shapes={0: SQUARE}, tag='smooth')
        res[str(sm)] = sample(m, 0.0005)
        print('smooth %g: ' % sm + ' '.join('%.3f' % m[2 * FS + int(k * 0.002 * FS)] for k in (0, 1, 2, 5, 10, 25, 50, 100, 250)))
    save('smooth', res)


def exp_level():
    res = {}
    for lv in (1.0, 0.5, 0.25):
        m = lfo_mix(50.0, 50, extra_changes={**FREE(1.0), S.M_LEVEL: lv, S.M_SMOOTH: 1.0}, secs=3.0, tag='level')
        q = m[FS // 10:]
        res[str(lv)] = dict(min=float(q.min()), max=float(q.max()))
        print('level %g: Mix %.4f .. %.4f' % (lv, q.min(), q.max()))
    save('level', res)


def exp_seq():
    """the sequencer: Type Seq, the step values in the table's second half (value / 24 from place 256); 4 and 16
    steps, free 4 Hz (Rate is one step's time)"""
    res = {}
    vals = np.array([0.5, -0.25, 1.0, -1.0, 0.125, 0.0, -0.5, 0.75, 0.25, -0.75, 0.375, -0.125, 0.625, -0.375, 0.875, -0.625])
    table = np.concatenate([np.zeros(256), vals, np.zeros(256 - 16)]).astype(np.float32)
    for steps in (4, 16):
        m = lfo_mix(50.0, 50, extra_changes={**FREE(4.0), S.M_TYPE: 1, S.M_STEPS: steps, S.M_SMOOTH: 1.0}, secs=3.0, shapes={0: table}, tag='seq')
        res[str(steps)] = sample(m, 0.001)
        lv = [float(np.median(m[FS + int((k + 0.5) * FS / 4) - 200:FS + int((k + 0.5) * FS / 4) + 200])) for k in range(8)]
        res['levels%d' % steps] = lv
        print('steps %d: step levels %s' % (steps, ' '.join('%.4f' % v for v in lv)))
    save('seq', res)


def exp_am():
    """AM: 1 kHz bursts at -40, -20, -6 dBFS (0.5 s on, 0.5 s off) on noise-carried Mix; Attack / Release defaults
    and 1 / 100 ms"""
    res = {}
    t = np.arange(int(6 * FS)) / FS
    lv = np.repeat([-40, -20, -6, -30, -12, -3], FS)
    gate = ((t % 1.0) < 0.5).astype(np.float64)
    x = np.repeat((10 ** (lv / 20) * gate * np.sin(2 * np.pi * 1000 * t))[None, :].astype(np.float32), 2, 0)
    for att, rel in ((10.0, 100.0), (1.0, 100.0), (100.0, 10.0)):
        m = mix_curve(x, {S.MIX: 0.0, **S.slot('mix', 0, S.AM, 100), S.AM_ATTACK: att, S.AM_RELEASE: rel}, tag='am')
        res['%g_%g' % (att, rel)] = sample(m, 0.0005)
        print('att %g rel %g: held Mix per burst %s' % (att, rel, ' '.join('%.4f' % m[int((k + 0.45) * FS)] for k in range(6))))
    save('am', res)


def exp_pt():
    """PT: a sine stepping through pitches (110 .. 880 Hz), PT on Mix at base 50 depth 50; Offset 0 / 0.5"""
    res = {}
    f0 = np.repeat([110.0, 220.0, 440.0, 880.0, 165.0, 330.0], FS)
    ph = 2 * np.pi * np.cumsum(f0) / FS
    x = np.repeat((0.25 * np.sin(ph))[None, :].astype(np.float32), 2, 0)
    for off in (0.0, 0.5):
        m = mix_curve(x, {S.MIX: 50.0, **S.slot('mix', 0, S.PT, 50), S.PT_OFFSET: off, S.PT_SMOOTH: 1.0}, tag='pt')
        res[str(off)] = sample(m, 0.001)
        print('offset %g: held Mix per pitch %s' % (off, ' '.join('%.4f' % np.median(m[int((k + 0.5) * FS):int((k + 0.9) * FS)]) for k in range(6))))
    save('pt', res)


def inst_freq(y):
    from scipy.signal import hilbert
    a = hilbert(y.astype(np.float64))
    return np.diff(np.unwrap(np.angle(a))) * FS / (2 * np.pi)


def exp_pitchdepth():
    """M1 on Pitch: a 220 Hz sine, base 0, depth 25 / 50 / 100, free 0.5 Hz, Fine on (cents): the output's frequency"""
    res = {}
    x = np.repeat((0.25 * np.sin(2 * np.pi * 220 * np.arange(4 * FS) / FS))[None, :].astype(np.float32), 2, 0)
    for depth in (25, 50, 100):
        y = S.render(x, FS, {S.SEMITONES: 0, **S.slot('pitch', 0, S.M1, depth), **FREE(0.5), S.M_SMOOTH: 1.0}, tag='pdepth')
        f = inst_freq(y[0])
        k = np.ones(480) / 480
        st = 12 * np.log2(np.maximum(np.convolve(f, k, 'same'), 1.0) / 220.0)
        q = st[FS // 2:-FS // 10]
        res[str(depth)] = dict(min=float(np.percentile(q, 1)), max=float(np.percentile(q, 99)), curve=[round(float(v), 3) for v in st[::480]])
        print('depth %d: output %.2f .. %.2f semitones' % (depth, res[str(depth)]['min'], res[str(depth)]['max']))
    save('pitchdepth', res)


def exp_origin():
    """a free LFO's phase at the start of the input (after the 1 s pre-roll) at 0.7 Hz, and its lag at block 512 and
    256 (the ramp table, Smooth 1)"""
    res = {}
    for block in (512, 256):
        m = lfo_mix(50.0, 50, extra_changes={**FREE(0.7), S.M_SMOOTH: 1.0}, secs=3.0, shapes={0: RAMP}, tag='origin%d' % block)             if block == 512 else mix_curve(noise(3.0), {S.MIX: 50.0, **S.slot('mix', 0, S.M1, 50), **FREE(0.7), S.M_SMOOTH: 1.0},
                                           {0: RAMP}, 'origin256', extra=('block=256',))
        res[str(block)] = sample(m, 0.001)
        print('block %d: m at 0.1, 0.5, 1.0 s = %.4f %.4f %.4f (a phase from the processing start: %.4f %.4f %.4f)' % (
            block, m[FS // 10], m[FS // 2], m[FS], (1.7 * 0.7 + 0.0) % 1, (1.5 * 0.7) % 1, (2.0 * 0.7) % 1))
    save('origin', res)


EXPERIMENTS = {'origin': exp_origin, 'depth': exp_depth, 'shape': exp_shape, 'rate': exp_rate, 'phase': exp_phase, 'warp': exp_warp,
               'smooth': exp_smooth, 'level': exp_level, 'seq': exp_seq, 'am': exp_am, 'pt': exp_pt,
               'pitchdepth': exp_pitchdepth}

if __name__ == '__main__':
    names = sys.argv[1:] or ['all']
    for n in (EXPERIMENTS if names == ['all'] else names):
        print('==', n, flush=True)
        EXPERIMENTS[n]()
