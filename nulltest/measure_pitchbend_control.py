"""Flatwater Pitch Bend, Phase 3: what the reference's shifter keys on the controls' own values (the Pitch and Formant
knobs) and what on the modulated totals. The reference with its controls at c and the rest moved by M1 as a constant
step sequence (measure_pitchbend_level_flatten.mod_job's recipe, here on Pitch and on Formant), against ours
(plugins/FwPitchBend/tools/PbShifterRender pitch=<total> formant=<total> pitchctl=<c> formantctl=<c>).

    python -I -B measure_pitchbend_control.py [ref | all]

ref: the reference only (prints its numbers); all: both sides, writes measurements/fwpitchbend/shifter/control.json.
FW_CONTROL_IGNORE=1 gives ours the totals only (the interface before the controls were passed); a later run keeps that
run's numbers as "before".
Per case and material: level (output rms minus input rms from 0.5 s, dB), the output f0, the envelope shift (vowels:
harmonic amplitudes against the resonators, semitones) and, for noise, the correlation with the delayed input (about
1: passed through, about 0: re-granulated). Judged: level within 1 dB, envelope within 0.5 st, on the cells where the
reference's own spread allows (see judge()).
"""
import sys, os, json, hashlib, subprocess, tempfile
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import numpy as np
from concurrent.futures import ThreadPoolExecutor
import measure_pitchbend_shifter as M
import measure_pitchbend_level_flatten as L
import score_pitchbend_shifter as O
import pb_host as H

FS = M.FS
OUT = os.path.join(M.OUT, 'control.json')
FORMANT_SLOT0 = 1465                      # the Formant target's first slot (pb_state.TARGETS)
IGNORE_CONTROLS = bool(os.environ.get('FW_CONTROL_IGNORE'))    # (ours given the totals only: the earlier interface)
PER_DEPTH = 0.24                          # semitones per unit of depth for a step value of 1 (measured on both targets)


# (name, Pitch control, Formant control, Pitch modulated by, Formant modulated by, Flatten note or None)
CASES = ([('P ctl 0, mod %+d' % p, 0, 0, p, 0, None) for p in (-24, -16, -12, -7, -5, -3, -1, 1, 3, 5, 7, 12, 16, 24)]
         + [('F ctl 0, mod %+d' % f, 0, 0, 0, f, None) for f in (-12, -7, -3, 3, 7, 12)]
         + [('P ctl %+d, mod %+d' % (c, p), c, 0, p, 0, None) for c, p in ((-5, 5), (5, -5), (-7, 1), (5, 2), (-12, -4), (12, 4), (1, -1))]
         + [('F ctl %+d, mod %+d' % (c, f), 0, c, 0, f, None) for c, f in ((7, -7), (-7, 7), (7, 3), (-7, -3), (3, -6))]
         + [('P ctl 0 mod +5, F ctl +7', 0, 7, 5, 0, None), ('P ctl +5, F ctl 0 mod +7', 5, 0, 0, 7, None),
            ('P ctl 0 mod -7, F ctl 0 mod +7', 0, 0, -7, 7, None)]
         + [('Flatten C2, P ctl 0, mod %+d' % p, 0, 0, p, 0, 0) for p in (-12, -5, 5, 12)]
         + [('Flatten B3, P ctl 0, mod %+d' % p, 0, 0, p, 0, 23) for p in (-12, -5)]
         + [('Flatten C2, P ctl +5, mod -5', 5, 0, -5, 0, 0), ('Flatten C2, P ctl 0, F ctl 0 mod +7', 0, 0, 0, 7, 0),
            ('Flatten C2, no modulation', 0, 0, 0, 0, 0)])


def materials():
    mats = {'vowel u 110 Hz': (M.vowel(3, 110, formants=M.VOWELS['u'])[0], 'u'), 'vowel a 150 Hz': (M.vowel(3, 150)[0], 'a'),
            'vowel i 300 Hz': (M.vowel(3, 300, formants=M.VOWELS['i'])[0], 'i'), 'vowel a 400 Hz': (M.vowel(3, 400)[0], 'a')}
    for name, grp, x in M.corpus_phrases(1.5, 2, 'sung'): mats['sung ' + name] = (x, None)
    for name, grp, x in M.corpus_phrases(1.5, 1, 'rap'): mats['rap ' + name] = (x, None)
    mats['noise'] = (M.noise(3), None)
    return mats


def ref_job(x, pc, fc, pm, fm, note):
    s = {M.SEMITONES: 0, M.PITCH: 100.0 * pc, M.FORMANT: 100.0 * fc, M.MIX: 100.0, L.M_TYPE: 1, L.M_STEPS: 4}
    if pm: s.update({L.PITCH_SLOT0: pm / PER_DEPTH, L.PITCH_SLOT0 + 1: 1})
    if fm: s.update({FORMANT_SLOT0: fm / PER_DEPTH, FORMANT_SLOT0 + 1: 1})
    if note is not None: s.update({M.FLATTEN_ON: 1, M.FLATTEN_NOTE: int(note)})
    if not (pm or fm): return M.job(x, FS, state=s)
    j = M.job(x, FS, state=s)
    j['shapes'] = {0: np.concatenate([np.zeros(256), np.ones(256)])}
    j['key'] = hashlib.sha1((j['key'] + 'seq1').encode()).hexdigest()[:20]
    return j


def ours_job(x, pc, fc, pm, fm, note):
    P, F = float(np.clip(pc + pm, -24, 24)), float(np.clip(fc + fm, -12, 12))
    fl = L.note_hz(note) if note is not None else 0.0
    x = np.asarray(x, np.float32)[None, :]
    args = ['pitch=%r' % P, 'formant=%r' % F, 'flatten=%r' % fl]
    if not IGNORE_CONTROLS: args += ['pitchctl=%r' % float(pc), 'formantctl=%r' % float(fc)]
    h = hashlib.sha1(x.tobytes()); h.update(repr((O.exe_hash(), FS, tuple(args), 512)).encode())
    return dict(x=x, args=args, key='c' + h.hexdigest()[:19])


def _render_ours(j):
    f = os.path.join(O.CACHE, j['key'] + '.npy')
    if os.path.exists(f): return
    d = tempfile.mkdtemp(prefix='pbc_', dir=H.TMP)
    fi, fo = os.path.join(d, 'in.f32'), os.path.join(d, 'out.f32')
    j['x'].T.astype(np.float32).tofile(fi)
    cmd = [O.EXE, fi, fo, '1', str(FS)] + j['args']
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode or not os.path.exists(fo): raise RuntimeError(' '.join(cmd) + '\n' + r.stderr)
    np.save(f, np.fromfile(fo, np.float32)[None, :])
    for q in (fi, fo): os.remove(q)
    os.rmdir(d)


def render_ours(jobs):
    with ThreadPoolExecutor(8) as ex: list(ex.map(_render_ours, jobs))
    return [np.load(os.path.join(O.CACHE, j['key'] + '.npy')).astype(np.float64)[0] for j in jobs]


def measures(x, vn, y, is_noise):
    q = dict(level_db=round(L.level(x, y), 2))
    if is_noise:
        q['corr_delayed'] = round(float(np.corrcoef(y[FS // 2:-200], M.frac_delay(x, 105.53125)[FS // 2:-200])[0, 1]), 3)
        return q
    f0 = L.f0_med(y); q['out_f0'] = None if f0 is None else round(f0, 1)
    if vn and f0 is not None and f0 > 45.0:                  # (lower: too few samples per harmonic for the fit)
        fo_, fo, do = M.harmonic_env(y)
        if np.isfinite(fo_):
            a, e, e1 = M.env_shift_fit(fo, do, (lambda fm: (lambda f: M.resonator_db(f, fm)))(M.VOWELS[vn]))
            q['env_st'] = round(float(12 * np.log2(a)), 2)
    return q


def judge(rows):
    """level: within 1 dB, always on noise and under Flatten (there the spread over materials follows each one's f0
    against the note, which the level model takes in); otherwise on voice where the reference's spread over the
    voiced materials of that case is under 3 dB (wider: material-specific, reported). Envelope: within 0.5 st on the
    /a/ vowel at 150 Hz (at 400 Hz the harmonics are too sparse for the fit)."""
    out = []
    for name in dict.fromkeys(r['case'] for r in rows):
        rs = [r for r in rows if r['case'] == name]
        voiced = [r['ref']['level_db'] for r in rs if r['material'] != 'noise']
        spread = max(voiced) - min(voiced)
        for r in rs:
            dl = r['ours']['level_db'] - r['ref']['level_db']
            r['level_diff_db'] = round(dl, 2)
            r['level_judged'] = r['material'] == 'noise' or name.startswith('Flatten') or spread < 3.0
            r['level_ok'] = abs(dl) <= 1.0
            if 'env_st' in r['ref'] and 'env_st' in r['ours'] and r['material'] == 'vowel a 150 Hz':
                de = r['ours']['env_st'] - r['ref']['env_st']
                r['env_diff_st'] = round(de, 2); r['env_ok'] = abs(de) <= 0.5
            if r['material'] == 'noise':
                r['noise_mode_ok'] = (r['ref']['corr_delayed'] > 0.5) == (r['ours']['corr_delayed'] > 0.5)
        out.append(dict(case=name, ref_voiced_spread_db=round(spread, 2)))
    return out


def summary(rows):
    lj = [r for r in rows if r.get('level_judged')]
    ej = [r for r in rows if 'env_ok' in r]
    nj = [r for r in rows if 'noise_mode_ok' in r]
    s = dict(level_cells=len(lj), level_within_1db=sum(r['level_ok'] for r in lj),
             level_worst=max((r['level_diff_db'] for r in lj), key=abs) if lj else None,
             level_unjudged_cells=sum(1 for r in rows if 'level_judged' in r and not r['level_judged']),
             env_cells=len(ej), env_within_05st=sum(r['env_ok'] for r in ej), env_worst=max((r['env_diff_st'] for r in ej), key=abs) if ej else None,
             noise_cases=len(nj), noise_mode_same=sum(r['noise_mode_ok'] for r in nj))
    s = json.loads(json.dumps(s, default=lambda v: v.item() if hasattr(v, 'item') else str(v)))
    print(json.dumps(s))
    return s


def main(cmd):
    mats = materials()
    meta, rj, oj = [], [], []
    for case in CASES:
        name, pc, fc, pm, fm, note = case
        for k, (x, vn) in mats.items():
            meta.append((name, k, x, vn)); rj.append(ref_job(x, pc, fc, pm, fm, note))
            if cmd == 'all': oj.append(ours_job(x, pc, fc, pm, fm, note))
    ry = L.render_ref(rj)
    oy = render_ours(oj) if cmd == 'all' else [None] * len(ry)
    rows = []
    for (name, k, x, vn), yr, yo in zip(meta, ry, oy):
        r = dict(case=name, material=k, ref=measures(x, vn, yr, k == 'noise'))
        if yo is not None: r['ours'] = measures(x, vn, yo, k == 'noise')
        rows.append(r)
        print('%-36s %-22s ref %s%s' % (name, k[:22], json.dumps(r['ref']), '' if yo is None else '  ours ' + json.dumps(r['ours'])))
    if cmd != 'all': return
    spreads = judge(rows)
    s = summary(rows)
    prev = json.load(open(OUT)) if os.path.exists(OUT) else None
    res = dict(what='the reference with its controls at c and the rest moved by M1 (a constant step sequence on Pitch and / or '
                    'Formant, %g st per unit of depth) against ours given the totals and the controls; level dB from 0.5 s, '
                    'output f0, envelope shift st (vowels), noise correlation with the delayed input' % PER_DEPTH,
               renderer=os.path.basename(O.EXE), rows=rows, spreads=spreads, summary=s)
    if IGNORE_CONTROLS: res['ours_given'] = 'the totals only'
    elif prev:
        res['before'] = prev.get('before') or dict(ours_given=prev.get('ours_given'), renderer=prev.get('renderer'), summary=prev['summary'],
                                                   level_db={'%s | %s' % (r['case'], r['material']): r['ours']['level_db'] for r in prev['rows']},
                                                   env_st={'%s | %s' % (r['case'], r['material']): r['ours'].get('env_st') for r in prev['rows']})
    json.dump(res, open(OUT, 'w', newline='\n'), indent=1, default=lambda v: v.item() if hasattr(v, 'item') else str(v))


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else 'all')
