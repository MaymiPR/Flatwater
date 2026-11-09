"""Flatwater Pitch Bend, Phase 3: our modulators against the reference's, through Mix (MEASUREMENTS.md, Flatwater
Pitch Bend). Mix is an exact crossfade on both sides, so a modulator on Mix reads back as m(t), the Mix applied, sample
by sample: the reference's with Pitch +12 (its wet far from its dry), ours at no shift (our wet is the input 105.53
samples later, far enough from the dry on noise). Each case: the rms and the largest difference of the two m(t) after
0.1 s, over the whole curve and away from its jumps (more than 600 samples from a step or a wrap, where both sides'
readings are blurred by the reading window); an exact law is judged away from the jumps (target 0.003 rms), a fitted
one over the whole curve (0.01).

    python -I null_mods.py [case ...]        -> measurements/fwpitchbend/mods/null.json and a table
"""
import sys, os, json, subprocess
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import numpy as np
import vb as H
import editor_state as S
import measure_mods as M

FS = 48000
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'measurements', 'fwpitchbend', 'mods', 'null.json')


def ours(x, keys, tables=None):
    """our m(t): our render with the keys (and tables) against ours with Mix 0 / 100 (no shift)"""
    extra = []
    for k, t in (tables or {}).items():
        f = os.path.join(H.TMP, 'table%d_%d.f32' % (k, os.getpid()))
        np.asarray(t, '<f4').tofile(f)
        extra.append('table=%d:%s' % (k, f))
    def render(kv):
        fi, fo = os.path.join(H.TMP, 'nin%d.f32' % os.getpid()), os.path.join(H.TMP, 'nout%d.f32' % os.getpid())
        x.T.astype(np.float32).tofile(fi)
        args = [H.RENDER, fi, fo, str(x.shape[0]), str(FS), 'preroll=1'] + ['%s=%s' % (('slot' if k_.startswith('slot') else k_), v_) for k_, v_ in kv.items()] + extra
        r = subprocess.run(args, capture_output=True, text=True)
        if r.returncode: raise RuntimeError(r.stderr)
        return np.fromfile(fo, np.float32).reshape(-1, x.shape[0]).T
    base = {k: v for k, v in keys.items() if not k.startswith('slot')}
    wet = render({**base, 'mix': 100, 'bpm': keys.get('bpm', 120)})
    out = render(keys)
    d = (wet - x).astype(np.float64); e = (out - x).astype(np.float64)
    k = np.ones(M.WIN)
    num = sum(np.convolve(d[c] * e[c], k, 'same') for c in range(d.shape[0]))
    den = sum(np.convolve(d[c] * d[c], k, 'same') for c in range(d.shape[0]))
    return num / np.maximum(den, 1e-20)


SINE = np.concatenate([np.sin(2 * np.pi * np.arange(256) / 256), np.zeros(256)])

# case: (class, reference state changes, our keys, tables, input secs, bpm)
CASES = {
    'depth25':   ('exact', {S.MIX: 50.0, **S.slot('mix', 0, S.M1, 25)}, {'mix': 50, 'slot': 'mix:0:1:25'}, None, 4.0, 120),
    'shape':     ('exact', {S.MIX: 50.0, **S.slot('mix', 0, S.M1, 50), S.M_SMOOTH: 1.0}, {'mix': 50, 'slot': 'mix:0:1:50', 'm1Smooth': 1}, None, 4.0, 120),
    'free3.7Hz': ('exact', {S.MIX: 50.0, **S.slot('mix', 0, S.M1, 50), **M.FREE(3.7), S.M_SMOOTH: 1.0},
                  {'mix': 50, 'slot': 'mix:0:1:50', 'm1Sync': 1, 'm1Rate': 3.7, 'm1Smooth': 1}, None, 3.0, 120),
    'beat90':    ('exact', {S.MIX: 50.0, **S.slot('mix', 0, S.M1, 50), S.M_NOTE: 13, S.M_SMOOTH: 1.0},
                  {'mix': 50, 'slot': 'mix:0:1:50', 'm1RateSynced': 10, 'm1Smooth': 1, 'bpm': 90}, None, 3.0, 90),
    'phase.25':  ('exact', {S.MIX: 50.0, **S.slot('mix', 0, S.M1, 50), **M.FREE(1.0), S.M_PHASE: 0.25, S.M_SMOOTH: 1.0},
                  {'mix': 50, 'slot': 'mix:0:1:50', 'm1Sync': 1, 'm1Rate': 1.0, 'm1Phase': 0.25, 'm1Smooth': 1}, {0: M.RAMP}, 3.0, 120),
    'warp0.1':   ('exact', {S.MIX: 50.0, **S.slot('mix', 0, S.M1, 50), **M.FREE(1.0), S.M_WARP: 0.1, S.M_SMOOTH: 1.0},
                  {'mix': 50, 'slot': 'mix:0:1:50', 'm1Sync': 1, 'm1Rate': 1.0, 'm1Warp': 0.1, 'm1Smooth': 1}, {0: M.RAMP}, 3.0, 120),
    'level0.5':  ('exact', {S.MIX: 50.0, **S.slot('mix', 0, S.M1, 50), **M.FREE(1.0), S.M_LEVEL: 0.5, S.M_SMOOTH: 1.0},
                  {'mix': 50, 'slot': 'mix:0:1:50', 'm1Sync': 1, 'm1Rate': 1.0, 'm1Level': 0.5, 'm1Smooth': 1}, None, 3.0, 120),
    'seq4':      ('exact', {S.MIX: 50.0, **S.slot('mix', 0, S.M1, 50), **M.FREE(4.0), S.M_TYPE: 1, S.M_STEPS: 4, S.M_SMOOTH: 1.0},
                  {'mix': 50, 'slot': 'mix:0:1:50', 'm1Sync': 1, 'm1Rate': 4.0, 'm1Type': 1, 'm1Steps': 4, 'm1Smooth': 1},
                  {0: np.concatenate([np.zeros(256), [0.5, -0.25, 1.0, -1.0], np.zeros(252)])}, 3.0, 120),
    'smooth100': ('fitted', {S.MIX: 50.0, **S.slot('mix', 0, S.M1, 50), **M.FREE(0.5), S.M_SMOOTH: 100.0},
                  {'mix': 50, 'slot': 'mix:0:1:50', 'm1Sync': 1, 'm1Rate': 0.5, 'm1Smooth': 100}, {0: M.SQUARE}, 5.0, 120),
    'am':        ('fitted', {S.MIX: 0.0, **S.slot('mix', 0, S.AM, 100)}, {'mix': 0, 'slot': 'mix:0:3:100'}, None, 'bursts', 120),
}
CASES['am_gate'] = ('fitted', {S.MIX: 0.0, **S.slot('mix', 0, S.AM, 100)}, {'mix': 0, 'slot': 'mix:0:3:100'}, None, 'gaps', 120)
# PT on Mix (base 50, depth 50: m = 0.5 + 0.5 PT): a glide (exact scale and timing), a sung and a spoken take (the
# detectors are each their own: the sung take is judged, the spoken one reported, where the two disagree on octaves)
for _n, _cls in (('glide', 'fitted'), ('sung', 'fitted'), ('spoken', 'report')):
    CASES['pt_' + _n] = (_cls, {S.MIX: 50.0, **S.slot('mix', 0, S.PT, 50), S.PT_SMOOTH: 1.0},
                         {'mix': 50, 'slot': 'mix:0:4:50', 'ptSmooth': 1}, None, _n, 120)
# a modulator on M1's own controls: M2 a sequence (constant +1, or two steps +1 / -1) on M1's Level, Rate (free and
# synced), Warp and Phase, read through M1 on Mix (base 50, depth 50)
_ONE = np.concatenate([np.zeros(256), np.ones(256)])
_TWO = np.concatenate([np.zeros(256), np.tile([1.0, -1.0], 128)])
_M2SEQ = {S.M_TYPE + 1: 1, S.M_STEPS + 1: 4, S.M_SMOOTH + 1: 1.0}
_M2KEYS = {'m2Type': 1, 'm2Steps': 4, 'm2Smooth': 1}
_MIX = {S.MIX: 50.0, **S.slot('mix', 0, S.M1, 50), S.M_SMOOTH: 1.0}
_MIXK = {'mix': 50, 'slot': 'mix:0:1:50', 'm1Smooth': 1}
CASES.update({
    'mod_level':    ('exact', {**_MIX, **M.FREE(1.0), S.M_LEVEL: 0.5, **_M2SEQ, **S.slot('m_level', 0, S.M2, 25)},
                     {**_MIXK, 'm1Sync': 1, 'm1Rate': 1.0, 'm1Level': 0.5, **_M2KEYS, 'slot2': 'm1Level:0:2:25'}, {1: _ONE}, 3.0, 120),
    'mod_ratefree': ('exact', {**_MIX, **M.FREE(1.0), **_M2SEQ, **S.slot('m_rate', 0, S.M2, 10)},
                     {**_MIXK, 'm1Sync': 1, 'm1Rate': 1.0, **_M2KEYS, 'slot2': 'm1Rate:0:2:10'}, {1: _ONE}, 3.0, 120),
    'mod_ratesync': ('exact', {**_MIX, S.M_NOTE: 13, **_M2SEQ, **S.slot('m_rate', 0, S.M2, 10)},
                     {**_MIXK, 'm1RateSynced': 10, **_M2KEYS, 'slot2': 'm1Rate:0:2:10'}, {1: _ONE}, 3.0, 120),
    'mod_warp':     ('exact', {**_MIX, **M.FREE(1.0), **_M2SEQ, **S.slot('m_warp', 0, S.M2, 10)},
                     {**_MIXK, 'm1Sync': 1, 'm1Rate': 1.0, **_M2KEYS, 'slot2': 'm1Warp:0:2:10'}, {0: M.RAMP, 1: _ONE}, 3.0, 120),
    'mod_phase':    ('exact', {**_MIX, **M.FREE(1.0), **_M2SEQ, **S.slot('m_phase', 0, S.M2, 25)},
                     {**_MIXK, 'm1Sync': 1, 'm1Rate': 1.0, **_M2KEYS, 'slot2': 'm1Phase:0:2:25'}, {0: M.RAMP, 1: _ONE}, 3.0, 120),
    'mod_levelstep': ('exact', {**_MIX, **M.FREE(1.0), S.M_LEVEL: 0.5, **_M2SEQ, **M.FREE(2.0, 1), S.M_STEPS + 1: 2,
                                **S.slot('m_level', 0, S.M2, 25)},
                      {**_MIXK, 'm1Sync': 1, 'm1Rate': 1.0, 'm1Level': 0.5, **_M2KEYS, 'm2Steps': 2, 'm2Sync': 1, 'm2Rate': 2.0,
                       'slot2': 'm1Level:0:2:25'}, {1: _TWO}, 3.0, 120),
    'mod_ratestep_sync': ('exact', {**_MIX, S.M_NOTE: 13, **_M2SEQ, **M.FREE(2.0, 1), S.M_STEPS + 1: 2,
                                    **S.slot('m_rate', 0, S.M2, 10)},
                          {**_MIXK, 'm1RateSynced': 10, **_M2KEYS, 'm2Steps': 2, 'm2Sync': 1, 'm2Rate': 2.0,
                           'slot2': 'm1Rate:0:2:10'}, {1: _TWO}, 3.0, 120),
    'mod_ratestep_free': ('exact', {**_MIX, **M.FREE(1.0), **_M2SEQ, **M.FREE(2.0, 1), S.M_STEPS + 1: 2,
                                    **S.slot('m_rate', 0, S.M2, 10)},
                          {**_MIXK, 'm1Sync': 1, 'm1Rate': 1.0, **_M2KEYS, 'm2Steps': 2, 'm2Sync': 1, 'm2Rate': 2.0,
                           'slot2': 'm1Rate:0:2:10'}, {1: _TWO}, 3.0, 120),
    'mod_warpstep':  ('exact', {**_MIX, **M.FREE(1.0), **_M2SEQ, **M.FREE(2.0, 1), S.M_STEPS + 1: 2, **S.slot('m_warp', 0, S.M2, 25)},
                      {**_MIXK, 'm1Sync': 1, 'm1Rate': 1.0, **_M2KEYS, 'm2Steps': 2, 'm2Sync': 1, 'm2Rate': 2.0,
                       'slot2': 'm1Warp:0:2:25'}, {0: M.RAMP, 1: _TWO}, 3.0, 120),
    'mod_phasestep': ('exact', {**_MIX, **M.FREE(1.0), **_M2SEQ, **M.FREE(2.0, 1), S.M_STEPS + 1: 2, **S.slot('m_phase', 0, S.M2, 25)},
                      {**_MIXK, 'm1Sync': 1, 'm1Rate': 1.0, **_M2KEYS, 'm2Steps': 2, 'm2Sync': 1, 'm2Rate': 2.0,
                       'slot2': 'm1Phase:0:2:25'}, {0: M.RAMP, 1: _TWO}, 3.0, 120),
    'mod_amlevel':   ('fitted', {S.MIX: 0.0, **S.slot('mix', 0, S.AM, 100), **_M2SEQ, **S.slot('am_level', 0, S.M2, -10)},
                      {'mix': 0, 'slot': 'mix:0:3:100', **_M2KEYS, 'slot2': 'amLevel:0:2:-10'}, {1: _ONE}, 'steps', 120),
    'mod_ptlevel':   ('fitted', {S.MIX: 50.0, **S.slot('mix', 0, S.PT, 50), S.PT_SMOOTH: 1.0, **_M2SEQ, **S.slot('pt_level', 0, S.M2, -10)},
                      {'mix': 50, 'slot': 'mix:0:4:50', 'ptSmooth': 1, **_M2KEYS, 'slot2': 'ptLevel:0:2:-10'}, {1: _ONE}, 'glide', 120),
    'seq_warp':      ('exact', {**_MIX, **M.FREE(4.0), S.M_TYPE: 1, S.M_STEPS: 4, S.M_WARP: 0.53},
                      {**_MIXK, 'm1Sync': 1, 'm1Rate': 4.0, 'm1Type': 1, 'm1Steps': 4, 'm1Warp': 0.53},
                      {0: np.concatenate([np.zeros(256), [0.5, -0.25, 1.0, -1.0], np.zeros(252)])}, 3.0, 120),
    'smooth10':      ('fitted', {**_MIX, **M.FREE(0.5), S.M_SMOOTH: 10.0}, {**_MIXK, 'm1Sync': 1, 'm1Rate': 0.5, 'm1Smooth': 10},
                      {0: M.SQUARE}, 5.0, 120),
    'smooth1000':    ('fitted', {**_MIX, **M.FREE(0.5), S.M_SMOOTH: 1000.0}, {**_MIXK, 'm1Sync': 1, 'm1Rate': 0.5, 'm1Smooth': 1000},
                      {0: M.SQUARE}, 5.0, 120),
    'mod_amattack':  ('fitted', {S.MIX: 0.0, **S.slot('mix', 0, S.AM, 100), **_M2SEQ, **S.slot('am_attack', 0, S.M2, 25)},
                      {'mix': 0, 'slot': 'mix:0:3:100', **_M2KEYS, 'slot2': 'amAttack:0:2:25'}, {1: _ONE}, 'steps', 120),
    'mod_amrelease': ('fitted', {S.MIX: 0.0, **S.slot('mix', 0, S.AM, 100), **_M2SEQ, **S.slot('am_release', 0, S.M2, -25)},
                      {'mix': 0, 'slot': 'mix:0:3:100', **_M2KEYS, 'slot2': 'amRelease:0:2:-25'}, {1: _ONE}, 'steps', 120),
    'pt_smooth300':  ('fitted', {S.MIX: 50.0, **S.slot('mix', 0, S.PT, 50), S.PT_SMOOTH: 300.0},
                      {'mix': 50, 'slot': 'mix:0:4:50', 'ptSmooth': 300}, None, 'glide', 120),
})
# AM on a sine whose level steps but never stops (so the release reads too), at attack / release pairs
for _a, _r in ((10, 100), (1, 1000), (100, 10), (1000, 1), (30, 300), (300, 1)):
    CASES['am%g_%g' % (_a, _r)] = ('fitted', {S.MIX: 0.0, **S.slot('mix', 0, S.AM, 100), S.AM_ATTACK: float(_a), S.AM_RELEASE: float(_r)},
                                   {'mix': 0, 'slot': 'mix:0:3:100', 'amAttack': _a, 'amRelease': _r}, None, 'steps', 120)
# AM on the spoken take at a fast preset's attack and release (Vinyl Scrub: 6.3 / 26.3 ms)
CASES['am_spoken'] = ('fitted', {S.MIX: 0.0, **S.slot('mix', 0, S.AM, 100), S.AM_ATTACK: 6.3, S.AM_RELEASE: 26.3},
                      {'mix': 0, 'slot': 'mix:0:3:100', 'amAttack': 6.3, 'amRelease': 26.3}, None, 'spoken', 120)


def bursts():
    t = np.arange(int(6 * FS)) / FS
    lv = np.repeat([-40, -20, -6, -30, -12, -3], FS)
    return np.repeat((10 ** (lv / 20) * ((t % 1.0) < 0.5) * np.sin(2 * np.pi * 1000 * t))[None, :].astype(np.float32), 2, 0)


def gaps():
    """a 1 kHz sine at 0.3 with silent gaps of 2 ms .. 0.3 s, then over floors of 3e-5 .. 1e-3 (the AM gate)"""
    segs = [(0.3, 0.3)] + sum([[(0.0, g), (0.3, 0.3)] for g in (0.002, 0.02, 0.1, 0.12, 0.3)], [])
    segs += sum([[(f, 0.3), (0.3, 0.3)] for f in (3e-5, 4e-4, 6e-4, 1e-3)], [])
    a = np.concatenate([np.full(int(d * FS), v) for v, d in segs])
    return np.repeat((a * np.sin(2 * np.pi * 1000 * np.arange(len(a)) / FS))[None, :], 2, 0).astype(np.float32)


def glide():
    t = np.arange(3 * FS) / FS
    f = 80.0 * (1000.0 / 80.0) ** (t / 3.0)
    return np.repeat((0.3 * np.sin(2 * np.pi * np.cumsum(f) / FS))[None, :], 2, 0).astype(np.float32)


def take(group, secs=6.0):
    """the corpus's phrases of one kind strung to secs, -18 dBFS rms (measure_shifter.corpus_phrases)"""
    import measure_shifter as MS
    parts, n = [], 0
    for _, _, ph in MS.corpus_phrases(1.0, group=group):
        parts += [ph, np.zeros(FS // 5)]; n += len(ph) + FS // 5
        if n > secs * FS: break
    if not parts: raise SystemExit('the vocal corpus is not decoded (scripts/nulltest/vocal_corpus.py)')
    x = np.concatenate(parts)[:int(secs * FS)]
    x *= 10 ** (-18 / 20) / np.sqrt(np.mean(x[np.abs(x) > 1e-4] ** 2))
    return np.repeat(x[None, :].astype(np.float32), 2, 0)


def steps():
    a = np.repeat([0.02, 0.4, 0.02, 0.1, 0.02, 0.6, 0.1, 0.4, 0.02], FS // 2)
    return np.repeat((a * np.sin(2 * np.pi * 1000 * np.arange(len(a)) / FS))[None, :], 2, 0).astype(np.float32)


def run(name):
    cls, ref_changes, keys, tables, secs, bpm = CASES[name]
    signals = {'bursts': bursts, 'steps': steps, 'gaps': gaps, 'glide': glide, 'sung': lambda: take('sung'), 'spoken': lambda: take('rap')}
    x = signals[secs]() if secs in signals else M.noise(secs)
    shapes = {k: np.asarray(v, np.float32) for k, v in (tables or {}).items()}
    if 'slot' in keys:
        keys = dict(keys)
    keys.setdefault('bpm', bpm)
    m_ref = M.mix_curve(x, ref_changes, shapes or None, 'null_' + name, bpm)
    m_ours = ours(x, keys, shapes)
    a = FS // 10
    ok = np.isfinite(m_ref) & np.isfinite(m_ours)
    if secs in ('gaps', 'sung', 'spoken'):                        # (no reading in the gaps)
        on = (np.convolve(np.abs(x[0]) > 1e-4, np.ones(49), 'same') > 0).astype(float)
        ok &= np.convolve(on, np.ones(2 * M.WIN + 1), 'same') >= 2 * M.WIN + 1 - 0.5
    if secs == 'bursts':                                          # (between the bursts there is no wet to read m from, and
        on = (np.convolve(np.abs(x[0]) > 1e-4, np.ones(49), 'same') > 0).astype(float)   # at their edges the reading
                                                                  # window is half empty)
        ok &= np.convolve(on, np.ones(2 * M.WIN + 1), 'same') >= 2 * M.WIN + 1 - 0.5
    ok[:a] = False
    ok &= m_ref < 0.995                                           # (Mix clipped at 100: no reading of the modulator)
    d = (m_ours - m_ref)[ok]
    jumps = np.concatenate([[False], np.abs(np.diff(m_ref)) > 0.02])
    away = ok & ~(np.convolve(jumps.astype(float), np.ones(1201), 'same') > 0)
    da = (m_ours - m_ref)[away]
    res = dict(cls=cls, rms=float(np.sqrt(np.mean(d * d))), max=float(np.max(np.abs(d))),
               rms_away=float(np.sqrt(np.mean(da * da))) if da.size else float('nan'),
               target=0.003 if cls == 'exact' else 0.01)
    res['pass'] = (res['rms_away'] if cls == 'exact' else res['rms']) <= res['target'] if cls != 'report' else None
    return res


def main():
    names = sys.argv[1:] or list(CASES)
    out = json.load(open(OUT)) if os.path.exists(OUT) else {}
    for n in names:
        out[n] = run(n)
        r = out[n]
        print('%-10s %-6s rms %.4f (away from jumps %.4f) max %.4f, target %.3f %s' % (n, r['cls'], r['rms'], r['rms_away'], r['max'], r['target'], {True: 'PASS', False: 'FAIL', None: 'report'}[r['pass']]), flush=True)
    json.dump(out, open(OUT, 'w'), indent=1)


if __name__ == '__main__':
    main()
