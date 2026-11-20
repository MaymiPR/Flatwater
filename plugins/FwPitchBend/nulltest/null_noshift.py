"""Null test of Flatwater Pitch Bend with no shift: our processor (PbRender at its defaults: Pitch 0, Formant 0, Mix
100, no modulation) against the reference at the same settings, both stereo at 48 kHz, on the sung and the spoken
take of null_presets.py and on steady vowels. With no shift both delay the voice by the same time; this is the one
setting where the two are compared sample for sample.

    python -I null_noshift.py [--save]

Residual = 20 log10( rms(ours - reference) / rms(reference) ) after the first 0.1 s, as rendered (no alignment); the
lag is the shift of ours against the reference that the cross-correlation finds (0: the delays agree to the sample).
Numbers are reported as they come: no case is labelled a null unless the data shows it. --save writes
measurements/FwPitchBend/noshift_null.json and the part of NULLTESTS.md.
"""
import os, sys, json
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.append(HERE)
sys.path.append(os.path.join(HERE, '..', '..', '..', 'scripts', 'nulltest'))   # scripts/nulltest/
import numpy as np
import pb_ref as H
import nulltest_lib as nl
import measure_shifter as M

FS = 48000
SKIP = FS // 10
OUT_JSON = os.path.join(HERE, '..', '..', '..', 'measurements', 'FwPitchBend', 'noshift_null.json')
REF = {'Pitch:': 0.5, 'Formant:': 0.5, 'Mix:': 1.0}        # 0 semitones, 0 semitones, 100 %, by normalised value


def take(group):
    """the longest phrases of a kind strung to about 8 s at -18 dBFS rms, stereo (null_presets.takes)"""
    ph = M.corpus_phrases(1.0, group=group)
    if not ph: raise SystemExit('the vocal corpus is not decoded (scripts/nulltest/vocal_corpus_lib.py)')
    parts, n = [], 0
    for _, _, p in ph:
        parts += [p, np.zeros(FS // 4)]; n += len(p) + FS // 4
        if n > 8 * FS: break
    x = np.concatenate(parts)[:8 * FS]
    x *= 10 ** (-18 / 20) / np.sqrt(np.mean(x[np.abs(x) > 1e-4] ** 2))
    return np.tile(x.astype(np.float32), (2, 1))


def cases():
    out = [('sung take', take('sung')), ('spoken take', take('rap'))]
    out += [('vowel, %d Hz' % f0, H.vowel(4.0, f0, -18.0, FS)) for f0 in (100, 150, 300)]
    out.append(('vowel, 150 Hz, vibrato 100 cents', H.vowel(4.0, 150, -18.0, FS, vib=100.0)))
    out.append(('white noise, -18 dBFS rms', H.noise(4.0, -18.0, FS)))
    return out


def main():
    rows, shown = [], False
    for name, x in cases():
        b, log = H.render_ref(x, FS, REF, log=True)
        if not shown:
            print(log.strip(), flush=True); shown = True      # (one case's log: the values the reference took)
        a = H.render_ours(x, FS)
        n = min(a.shape[1], b.shape[1])
        res = H.residual_db(b[:, :n], a[:, :n], SKIP)
        lag = int(nl.find_lag(b[0, :n], a[0, :n], 4096))
        rows.append(dict(case=name, residual_db=round(float(res), 1), lag=lag))
        print('%-36s %7.1f dB   lag %d' % (name, res, lag), flush=True)
    v = np.array([r['residual_db'] for r in rows])
    print('median %.1f dB, best %.1f, worst %.1f (%d cases)' % (np.median(v), v.min(), v.max(), len(v)))
    if '--save' in sys.argv:
        json.dump(rows, open(OUT_JSON, 'w', encoding='utf-8', newline='\n'), indent=1)
        import report_lib as RL
        with RL.Part('FwPitchBend', 'noshift') as f:
            f.write('## No shift\n\n')
            f.write('Flatwater Pitch Bend at its defaults (Pitch 0, Formant 0, Mix 100, no modulation) against the reference at the '
                    'same settings, sample for sample (Stereo, %d Hz; the residual after the first 0.1 s, as rendered, dB re the '
                    'reference output; the lag is the shift of ours against the reference in samples). Produced by '
                    '`plugins/FwPitchBend/nulltest/null_noshift.py` (`measurements/FwPitchBend/noshift_null.json`).\n\n' % FS)
            f.write('| case | residual (dB) | lag (samples) |\n|---|---:|---:|\n')
            for r in rows: f.write('| %s | %.1f | %d |\n' % (r['case'], r['residual_db'], r['lag']))
            f.write('\nmedian %.1f dB, best %.1f, worst %.1f; %d cases\n' % (np.median(v), v.min(), v.max(), len(v)))


if __name__ == '__main__':
    main()
