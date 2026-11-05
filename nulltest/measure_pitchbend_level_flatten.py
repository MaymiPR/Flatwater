"""Flatwater Pitch Bend, Phase 3: the shifter's voiced level where the level battery (measure_pitchbend_shifter.py level)
did not reach: under Flatten (the output forced onto one note, Pitch on top) and at Pitch past +-12 semitones (reached
through modulation). The reference against ours (tools/FwPitchBendShifterRender), output rms minus input rms from
0.5 s on, Mix 100, the reference's Mono component.

    python -I -B measure_pitchbend_level_flatten.py [grid | ours | modpath]

grid: renders the reference (cached in nulltest/renders/pbshift/) and ours (cached in nulltest/renders/pbshift_ours/),
writes measurements/fwpitchbend/shifter/level_flatten.json and prints the table. ours: the same with only our side
re-rendered against the saved reference numbers (the first such run keeps the earlier numbers as "before").
modpath: the reference only, how its level and envelope follow a Pitch reached through modulation against the same
Pitch set directly (the "modpath" section of the JSON). FW_SHIFTER_RENDER picks our renderer, as in the scorecard.

The reference's Pitch past +-12: its own Pitch control stops at +-12, so the Pitch target is driven by M1 as a step
sequencer whose steps all hold one value (the output pitch is base + value x depth x 0.24 semitones, clipped at +-24).
Measured (modpath): the reference's level follows the Pitch control (the base), not the modulated total, and with the
base at 0 its shifted output is not level-corrected at all (+2 .. +5 dB) while unvoiced input is not re-granulated.
Our shifter sees only the total, so Pitch past +-12 is judged against the base at +-12 with the rest modulated; the
base-0 cells are reported, not judged.
"""
import sys, os, json, hashlib, math
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import numpy as np
import measure_pitchbend_shifter as M
import score_pitchbend_shifter as O
import pb_host as H

FS = M.FS
OUT = os.path.join(M.OUT, 'level_flatten.json')
# the preset array's places for the M1 step sequence and the Pitch target's first slot (pb_state.py)
M_TYPE, M_STEPS, PITCH_SLOT0 = 420, 416, 1457
NOTES = {0: 'C2', 5: 'F2', 12: 'C3', 18: 'F#3', 23: 'B3'}


def note_hz(k):
    return 440.0 * 2 ** ((48 + k - 69) / 12)                          # note 0 = MIDI 48 = 130.81 Hz (measured)


def materials():
    mats = {'vowel u 110 Hz': M.vowel(3, 110, formants=M.VOWELS['u'])[0], 'vowel a 150 Hz': M.vowel(3, 150)[0],
            'vowel i 300 Hz': M.vowel(3, 300, formants=M.VOWELS['i'])[0], 'vowel a 400 Hz': M.vowel(3, 400)[0],
            'vowel i 500 Hz': M.vowel(3, 500, formants=M.VOWELS['i'])[0]}
    for name, grp, x in M.corpus_phrases(1.5, 4, 'sung'): mats['sung ' + name] = x
    for name, grp, x in M.corpus_phrases(1.5, 1, 'rap'): mats['rap ' + name] = x
    return mats


# (flatten note or None, pitch st, the reference's base Pitch when the rest is modulated)
SETTINGS = ([(k, 0, None) for k in (0, 5, 12, 18, 23)] + [(0, p, None) for p in (-12, -5, 5, 12)]
            + [(23, p, None) for p in (-12, -5, 5)] + [(12, p, None) for p in (-5, 5)]
            + [(None, p, 12 if p > 0 else -12) for p in (-24, -19, -16, -13, 13, 16, 19, 24)]
            + [(None, p, 0) for p in (-24, -19, -16, -13, 13, 16, 19, 24)])


def mod_job(x, base_st, mod_st, flatten=None):
    """the reference with Pitch at base_st (cents through the state) and mod_st more through M1 (a constant step
    sequence on the Pitch target's slot 0)"""
    s = {M.SEMITONES: 0, M.PITCH: 100.0 * base_st, M.FORMANT: 0.0, M.MIX: 100.0, M_TYPE: 1, M_STEPS: 4,
         PITCH_SLOT0: mod_st / 0.24, PITCH_SLOT0 + 1: 1}
    if flatten is not None: s.update({M.FLATTEN_ON: 1, M.FLATTEN_NOTE: int(flatten)})
    j = M.job(x, FS, state=s)
    j['shapes'] = {0: np.concatenate([np.zeros(256), np.ones(256)])}
    j['key'] = hashlib.sha1((j['key'] + 'seq1').encode()).hexdigest()[:20]
    return j


def ref_job(x, note, P, base=None):
    if note is not None:
        return M.st_job(x, 100.0 * P, 0.0, flatten=note)
    return mod_job(x, base or 0, P - (base or 0))


def setting_name(note, P, base):
    if note is not None: return 'flatten %s%+d' % (NOTES[note], P)
    return 'pitch %+d (base %+d)' % (P, base)


def render_ref(jobs):
    from concurrent.futures import ThreadPoolExecutor
    todo = [j for j in jobs if not os.path.exists(os.path.join(M.CACHE, j['key'] + '.npy'))]
    if todo:
        import pb_state as S
        for j in [j for j in todo if j['state']]:
            j['statefile'] = S.state_file(j['state'], 'pbs_' + j['key'], H.COMPONENTS[1].split('@')[0], check=False, shapes=j.get('shapes'))
        print('rendering %d of %d (reference)' % (len(todo), len(jobs)), flush=True)
        with ThreadPoolExecutor(4) as ex: list(ex.map(M._render_one, todo))
    return [np.load(os.path.join(M.CACHE, j['key'] + '.npy')).astype(np.float64) for j in jobs]


def level(x, y):
    s0 = FS // 2
    return H.rms_db(y[s0:]) - H.rms_db(x[s0:])


def f0_med(y):
    t, f, _ = M.yin(y, FS, 40, 1000, hop=480)
    f = f[t > 0.5]
    return float(np.nanmedian(f)) if np.isfinite(f).any() else None


def judged(q):
    return q['flatten_note'] is not None or q['base_st'] != 0


def summary(rows, key='diff_db'):
    def part(sel):
        d = np.array([q[key] for q in sel if q.get(key) is not None])
        if not len(d): return None
        return dict(cells=int(len(d)), within_1db=int(np.sum(np.abs(d) <= 1.0)), worst=round(float(d[np.argmax(np.abs(d))]), 2),
                    median=round(float(np.median(d)), 2), abs_median=round(float(np.median(np.abs(d))), 2))
    s = dict(judged=part([q for q in rows if judged(q)]), flatten=part([q for q in rows if q['flatten_note'] is not None]),
             pitch_past_12_base_12=part([q for q in rows if q['flatten_note'] is None and q['base_st'] != 0]),
             pitch_past_12_base_0_not_judged=part([q for q in rows if q['flatten_note'] is None and q['base_st'] == 0]))
    print(json.dumps(s))
    return s


def table(rows):
    for q in rows:
        print('%-26s %-24s in f0 %6s out f0 %7s  shift %6s  ref %+6.2f  ours %+6.2f  diff %+6.2f%s'
              % (q['material'][:26], q['setting'], q['in_f0'], q['ref_out_f0'], q['shift_st'], q['ref_db'], q['ours_db'], q['diff_db'],
                 '' if judged(q) else '  (not judged)'))


def main_grid(cmd):
    mats = materials()
    meta, rjobs, ojobs = [], [], []
    for k, x in mats.items():
        for note, P, base in SETTINGS:
            meta.append((k, note, P, base, x))
            ojobs.append(O.job(x, P, 0.0, flatten=note_hz(note) if note is not None else 0.0))
            if cmd == 'grid': rjobs.append(ref_job(x, note, P, base))
    prev = json.load(open(OUT)) if os.path.exists(OUT) else None
    ry = render_ref(rjobs) if cmd == 'grid' else None
    oy = O.render(ojobs)
    rows = []
    for i, ((k, note, P, base, x), yo) in enumerate(zip(meta, oy)):
        name = setting_name(note, P, base)
        if cmd == 'grid':
            yr = ry[i]
            fi, fr = f0_med(x), f0_med(yr)
            q = dict(material=k, setting=name, flatten_note=note, pitch_st=P, base_st=base, in_f0=None if fi is None else round(fi, 1),
                     ref_out_f0=None if fr is None else round(fr, 1), shift_st=None if not (fi and fr) else round(12 * math.log2(fr / fi), 2),
                     ref_db=round(level(x, yr), 2))
        else:
            q = dict(next(r for r in prev['rows'] if r['material'] == k and r['setting'] == name))
        q['ours_db'] = round(level(x, yo), 2)
        q['diff_db'] = round(q['ours_db'] - q['ref_db'], 2)
        rows.append(q)
    table(rows)
    s = summary(rows)
    res = dict(what='voiced level under Flatten and at Pitch past +-12: output rms minus input rms (dB) from 0.5 s on, Mix 100; '
                    'the reference against ours. shift_st: the reference output f0 over the input f0 (YIN medians). Pitch past '
                    '+-12 on the reference: the Pitch control at base_st and the rest through M1 (a constant step sequence on '
                    'Pitch); judged against the base at +-12 (the reference keys its level on the control, see modpath)',
               renderer=os.path.basename(O.EXE), rows=rows, summary=s)
    if prev:
        for k in ('before', 'modpath'):
            if k in prev: res[k] = prev[k]
        if 'before' not in res and cmd == 'ours':
            res['before'] = dict(summary=prev['summary'], ours_db={'%s | %s' % (r['material'], r['setting']): r['ours_db'] for r in prev['rows']})
    json.dump(res, open(OUT, 'w', newline='\n'), indent=1)


def main_modpath():
    """the reference: Pitch through modulation against the same Pitch set directly; level, f0 and (vowels) the
    envelope shift (harmonic amplitudes against the resonators, measure_pitchbend_shifter.env_shift_fit)"""
    mats = {'vowel a 150 Hz': (M.vowel(3, 150)[0], 'a'), 'vowel i 300 Hz': (M.vowel(3, 300, formants=M.VOWELS['i'])[0], 'i'),
            'noise': (M.noise(3), None)}
    cases = []
    for k, (x, vn) in mats.items():
        for P in (-12, -7, -5, 5, 7, 12):
            cases.append((k, 'Pitch %+d set directly' % P, M.st_job(x, 100.0 * P)))
            cases.append((k, 'Pitch 0, modulated %+d' % P, mod_job(x, 0, P)))
    x = mats['vowel a 150 Hz'][0]
    for b, m in ((-7, 1), (-5, 5), (5, 1), (12, 4), (-12, -4), (12, 12), (-12, -12)):
        cases.append(('vowel a 150 Hz', 'Pitch %+d, modulated %+d' % (b, m), mod_job(x, b, m)))
    cases.append(('vowel a 150 Hz', 'Flatten C2, Pitch 0, modulated -5', mod_job(x, 0, -5, flatten=0)))
    cases.append(('vowel a 150 Hz', 'Flatten C2, Pitch -5 set directly', M.st_job(x, -500, flatten=0)))
    ys = render_ref([c[2] for c in cases])
    from scipy.signal import welch

    def centroid(y):
        f, p = welch(y[FS // 2:], FS, nperseg=4096); m = (f > 4000) & (f < 10000)
        return float(np.sum(f[m] * p[m]) / np.sum(p[m]))
    out = []
    for (k, what, j), y in zip(cases, ys):
        x, vn = mats[k]
        q = dict(material=k, setting=what, level_db=round(level(x, y), 2))
        if vn:
            f0 = f0_med(y); q['out_f0'] = None if f0 is None else round(f0, 2)
            fo_, fo, do = M.harmonic_env(y)
            fm = M.VOWELS[vn]
            a, e, e1 = M.env_shift_fit(fo, do, lambda f: M.resonator_db(f, fm))
            q['envelope_shift_st'] = round(12 * np.log2(a), 2)
        else:
            q['centroid_4_10k_shift_st'] = round(12 * np.log2(centroid(y) / centroid(x)), 2)
        out.append(q)
        print(json.dumps(q))
    res = json.load(open(OUT)) if os.path.exists(OUT) else {}
    res['modpath'] = dict(what='the reference: Pitch reached through M1 (a constant step sequence on Pitch) against the same '
                               'Pitch set directly (cents through the state); level dB (output minus input from 0.5 s), output '
                               'f0, the envelope shift (vowels) or the 4 .. 10 kHz centroid shift (noise), semitones',
                          rows=out)
    json.dump(res, open(OUT, 'w', newline='\n'), indent=1)


if __name__ == '__main__':
    c = sys.argv[1] if len(sys.argv) > 1 else 'grid'
    main_modpath() if c == 'modpath' else main_grid(c)
