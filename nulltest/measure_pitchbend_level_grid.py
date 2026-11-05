"""Flatwater Pitch Bend, Phase 3: the reference's voiced level per material at fixed settings, and our shifter against it
on the factory presets' two takes.

    python -I -B measure_pitchbend_level_grid.py ref     the reference on a Pitch x Formant grid (whole semitones, the
                                                         host parameters) over vowels and phrases; writes
                                                         measurements/fwpitchbend/shifter/levelgrid.json
    python -I -B measure_pitchbend_level_grid.py fit     the fit per setting (a straight line in the input's ln kurtosis)
                                                         and the C++ tables FwPitchBendShifter.cpp carries (printed)
    python -I -B measure_pitchbend_level_grid.py takes   ours (FW_SHIFTER_RENDER, as score_pitchbend_shifter.py) against
                                                         the reference at fixed settings on the presets' sung and spoken
                                                         takes, mono: level, tone and the tone per band; writes the
                                                         'takes' part of levelgrid.json

What the grid found: at one setting the reference's level moves with the material by up to 3.5 dB, and most of that
follows how peaky the voice's waveform is. The kurtosis of the input (E x^4 / (E x^2)^2 over 20 ms windows, the
windows' logs averaged by their energy) explains it: per setting the level is a straight line in ln kurtosis, with
a slope of about -1 .. -2.7 dB per unit at most settings (a low voice or wide formants: peakier, quieter). The rest
per setting is 0.2 .. 0.7 dB rms. Two ridges are not explained by it: where F + beta equals Pitch (the grains read at
the pitch ratio, e.g. +5 / +3) the level drops by about 3 dB, and at Pitch +12 with Formant -3 .. -5 high voices
(300 Hz) drop by 4 .. 7 dB; the fit leaves those to the grid points and its outlier rejection.
"""
import sys, os, json
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import numpy as np
import measure_pitchbend_shifter as M
import score_pitchbend_shifter as O
import pb_host as H

FS = M.FS
OUT = os.path.join(M.OUT, 'levelgrid.json')
PG = [-12, -9, -7, -5, -3, -1, 0, 1, 3, 5, 7, 9, 12]
FG = [-12, -9, -7, -5, -3, 0, 3, 5, 7, 9, 12]
KURT_REF = 0.83                                  # ln kurtosis of a typical voice (ln 2.3)
TAKE_SETTINGS = [(12, 12), (12, -3), (0, -5), (12, 0), (0, 12), (5, 0), (-7, 0), (0, 7), (0, -7), (-12, 0), (5, -5), (-7, 7)]


def materials():
    mats = {}
    for vn in ('a', 'i', 'u'):
        for f0 in (80, 150, 220, 300): mats['%s %d' % (vn, f0)] = M.vowel(2.0, f0, formants=M.VOWELS[vn])[0]
    for bw, f0 in ((2.0, 110), (0.5, 220)):
        fm = tuple((a, b * bw, c) for a, b, c in M.VOWELS['a'])
        mats['a bw%g %d' % (bw, f0)] = M.vowel(2.0, f0, formants=fm)[0]
    for k, x in O.level_materials().items():
        if k.startswith('sung') or k.startswith('rap'): mats[k] = x
    return mats


def ln_kurtosis(x, win=0.02):
    """the input's peakiness: ln (E x^4 / (E x^2)^2) over win-long windows, averaged weighted by the windows' energy"""
    x = np.asarray(x, np.float64)[FS // 2:]
    n = int(win * FS); k = np.ones(n) / n
    e2 = np.convolve(x * x, k, 'valid'); e4 = np.convolve(x ** 4, k, 'valid')
    m = e2 > 1e-3 * np.max(e2)
    return float(np.sum(np.log(e4[m] / e2[m] ** 2) * e2[m]) / np.sum(e2[m]))


def level(x, y):
    return H.rms_db(y[FS // 2:]) - H.rms_db(x[FS // 2:])


def load():
    return json.load(open(OUT)) if os.path.exists(OUT) else {}


def cmd_ref():
    mats = materials()
    jobs, meta = [], []
    for k, x in mats.items():
        for P in PG:
            for F in FG:
                if P == 0 and F == 0: continue
                jobs.append(M.semi_job(x, P, F)); meta.append((k, P, F))
    ys = M.render_all(jobs)
    lev = {}
    for (k, P, F), y in zip(meta, ys): lev.setdefault(k, {})['%+d/%+d' % (P, F)] = round(level(mats[k], y), 3)
    d = load()
    d.update(what='the reference\'s voiced level (output minus input rms, dB, from 0.5 s on, Mix 100, whole semitones through '
                  'the host parameters) per material and Pitch/Formant setting; ln_kurtosis: the input\'s peakiness '
                  '(20 ms windows, energy-weighted)', pitch_grid=PG, formant_grid=FG,
             ln_kurtosis={k: round(ln_kurtosis(x), 4) for k, x in mats.items()}, level=lev)
    json.dump(d, open(OUT, 'w'), indent=1)
    print('%d materials x %d settings' % (len(mats), len(PG) * len(FG) - 1))


def fit(d):
    K, lev = d['ln_kurtosis'], d['level']
    I = np.zeros((len(PG), len(FG))); S = np.zeros_like(I); R = np.zeros_like(I); SD = np.zeros_like(I); NO = np.zeros_like(I)
    for i, P in enumerate(PG):
        for j, F in enumerate(FG):
            if P == 0 and F == 0: continue
            s = '%+d/%+d' % (P, F)
            ks = [k for k in lev if s in lev[k]]
            y = np.array([lev[k][s] for k in ks]); a = np.array([K[k] for k in ks]) - KURT_REF
            keep = np.ones(len(y), bool)
            for _ in range(3):                                   # (robust: points over 1 dB and 3 MADs off left out)
                c, *_ = np.linalg.lstsq(np.c_[np.ones(keep.sum()), a[keep]], y[keep], rcond=None)
                res = y - (c[0] + c[1] * a)
                mad = np.median(np.abs(res[keep])) * 1.4826 + 0.05
                keep = np.abs(res) < max(1.0, 3 * mad)
            I[i, j], S[i, j] = c
            R[i, j], SD[i, j], NO[i, j] = np.std(res[keep]), np.std(y), (~keep).sum()
    return I, S, R, SD, NO


def cmd_fit():
    d = load()
    I, S, R, SD, NO = fit(d)
    for name, T in (('level at kurtRef (dB)', I), ('slope (dB per unit ln kurtosis)', S), ('spread per setting (dB)', SD),
                    ('left after the fit (dB)', R), ('outliers', NO)):
        print(name + '; rows Pitch %s, columns Formant %s' % (PG, FG))
        for i, P in enumerate(PG): print('  %+3d ' % P + ' '.join('%6.2f' % v for v in T[i]))
    arr = lambda T: '{\n' + ',\n'.join('    { ' + ', '.join('%.2f' % v for v in row) + ' }' for row in T) + '\n}'
    print('\nconst double gridI[gridNP][gridNF] = %s;\nconst double gridS[gridNP][gridNF] = %s;\nconstexpr double kurtRef = %.2f;' % (arr(I), arr(S), KURT_REF))
    d['fit'] = dict(kurt_ref=KURT_REF, level_at_kurt_ref=np.round(I, 2).tolist(), slope=np.round(S, 2).tolist(),
                    spread_db=np.round(SD, 2).tolist(), left_db=np.round(R, 2).tolist(), outliers=NO.astype(int).tolist(),
                    spread_median=round(float(np.median(SD[SD > 0])), 2), left_median=round(float(np.median(R[R > 0])), 2))
    json.dump(d, open(OUT, 'w'), indent=1)
    print('\nspread per setting, median %.2f dB; left after the fit, median %.2f dB' % (d['fit']['spread_median'], d['fit']['left_median']))


def takes():
    sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'tools'))
    import null_pitchbend_presets as NP
    return [(n, x[0].astype(np.float64)) for n, x in NP.takes()]


def cmd_takes():
    skip = FS // 10
    tk = takes()
    rj, oj, meta = [], [], []
    for tn, x in tk:
        for P, F in TAKE_SETTINGS:
            rj.append(M.semi_job(x, P, F)); oj.append(O.job(x, P, F)); meta.append((tn, P, F, x))
    R = M.render_all(rj); Y = O.render(oj)
    edges = [100, 200, 400, 800, 1600, 3200, 5000, 8000]
    rows = []
    print('%-7s %4s %4s %7s %7s %6s  tone per band, ours minus the reference (dB): %s' % ('take', 'P', 'F', 'ref', 'level', 'tone', ' '.join('%d' % e for e in edges[:-1])))
    for (tn, P, F, x), a, b in zip(meta, R, Y):
        f, la = M.ltas_db(a[skip:]); _, lb = M.ltas_db(b[skip:])
        dl = (lb - lb.mean()) - (la - la.mean())
        bands = [float(np.mean(dl[(f >= lo) & (f < hi)])) for lo, hi in zip(edges[:-1], edges[1:])]
        q = dict(take=tn, P=P, F=F, ref_level_db=round(H.rms_db(a[skip:]) - H.rms_db(x[skip:]), 2),
                 level_db=round(H.rms_db(b[skip:]) - H.rms_db(a[skip:]), 2), tone_db=round(float(np.sqrt(np.mean(dl * dl))), 2),
                 bands_db=[round(v, 1) for v in bands])
        rows.append(q)
        print('%-7s %+4d %+4d %+7.2f %+7.2f %6.2f  %s' % (tn, P, F, q['ref_level_db'], q['level_db'], q['tone_db'], ' '.join('%+5.1f' % v for v in bands)))
    s = dict(mean_abs_level_db=round(float(np.mean([abs(q['level_db']) for q in rows])), 2), mean_tone_db=round(float(np.mean([q['tone_db'] for q in rows])), 2))
    print(json.dumps(s))
    d = load()
    prev = d.get('takes')
    d['takes'] = dict(what='ours against the reference at fixed settings (Mix 100, mono) on the presets\' takes, after the first '
                           '0.1 s: level (ours minus the reference, dB), tone (as null_pitchbend_presets.py)',
                      renderer=os.path.basename(O.EXE), rows=rows, summary=s)
    if prev and 'before' in prev: d['takes']['before'] = prev['before']
    elif prev: d['takes']['before'] = dict(renderer=prev.get('renderer'), summary=prev.get('summary'), rows=prev.get('rows'))
    json.dump(d, open(OUT, 'w'), indent=1)


if __name__ == '__main__':
    dict(ref=cmd_ref, fit=cmd_fit, takes=cmd_takes)[sys.argv[1] if len(sys.argv) > 1 else 'fit']()
