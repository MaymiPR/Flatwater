"""The first null test of Flatwater Vocal Chain: proves the harness, the hosts and the build before any measurement. The
reference is the shell named by local.yaml's ref_vst3 and its component ch_ref.REF_NAME (the harness prints both).

    python -I first_null.py            # the checks below
    python -I first_null.py --self     # without the reference: checks 3 and 4 only (the harness and the build)

Checks (PASS / FAIL), on 3 s of noise at -18 dBFS:
  1. The reference renders through tools/RefRender: not silent
  2. The reference's host Bypass is the input (both lined up by the reported latency): below -80 dB; if not, the
     latency or the state is wrong
  3. our built VST3 renders through tools/RefRender: not silent
  4. our VST3 equals our renderer (the harness's CHECK settings aside; ChRender) at the defaults: below -100 dB, the check every number passes
"""
import argparse, os, sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import numpy as np
import ch_ref as h
import nulltest_lib as nl


def check(name, db, limit):
    ok = db < limit
    print('%s  %s: %7.1f dB (limit %g)' % ('PASS' if ok else 'FAIL', name, db, limit), flush=True)
    return ok


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--self', action='store_true', help='skip the reference (not installed here)')
    a = ap.parse_args()
    fs = 48000
    x = nl.stereo(nl.noise(3.0, -18.0, fs=fs, seed=7))
    skip = fs // 2
    ok = True
    if not a.self:
        print('reference: %s in %s' % (h.REF_NAME, h.R.WAVESHELL))
        y = h.render_ref_host(x, fs)
        ok &= check('reference not silent (rms above -60 dB)', -nl.rms_db(y), 60.0)
        y = h.render_ref_host(x, fs, {'Bypass:': 1})
        ok &= check('reference: host Bypass = input', nl.residual_db(x, y, skip), -80.0)
    print('ours: %s, %s' % (os.path.relpath(h.VST3), os.path.relpath(h.RENDER)))
    params, keys = getattr(h, 'CHECK', ({}, {}))      # settings both renders need to be deterministic
    y_vst3 = h.render_vst3(x, fs, params)
    ok &= check('our VST3 not silent (rms above -60 dB)', -nl.rms_db(y_vst3), 60.0)
    y_model = h.render_model(x, fs, **keys)
    n = min(y_vst3.shape[1], y_model.shape[1])
    ok &= check('our VST3 = our renderer at the defaults', nl.residual_db(y_model[:, :n], y_vst3[:, :n]), -100.0)
    print('ALL PASS' if ok else 'SOMETHING FAILED')
    sys.exit(0 if ok else 1)


if __name__ == '__main__':
    main()
