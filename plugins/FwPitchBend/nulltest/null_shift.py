"""Sample nulls of Flatwater Pitch Bend with a shift, for the record: a steady 150 Hz vowel through our processor
(PbRender, its Mono component, Fine on) and through the reference with the same Pitch and Formant (set in cents through its
state, measure_shifter.st_job), 48 kHz. A shifter that is not a copy cannot null sample for sample; these numbers
show how near the grains sit, not a null.

    python -I null_shift.py [--save]

Residual = 20 log10( rms(ours - reference) / rms(reference) ) from 0.5 s to 0.25 s before the end: as rendered, and
lined up (ours with Fine on, moved by up to 3 samples in fractions of one and scaled by the gain that fit the reference
best: measure_shifter.best_delay). --save writes measurements/FwPitchBend/shift_null.json and the part of
NULLTESTS.md.
"""
import os, sys, json
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.append(HERE)
sys.path.append(os.path.join(HERE, '..', '..', '..', 'scripts', 'nulltest'))   # scripts/nulltest/
import numpy as np
import pb_ref as H
import measure_shifter as M

FS = 48000
OUT_JSON = os.path.join(HERE, '..', '..', '..', 'measurements', 'FwPitchBend', 'shift_null.json')
CASES = (('Formant +0.01', 0.0, 0.01), ('Formant +0.5', 0.0, 0.5), ('Formant +1', 0.0, 1.0), ('Formant -1', 0.0, -1.0),
         ('Formant -3', 0.0, -3.0), ('Formant -6', 0.0, -6.0), ('Pitch +5', 5.0, 0.0))      # (name, Pitch, Formant in semitones)


def main():
    x = M.vowel(3.0, 150.0)[0]
    refs = M.render_all([M.st_job(x, pitch_c=100.0 * p, formant_c=100.0 * f) for _, p, f in CASES], workers=1)
    s0, s1 = FS // 2, len(x) - FS // 4
    rows = []
    for (name, p, f), b in zip(CASES, refs):
        a = H.render_ours(x[None].astype(np.float32), FS, {'fine': 1, 'pitch': p, 'formant': f})[0].astype(np.float64)
        n = min(len(a), len(b)); a, b = a[:n], b[:n]
        raw = H.rms_db(a[s0:s1] - b[s0:s1]) - H.rms_db(b[s0:s1])
        d, g, lined = M.best_delay(a, b, -3.0, 3.0, FS)
        rows.append(dict(setting=name, raw_db=round(float(raw), 1), lined_up_db=round(float(lined), 1), shift_samples=round(d, 3),
                         gain_db=round(float(20 * np.log10(abs(g) + 1e-30)), 2)))
        print('%-14s raw %6.1f dB   lined up %6.1f dB (ours moved by %+.3f samples, %+.2f dB)' % (name, raw, lined, d, rows[-1]['gain_db']), flush=True)
    if '--save' in sys.argv:
        json.dump(rows, open(OUT_JSON, 'w', encoding='utf-8', newline='\n'), indent=1)
        import report_lib as RL
        with RL.Part('FwPitchBend', 'shift') as f:
            f.write("Click probes and pulse trains (MEASUREMENTS.md, section 11, Sample nulls with a shift) show where the reference's "
                    'grains sit and what they read; ours read the same for Formant up to +1.5. On a steady 150 Hz vowel the residual '
                    '(dB re the reference output; as rendered, then with ours moved by up to 3 samples, in fractions of one, and matched in level). '
                    'None of these is a null. Produced by `plugins/FwPitchBend/nulltest/null_shift.py` '
                    '(`measurements/FwPitchBend/shift_null.json`).\n\n')
            f.write('| setting | raw (dB) | lined up (dB) |\n|---|---:|---:|\n')
            for r in rows: f.write('| %s | %.1f | %.1f |\n' % (r['setting'], r['raw_db'], r['lined_up_db']))


if __name__ == '__main__':
    main()
