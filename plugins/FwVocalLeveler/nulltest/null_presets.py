"""Flatwater Vocal Leveler's factory presets against the reference's own (build/ref_presets_FwVocalLeveler.json, written by
plugins/FwVocalLeveler/nulltest/read_ref_presets.py): the reference with its preset array loaded into its state (refstate.apply_values,
so it takes the values exactly, hidden ones included), Flatwater Vocal Leveler with its factory preset (VlRender preset=k),
both Mono, on three held-out vocal parts; the output residual after the first 0.1 s. Writes
the Presets section of plugins/FwVocalLeveler/NULLTESTS.md.

    python -I null_presets.py
"""
import sys, os, json, subprocess
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'scripts', 'nulltest'))   # scripts/nulltest/
import numpy as np
import refhost as R, ref_state_lib as W, vocal_corpus_lib as V
import nulltest as N

HERE = os.path.dirname(os.path.abspath(__file__))
import local_config_lib as LC
SRC = LC.local_file('build/ref_presets_FwVocalLeveler.json')
FS = 48000
COMP = LC.ref_component('FwVocalLeveler', 'Mono', required=False)
PARTS = (24, 27, 33)


def names():
    src = open(os.path.join(HERE, '..', '..', '..', 'scripts', 'make_presets.py'), encoding='utf-8').read()
    a = src.index('VOCALLEVELER_NAMES = [') + len('VOCALLEVELER_NAMES = ')
    return eval(src[a:src.index(']', a) + 1])


def ref(x, raw):
    R.fresh(COMP); p = R.load(COMP)
    W.apply_values(p, raw)
    return p(np.asarray(x, np.float32)[None], FS, buffer_size=512, reset=True)[0]


def ours(x, k):
    fi, fo = os.path.join(N.TMP, 'vpin%d.f32' % os.getpid()), os.path.join(N.TMP, 'vpout%d.f32' % os.getpid())
    np.asarray(x, np.float32).tofile(fi)
    subprocess.run([N.EXE, fi, fo, '1', str(FS), 'mode=1', 'preset=%d' % k], check=True, capture_output=True)
    return np.fromfile(fo, np.float32)


def residual(y, r):
    a = int(0.1 * FS)
    return 20 * np.log10(np.sqrt(np.mean((y[a:] - r[a:]) ** 2)) / np.sqrt(np.mean(r[a:] ** 2)) + 1e-30)


def main():
    data = json.load(open(SRC)); nm = names()
    assert len(data) == len(nm)
    mats = [(k, V.part(k)[0]) for k in PARTS]
    rows = []
    for k, (name, d) in enumerate(zip(nm, data)):
        res = [residual(ours(x, k), ref(x, d['raw'])) for _, x in mats]
        rows.append((name, res))
        print('%-14s %s' % (name, '  '.join('part %d %6.1f dB' % (p, v) for (p, _), v in zip(mats, res))), flush=True)
    allv = [v for _, r in rows for v in r]
    import report_lib as RL
    with RL.Part('FwVocalLeveler', 'presets', 'Presets') as f:
        f.write('# Flatwater Vocal Leveler: the factory presets against the reference\'s own\n\n')
        f.write('Each of the reference\'s 13 presets loaded into its state (its stored values, hidden ones included), against\n'
                'Flatwater Vocal Leveler\'s preset of the same settings (Flatwater names), both Mono, on held-out vocal parts of the corpus\n'
                '(`scripts/nulltest/vocal_corpus_lib.py`); the output residual after the first 0.1 s, dB (lower is better). Made by\n'
                '`plugins/FwVocalLeveler/nulltest/null_presets.py`. The Music Sensitivity a preset stores has no effect on either\n'
                'side here (no side-chain).\n\n')
        f.write('| preset | %s |\n|---|%s\n' % (' | '.join('part %d' % p for p in PARTS), '---|' * len(PARTS)))
        for name, r in rows: f.write('| %s | %s |\n' % (name, ' | '.join('%.1f' % v for v in r)))
        f.write('\nMedian %.1f dB, worst %.1f dB over %d renders.\n' % (np.median(allv), max(allv), len(allv)))
    print('median %.1f worst %.1f' % (np.median(allv), max(allv)))


if __name__ == '__main__':
    main()
