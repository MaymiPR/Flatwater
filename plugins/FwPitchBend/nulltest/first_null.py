"""The first null test of Flatwater Pitch Bend: proves the hosts and the build before any DSP is written (the house
workflow's Phase 0 gate). PASS / FAIL per check:

  1. the reference renders (Stereo and Mono components): not silent, no latency reported
  2. the reference's host Bypass is the input: below -80 dB
  3. the reference renders the same twice (fresh instances, the shifter on: Pitch -5): below -120 dB (determinism)
  4. our renderer at Mix 0 is the input: below -100 dB
  5. our VST3 at Mix 0 is the input: below -100 dB (the build and the renderer agree on the dry path)
  6. our VST3 equals our renderer at the defaults and at Mix 50: below -100 dB

    python -I first_null.py
"""
import sys, os, re
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import numpy as np
import vb as H

FS = 48000


def check(name, db, limit):
    ok = db < limit
    print('%s  %s: %7.1f dB (limit %g)' % ('PASS' if ok else 'FAIL', name, db, limit), flush=True)
    return ok


def main():
    x = H.noise(3.0, -18.0)
    v = H.vowel(3.0, 196.0, -18.0, vib=30.0)
    skip = FS // 10
    ok = True

    for ch in (2, 1):
        y, log = H.render_ref(v[:ch], FS, log=True)
        m = re.search(r'latency (\d+)', log)
        lat = int(m.group(1)) if m else -1
        print('reference %s: rms %.1f dB, latency %d samples' % (H.COMPONENTS[ch], H.rms_db(y), lat))
        ok &= check('reference %d ch not silent (rms above -60 dB)' % ch, -H.rms_db(y), 60.0)
        ok &= check('reference %d ch reports no latency (samples)' % ch, abs(lat), 0.5)

    y = H.render_ref(x, FS, {'Bypass:': 1.0})
    ok &= check('reference: Bypass = input', H.residual_db(x, y, skip), -80.0)

    # (the reference reads a typed "-5" as 0 semitones: its Pitch is set by normalised value, (-5 + 12) / 24)
    y1 = H.render_ref(v, FS, {'Pitch:': 7 / 24})
    y2 = H.render_ref(v, FS, {'Pitch:': 7 / 24})
    print('reference at Pitch -5 against the input: %.1f dB (the shifter is on)' % H.residual_db(v, y1, skip))
    ok &= check('reference renders the same twice (Pitch -5)', H.residual_db(y1, y2, 0), -120.0)

    ok &= check('our renderer: Mix 0 = input', H.residual_db(x, H.render_ours(x, FS, {'mix': 0}), 0), -100.0)
    ok &= check('our VST3: Mix 0 = input', H.residual_db(x, H.render_vst3(x, FS, {'Mix': '0'}), 0), -100.0)
    ok &= check('our VST3 = our renderer at the defaults', H.residual_db(H.render_ours(v, FS), H.render_vst3(v, FS), 0), -100.0)
    ok &= check('our VST3 = our renderer at Mix 50', H.residual_db(H.render_ours(v, FS, {'mix': 50}), H.render_vst3(v, FS, {'Mix': '50'}), 0), -100.0)

    print('ALL PASS' if ok else 'SOMETHING FAILED')
    sys.exit(0 if ok else 1)


if __name__ == '__main__':
    main()
