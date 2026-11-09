"""Flatwater Pitch Bend, Phase 3: the shifter's scorecard (measurements/FwPitchBend/shifter/SUMMARY.md, M1 .. M8) run on
OUR shifter alone (plugins/FwPitchBend/tools/PbShifterRender) next to the reference's numbers (the JSON files of
measure_shifter.py in the same folder). The same signals and the same measures as that battery.

    python -I -B score_shifter.py [m1 m1b m2 m3 m4 m5 m6 m7 m8 blocks cpu | all]

Writes measurements/FwPitchBend/shifter/ours.json (merged per metric) and prints: metric, reference, ours, threshold,
PASS / FAIL. Renders of ours are cached in nulltest/renders/pbshift_ours/ by the renderer's own hash, the input and
the settings. FW_SHIFTER_RENDER overrides the renderer's path (default: the build folder's Release exe).
"""
import sys, os, json, hashlib, subprocess, tempfile, time
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import numpy as np
from concurrent.futures import ThreadPoolExecutor
import measure_shifter as M
import vb as H

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = M.OUT
CACHE = os.path.join(H.TMP, 'pbshift_ours'); os.makedirs(CACHE, exist_ok=True)
EXE = os.environ.get('FW_SHIFTER_RENDER', os.path.join(H.BUILD, 'PbShifterRender_artefacts', 'Release', 'PbShifterRender.exe'))
FS = M.FS
_EXE_HASH = None


def exe_hash():
    global _EXE_HASH
    if _EXE_HASH is None:
        _EXE_HASH = hashlib.sha1(open(EXE, 'rb').read()).hexdigest()[:12]
    return _EXE_HASH


def ref(name):
    return json.load(open(os.path.join(OUT, name + '.json')))


def beta(P):
    return float(np.clip(0.418 * P, -2.51, 2.51))


# ---- rendering ours -------------------------------------------------------------------------------------------------
def job(x, P=0.0, F=0.0, flatten=0.0, fs=FS, block=512):
    x = np.asarray(x, np.float32)
    x = x[None, :] if x.ndim == 1 else x
    h = hashlib.sha1(x.tobytes()); h.update(repr((exe_hash(), fs, float(P), float(F), float(flatten), block)).encode())
    return dict(x=x, P=P, F=F, flatten=flatten, fs=fs, block=block, key=h.hexdigest()[:20])


def _render(j):
    f = os.path.join(CACHE, j['key'] + '.npy')
    if os.path.exists(f): return
    d = tempfile.mkdtemp(prefix='pbo_', dir=H.TMP)
    fi, fo = os.path.join(d, 'in.f32'), os.path.join(d, 'out.f32')
    ch = j['x'].shape[0]
    j['x'].T.astype(np.float32).tofile(fi)
    cmd = [EXE, fi, fo, str(ch), str(j['fs']), 'pitch=%r' % float(j['P']), 'formant=%r' % float(j['F']),
           'flatten=%r' % float(j['flatten']), 'block=%d' % j['block']]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode or not os.path.exists(fo): raise RuntimeError(' '.join(cmd) + '\n' + r.stderr)
    y = np.fromfile(fo, np.float32).reshape(-1, ch).T
    np.save(f, y)
    for q in (fi, fo): os.remove(q)
    os.rmdir(d)


def render(jobs, mono=True):
    with ThreadPoolExecutor(8) as ex: list(ex.map(_render, jobs))
    ys = [np.load(os.path.join(CACHE, j['key'] + '.npy')).astype(np.float64) for j in jobs]
    return [y[0] for y in ys] if mono else ys


# ---- table ----------------------------------------------------------------------------------------------------------
ROWS = []


def row(metric, what, reference, ours, threshold, ok):
    ROWS.append(dict(metric=metric, what=what, reference=reference, ours=ours, threshold=threshold,
                     result=None if ok is None else ('PASS' if ok else 'FAIL')))


def fmt(v):
    if isinstance(v, float): return '%.3g' % v
    return str(v)


def print_rows(rows):
    for q in rows:
        print('%-4s %-34s ref %-22s ours %-22s thr %-18s %s' % (q['metric'], q['what'][:34], fmt(q['reference'])[:22], fmt(q['ours'])[:22],
                                                             fmt(q['threshold'])[:18], q['result'] or 'report'))


def r_(v, k=2):
    return None if v is None or not np.isfinite(v) else round(float(v), k)


# ---- M1: pitch on the vowel grid ------------------------------------------------------------------------------------
def m1():
    shifts = [-12, -7, -1, 0, 1, 5, 12]; f0s = [80, 110, 150, 220, 300, 440, 600]
    jobs, meta = [], []
    for vn, fm in M.VOWELS.items():
        for f0 in f0s:
            x = M.vowel(2.0, f0, formants=fm)[0]
            for s in shifts: jobs.append(job(x, s)); meta.append((vn, f0, s, x))
    ys = render(jobs)
    out = []
    for (vn, f0, s, x), y in zip(meta, ys):
        c, gross, _ = M.f0_error(x, y, 2 ** (s / 12), hop=FS // 200, lag=106, fmin=min(55.0, 0.6 * f0))
        st = M.stats(c)
        out.append(dict(material=vn, f0=f0, shift_st=s, gross=r_(gross, 3), median=st.get('median'), abs_p90=st.get('abs_p90')))
    med = max(abs(q['median'] or 0) for q in out); p90 = max(q['abs_p90'] or 0 for q in out); g = max(q['gross'] or 0 for q in out)
    rs = ref('pitch')['synthetic']
    rmed = max(abs(q.get('cents_median') or 0) for q in rs); rp90 = max(q.get('cents_abs_p90') or 0 for q in rs); rg = max(q['gross'] or 0 for q in rs)
    row('M1', 'vowel grid |median| max (c)', rmed, med, '<= 1', med <= 1.0)
    row('M1', 'vowel grid abs p90 max (c)', rp90, p90, '<= 3', p90 <= 3.0)
    row('M1', 'vowel grid gross max', rg, g, '0', g == 0)
    # 44.1 and 96 kHz (pitch.json 'rates': -7 0 +5 +12 at f0 150 / 300)
    jobs, meta = [], []
    for fs in (44100, 96000):
        for f0 in (150, 300):
            x = M.vowel(2.0, f0, fs=fs)[0]
            for s in (-7, 0, 5, 12): jobs.append(job(x, s, fs=fs)); meta.append((fs, f0, s, x))
    rates = []
    for (fs, f0, s, x), y in zip(meta, render(jobs)):
        c, gross, _ = M.f0_error(x, y, 2 ** (s / 12), fs=fs, hop=fs // 200, lag=int(round(105.5 * fs / 48000)), fmin=min(55.0, 0.6 * f0))
        st = M.stats(c)
        rates.append(dict(fs=fs, f0=f0, shift_st=s, gross=r_(gross, 3), median=st.get('median'), abs_p90=st.get('abs_p90')))
    rr = ref('pitch')['rates']
    row('M1', '44.1/96k: |median| max, p90 max (c)', '%.2f, %.2f' % (max(abs(q.get('cents_median') or 0) for q in rr), max(q.get('cents_abs_p90') or 0 for q in rr)),
        '%.2f, %.2f' % (max(abs(q['median'] or 0) for q in rates), max(q['abs_p90'] or 0 for q in rates)), '<= 1, <= 3',
        max(abs(q['median'] or 0) for q in rates) <= 1 and max(q['abs_p90'] or 0 for q in rates) <= 3 and max(q['gross'] or 0 for q in rates) == 0)
    bad = [(q['material'], q['f0'], q['shift_st'], q['median'], q['abs_p90'], q['gross']) for q in out
           if abs(q['median'] or 0) > 1 or (q['abs_p90'] or 0) > 3 or (q['gross'] or 0) > 0]
    return dict(rows=out, rates=rates, worst=bad[:20])


def m1b():
    ph = M.corpus_phrases(1.2, 14, 'sung')
    jobs, meta = [], []
    for name, grp, x in ph:
        for s in (-12, -7, -1, 1, 5, 12): jobs.append(job(x, s)); meta.append((name, s, x))
    ys = render(jobs)
    out = []
    for (name, s, x), y in zip(meta, ys):
        c, gross, _ = M.f0_error(x, y, 2 ** (s / 12), hop=FS // 200, lag=106, fmin=55.0)
        st = M.stats(c)
        out.append(dict(phrase=name, shift_st=s, gross=r_(gross, 3), median=st.get('median'), abs_p90=st.get('abs_p90')))
    p90 = float(np.median([q['abs_p90'] for q in out if q['abs_p90'] is not None])); g = max(q['gross'] or 0 for q in out)
    rs = ref('pitch')['corpus_summary_sung']
    row('M1b', 'sung phrases: median p90 (c)', rs['abs_p90_median'], r_(p90), '<= 10', p90 <= 10)
    row('M1b', 'sung phrases: max gross', rs['gross_max'], r_(g, 3), '<= 0.15', g <= 0.15)
    return dict(rows=out)


# ---- M2: envelope ---------------------------------------------------------------------------------------------------
def m2():
    jobs, meta = [], []
    for vn, fm in M.VOWELS.items():
        for f0 in (110, 150, 220):
            x = M.vowel(2.0, f0, formants=fm)[0]
            for F in (-12, -7, -3, 3, 7, 12): jobs.append(job(x, 0, F)); meta.append(('formant', vn, f0, 0, F))
            for P in (-12, -7, -5, -1, 1, 5, 7, 12): jobs.append(job(x, P, 0)); meta.append(('pitch', vn, f0, P, 0))
            for P, F in ((-7, 7), (-7, -7), (5, -5), (5, 5), (12, -12), (-12, 12), (7, 3)): jobs.append(job(x, P, F)); meta.append(('combined', vn, f0, P, F))
    ys = render(jobs)
    out = []
    for (kind, vn, f0, P, F), y in zip(meta, ys):
        fo_, fo, do = M.harmonic_env(y)
        env = (lambda fm: (lambda f: M.resonator_db(f, fm)))(M.VOWELS[vn])
        a, e, e1 = M.env_shift_fit(fo, do, env)
        out.append(dict(kind=kind, vowel=vn, f0=f0, P=P, F=F, alpha_st=r_(12 * np.log2(a)), expected_st=r_(F + beta(P)), err_db=r_(e)))
    rf = ref('formant')
    refrows = {('formant', q['vowel'], q['f0'], q['pitch_st'], q['formant_st']): q for q in rf['formant_at_pitch0']}
    refrows.update({('pitch', q['vowel'], q['f0'], q['pitch_st'], q['formant_st']): q for q in rf['pitch_at_formant0']})
    refrows.update({('combined', q['vowel'], q['f0'], q['pitch_st'], q['formant_st']): q for q in rf['combined']})
    per = {}
    for q in out: per.setdefault((q['kind'], q['P'], q['F']), []).append(q)
    worst_dev, worst_err, settings, fails = 0.0, -99.0, [], 0
    for (kind, P, F), qs in sorted(per.items()):
        dev = float(np.median([q['alpha_st'] - q['expected_st'] for q in qs]))
        err = float(np.median([q['err_db'] for q in qs]))
        rq = [refrows[(kind, q['vowel'], q['f0'], P, F)] for q in qs if (kind, q['vowel'], q['f0'], P, F) in refrows]
        rdev = float(np.median([q['alpha_in_st'] - (F + beta(P)) for q in rq])) if rq else None
        rerr = float(np.median([q['err_db'] for q in rq])) if rq else None
        ok = abs(dev) <= 0.5 and (rerr is None or err <= rerr + 1.0)
        fails += not ok
        worst_dev = max(worst_dev, abs(dev)); worst_err = max(worst_err, err - (rerr or err))
        settings.append(dict(kind=kind, P=P, F=F, ours_dev_st=r_(dev), ref_dev_st=r_(rdev), ours_err_db=r_(err), ref_err_db=r_(rerr), ok=ok))
    rworst = max(abs(s['ref_dev_st']) for s in settings if s['ref_dev_st'] is not None)
    row('M2', 'envelope |median - (F+beta)| max (st)', r_(rworst), r_(worst_dev), '<= 0.5', worst_dev <= 0.5)
    row('M2', 'envelope err minus ref, worst (dB)', 0.0, r_(worst_err), '<= +1', worst_err <= 1.0)
    return dict(settings=settings, rows=out, settings_failing=fails)


# ---- M3: level ------------------------------------------------------------------------------------------------------
def level_materials():
    mats = {'vowel a 150 Hz': M.vowel(3, 150)[0], 'vowel i 300 Hz': M.vowel(3, 300, formants=M.VOWELS['i'])[0],
            'vowel u 110 Hz': M.vowel(3, 110, formants=M.VOWELS['u'])[0], 'noise': M.noise(3),
            'noise band 4-10 kHz (sibilant)': M.band(M.noise(3, 0.0), 4000, 10000) * 10 ** (-18 / 20) / np.sqrt(np.mean(M.band(M.noise(3, 0.0), 4000, 10000) ** 2))}
    for name, grp, x in M.corpus_phrases(1.5, 4, 'sung'): mats['sung ' + name] = x
    for name, grp, x in M.corpus_phrases(1.5, 2, 'rap'): mats['rap ' + name] = x
    return mats


def m3():
    mats = level_materials()
    settings = [(0, 0), (-12, 0), (-7, 0), (-1, 0), (1, 0), (5, 0), (12, 0), (0, -12), (0, -7), (0, 7), (0, 12), (-7, 7), (5, -5)]
    jobs, meta = [], []
    for k, x in mats.items():
        for P, F in settings: jobs.append(job(x, P, F)); meta.append((k, P, F, x))
    ys = render(jobs)
    tab = ref('level')['table_out_minus_in_db']
    cells, n_ok, worst = [], 0, (0.0, None)
    for (k, P, F, x), y in zip(meta, ys):
        s0 = FS // 2
        ours = H.rms_db(y[s0:]) - H.rms_db(x[s0:])
        key = '%+d/%+d' % (P, F)
        rv = tab.get(k, {}).get(key)
        noise_pm1 = ('noise' in k) and abs(P) == 1                      # (M8: not copied)
        d = None if rv is None else ours - rv
        ok = d is not None and (abs(d) <= 1.0 or noise_pm1)
        n_ok += ok
        if d is not None and not noise_pm1 and abs(d) > abs(worst[0]): worst = (d, (k, key))
        cells.append(dict(material=k, setting=key, ref_db=rv, ours_db=r_(ours), diff_db=r_(d), ok=bool(ok)))
    row('M3', 'level cells within 1 dB of ref', '%d cells' % len(cells), '%d of %d' % (n_ok, len(cells)), 'all', n_ok == len(cells))
    row('M3', 'level worst diff (dB)', 0.0, '%+.2f %s' % (worst[0], worst[1][1] if worst[1] else ''), '+-1', abs(worst[0]) <= 1.0)
    return dict(cells=cells, worst=[worst[0], worst[1]])


# ---- M4: delay and timing -------------------------------------------------------------------------------------------
def m4():
    res = {'delay': [], 'onsets': [], 'jumps': []}
    jobs, meta = [], []
    for fs, dref in ((48000, 105.53125), (96000, 211.0625), (44100, 120.4375)):
        for name, x in (('vowel a 150 Hz', M.vowel(3, 150, fs=fs)[0]), ('vowel i 220 Hz', M.vowel(3, 220, fs=fs, formants=M.VOWELS['i'])[0]),
                        ('vowel a 150 Hz vibrato 100 c', M.vowel(3, 150, fs=fs, vib=100)[0])):
            jobs.append(job(x, fs=fs)); meta.append((fs, dref, name, x))
    for name, grp, x in M.corpus_phrases(2.0, 2, 'sung'):
        jobs.append(job(x)); meta.append((48000, 105.53125, 'sung ' + name, x))
    ys = render(jobs)
    worst_d, worst_res = 0.0, -999.0
    for (fs, dref, name, x), y in zip(meta, ys):
        sc = fs / 48000
        d, g, e = M.best_delay(x, y, 90 * sc, 140 * sc, fs)
        xd = M.frac_delay(x, dref)
        res_db = H.rms_db(y[fs // 2:] - xd[fs // 2:]) - H.rms_db(xd[fs // 2:])
        worst_d = max(worst_d, abs(d - dref)); worst_res = max(worst_res, res_db)
        res['delay'].append(dict(fs=fs, material=name, delay=r_(d, 4), expected=dref, residual_vs_delayed_input_db=r_(res_db, 1)))
    row('M4', 'no-shift delay error max (samples)', 0.0, r_(worst_d, 4), '<= 0.05', worst_d <= 0.05)
    row('M4', 'no-shift residual on voice, worst (dB)', -57.0, r_(worst_res, 1), '< -55', worst_res < -55)
    # onsets and jumps, as measure_shifter.cmd_timing
    t0 = int(0.5 * FS)
    rt = ref('timing')
    ron = {(q['f0'], q['pitch_st']): q for q in rt['onsets']}
    rj = {(q['f_from'], q['f_to'], q['pitch_st']): q for q in rt['jumps']}
    jobs, meta = [], []
    for f0 in (100, 200, 400):
        x = np.zeros(int(1.5 * FS)); v = M.vowel(1.0, f0)[0]; x[t0:t0 + len(v)] = v
        for P in (0, -7, 5, 12): jobs.append(job(x, P)); meta.append(('on', f0, P, x))
    for fa, fb in ((150, 225), (225, 150), (200, 400), (400, 200), (150, 160)):
        n = int(2.5 * FS); tt = np.arange(n) / FS
        f = np.where(tt < 1.25, fa, fb).astype(float)
        ph = np.cumsum(f) / FS; src = np.diff(np.floor(ph), prepend=0.0)
        yv = np.zeros(n)
        from scipy.signal import lfilter
        for fc, bw, g in M.VOWELS['a']:
            rr = np.exp(-np.pi * bw / FS)
            yv += g * lfilter([1 - rr], [1, -2 * rr * np.cos(2 * np.pi * fc / FS), rr * rr], src)
        x = yv * 10 ** (-18 / 20) / np.sqrt(np.mean(yv ** 2))
        for P in (0, 5, -7): jobs.append(job(x, P)); meta.append(('jump', (fa, fb), P, x))
    ys = render(jobs)
    on_fail = j_fail = 0; on_worst = j_worst = -999.0; od_worst = 0
    for (kind, f, P, x), y in zip(meta, ys):
        ratio = 2 ** (P / 12)
        if kind == 'on':
            f0 = f
            i_in = M.env_onset(x, t0 - 100, ref=np.median(np.sqrt(np.convolve(x ** 2, np.ones(32) / 32, 'same'))[t0 + FS // 4:t0 + FS // 2]))
            i_out = M.env_onset(y, t0 - 100)
            lo = 0.7 * f0 * min(1, ratio); hi = 1.5 * f0 * max(1, ratio)
            t, fo, _ = M.yin(y, FS, lo, hi, hop=48, win=int(2.2 * FS / (f0 * min(1, ratio))))
            st = M.settle_time(t, M.cents(fo, f0 * ratio), (t0 + 106) / FS)
            st = None if st is None else 1000 * st
            rq = ron[(f0, P)]
            ok = st is not None and st <= rq['f0_settle_ms'] + 10
            on_fail += not ok
            if st is not None: on_worst = max(on_worst, st - rq['f0_settle_ms'])
            od = None if i_out is None else int(i_out - i_in)
            if od is not None: od_worst = max(od_worst, abs(od - rq['onset_delay_samples']))
            res['onsets'].append(dict(f0=f0, P=P, settle_ms=r_(st, 1), ref_settle_ms=rq['f0_settle_ms'], onset_delay=od, ref_onset_delay=rq['onset_delay_samples'], ok=bool(ok)))
        else:
            fa, fb = f
            lo = 0.7 * min(fa, fb) * min(1, ratio); hi = 1.5 * max(fa, fb) * max(1, ratio)
            t, fo, _ = M.yin(y, FS, lo, hi, hop=48, win=int(2.2 * FS / lo))
            st = M.settle_time(t, M.cents(fo, fb * ratio), 1.25 + 106 / FS)
            st = None if st is None else 1000 * st
            rq = rj[(fa, fb, P)]
            rs = rq['f0_settle_ms']
            ok = st is not None and (rs is None or st <= rs + 10)
            j_fail += not ok
            if st is not None and rs is not None: j_worst = max(j_worst, st - rs)
            res['jumps'].append(dict(f_from=fa, f_to=fb, P=P, settle_ms=r_(st, 1), ref_settle_ms=rs, ok=bool(ok)))
    row('M4', 'onset settle, worst minus ref (ms)', 0.0, r_(on_worst, 1), '<= +10', on_fail == 0)
    row('M4', 'jump settle, worst minus ref (ms)', 0.0, r_(j_worst, 1), '<= +10', j_fail == 0)
    row('M4', 'onset delay, worst |ours - ref|', 0, od_worst, '+-20 samples', None)
    return res


# ---- M5: unvoiced ---------------------------------------------------------------------------------------------------
def centroid(z, lo=50, hi=20000):
    f, P = M.spectrum_db(z[FS // 2:]); m = (f > lo) & (f < hi)
    return float(np.sum(f[m] * P[m]) / np.sum(P[m]))


def m5():
    mats = {'white noise': M.noise(2.0), 'band 4-10 kHz': M.band(M.noise(2.0, 0.0), 4000, 10000), 'band 1.5-3 kHz': M.band(M.noise(2.0, 0.0), 1500, 3000)}
    for k in list(mats)[1:]: mats[k] = mats[k] * 10 ** (-18 / 20) / np.sqrt(np.mean(mats[k] ** 2))
    settings = ((-12, 0), (-7, 0), (-1, 0), (1, 0), (5, 0), (12, 0), (0, 7), (0, -7))
    jobs, meta = [], []
    for k, x in mats.items():
        for P, F in settings: jobs.append(job(x, P, F)); meta.append((k, P, F, x))
    ys = render(jobs)
    rn = {(q['material'], q['pitch_st'], q['formant_st']): q for q in ref('unvoiced')['noise']}
    out, lev_worst, cen_worst = [], 0.0, 0.0
    for (k, P, F, x), y in zip(meta, ys):
        s0 = FS // 2
        lev = H.rms_db(y[s0:]) - H.rms_db(x[s0:])
        xd = M.frac_delay(x, 105.53125)
        corr = float(np.dot(y[s0:], xd[s0:]) / np.sqrt(np.dot(y[s0:], y[s0:]) * np.dot(xd[s0:], xd[s0:]) + 1e-30))
        cin, cout = centroid(x), centroid(y)
        sh = 12 * np.log2(cout / cin)
        rq = rn.get((k, P, F), {})
        rsh = 12 * np.log2(rq['centroid_out_hz'] / rq['centroid_in_hz']) if rq else None
        q = dict(material=k, P=P, F=F, level_db=r_(lev), ref_level_db=rq.get('out_minus_in_db'), centroid_shift_st=r_(sh),
                 ref_centroid_shift_st=r_(rsh), expected_st=r_(F + beta(P)), corr_with_delayed_input=r_(corr, 3),
                 ref_corr=rq.get('correlation_with_delayed_input'))
        out.append(q)
        if abs(P) != 1:
            lev_worst = max(lev_worst, abs(lev + 0.6))
            if k == 'band 4-10 kHz': cen_worst = max(cen_worst, abs(sh - (F + beta(P))))
    row('M5', 'noise level vs -0.6 dB, worst (dB)', 0.05, r_(lev_worst), '+-1', lev_worst <= 1.0)
    row('M5', 'band 4-10k centroid vs F+beta (st)', 0.06, r_(cen_worst), '+-0.5', cen_worst <= 0.5)
    return dict(rows=out)


# ---- M6: Flatten ----------------------------------------------------------------------------------------------------
def m6():
    srcs = {'vowel a 110 Hz': M.vowel(2.0, 110)[0], 'vowel a 150 Hz': M.vowel(2.0, 150)[0], 'vowel a 220 Hz': M.vowel(2.0, 220)[0],
            'vowel a 300 Hz': M.vowel(2.0, 300)[0], 'vowel a 150 Hz vibrato 50 c': M.vowel(2.0, 150, vib=50)[0],
            'vowel a glide 110 to 330 Hz': M.vowel(2.0, 110, f0_end=330)[0]}
    for name, grp, x in M.corpus_phrases(1.5, 2, 'sung'): srcs['sung ' + name] = x
    on = np.zeros(int(1.5 * FS)); on[int(0.5 * FS):] = M.vowel(1.0, 150)[0]; srcs['vowel a 150 Hz after silence'] = on
    jobs, meta = [], []
    for k, x in srcs.items():
        for note in (0, 5, 12, 21, 23):
            for pc in (0, 500):
                if pc and note != 12: continue
                fn = 440.0 * 2 ** ((48 + note - 69) / 12)
                jobs.append(job(x, pc / 100, 0, fn)); meta.append((k, note, pc, fn))
    ys = render(jobs)
    out = []
    for (k, note, pc, fn), y in zip(meta, ys):
        target = fn * 2 ** (pc / 1200)
        t, fo, _ = M.yin(y, FS, 40, 1000, hop=240)
        m = t > (0.7 if 'after silence' in k else 0.3)
        st = M.stats(M.cents(fo[m], target))
        q = dict(material=k, note=note, pitch_cents=pc, median=st.get('median'), abs_p90=st.get('abs_p90'))
        if 'after silence' in k:
            t2, f2, _ = M.yin(y, FS, 40, 1000, hop=48)
            lk = M.settle_time(t2, M.cents(f2, target), 0.5 + 106 / FS)
            q['lock_ms'] = None if lk is None else r_(1000 * lk, 1)
        out.append(q)
    steady = [q for q in out if k_steady(q['material'])]
    smed = max(abs(q['median'] or 0) for q in steady)
    vib = max(q['abs_p90'] or 0 for q in out if 'vibrato' in q['material'])
    locks = [q.get('lock_ms') for q in out if 'after silence' in q['material']]
    lk = max((l if l is not None else 1e9) for l in locks)
    rr = ref('flatten')['rows']
    rsmed = max(abs(q['cents_median'] or 0) for q in rr if k_steady(q['material']))
    rvib = max(q['cents_abs_p90'] or 0 for q in rr if 'vibrato' in q['material'])
    rlk = max(q['lock_ms_after_onset'] for q in rr if 'after silence' in q['material'])
    row('M6', 'Flatten steady |median| max (c)', rsmed, r_(smed), '<= 1', smed <= 1.0)
    row('M6', 'Flatten vibrato p90 max (c)', rvib, r_(vib), '<= 10', vib <= 10.0)
    row('M6', 'Flatten lock after onset, max (ms)', rlk, r_(lk, 1), '<= 60', lk <= 60.0)
    return dict(rows=out)


def k_steady(name):
    return name in ('vowel a 110 Hz', 'vowel a 150 Hz', 'vowel a 220 Hz', 'vowel a 300 Hz')


# ---- M7: gate and noise robustness ----------------------------------------------------------------------------------
def m7():
    jobs, meta = [], []
    for db in (-80, -60, -56, -54, -53, -52, -51, -50, -49, -48, -40, -30):
        x = M.vowel(2.0, 150, db)[0]; jobs.append(job(x, 5)); meta.append(('gate', db, 5, x))
    for snr in (30, 20, 10, 5, 0):
        x = M.vowel(2.0, 150)[0] + M.noise(2.0, -18 - snr, seed=11)
        for P in (-7, 5): jobs.append(job(x, P)); meta.append(('snr', snr, P, x))
    ys = render(jobs)
    out = {'gate': [], 'snr': []}
    for (kind, v, P, x), y in zip(meta, ys):
        c, gross, _ = M.f0_error(x, y, 2 ** (P / 12), fmin=90, fmax=400)
        out[kind].append(dict(level_or_snr=v, P=P, gross=r_(gross, 3), median=M.stats(c).get('median'), abs_p90=M.stats(c).get('abs_p90'),
                              out_minus_in_db=r_(H.rms_db(y[FS // 2:]) - H.rms_db(x[FS // 2:]))))
    shifted = [q['level_or_snr'] for q in out['gate'] if (q['gross'] or 0) == 0]
    thr = min(shifted) if shifted else None
    row('M7', 'gate: lowest shifted level (dBFS)', -50, thr, '-51 +-3', thr is not None and abs(thr + 51) <= 3)
    ok_snr = [q['level_or_snr'] for q in out['snr'] if q['gross'] is not None]
    worst_snr = min((s for s in (30, 20, 10, 5, 0) if all((q['gross'] or 0) == 0 for q in out['snr'] if q['level_or_snr'] >= s and q['gross'] is not None)), default=None)
    row('M7', 'lowest SNR with no gross errors (dB)', 10, worst_snr, '<= 10', worst_snr is not None and worst_snr <= 10)
    return out


# ---- M8: behaviours decided ------------------------------------------------------------------------------------------
def m8():
    out = {}
    # chords
    t = np.arange(2 * FS) / FS
    chords = {'two sines 220 + 330 Hz': ([220.0, 330.0], None), 'harmonic complexes C3 E3 G3': ([130.81, 164.81, 196.0], 'hc'),
              'harmonic complexes A2 + E3': ([110.0, 164.81], 'hc')}
    jobs, meta = [], []
    for k, (fr, kind) in chords.items():
        if kind is None: x = sum(0.05 * np.sin(2 * np.pi * f * t) for f in fr); parts = fr
        else:
            x = sum(M.harmonic_complex(f) for f in fr); parts = sorted(set(round(f * h, 2) for f in fr for h in range(1, 13) if f * h < 6000))
        for P in (-7, 5, 12): jobs.append(job(x, P)); meta.append((k, P, parts))
    ys = render(jobs)
    rc = {(q['material'], q['pitch_st']): q for q in ref('unvoiced')['chords']}
    out['chords'] = []
    for (k, P, parts), y in zip(meta, ys):
        e = M.partial_energy(y, parts, 2 ** (P / 12))
        out['chords'].append(dict(material=k, P=P, ours=e, ref=rc[(k, P)]['energy_share']))
        row('M8', 'chord %s P%+d shifted share' % (k.split(' ')[-3] if 'complex' in k else 'sines', P), rc[(k, P)]['energy_share']['shifted'], e['shifted'], 'report', None)
    # octave down
    m4r = DATA.get('m4', {}).get('jumps', [])
    od = [q for q in m4r if q['f_from'] == 400 and q['f_to'] == 200 and q['P'] != 0]
    row('M8', 'octave down 400->200 settle (ms)', 'never', ', '.join(str(q['settle_ms']) for q in od) or 'run m4', 'follows (decided)', None)
    m5r = DATA.get('m5', {}).get('rows', [])
    pm1 = [q['level_db'] for q in m5r if abs(q['P']) == 1]
    corr = [abs(q['corr_with_delayed_input']) for q in m5r if abs(q['P']) != 1 and (q['P'] or q['F'])]
    row('M8', 'noise re-granulated: max |corr|', 0.02, r_(max(corr), 3) if corr else 'run m5', 'report', None)
    row('M8', 'noise at P +-1 level (dB)', 1.8, ', '.join('%+.2f' % v for v in pm1) or 'run m5', 'not copied (decided)', None)
    return out


# ---- block size and CPU ---------------------------------------------------------------------------------------------
def blocks():
    x1 = M.vowel(2.0, 150, vib=50)[0]
    ph = M.corpus_phrases(2.0, 1, 'sung')
    x2 = ph[0][2][:3 * FS] if ph else M.vowel(3.0, 220)[0]
    st = np.stack([x2, 0.7 * x2[::-1].copy()])
    cases = [(x1, 5, 0, 0), (x1, -7, 3, 0), (x2, -12, 0, 0), (x2, 0, 0, 196.0), (st, 7, -2, 0), (x2, 0, 0, 0)]
    worst = -999.0
    out = []
    for x, P, F, fl in cases:
        ys = render([job(x, P, F, fl, block=b) for b in (64, 512, 4096, 1)], mono=False)
        for b, y in zip((512, 4096, 1), ys[1:]):
            d = H.rms_db(y - ys[0]) - H.rms_db(ys[0])
            worst = max(worst, d)
            out.append(dict(P=P, F=F, flatten=fl, block=b, null_vs_64_db=r_(d, 1)))
    row('blk', 'block 64 vs 1/512/4096, worst null', None, r_(worst, 1), '< -90 dB', worst < -90)
    return dict(rows=out)


def cpu():
    x = np.stack([M.vowel(10.0, 180, vib=40)[0], M.vowel(10.0, 180, vib=40)[0] * 0.8]).astype(np.float32)
    d = tempfile.mkdtemp(prefix='pbc_', dir=H.TMP)
    fi, fo = os.path.join(d, 'in.f32'), os.path.join(d, 'out.f32')
    x.T.tofile(fi)
    res = {}
    for name, args in (('shifted P+5 F-2', ['pitch=5', 'formant=-2']), ('no shift', []), ('flatten', ['flatten=196']),
                       ('LFO through zero', ['lfo=3', 'lforate=1'])):
        r = subprocess.run([EXE, fi, fo, '2', '48000', 'block=256', 'repeat=5'] + args, capture_output=True, text=True, check=True)
        secs = float(r.stdout.split()[0])                           # (the renderer's own timing loop: processing only)
        res[name] = r_(100 * secs / 10.0, 2)
        row('cpu', 'stereo 48k %s (%% of a core)' % name, None, res[name], '< 5', res[name] < 5)
    for q in (fi, fo): os.remove(q)
    os.rmdir(d)
    return res


# ---- main -----------------------------------------------------------------------------------------------------------
def load_ours():
    p = os.path.join(OUT, 'ours.json')
    return json.load(open(p)) if os.path.exists(p) else {}


CMDS = dict(m1=m1, m1b=m1b, m2=m2, m3=m3, m4=m4, m5=m5, m6=m6, m7=m7, m8=m8, blocks=blocks, cpu=cpu)


DATA = {}


def main():
    which = sys.argv[1:] or ['all']
    if which == ['all']: which = list(CMDS)
    data = DATA
    data.update(load_ours())
    table = {q['metric'] + '|' + q['what']: q for q in data.get('table', [])}
    for w in which:
        n0 = len(ROWS)
        t = time.time()
        data[w] = CMDS[w]()
        for q in ROWS[n0:]:
            for k in [k for k in table if table[k]['metric'] == q['metric'] and k.split('|')[1] == q['what']]: del table[k]
            table[q['metric'] + '|' + q['what']] = q
        print_rows(ROWS[n0:])
        print('  (%s: %.0f s)' % (w, time.time() - t), flush=True)
    data['table'] = sorted(table.values(), key=lambda q: (['M1', 'M1b', 'M2', 'M3', 'M4', 'M5', 'M6', 'M7', 'M8', 'blk', 'cpu'].index(q['metric']), q['what']))
    data['renderer'] = 'plugins/FwPitchBend/tools/PbShifterRender (sha1 %s)' % exe_hash()
    with open(os.path.join(OUT, 'ours.json'), 'w') as f: json.dump(data, f, indent=1)


if __name__ == '__main__':
    main()
