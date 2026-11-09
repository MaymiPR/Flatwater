"""Flatwater Pitch Bend, Phase 1 track B2: the reference's pitch / formant shifter measured as behaviour, not as a null.
(A pitch shifter does not null sample for sample; these are the numbers our own shifter is judged by.)

    python -I -B measure_shifter.py <command> [...]

Commands (each writes JSON / PNG to measurements/fwpitchbend/shifter/):
    noshift     what the wet path does at Pitch 0 / Formant 0: delay, fractional delay, residual, on several inputs
    structure   pulse trains, clicks and a sweep at no shift and shifted: where the output's pulses are
    pitch       pitch accuracy: vowels and corpus voice at f0 80 .. 600 Hz, semitone and cents shifts, settling
    formant     the spectral envelope under Formant and under Pitch
    level       output level against input level per setting and material
    timing      onset delay of the wet path, voiced and unvoiced, against f0
    unvoiced    noise bursts, sibilant bands, a chord, silence
    flatten     Flatten at a few notes: output f0 against the note, lock time, Pitch on top
    all         every command in turn

Renders are cached in nulltest/renders/pbshift/ (git-ignored) by a hash of the input and the settings, so a re-run
only renders what is new. The reference is rendered in a fresh instance per render (vb.render_ref).
"""
import sys, os, json, hashlib, time
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import numpy as np
from concurrent.futures import ThreadPoolExecutor
from scipy.signal import lfilter, butter, sosfiltfilt, correlate
import vb as H

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, '..', '..', '..', 'measurements', 'fwpitchbend', 'shifter')
CACHE = os.path.join(H.TMP, 'pbshift')
os.makedirs(OUT, exist_ok=True); os.makedirs(CACHE, exist_ok=True)
FS = 48000

# the preset array's places (editor_state.py; repeated here so a cached run need not load the reference's state)
PITCH, FORMANT, MIX, SEMITONES, FLATTEN_NOTE, FLATTEN_ON = 1372, 1373, 1456, 1484, 1495, 1497


# ---- rendering ------------------------------------------------------------------------------------------------------
def _key(x, fs, state, params):
    h = hashlib.sha1(np.ascontiguousarray(x, np.float32).tobytes())
    h.update(repr((fs, sorted((state or {}).items()), sorted((params or {}).items()))).encode())
    return h.hexdigest()[:20]


def job(x, fs=FS, state=None, params=None):
    """one render request: x mono (n,) or (1, n); state {place: value} through the saved state; params host params"""
    x = np.asarray(x, np.float32).reshape(1, -1)
    return dict(x=x, fs=fs, state=state, params=params, key=_key(x, fs, state, params))


def _render_one(j):
    f = os.path.join(CACHE, j['key'] + '.npy')
    if os.path.exists(f): return np.load(f)
    extra = ('preroll=1',)
    if j['state']:
        extra = ('preroll=1', 'state=' + j['statefile'], 'bpm=120')
    for attempt in range(3):
        try:
            y = H.render_ref(j['x'], j['fs'], j['params'], extra=extra)[0]
            break
        except Exception:
            if attempt == 2: raise
            time.sleep(1)
    np.save(f, y.astype(np.float32))
    return y


def render_all(jobs, workers=4):
    """render a list of jobs (cached), in parallel; returns the outputs (mono float32 arrays) in order"""
    todo = [j for j in jobs if not os.path.exists(os.path.join(CACHE, j['key'] + '.npy'))]
    if todo and any(j['state'] for j in todo):
        import editor_state as S                                    # (loads the reference once to write the state files)
        for j in todo:
            if j['state']: j['statefile'] = S.state_file(j['state'], 'pbs_' + j['key'], H.COMPONENTS[1].split('@')[0], check=False)
    if todo: print('rendering %d of %d' % (len(todo), len(jobs)), flush=True)
    t0 = time.time()
    with ThreadPoolExecutor(workers) as ex:
        list(ex.map(_render_one, todo))
    if todo: print('  %.0f s' % (time.time() - t0), flush=True)
    return [np.load(os.path.join(CACHE, j['key'] + '.npy')).astype(np.float64) for j in jobs]


def st_job(x, pitch_c=0.0, formant_c=0.0, fs=FS, mix=100, flatten=None, extra=None):
    """the reference with Fine on and Pitch / Formant in cents (through the state); flatten: a note 0 .. 23"""
    s = {SEMITONES: 0, PITCH: float(pitch_c), FORMANT: float(formant_c), MIX: float(mix)}
    if flatten is not None: s.update({FLATTEN_ON: 1, FLATTEN_NOTE: int(flatten)})
    if extra: s.update(extra)
    return job(x, fs, state=s)


def semi_job(x, pitch_st=0, formant_st=0, fs=FS):
    """the reference with Fine off and whole semitones through the host parameters"""
    return job(x, fs, params={'Pitch:': (pitch_st + 12) / 24.0, 'Formant:': (formant_st + 12) / 24.0})


# ---- signals ----------------------------------------------------------------------------------------------------
def vowel(secs, f0, db=-18.0, fs=FS, vib=0.0, f0_end=None, formants=((800, 80, 1.0), (1150, 90, 0.5), (2900, 120, 0.25))):
    """vb.vowel's /a/ stand-in, mono; f0 may glide (exponentially) to f0_end; vib cents at 5 Hz"""
    n = int(secs * fs)
    t = np.arange(n) / fs
    f = f0 * (f0_end / f0) ** (t / secs) if f0_end else np.full(n, float(f0))
    f = f * 2 ** (vib / 1200 * np.sin(2 * np.pi * 5 * t))
    ph = np.cumsum(f) / fs
    src = np.diff(np.floor(ph), prepend=0.0)
    y = np.zeros(n)
    for fc, bw, g in formants:
        r = np.exp(-np.pi * bw / fs)
        y += g * lfilter([1 - r], [1, -2 * r * np.cos(2 * np.pi * fc / fs), r * r], src)
    y *= 10 ** (db / 20) / (np.sqrt(np.mean(y ** 2)) + 1e-30)
    return y, f


def pulses(secs, period, fs=FS):
    n = int(secs * fs)
    x = np.zeros(n)
    x[np.round(np.arange(period, n - 1, period)).astype(int)] = 0.25
    return x


def noise(secs, db=-18.0, fs=FS, seed=7):
    return np.random.default_rng(seed).standard_normal(int(secs * fs)) * 10 ** (db / 20)


def band(x, lo, hi, fs=FS):
    return sosfiltfilt(butter(4, [lo, hi], 'bandpass', fs=fs, output='sos'), x)


def corpus_group(fname):
    """'sung' (the Laurie Webb phrases), 'rap' (FLS Freestyle / spoken word / DJ), 'processed' (the VOC phrases:
    effected or noisy voice, mostly aperiodic to YIN)"""
    b = os.path.basename(fname)
    if b.startswith('Laurie Webb'): return 'sung'
    if b.startswith('FLS_') and 'ocoder' not in b: return 'rap'
    return 'processed'


def corpus_phrases(min_secs=1.0, k=None, group=None):
    """the decoded FL Studio vocal phrases (read-only cache), longest first: (name, group, x); [] when the cache is
    not there. The cache lives in the main checkout's reference/ (git-ignored); a worktree looks three levels up."""
    for c in (os.path.join(HERE, '..', '..', '..', 'reference', 'fwvocalleveler', 'samples'),
              os.path.join(HERE, '..', '..', '..', '..', '..', '..', 'reference', 'fwvocalleveler', 'samples')):
        if os.path.exists(os.path.join(c, 'index.json')):
            idx = json.load(open(os.path.join(c, 'index.json')))
            names = sorted((v['secs'], n) for n, v in idx.items() if v['secs'] >= min_secs and (group is None or corpus_group(v['file']) == group))[::-1]
            out = [(n, corpus_group(idx[n]['file']), np.fromfile(os.path.join(c, n + '.f32'), np.float32).astype(np.float64)) for s, n in names]
            return out[:k] if k else out
    return []


# ---- measures ---------------------------------------------------------------------------------------------------
def yin(x, fs=FS, fmin=60.0, fmax=1000.0, hop=240, win=None, thr=0.12):
    """YIN f0 track: (times, f0 Hz or nan when unvoiced, aperiodicity). Cumulative-mean-normalised difference with
    parabolic interpolation; win defaults to 2.5 periods of fmin."""
    x = np.asarray(x, np.float64)
    tmax = int(fs / fmin) + 2; tmin = max(2, int(fs / fmax))
    W = win or int(2.5 * fs / fmin)
    starts = np.arange(0, len(x) - W - tmax, hop)
    f0 = np.full(len(starts), np.nan); ap = np.ones(len(starts))
    for i, s in enumerate(starts):
        fr = x[s:s + W + tmax]
        e = fr[:W]
        if np.dot(e, e) < 1e-12: continue
        # difference function via FFT
        N = 1 << int(np.ceil(np.log2(2 * (W + tmax))))
        F = np.fft.rfft(fr, N); G = np.fft.rfft(e, N)
        cc = np.fft.irfft(np.conj(G) * F, N)[:tmax + 1]
        c2 = np.concatenate([[0], np.cumsum(fr ** 2)])
        en = c2[np.arange(tmax + 1) + W] - c2[np.arange(tmax + 1)]
        d = np.dot(e, e) + en - 2 * cc
        d[0] = 0
        cm = d[1:] * np.arange(1, tmax + 1) / np.maximum(np.cumsum(d[1:]), 1e-30)
        cm = np.concatenate([[1], cm])
        gmin = float(np.min(cm[tmin:tmax]))
        cand = np.where(cm[tmin:tmax] < min(thr, gmin + 0.05))[0]        # (the first dip near the deepest: no octave-up errors)
        if len(cand):
            t = cand[0] + tmin
            while t + 1 < tmax and cm[t + 1] < cm[t]: t += 1
        else:
            t = int(np.argmin(cm[tmin:tmax])) + tmin
        ap[i] = cm[t]
        if cm[t] > 0.35 or t >= tmax - 1: continue
        a, b, c = cm[t - 1], cm[t], cm[t + 1]
        den = a - 2 * b + c
        dt = 0.5 * (a - c) / den if abs(den) > 1e-12 else 0.0
        f0[i] = fs / (t + dt)
    return (starts + W / 2) / fs, f0, ap


def cents(a, b):
    return 1200 * np.log2(np.asarray(a) / np.asarray(b))


def stats(c):
    c = np.asarray(c, np.float64); c = c[np.isfinite(c)]
    if len(c) == 0: return dict(n=0)
    a = np.abs(c)
    return dict(n=int(len(c)), median=round(float(np.median(c)), 2), abs_p50=round(float(np.percentile(a, 50)), 2),
                abs_p90=round(float(np.percentile(a, 90)), 2), mean=round(float(np.mean(c)), 2))


def xcorr_lag(y, x, maxlag=2000):
    """the lag (samples, parabolic) at which y best matches x delayed, and the normalised peak"""
    cc = correlate(y, x, 'full', method='fft'); lags = np.arange(-len(x) + 1, len(y))
    m = (lags >= -maxlag) & (lags <= maxlag)
    c = cc[m]; L = lags[m]; i = int(np.argmax(c))
    d = 0.0
    if 0 < i < len(c) - 1:
        den = c[i - 1] - 2 * c[i] + c[i + 1]
        d = 0.5 * (c[i - 1] - c[i + 1]) / den if den else 0.0
    return float(L[i] + d), float(c[i] / np.sqrt(np.dot(x, x) * np.dot(y, y) + 1e-30))


def frac_delay(x, d):
    """x delayed by d samples (fractional, FFT)"""
    n = len(x); N = 1 << int(np.ceil(np.log2(n + abs(d) + 16)))
    X = np.fft.rfft(x, N); w = np.fft.rfftfreq(N) * 2 * np.pi
    return np.fft.irfft(X * np.exp(-1j * w * d), N)[:n]


def save(name, obj):
    with open(os.path.join(OUT, name + '.json'), 'w') as f: json.dump(obj, f, indent=1)
    print('wrote', name + '.json', flush=True)


def r(v, k=3):
    return None if v is None or not np.isfinite(v) else round(float(v), k)


def resonator_db(f, formants=((800, 80, 1.0), (1150, 90, 0.5), (2900, 120, 0.25)), fs=FS):
    """the vowel stand-in's envelope (dB) at frequencies f: the three resonators' summed response"""
    z = np.exp(-2j * np.pi * np.asarray(f, np.float64) / fs)
    h = 0
    for fc, bw, g in formants:
        rr = np.exp(-np.pi * bw / fs)
        h = h + g * (1 - rr) / (1 - 2 * rr * np.cos(2 * np.pi * fc / fs) * z + rr * rr * z * z)
    return 20 * np.log10(np.abs(h) + 1e-30)


def harmonics_db(y, f0, fs=FS, fmax=5000.0):
    """amplitudes (dB) of the harmonics k f0 of a steady segment y (least squares on cos / sin at each harmonic)"""
    n = len(y); t = np.arange(n) / fs
    ks = np.arange(1, int(fmax / f0) + 1)
    w = np.hanning(n)
    out = []
    for k in ks:
        e = np.exp(-2j * np.pi * k * f0 * t)
        out.append(2 * np.abs(np.sum(w * y * e)) / np.sum(w))
    return ks * f0, 20 * np.log10(np.array(out) + 1e-30)


def ltas_db(y, fs=FS, nfft=4096, smooth_oct=1 / 6, fgrid=None):
    """long-term average spectrum (dB), smoothed over smooth_oct octaves, on fgrid (log-spaced 100 .. 8000 Hz)"""
    from scipy.signal import welch
    f, P = welch(y, fs=fs, nperseg=nfft)
    fgrid = np.geomspace(100, 8000, 200) if fgrid is None else fgrid
    out = np.empty(len(fgrid))
    for i, fc in enumerate(fgrid):
        m = (f >= fc * 2 ** (-smooth_oct / 2)) & (f <= fc * 2 ** (smooth_oct / 2))
        out[i] = 10 * np.log10(np.mean(P[m]) + 1e-30)
    return fgrid, out


def env_shift_fit(f_out, db_out, env_in, lo=200.0, hi=4500.0, alphas=None):
    """the factor alpha with db_out(f) ~ env_in(f / alpha) + g best (rms dB over lo .. hi): returns alpha, rms error dB,
    and the rms error at alpha = 1 (preserved); env_in a callable (dB at frequency)"""
    alphas = np.geomspace(0.45, 2.2, 401) if alphas is None else alphas
    m = (f_out >= lo) & (f_out <= hi)
    fo, do = f_out[m], db_out[m]
    best = (np.inf, 1.0)
    errs = []
    for a in alphas:
        e = do - env_in(fo / a); e = e - np.mean(e)
        errs.append(np.sqrt(np.mean(e ** 2)))
    errs = np.array(errs); i = int(np.argmin(errs))
    e1 = do - env_in(fo); e1 = e1 - np.mean(e1)
    return float(alphas[i]), float(errs[i]), float(np.sqrt(np.mean(e1 ** 2)))


VOWELS = {'a': ((800, 80, 1.0), (1150, 90, 0.5), (2900, 120, 0.25)),
          'i': ((300, 60, 1.0), (2300, 100, 0.4), (3000, 120, 0.3)),
          'u': ((320, 60, 1.0), (800, 80, 0.4), (2300, 120, 0.08))}


def f0_error(x, y, ratio, fs=FS, fmin=60.0, fmax=1200.0, hop=240, skip=0.3, lag=106):
    """per-frame cents of the output's f0 against the input's f0 x ratio, frames voiced in both (output advanced by
    the wet path's delay); also the share of input-voiced frames the output is unvoiced or off by more than 50 cents"""
    n = min(len(x), len(y) - lag)
    lo, hi = fmin * min(1.0, ratio) * 0.8, fmax * max(1.0, ratio) * 1.1
    t, fi, ai = yin(x[:n], fs, lo, hi, hop)
    _, fo, ao = yin(y[lag:lag + n], fs, lo, hi, hop)
    m = (t > skip) & np.isfinite(fi)
    c = cents(fo[m], fi[m] * ratio)
    gross = float(np.mean(~np.isfinite(c) | (np.abs(c) > 50))) if m.any() else None
    return c, gross, t[m]


def cmd_pitch():
    res = {'what': 'pitch accuracy: output f0 against input f0 x 2^(shift/12), cents (YIN, frames voiced in the input)',
           'synthetic': [], 'cents': [], 'rates': [], 'corpus': [], 'vibrato': []}
    shifts = [-12, -7, -1, 0, 1, 5, 12]
    f0s = [80, 110, 150, 220, 300, 440, 600]
    jobs, meta = [], []
    for vn, fm in VOWELS.items():
        for f0 in f0s:
            x = vowel(2.0, f0, formants=fm)[0]
            for s in shifts:
                jobs.append(semi_job(x, s)); meta.append(('synthetic', vn, f0, s, x, FS))
    # Fine on: cents
    for f0 in (150, 300):
        x = vowel(2.0, f0)[0]
        for c in (-1150, -350, -50, -13, 13, 50, 350, 1150):
            jobs.append(st_job(x, c)); meta.append(('cents', 'a', f0, c / 100, x, FS))
    # other sample rates
    for fs in (44100, 96000):
        for f0 in (150, 300):
            x = vowel(2.0, f0, fs=fs)[0]
            for s in (-7, 0, 5, 12):
                jobs.append(semi_job(x, s, fs=fs)); meta.append(('rates', 'a', f0, s, x, fs))
    # vibrato: 30 and 100 cents at 5 Hz
    for f0 in (150, 300):
        for vib in (30, 100):
            x, f = vowel(2.0, f0, vib=vib)
            for s in (-7, 5):
                jobs.append(semi_job(x, s)); meta.append(('vibrato', 'a%d' % vib, f0, s, x, FS))
    # corpus voice
    ph = corpus_phrases(1.2, 14, 'sung') + corpus_phrases(1.2, None, 'rap') + corpus_phrases(1.2, 4, 'processed')
    for name, grp, x in ph:
        for s in (-12, -7, -1, 0, 1, 5, 12):
            jobs.append(semi_job(x, s)); meta.append(('corpus', grp + ':' + name, None, s, x, FS))
    ys = render_all(jobs)
    for (kind, vn, f0, s, x, fs), y in zip(meta, ys):
        lag = int(round(105.5 * fs / 48000))
        c, gross, _ = f0_error(x, y, 2 ** (s / 12), fs=fs, hop=fs // 200, lag=lag,
                               fmin=55.0 if f0 is None else min(55.0, 0.6 * f0))
        row = dict(material=vn, f0=f0, shift_st=s, fs=fs, gross=r(gross), **{'cents_' + k: v for k, v in stats(c).items()})
        res[kind].append(row)
    # summaries
    syn = res['synthetic']
    res['synthetic_summary'] = {
        'medians': stats([row['cents_median'] for row in syn if row.get('cents_n')]),
        'worst_abs_p90': max(row.get('cents_abs_p90', 0) for row in syn),
        'gross_max': max(row['gross'] or 0 for row in syn),
        'gross_rows': [(row['material'], row['f0'], row['shift_st'], row['gross']) for row in syn if (row['gross'] or 0) > 0.05]}
    for grp in ('sung', 'rap', 'processed'):
        rows = [q for q in res['corpus'] if q['material'].startswith(grp + ':') and q['shift_st'] != 0]
        if rows:
            res['corpus_summary_' + grp] = dict(phrases=len(rows) // 6, median_of_medians=r(np.median([q.get('cents_median', np.nan) for q in rows]), 2),
                                                abs_p90_median=r(np.median([q.get('cents_abs_p90', np.nan) for q in rows]), 2),
                                                gross_median=r(np.median([q['gross'] for q in rows]), 3),
                                                gross_max=r(max(q['gross'] for q in rows), 3))
    save('pitch', res)
    for kind in ('synthetic', 'cents', 'rates', 'vibrato', 'corpus'):
        rows = res[kind]
        bad = [(q['material'], q['f0'], q['shift_st'], q.get('cents_median'), q['gross']) for q in rows if (q['gross'] or 0) > 0.05 or abs(q.get('cents_median') or 0) > 5]
        print('%-9s %3d rows, |median| max %.2f c, abs p90 max %.2f c, flagged %d: %s' % (
            kind, len(rows), max(abs(q.get('cents_median') or 0) for q in rows), max(q.get('cents_abs_p90') or 0 for q in rows), len(bad), bad[:12]))


def harmonic_env(y, fs=FS, t0=0.6, t1=1.9):
    """f0 (YIN median) and the harmonic amplitudes (dB) of a steady stretch of y"""
    seg = y[int(t0 * fs):int(t1 * fs)]
    _, ff, _ = yin(seg, fs, 30.0, 1500.0)
    f0 = float(np.nanmedian(ff))
    fo, do = harmonics_db(seg, f0, fs)
    return f0, fo, do


def cmd_formant():
    res = {'what': 'spectral envelope of the output against the input: alpha is the factor with out(f) ~ in(f / alpha) '
                   '(harmonic amplitudes against the vowel stand-in resonator envelope, 200 .. 4500 Hz, a free gain); '
                   'err_db the rms dB left at that alpha, err_preserved_db at alpha 1',
           'formant_at_pitch0': [], 'pitch_at_formant0': [], 'combined': [], 'corpus_ltas': []}
    jobs, meta = [], []
    for vn, fm in VOWELS.items():
        for f0 in (110, 150, 220):
            x = vowel(2.0, f0, formants=fm)[0]
            for F in (-12, -7, -3, 0, 3, 7, 12):
                jobs.append(semi_job(x, 0, F)); meta.append(('formant_at_pitch0', vn, f0, 0, F))
            for P in (-12, -7, -5, -1, 1, 5, 7, 12):
                jobs.append(semi_job(x, P, 0)); meta.append(('pitch_at_formant0', vn, f0, P, 0))
            for P, F in ((-7, 7), (-7, -7), (5, -5), (5, 5), (12, -12), (-12, 12), (7, 3)):
                jobs.append(semi_job(x, P, F)); meta.append(('combined', vn, f0, P, F))
    ys = render_all(jobs)
    for (kind, vn, f0, P, F), y in zip(meta, ys):
        fo_, fo, do = harmonic_env(y)
        env = (lambda fm: (lambda f: resonator_db(f, fm)))(VOWELS[vn])
        a, e, e1 = env_shift_fit(fo, do, env)
        res[kind].append(dict(vowel=vn, f0=f0, pitch_st=P, formant_st=F, out_f0=r(fo_, 2), alpha=r(a, 4),
                              alpha_expected_formant=r(2 ** (F / 12), 4), alpha_in_st=r(12 * np.log2(a), 2),
                              err_db=r(e, 2), err_preserved_db=r(e1, 2)))
    # corpus: the harmonic envelope averaged over voiced frames (voice_env), sung phrases
    jobs, meta = [], []
    for name, grp, x in corpus_phrases(1.5, 8, 'sung'):
        for P, F in ((0, 0), (-12, 0), (-7, 0), (-5, 0), (5, 0), (7, 0), (12, 0), (0, -7), (0, -3), (0, 3), (0, 7), (0, 12)):
            jobs.append(semi_job(x, P, F)); meta.append((name, P, F, x))
    ys = render_all(jobs)
    for (name, P, F, x), y in zip(meta, ys):
        fg, li = voice_env(x); _, lo = voice_env(y[106:])
        env = lambda f: np.interp(np.log(f), np.log(fg), li)
        a, e, e1 = env_shift_fit(fg, lo, env, lo=300, hi=4500, alphas=np.geomspace(0.45, 2.2, 401))
        res['corpus_ltas'].append(dict(phrase=name, pitch_st=P, formant_st=F, alpha=r(a, 4), alpha_in_st=r(12 * np.log2(a), 2),
                                       err_db=r(e, 2), err_preserved_db=r(e1, 2)))
    # summaries: alpha in semitones against the setting
    def summ(rows, key):
        out = {}
        for v in sorted(set(q[key] for q in rows)):
            al = [q['alpha_in_st'] for q in rows if q[key] == v]
            er = [q['err_db'] for q in rows if q[key] == v]
            out[str(v)] = dict(alpha_st_median=r(np.median(al), 2), alpha_st_min=r(min(al), 2), alpha_st_max=r(max(al), 2), err_db_median=r(np.median(er), 2))
        return out
    res['summary_formant'] = summ(res['formant_at_pitch0'], 'formant_st')
    res['summary_pitch'] = summ(res['pitch_at_formant0'], 'pitch_st')
    res['summary_corpus_pitch'] = summ([q for q in res['corpus_ltas'] if q['formant_st'] == 0], 'pitch_st')
    res['summary_corpus_formant'] = summ([q for q in res['corpus_ltas'] if q['pitch_st'] == 0], 'formant_st')
    res['summary_combined'] = {'%+d/%+d' % (P, F): r(np.median([q['alpha_in_st'] for q in res['combined'] if (q['pitch_st'], q['formant_st']) == (P, F)]), 2)
                               for P, F in sorted(set((q['pitch_st'], q['formant_st']) for q in res['combined']))}
    save('formant', res)
    for k in ('summary_formant', 'summary_pitch', 'summary_corpus_pitch', 'summary_corpus_formant', 'summary_combined'):
        print(k, json.dumps(res[k]))


def voice_env(x, fs=FS, fgrid=None, nwin=2048):
    """the spectral envelope of a voice (dB on fgrid): per voiced frame (YIN aperiodicity < 0.2) the harmonic peaks
    k f0 (the largest bin within +-0.3 f0), joined linearly in dB, averaged in dB over the frames"""
    fgrid = np.geomspace(150, 6000, 160) if fgrid is None else fgrid
    t, f0, ap = yin(x, fs, 60.0, 1000.0, hop=480)
    w = np.hanning(nwin); N = 8192
    fb = np.fft.rfftfreq(N, 1 / fs)
    acc, n = np.zeros(len(fgrid)), 0
    for tt, f, a in zip(t, f0, ap):
        if not np.isfinite(f) or a > 0.2: continue
        c = int(tt * fs); s0 = c - nwin // 2
        if s0 < 0 or s0 + nwin > len(x): continue
        fr = x[s0:s0 + nwin]
        if np.dot(fr, fr) < 1e-8: continue
        S = 20 * np.log10(np.abs(np.fft.rfft(fr * w, N)) + 1e-12)
        ks = np.arange(1, int(fgrid[-1] * 1.05 / f) + 1)
        hf, hd = [], []
        for k in ks:
            m = (fb >= (k - 0.3) * f) & (fb <= (k + 0.3) * f)
            i = np.argmax(S[m]); hf.append(fb[m][i]); hd.append(S[m][i])
        acc += np.interp(fgrid, hf, hd); n += 1
    return fgrid, acc / max(n, 1)


def best_delay(x, y, lo=95.0, hi=120.0, fs=FS, s0=None, s1=None):
    """the fractional delay d (and gain) with y ~ g x(t - d) best over [s0, s1); residual dB (relative to y)"""
    from scipy.optimize import minimize_scalar
    s0 = s0 or fs // 2; s1 = s1 or len(x) - fs // 4
    def res(d):
        xd = frac_delay(x, d)[s0:s1]; yy = y[s0:s1]
        g = np.dot(yy, xd) / (np.dot(xd, xd) + 1e-30)
        return H.rms_db(yy - g * xd) - H.rms_db(yy), g
    grid = np.arange(lo, hi, 0.25)
    d0 = grid[int(np.argmin([res(d)[0] for d in grid]))]
    o = minimize_scalar(lambda d: res(d)[0], bounds=(d0 - 0.3, d0 + 0.3), method='bounded', options=dict(xatol=1e-4))
    e, g = res(o.x)
    return float(o.x), float(g), float(e)


def ls_fir_residual(x, y, centre, taps=64, fs=FS):
    """residual (dB) of the best FIR of `taps` taps around `centre` samples of delay (least squares): how far the wet
    path is from a linear time-invariant filter on this input"""
    s0, s1 = fs // 2, len(x) - fs // 4
    off = int(centre) - taps // 2 + 1
    X = np.stack([x[s0 - k - off: s1 - k - off] for k in range(taps)], 1)
    h = np.linalg.lstsq(X, y[s0:s1], rcond=None)[0]
    return H.rms_db(y[s0:s1] - X @ h) - H.rms_db(y[s0:s1]), h


def cmd_noshift():
    res = {'what': 'the wet path at Pitch 0 / Formant 0 / Mix 100: the fractional delay d and gain g with y ~ g x(t - d) '
                   'best (FFT fractional delay), the residual left (dB, relative to the output), the residual of the best '
                   '64-tap FIR (is it a time-invariant filter), per material and sample rate', 'rows': []}
    jobs, meta = [], []
    for fs in (44100, 48000, 96000):
        n = int(3 * fs); t = np.arange(n) / fs
        mats = {'vowel a 100 Hz': vowel(3, 100, fs=fs)[0], 'vowel a 150 Hz': vowel(3, 150, fs=fs)[0],
                'vowel a 300 Hz': vowel(3, 300, fs=fs)[0], 'vowel a 600 Hz': vowel(3, 600, fs=fs)[0],
                'vowel i 220 Hz': vowel(3, 220, fs=fs, formants=VOWELS['i'])[0],
                'noise -18 dBFS': noise(3, fs=fs), 'sine 300 Hz': 0.1 * np.sin(2 * np.pi * 300 * t),
                'sine 3 kHz': 0.1 * np.sin(2 * np.pi * 3000 * t), 'sine 10 kHz': 0.1 * np.sin(2 * np.pi * 10000 * t)}
        if fs == FS:
            ph = corpus_phrases(2.0, 2, 'sung')
            for name, grp, x in ph: mats['corpus ' + name] = x
            mats['vowel a 150 Hz, vibrato 100 c'] = vowel(3, 150, fs=fs, vib=100)[0]
            mats['vowel a glide 100 to 400 Hz'] = vowel(3, 100, fs=fs, f0_end=400)[0]
        for k, x in mats.items():
            jobs.append(job(x, fs)); meta.append((fs, k, x, 'host defaults'))
        if fs == FS:
            for k in ('vowel a 150 Hz', 'noise -18 dBFS'):
                jobs.append(st_job(mats[k], 0, 0)); meta.append((fs, k, mats[k], 'Fine on, 0 cents (state)'))
    ys = render_all(jobs)
    for (fs, k, x, how), y in zip(meta, ys):
        sc = fs / 48000
        d, g, e = best_delay(x, y, 90 * sc, 140 * sc, fs)
        efir, h = ls_fir_residual(x, y, d, 64, fs)
        res['rows'].append(dict(fs=fs, material=k, settings=how, delay_samples=r(d, 3), delay_ms=r(1000 * d / fs, 4), gain_db=r(20 * np.log10(abs(g)), 3),
                                residual_db=r(e, 1), fir64_residual_db=r(efir, 1), out_rms_minus_in_db=r(H.rms_db(y[fs:]) - H.rms_db(x[fs:]), 3)))
        print('%6d %-32s %-26s d %.3f (%.4f ms) g %+.3f dB res %.1f fir %.1f' % (fs, k, how, d, 1000 * d / fs, 20 * np.log10(abs(g)), e, efir))
    # the noise path's response (64-tap LS fit) and the click response, 48 kHz
    x = noise(3); y = render_all([job(x)])[0]
    efir, h = ls_fir_residual(x, y, 105.5, 64)
    cl = np.zeros(3 * FS); pos = np.arange(4800, 3 * FS - 4800, 12000)
    clicks = {}
    for a in (0.005, 0.05, 0.2, 0.5, 1.0):
        c = cl.copy(); c[pos] = a; clicks[a] = c
    yc = render_all([job(c) for c in clicks.values()])
    res['noise_fir64_taps_around_105'] = [r(v, 4) for v in h[64 // 2 - 8:64 // 2 + 8]]
    res['click_response'] = {}
    for (a, c), y in zip(clicks.items(), yc):
        p = pos[3]; k = y[p + 90:p + 122] / a
        res['click_response']['%g' % a] = dict(taps_95_to_116=[r(v, 4) for v in k[5:27]], sum=r(np.sum(y[p:p + 400]) / a, 4),
                                               energy=r(np.sum(y[p - 2000:p + 2000] ** 2) / a ** 2, 4),
                                               peak_index=int(np.argmax(np.abs(y[p:p + 400]))))
        print('click %.3f: sum %.4f energy %.4f peak at +%d' % (a, np.sum(y[p:p + 400]) / a, np.sum(y[p - 2000:p + 2000] ** 2) / a ** 2, np.argmax(np.abs(y[p:p + 400]))))
    save('noshift', res)


def cmd_level():
    res = {'what': 'output rms against input rms (dB), 0.5 s .. end, per setting and material; Mix 100', 'rows': []}
    mats = {'vowel a 150 Hz': vowel(3, 150)[0], 'vowel i 300 Hz': vowel(3, 300, formants=VOWELS['i'])[0],
            'vowel u 110 Hz': vowel(3, 110, formants=VOWELS['u'])[0], 'noise': noise(3),
            'noise band 4-10 kHz (sibilant)': band(noise(3, 0.0), 4000, 10000) * 10 ** (-18 / 20) / np.sqrt(np.mean(band(noise(3, 0.0), 4000, 10000) ** 2))}
    for name, grp, x in corpus_phrases(1.5, 4, 'sung'): mats['sung ' + name] = x
    for name, grp, x in corpus_phrases(1.5, 2, 'rap'): mats['rap ' + name] = x
    settings = [(0, 0), (-12, 0), (-7, 0), (-1, 0), (1, 0), (5, 0), (12, 0), (0, -12), (0, -7), (0, 7), (0, 12), (-7, 7), (5, -5)]
    jobs, meta = [], []
    for k, x in mats.items():
        for P, F in settings:
            jobs.append(semi_job(x, P, F)); meta.append((k, P, F, x))
    for db in (-40, -30, -18, -10, -6):
        x = vowel(3, 150, db)[0]
        for P in (-7, 0, 5):
            jobs.append(semi_job(x, P, 0)); meta.append(('vowel a 150 Hz at %d dBFS' % db, P, 0, x))
    ys = render_all(jobs)
    for (k, P, F, x), y in zip(meta, ys):
        s0 = FS // 2
        res['rows'].append(dict(material=k, pitch_st=P, formant_st=F, in_rms_db=r(H.rms_db(x[s0:]), 2), out_minus_in_db=r(H.rms_db(y[s0:]) - H.rms_db(x[s0:]), 2)))
    tab = {}
    for q in res['rows']: tab.setdefault(q['material'], {})['%+d/%+d' % (q['pitch_st'], q['formant_st'])] = q['out_minus_in_db']
    res['table_out_minus_in_db'] = tab
    for k, v in tab.items(): print('%-40s' % k[:40], ' '.join('%s:%+.1f' % (a, b) for a, b in v.items()))
    save('level', res)


def env_onset(y, t_from, fs=FS, rel_db=-20.0, ref=None):
    """the first sample after t_from where a 32-sample rms envelope reaches rel_db under the steady level"""
    B = 32
    e = np.sqrt(np.convolve(y ** 2, np.ones(B) / B, 'same'))
    ref = ref if ref is not None else np.median(e[int(t_from) + fs // 4:int(t_from) + fs // 2])
    i = np.where(e[int(t_from):] > ref * 10 ** (rel_db / 20))[0]
    return int(t_from) + int(i[0]) if len(i) else None


def settle_time(t, c, t_event, tol=20.0, hold=0.02):
    """seconds from t_event until the cents error stays within tol for hold seconds"""
    ok = np.isfinite(c) & (np.abs(c) < tol)
    dt = t[1] - t[0]; need = max(1, int(round(hold / dt)))
    for i in np.where(t >= t_event)[0]:
        if ok[i:i + need].all() and i + need <= len(ok): return float(t[i] - t_event)
    return None


def cmd_timing():
    res = {'what': 'timing of the wet path: onset delay (32-sample rms envelope reaching -20 dB under its steady level, '
                   'output against input), f0 settling after an onset and after a pitch jump (YIN, 1 ms hop: time until '
                   'the output stays within 20 cents of input f0 x ratio for 20 ms), the f0 lag on vibrato',
           'onsets': [], 'jumps': [], 'vibrato_lag': [], 'bursts': []}
    t0 = int(0.5 * FS)
    jobs, meta = [], []
    for f0 in (100, 200, 400):
        x = np.zeros(int(1.5 * FS)); v = vowel(1.0, f0)[0]; x[t0:t0 + len(v)] = v
        for P in (0, -7, 5, 12):
            jobs.append(semi_job(x, P)); meta.append(('onsets', dict(material='vowel a %d Hz after silence' % f0, f0=f0, pitch_st=P), x))
    # noise and a sibilant band after silence
    for name, nz in (('noise', noise(1.0)), ('band 4-10 kHz', band(noise(1.0), 4000, 10000) * 4)):
        x = np.zeros(int(1.5 * FS)); x[t0:t0 + len(nz)] = nz
        for P in (0, 5, -7):
            jobs.append(semi_job(x, P)); meta.append(('bursts', dict(material=name + ' after silence', pitch_st=P), x))
    # pitch jumps: f0 steps with a continuous excitation
    for fa, fb in ((150, 225), (225, 150), (200, 400), (400, 200), (150, 160)):
        n = int(2.5 * FS); tt = np.arange(n) / FS
        f = np.where(tt < 1.25, fa, fb).astype(float)
        ph = np.cumsum(f) / FS; src = np.diff(np.floor(ph), prepend=0.0)
        y = np.zeros(n)
        for fc, bw, g in VOWELS['a']:
            rr = np.exp(-np.pi * bw / FS)
            y += g * lfilter([1 - rr], [1, -2 * rr * np.cos(2 * np.pi * fc / FS), rr * rr], src)
        x = y * 10 ** (-18 / 20) / np.sqrt(np.mean(y ** 2))
        for P in (0, 5, -7):
            jobs.append(semi_job(x, P)); meta.append(('jumps', dict(material='vowel a %d to %d Hz at 1.25 s' % (fa, fb), f_from=fa, f_to=fb, pitch_st=P), x))
    for f0 in (150, 300):
        for vib in (30, 100):
            x, f = vowel(2.0, f0, vib=vib)
            for P in (-7, 5):
                jobs.append(semi_job(x, P)); meta.append(('vibrato_lag', dict(material='vowel a %d Hz vibrato %d c 5 Hz' % (f0, vib), f0=f0, pitch_st=P), x))
    ys = render_all(jobs)
    for (kind, row, x), y in zip(meta, ys):
        P = row['pitch_st']; ratio = 2 ** (P / 12)
        if kind in ('onsets', 'bursts'):
            i_in = env_onset(x, t0 - 100, ref=np.median(np.sqrt(np.convolve(x ** 2, np.ones(32) / 32, 'same'))[t0 + FS // 4:t0 + FS // 2]))
            i_out = env_onset(y, t0 - 100)
            row['onset_delay_samples'] = None if i_out is None else int(i_out - i_in)
            row['out_rms_first_20ms_minus_steady_db'] = r(H.rms_db(y[t0 + 106:t0 + 106 + 960]) - H.rms_db(y[t0 + FS // 4:t0 + FS // 2]), 2)
            row['in_rms_first_20ms_minus_steady_db'] = r(H.rms_db(x[t0:t0 + 960]) - H.rms_db(x[t0 + FS // 4:t0 + FS // 2]), 2)
        if kind == 'onsets':
            f0 = row['f0']
            lo = 0.7 * f0 * min(1, ratio); hi = 1.5 * f0 * max(1, ratio)
            t, fo, _ = yin(y, FS, lo, hi, hop=48, win=int(2.2 * FS / (f0 * min(1, ratio))))
            c = cents(fo, f0 * ratio)
            st = settle_time(t, c, (t0 + 106) / FS)
            row['f0_settle_ms'] = None if st is None else r(1000 * st, 1)
            # what f0 the output has in its first frames (before settling)
            m = (t > (t0 + 106) / FS) & (t < (t0 + 106) / FS + 0.03)
            row['first_30ms_cents_vs_target_median'] = r(np.nanmedian(c[m]), 1) if np.isfinite(c[m]).any() else None
            row['first_30ms_cents_vs_input_f0_median'] = r(np.nanmedian(cents(fo[m], f0)), 1) if np.isfinite(fo[m]).any() else None
        if kind == 'jumps':
            fa, fb = row['f_from'], row['f_to']
            lo = 0.7 * min(fa, fb) * min(1, ratio); hi = 1.5 * max(fa, fb) * max(1, ratio)
            t, fo, _ = yin(y, FS, lo, hi, hop=48, win=int(2.2 * FS / lo))
            c = cents(fo, fb * ratio)
            st = settle_time(t, c, 1.25 + 106 / FS)
            row['f0_settle_ms'] = None if st is None else r(1000 * st, 1)
            # the input's own (tracker) settle for reference
            ti, fi, _ = yin(x, FS, lo, hi, hop=48, win=int(2.2 * FS / lo))
            sti = settle_time(ti, cents(fi, fb), 1.25)
            row['tracker_settle_on_input_ms'] = None if sti is None else r(1000 * sti, 1)
            # glitch: frames between the jump and settling more than 100 cents from both targets
            m = (t > 1.25) & (t < 1.25 + 0.2)
            ca, cb = cents(fo[m], fa * ratio), cents(fo[m], fb * ratio)
            row['frames_off_both_by_100c_first_200ms'] = int(np.sum(~((np.abs(ca) < 100) | (np.abs(cb) < 100))))
        if kind == 'vibrato_lag':
            f0 = row['f0']
            lo = 0.7 * f0 * min(1, ratio) * 2 ** (-0.1); hi = 1.5 * f0 * max(1, ratio)
            t, fi, _ = yin(x, FS, lo, hi, hop=48, win=int(2.2 * FS / lo))
            _, fo, _ = yin(y, FS, lo, hi, hop=48, win=int(2.2 * FS / lo))
            m = (t > 0.3) & (t < 1.9) & np.isfinite(fi) & np.isfinite(fo)
            ci = cents(fi[m], f0); co = cents(fo[m], f0 * ratio)
            lags = np.arange(-40, 41)
            cc = [np.corrcoef(ci[max(0, -L):len(ci) - max(0, L)], co[max(0, L):len(co) - max(0, -L)])[0, 1] for L in lags]
            L = int(lags[int(np.argmax(cc))])
            row['f0_curve_lag_ms'] = r(L * 1.0, 1)                          # (hop 48 samples = 1 ms)
            row['depth_ratio'] = r(np.std(co) / np.std(ci), 3)
            # error once the lag is taken out
            e = co[max(0, L):len(co) - max(0, -L)] - ci[max(0, -L):len(ci) - max(0, L)]
            row['cents_err_after_lag'] = stats(e)
        res[kind].append(row)
    for k in ('onsets', 'bursts', 'jumps', 'vibrato_lag'):
        for q in res[k]: print(k, json.dumps(q))
    save('timing', res)


def marker_copies(d, thr=0.04):
    """copies of a marker click in a difference signal d (divided by the marker's amplitude; d[100] = the marker's own
    time): (delay, amplitude = the kernel's sum over 13 samples) per peak of |d| (20 samples apart, above thr)"""
    from scipy.signal import find_peaks
    pk, _ = find_peaks(np.abs(d), height=thr, distance=20)
    out = [(int(p) - 100, float(np.sum(d[max(0, p - 6):p + 7]))) for p in pk]
    return [(dl, a) for dl, a in out if abs(a) > 0.03]


def marker_probe(P, F, f0=150, step=8, span=2, amp=0.02):
    """the wet path's time-varying response on a steady vowel: one render per marker position m (a click of amp added
    at m0 + m), minus the render without it. Returns per m the copies (delay, amplitude)"""
    v = vowel(1.3, f0)[0]
    m0 = FS
    ms = np.arange(0, int(span * FS / f0), step)
    jobs = [semi_job(v, P, F)]
    for m in ms:
        z = v.copy(); z[m0 + m] += amp; jobs.append(semi_job(z, P, F))
    ys = render_all(jobs)
    rows = []
    for m, y in zip(ms, ys[1:]):
        d = (y - ys[0])[m0 + m - 100:m0 + m + 2400] / amp
        c = marker_copies(d)
        rows.append(dict(m=int(m), copies=[(dl, r(a, 3)) for dl, a in c]))
    return rows


def analyse_probe(rows, step, P, F, f0):
    """grain tracks (copies linked across marker positions), their slope (output samples per input sample: the
    grain's time scale), length and the read delays"""
    # a marker that moved the grain placement leaves a vowel waveform (many peaks, long delays), not click copies
    clean = [q for q in rows if len(q['copies']) <= 6 and all(dl < 1000 for dl, _ in q['copies'])]
    tracks = []                                                 # each: list of (m, delay, amp)
    open_ = []
    for q in clean:
        cs = [(dl, a) for dl, a in q['copies'] if abs(a) > 0.05]
        new_open = []
        taken = set()
        for tr in open_:
            m_l, d_l, _ = tr[-1]
            if q['m'] - m_l > 3 * step: tracks.append(tr); continue
            best = None
            for k, (dl, a) in enumerate(cs):
                if k in taken: continue
                if abs(dl - d_l) <= 0.3 * (q['m'] - m_l) + 1 and (best is None or abs(dl - d_l) < abs(cs[best][0] - d_l)): best = k
            if best is None: tracks.append(tr); continue
            taken.add(best); tr.append((q['m'], cs[best][0], cs[best][1])); new_open.append(tr)
        for k, (dl, a) in enumerate(cs):
            if k not in taken: new_open.append([(q['m'], dl, a)])
        open_ = new_open
    tracks += open_
    long_ = [np.array(t, float) for t in tracks if len(t) >= 6]
    slopes = []
    for t in long_:
        pf = np.polyfit(t[:, 0], t[:, 1], 1); slopes.append(1 + pf[0])
    sl = float(np.median(slopes)) if slopes else None
    amps = [abs(a) for q in clean for _, a in q['copies'] if abs(a) > 0.2]
    dels = [dl for q in clean for dl, a in q['copies'] if abs(a) > 0.2]
    ncop = [sum(1 for _, a in q['copies'] if abs(a) > 0.05) for q in clean]
    gain = [sum(a for _, a in q['copies']) for q in clean]
    spans = [float(t[-1, 0] - t[0, 0] + step) for t in long_]
    return dict(pitch_st=P, formant_st=F, f0=f0, period_in=r(FS / f0, 1), period_out=r(FS / f0 / 2 ** (P / 12), 1),
                markers=len(rows), markers_moving_the_grains=len(rows) - len(clean),
                grain_time_scale=r(sl, 4), grain_time_scale_in_st=r(-12 * np.log2(sl), 2) if sl else None,
                expected_from_formant_only_st=F, tracks=len(long_),
                grain_track_span_samples_median=r(np.median(spans), 1) if spans else None,
                grain_track_span_in_periods=r(np.median(spans) * f0 / FS, 2) if spans else None,
                copies_per_marker_mean=r(np.mean(ncop), 2), read_delay_min=min(dels) if dels else None, read_delay_max=max(dels) if dels else None,
                marker_gain_min=r(min(gain), 3), marker_gain_max=r(max(gain), 3), marker_gain_mean=r(np.mean(gain), 3),
                marker_gain_mean_per_output_period=r(np.mean(gain) / (2 ** (P / 12)), 3))


def cmd_structure():
    res = {'what': 'structure probes. markers: a click (0.02) added to a steady vowel /a/ at one position per render, the '
                   'render without it subtracted, so each copy of the click in the output shows where (delay) and how '
                   'strongly (amplitude: the kernel sum) that input instant is read. grain_time_scale: output samples per '
                   'input sample along a copy track (1: copied as is; below 1: read faster, formants up). tone: an '
                   'inharmonic sine on a vowel, energy near the tone and near tone x ratio',
           'markers': [], 'marker_rows': {}, 'tone': []}
    cfgs = [(12, 0, 150), (5, 0, 150), (1, 0, 150), (-1, 0, 150), (-7, 0, 150), (-12, 0, 150),
            (0, 7, 150), (0, -7, 150), (5, -5, 150), (5, 0, 250), (-7, 0, 250), (0, 0, 150)]
    for P, F, f0 in cfgs:
        rows = marker_probe(P, F, f0)
        a = analyse_probe(rows, 8, P, F, f0)
        res['markers'].append(a); res['marker_rows']['P%+d_F%+d_f%d' % (P, F, f0)] = rows
        print(json.dumps(a), flush=True)
    # an inharmonic tone (small, so it does not move the grains) on a vowel /u/: where does its energy go. The
    # difference of the renders with and without it; its spectral centroid (1 .. 12 kHz) against the tone
    t = np.arange(2 * FS) / FS
    v = vowel(2.0, 150, formants=VOWELS['u'])[0]
    for ft in (3070.0, 5070.0):
        x = v + 0.003 * np.sin(2 * np.pi * ft * t)
        for P in (-7, 5, 12):
            y, y0 = render_all([semi_job(x, P), semi_job(v, P)])
            ratio = 2 ** (P / 12)
            dd = (y - y0)[FS // 2:int(1.9 * FS)]
            S = np.abs(np.fft.rfft(dd * np.hanning(len(dd)))) ** 2; fr = np.fft.rfftfreq(len(dd), 1 / FS)
            Sx = np.abs(np.fft.rfft((x - v)[FS // 2:int(1.9 * FS)] * np.hanning(len(dd)))) ** 2
            m = (fr > 1000) & (fr < 12000)
            cen = float(np.sum(fr[m] * S[m]) / np.sum(S[m]))
            def share(fc, w=300):
                k = (fr > fc - w) & (fr < fc + w); return float(S[k].sum() / S[m].sum())
            res['tone'].append(dict(tone_hz=ft, pitch_st=P, response_energy_minus_tone_db=r(10 * np.log10(S.sum() / Sx.sum()), 1),
                                    centroid_hz=r(cen, 0), centroid_shift_st=r(12 * np.log2(cen / ft), 2),
                                    share_within_300hz_of_tone=r(share(ft), 3), share_within_300hz_of_tone_x_ratio=r(share(ft * ratio), 3)))
            print(json.dumps(res['tone'][-1]), flush=True)
    save('structure', res)


def spectrum_db(z, fs=FS, n=8192):
    from scipy.signal import welch
    f, P = welch(z, fs=fs, nperseg=n)
    return f, P


def partial_energy(y, partials, ratio, fs=FS, tol_c=15.0):
    """shares of y's energy (60 .. 6000 Hz) within tol_c cents (at least 3 bins) of the partials x ratio, of the
    partials unshifted, and elsewhere"""
    f, P = spectrum_db(y[fs // 2:], fs, 16384)
    m = (f > 60) & (f < 6000); f, P = f[m], P[m]
    tot = P.sum() + 1e-30
    def near(fr):
        k = np.zeros(len(f), bool)
        for p in fr: k |= np.abs(f - p) < max(p * (2 ** (tol_c / 1200) - 1), 3 * fs / 16384)   # (at least 3 bins)
        return k
    a = near([p * ratio for p in partials]); b = near(partials) & ~a
    return dict(shifted=r(P[a].sum() / tot, 3), unshifted=r(P[b].sum() / tot, 3), elsewhere=r(P[~(a | b)].sum() / tot, 3))


def harmonic_complex(f0, secs=2.0, fs=FS, nh=12, db=-24.0):
    t = np.arange(int(secs * fs)) / fs
    y = sum(np.sin(2 * np.pi * k * f0 * t + k) / k for k in range(1, nh + 1) if k * f0 < 6000)
    return y * 10 ** (db / 20) / np.sqrt(np.mean(y ** 2))


def cmd_unvoiced():
    res = {'what': 'unvoiced and difficult input: spectra (band shares and centroid), the residual against the input '
                   'delayed by the no-shift delay (105.53 samples: passed through or not), chords (shares of energy at the '
                   'shifted partials, the unshifted ones and elsewhere), level and noise robustness of the pitch shift',
           'noise': [], 'chords': [], 'bursts': [], 'silence': [], 'low_level': [], 'snr': [], 'odd': []}
    D = 105.53125

    def centroid(z):
        f, P = spectrum_db(z[FS // 2:]); m = (f > 50) & (f < 20000)
        return float(np.sum(f[m] * P[m]) / np.sum(P[m]))

    def band_share(z, lo, hi):
        f, P = spectrum_db(z[FS // 2:]); m = (f > 50) & (f < 22000)
        return float(P[(f >= lo) & (f < hi) & m].sum() / P[m].sum())
    mats = {'white noise': noise(2.0),
            'band 4-10 kHz': band(noise(2.0, 0.0), 4000, 10000),
            'band 1.5-3 kHz': band(noise(2.0, 0.0), 1500, 3000)}
    for k in list(mats)[1:]: mats[k] = mats[k] * 10 ** (-18 / 20) / np.sqrt(np.mean(mats[k] ** 2))
    jobs, meta = [], []
    for k, x in mats.items():
        for P, F in ((0, 0), (-12, 0), (-7, 0), (-1, 0), (1, 0), (5, 0), (12, 0), (0, 7), (0, -7)):
            jobs.append(semi_job(x, P, F)); meta.append((k, P, F, x))
    ys = render_all(jobs)
    for (k, P, F, x), y in zip(meta, ys):
        xd = frac_delay(x, D); s0 = FS // 2
        row = dict(material=k, pitch_st=P, formant_st=F, out_minus_in_db=r(H.rms_db(y[s0:]) - H.rms_db(x[s0:]), 2),
                   centroid_in_hz=r(centroid(x), 0), centroid_out_hz=r(centroid(y), 0),
                   residual_vs_delayed_input_db=r(H.rms_db(y[s0:] - xd[s0:]) - H.rms_db(xd[s0:]), 1),
                   correlation_with_delayed_input=r(np.dot(y[s0:], xd[s0:]) / np.sqrt(np.dot(y[s0:], y[s0:]) * np.dot(xd[s0:], xd[s0:])), 3))
        if k != 'white noise':
            lo, hi = (4000, 10000) if '4-10' in k else (1500, 3000)
            row['share_in_input_band'] = r(band_share(y, lo, hi), 3)
            row['share_in_band_x_ratio'] = r(band_share(y, lo * 2 ** (P / 12), hi * 2 ** (P / 12)), 3)
        _, fo, ap = yin(y[s0:], FS, 60, 1000)
        row['out_yin_voiced_share'] = r(np.mean(np.isfinite(fo)), 3)
        res['noise'].append(row)
    # chords: 2 and 3 harmonic complexes, two sines; the shifter is for one voice
    chords = {'two sines 220 + 330 Hz': ([220.0, 330.0], None),
              'harmonic complexes C3 E3 G3': ([130.81, 164.81, 196.0], 'hc'),
              'harmonic complexes A2 + E3': ([110.0, 164.81], 'hc')}
    jobs, meta = [], []
    for k, (fs_, kind) in chords.items():
        t = np.arange(2 * FS) / FS
        if kind is None:
            x = sum(0.05 * np.sin(2 * np.pi * f * t) for f in fs_); parts = fs_
        else:
            x = sum(harmonic_complex(f) for f in fs_)
            parts = sorted(set(round(f * h, 2) for f in fs_ for h in range(1, 13) if f * h < 6000))
        for P in (0, -7, 5, 12):
            jobs.append(semi_job(x, P)); meta.append((k, P, x, parts))
    ys = render_all(jobs)
    for (k, P, x, parts), y in zip(meta, ys):
        e = partial_energy(y, parts, 2 ** (P / 12))
        _, fo, _ = yin(y[FS // 2:], FS, 50, 1500)
        res['chords'].append(dict(material=k, pitch_st=P, energy_share=e, out_minus_in_db=r(H.rms_db(y[FS // 2:]) - H.rms_db(x[FS // 2:]), 2),
                                  out_yin_f0_median=r(np.nanmedian(fo), 1) if np.isfinite(fo).any() else None,
                                  out_yin_voiced_share=r(np.mean(np.isfinite(fo)), 3)))
    # bursts: 40 ms noise bursts every 200 ms; silence; a bare pulse train
    x = np.zeros(2 * FS)
    for k in range(int(0.1 * FS), 2 * FS - 3000, int(0.2 * FS)): x[k:k + int(0.04 * FS)] = noise(0.04, -18, seed=k)
    sil = np.zeros(2 * FS)
    pt = pulses(2.0, 240)
    jobs = [semi_job(x, P) for P in (0, -7, 5)] + [semi_job(sil, P) for P in (0, -7, 5)] + [semi_job(pt, P) for P in (-7, 5, 12)]
    ys = render_all(jobs)
    xd = frac_delay(x, D)
    for P, y in zip((0, -7, 5), ys[:3]):
        res['bursts'].append(dict(pitch_st=P, out_minus_in_db=r(H.rms_db(y) - H.rms_db(x), 2),
                                  residual_vs_delayed_input_db=r(H.rms_db(y - xd) - H.rms_db(xd), 1),
                                  energy_outside_bursts_db=r(H.rms_db(y * (np.abs(xd) < 1e-6)) - H.rms_db(xd), 1)))
    for P, y in zip((0, -7, 5), ys[3:6]):
        res['silence'].append(dict(pitch_st=P, out_peak=float(np.max(np.abs(y)))))
    for P, y in zip((-7, 5, 12), ys[6:]):
        _, fo, _ = yin(y[FS // 2:], FS, 50, 1500)
        res['odd'].append(dict(material='bare pulse train 200 Hz (no formants)', pitch_st=P, expected_f0=r(200 * 2 ** (P / 12), 1),
                               out_yin_f0_median=r(np.nanmedian(fo), 1), out_minus_in_db=r(H.rms_db(y[FS // 2:]) - H.rms_db(pt[FS // 2:]), 2)))
    # narrow single resonance vowels
    for fc, bw in ((1000, 60), (1000, 200), (500, 60)):
        for f0 in (100, 200):
            x = vowel(2.0, f0, formants=((fc, bw, 1.0),))[0]
            ys = render_all([semi_job(x, P) for P in (-12, -7, 5, 12)])
            for P, y in zip((-12, -7, 5, 12), ys):
                c, gross, _ = f0_error(x, y, 2 ** (P / 12), fmin=0.6 * f0, fmax=2 * f0)
                res['odd'].append(dict(material='vowel with one resonance %d Hz bw %d, f0 %d' % (fc, bw, f0), pitch_st=P,
                                       gross=r(gross, 3), **{'cents_' + k: v for k, v in stats(c).items()}))
    # level: is there a gate under which nothing is shifted
    for db in (-80, -70, -60, -58, -56, -54, -52, -50, -40, -30):
        x = vowel(2.0, 150, db)[0]
        y = render_all([semi_job(x, 5)])[0]
        c, gross, _ = f0_error(x, y, 2 ** (5 / 12), fmin=90, fmax=400)
        res['low_level'].append(dict(in_rms_dbfs=db, pitch_st=5, gross=r(gross, 3), cents_median=stats(c).get('median'),
                                     out_minus_in_db=r(H.rms_db(y[FS // 2:]) - H.rms_db(x[FS // 2:]), 2)))
    # vowel in noise
    for snr in (30, 20, 10, 5, 0):
        x = vowel(2.0, 150)[0] + noise(2.0, -18 - snr, seed=11)
        for P in (-7, 5):
            y = render_all([semi_job(x, P)])[0]
            c, gross, _ = f0_error(x, y, 2 ** (P / 12), fmin=90, fmax=400)
            res['snr'].append(dict(snr_db=snr, pitch_st=P, gross=r(gross, 3), **{'cents_' + k: v for k, v in stats(c).items()}))
    save('unvoiced', res)
    for k in ('noise', 'chords', 'bursts', 'silence', 'odd', 'low_level', 'snr'):
        for q in res[k]: print(k, json.dumps(q))


NOTE_NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']


def cmd_flatten():
    res = {'what': 'Flatten (state: on, note 0 .. 23, the editor names them C2 .. B3): the output f0 against the note '
                   'frequency. Measured: note 0 (its "C2") is 130.81 Hz, i.e. MIDI note 48 with A4 = 440 Hz (the naming '
                   'where middle C, 261.63 Hz, is C3). Cents against that, its spread, the lock time after an onset, Pitch '
                   'on top (Fine on, cents through the state)', 'rows': []}
    jobs, meta = [], []
    srcs = {'vowel a 110 Hz': vowel(2.0, 110)[0], 'vowel a 150 Hz': vowel(2.0, 150)[0], 'vowel a 220 Hz': vowel(2.0, 220)[0],
            'vowel a 300 Hz': vowel(2.0, 300)[0], 'vowel a 150 Hz vibrato 50 c': vowel(2.0, 150, vib=50)[0],
            'vowel a glide 110 to 330 Hz': vowel(2.0, 110, f0_end=330)[0]}
    for name, grp, x in corpus_phrases(1.5, 2, 'sung'): srcs['sung ' + name] = x
    on = np.zeros(int(1.5 * FS)); on[int(0.5 * FS):] = vowel(1.0, 150)[0]; srcs['vowel a 150 Hz after silence'] = on
    for k, x in srcs.items():
        for note in (0, 5, 12, 21, 23):
            for pc in (0, 500):
                if pc and note != 12: continue
                jobs.append(st_job(x, pc, 0, flatten=note)); meta.append((k, note, pc, x))
    ys = render_all(jobs)
    for (k, note, pc, x), y in zip(meta, ys):
        fnote = 440.0 * 2 ** ((48 + note - 69) / 12)                 # note 0 = MIDI 48 = 130.81 Hz (measured)
        target = fnote * 2 ** (pc / 1200)
        t, fo, _ = yin(y, FS, 40, 1000, hop=240)
        m = t > (0.7 if 'after silence' in k else 0.3)
        c = cents(fo[m], target)
        oct_ = cents(np.nanmedian(fo[m]), fnote) / 1200 if np.isfinite(fo[m]).any() else None
        row = dict(material=k, note=note, name='%s%d' % (NOTE_NAMES[note % 12], 2 + note // 12), note_hz=r(fnote, 2),
                   pitch_cents=pc, out_f0_median=r(np.nanmedian(fo[m]), 2), octaves_from_note=r(oct_, 3),
                   **{'cents_' + q: v for q, v in stats(c).items()}, voiced_share=r(np.mean(np.isfinite(fo[m])), 3))
        if 'after silence' in k:
            t2, f2, _ = yin(y, FS, 40, 1000, hop=48)
            st = settle_time(t2, cents(f2, target), 0.5 + 106 / FS)
            row['lock_ms_after_onset'] = None if st is None else r(1000 * st, 1)
        res['rows'].append(row)
        print(json.dumps(row))
    save('flatten', res)

def cmd_plots():
    """PNG plots from the JSON results (no renders)"""
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    def load(n):
        p = os.path.join(OUT, n + '.json')
        return json.load(open(p)) if os.path.exists(p) else None
    fm = load('formant')
    if fm:
        fig, ax = plt.subplots(1, 2, figsize=(10, 4))
        rows = fm['pitch_at_formant0']
        for vn, mk in (('a', 'o'), ('i', 's'), ('u', '^')):
            q = [x for x in rows if x['vowel'] == vn and abs(x['alpha_in_st']) < 6]
            ax[0].plot([x['pitch_st'] for x in q], [x['alpha_in_st'] for x in q], mk, ls='', label='vowel /%s/' % vn, alpha=0.7)
        q = [x for x in fm['corpus_ltas'] if x['formant_st'] == 0 and abs(x['alpha_in_st']) < 6]
        ax[0].plot([x['pitch_st'] for x in q], [x['alpha_in_st'] for x in q], 'kx', ls='', label='sung corpus')
        ax[0].plot([-12, 12], [-12, 12], ':', c='gray', label='formants follow pitch')
        be = load('beta')
        if be:
            k, L = be['fit_clip']['k'], be['fit_clip']['limit_st']
            pp = np.linspace(-12, 12, 241)
            ax[0].plot(pp, np.clip(k * pp, -L, L), '-', c='k', lw=1, label='grain time scale (markers): clip(%.3f P, %.2f)' % (k, L))
        ax[0].axhline(0, c='gray', lw=0.8)
        ax[0].set_xlabel('Pitch (semitones), Formant 0'); ax[0].set_ylabel('envelope shift (semitones)'); ax[0].set_ylim(-6, 6)
        ax[0].legend(fontsize=7); ax[0].set_title('Pitch moves the envelope a little')
        rows = fm['formant_at_pitch0']
        ax[1].plot([x['formant_st'] for x in rows], [x['alpha_in_st'] for x in rows], 'o', alpha=0.6, label='vowels')
        q = [x for x in fm['corpus_ltas'] if x['pitch_st'] == 0]
        ax[1].plot([x['formant_st'] for x in q], [x['alpha_in_st'] for x in q], 'kx', label='sung corpus')
        ax[1].plot([-12, 12], [-12, 12], ':', c='gray')
        ax[1].set_xlabel('Formant (semitones), Pitch 0'); ax[1].set_ylabel('envelope shift (semitones)'); ax[1].set_ylim(-14, 14)
        ax[1].legend(fontsize=7); ax[1].set_title('Formant moves the envelope by its setting')
        fig.tight_layout(); fig.savefig(os.path.join(OUT, 'formant_envelope_shift.png'), dpi=110); plt.close(fig)
    st = load('structure')
    if st:
        keys = [k for k in ('P+12_F+0_f150', 'P+5_F+0_f150', 'P-7_F+0_f150', 'P-12_F+0_f150', 'P+0_F+7_f150', 'P+0_F+0_f150') if k in st['marker_rows']]
        fig, axs = plt.subplots(2, 3, figsize=(12, 7)); axs = axs.ravel()
        for ax, k in zip(axs, keys):
            rows = st['marker_rows'][k]
            for q in rows:
                if len(q['copies']) > 6 or any(dl > 1000 for dl, _ in q['copies']):
                    ax.plot([q['m']], [0], 'rx', ms=3); continue
                for dl, a in q['copies']:
                    ax.scatter([q['m']], [dl], s=40 * min(abs(a), 1.5), c='C0' if a > 0 else 'C3', alpha=0.7)
            ax.set_title(k.replace('_', ' ') + ' (Hz)', fontsize=9); ax.set_xlabel('marker position in input (samples)')
            ax.set_ylabel('output delay of a copy (samples)')
        fig.suptitle('Where a click on a steady vowel /a/ comes out (dot size: copy amplitude; red x at 0: the click moved the grains)', fontsize=9)
        fig.tight_layout(); fig.savefig(os.path.join(OUT, 'structure_marker_maps.png'), dpi=110); plt.close(fig)
    tm = load('timing')
    if tm:
        fig, ax = plt.subplots(figsize=(7, 4))
        for P, c in ((-7, 'C0'), (5, 'C1'), (12, 'C2'), (0, 'gray')):
            q = [x for x in tm['onsets'] if x['pitch_st'] == P and x['f0_settle_ms'] is not None]
            ax.plot([x['f0'] for x in q], [x['f0_settle_ms'] for x in q], 'o-', c=c, label='Pitch %+d' % P)
        ax.set_xscale('log'); ax.set_xlabel('vowel f0 (Hz)'); ax.set_ylabel('f0 within 20 cents after onset (ms)')
        ax.legend(); ax.set_title('Settling after an onset from silence')
        fig.tight_layout(); fig.savefig(os.path.join(OUT, 'timing_onset_settle.png'), dpi=110); plt.close(fig)
    print('plots written')

def cmd_beta():
    """the grain time scale against Pitch (whole semitones, Formant 0), marker probes every 16 samples"""
    res = {'what': 'grain time scale (semitones; how far Pitch alone moves the formants) against Pitch, f0 150 Hz, '
                   'from marker probes (structure.json has the method)', 'rows': []}
    for P in (2, 3, 4, 6, 8, -2, -3, -4, -5, -6):
        rows = marker_probe(P, 0, 150, step=16)
        a = analyse_probe(rows, 16, P, 0, 150)
        res['rows'].append(a)
        print(P, a['grain_time_scale_in_st'], a['tracks'], flush=True)
    st = os.path.join(OUT, 'structure.json')
    pts = [(q['pitch_st'], q['grain_time_scale_in_st']) for q in res['rows']]
    if os.path.exists(st):
        pts += [(q['pitch_st'], q['grain_time_scale_in_st']) for q in json.load(open(st))['markers'] if q['formant_st'] == 0 and q['f0'] == 150]
    pts = sorted((p, b) for p, b in pts if b is not None)
    res['beta_by_pitch'] = pts
    # fit: beta = clip(k P, -L, L)
    best = None
    for k in np.arange(0.30, 0.55, 0.002):
        for L in np.arange(1.8, 3.2, 0.01):
            e = sum((b - np.clip(k * p, -L, L)) ** 2 for p, b in pts)
            if best is None or e < best[0]: best = (e, k, L)
    res['fit_clip'] = dict(k=r(best[1], 3), limit_st=r(best[2], 2), rms_st=r(np.sqrt(best[0] / len(pts)), 3))
    print(res['fit_clip'])
    save('beta', res)


COMMANDS = {}


def main():
    COMMANDS.update({k[4:]: v for k, v in globals().items() if k.startswith('cmd_')})
    which = sys.argv[1:] or ['all']
    if which == ['all']: which = [k for k in ('noshift', 'structure', 'pitch', 'formant', 'level', 'timing', 'beta', 'unvoiced', 'flatten', 'plots') if k in COMMANDS]
    for w in which: COMMANDS[w]()


if __name__ == '__main__':
    main()
