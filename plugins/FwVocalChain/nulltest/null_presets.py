"""Flatwater Vocal Chain's 43 factory presets against the reference's own: the reference set from its preset file (the
faders, colours and Balance it stores, set as host parameters, the only way they take: its state reads the colours back
as 0; Sensitivity and Output stay at 0, as the reference keeps them when a preset loads), Flatwater Vocal Chain loading
its factory preset (ChRender preset=k). Three held-out vocal parts at -6 dBFS peaks after 6 s of silence (the
delays settled, the doubler started in step), 48 kHz; each preset also with the Pitch stage clear in both. Writes
measurements/FwVocalChain/presets.json and the Presets section of plugins/FwVocalChain/NULLTESTS.md.

    python -I null_presets.py [jobs]
"""
import ast, json, os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.append(HERE)
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'scripts', 'nulltest'))   # scripts/nulltest/
import numpy as np
import ch_ref as C
import vocal_corpus_lib as V
import measure_campaign as CV
import nulltest as N

FS = 48000
PARTS = (24, 27, 33)
OUT_JSON = os.path.join(HERE, '..', '..', '..', 'measurements', 'FwVocalChain', 'presets.json')


def names():
    src = open(os.path.join(HERE, '..', '..', '..', 'scripts', 'make_presets.py'), encoding='utf-8').read()
    a = src.index('VOCALCHAIN_NAMES = [') + len('VOCALCHAIN_NAMES = ')
    return ast.literal_eval(src[a:src.index(')]', a) + 2])


def job(j):
    k, kw, part = j
    pad = 6 * FS
    v = V.part(part)[0][:FS * 10]; v = v / np.abs(v).max() * 10 ** (-6 / 20)
    x = np.concatenate([np.zeros(pad), v]).astype(np.float32)
    out = dict(preset=k + 1, part=part, pitch=N.PITCH[kw.get('pitch_color', 1)])
    for key, rk, ok in (('null', kw, {'preset': k + 1}), ('without_doubler', dict(kw, pitch_color=0), {'preset': k + 1, 'pitch_color': 0})):
        y = C.render(x, FS, {C.NOISE: 0}, C.host(**rk), preroll=0.0)[:, pad:]
        z = ours(x, ok)[:, pad:]
        out[key] = round(float(10 * np.log10(np.sum((y - z) ** 2) / np.sum(y ** 2))), 2)
    return out


def ours(x, ok):
    import subprocess, tempfile
    xs = np.stack([x, x])
    d = tempfile.mkdtemp(prefix='wvcp_')
    fin, fout = os.path.join(d, 'in.f32'), os.path.join(d, 'out.f32')
    xs.T.astype(np.float32).tofile(fin)
    args = [N.RENDER, fin, fout, '2', str(FS), 'noise=0', 'preset=%d' % ok['preset']]
    if 'pitch_color' in ok: args.append('pitchColor=%d' % ok['pitch_color'])
    subprocess.run(args, check=True, capture_output=True, timeout=600)
    return np.fromfile(fout, np.float32).reshape(-1, 2).T


def main(jobs=6):
    from concurrent.futures import ProcessPoolExecutor
    nm = names()
    presets = CV.factory_presets()
    assert len(presets) == len(nm) == 43
    work = [(k, kw, part) for k, (_, kw) in enumerate(presets) for part in PARTS]
    res = []
    with ProcessPoolExecutor(jobs) as ex:
        for r in ex.map(job, work):
            res.append(r)
            print('  %2d part %d  %6.1f  %6.1f' % (r['preset'], r['part'], r['null'], r['without_doubler']), flush=True)
    os.makedirs(os.path.dirname(OUT_JSON), exist_ok=True)
    json.dump(res, open(OUT_JSON, 'w', encoding='utf-8', newline='\n'), indent=1)
    write_md(res, nm)


def write_md(res, nm):
    import datetime
    L = ['# Flatwater Vocal Chain: the factory presets against the reference\'s own', '',
         'Each of the reference\'s 43 presets (the faders, colours and Balance from its preset file, set as host parameters;',
         'Sensitivity and Output at 0, as the reference keeps them when a preset loads) against Flatwater Vocal Chain loading',
         'its factory preset of the same place (our names). Residual in dB, lower is better: three held-out vocal parts at',
         '-6 dBFS peaks after 6 s of silence, 48 kHz, the Noise off in both. "Pitch clear" is the same preset with the Pitch',
         'stage cleared in both: the doubler\'s Wide and Spreader colours pick their splice points from the signal and are',
         'matched in behaviour, not sample for sample (NULLTESTS.md). Written by `plugins/FwVocalChain/nulltest/null_presets.py`',
         '(%s).' % datetime.date.today().isoformat(), '',
         '| # | preset | doubler | %s | Pitch clear, median (dB) |' % ' | '.join('part %d (dB)' % p for p in PARTS),
         '|---:|---|---|' + '---:|' * len(PARTS) + '---:|']
    for k, (group, name) in enumerate(nm):
        rs = sorted((r for r in res if r['preset'] == k + 1), key=lambda r: r['part'])
        L.append('| %d | %s: %s | %s | %s | %.1f |' % (k + 1, group, name, rs[0]['pitch'], ' | '.join('%.1f' % r['null'] for r in rs),
                                                    np.median([r['without_doubler'] for r in rs])))
    a = [r['null'] for r in res]; b = [r['without_doubler'] for r in res]
    L += ['', 'Over the %d renders: median **%.1f dB**, worst %.1f; with the Pitch stage clear median **%.1f dB**, worst %.1f.'
          % (len(a), np.median(a), max(a), np.median(b), max(b))]
    for col in ('clear', 'Stereo', 'Wide', 'Spreader'):
        v = [r['null'] for r in res if r['pitch'] == col]
        if v: L.append('- Doubler %s: %d renders, median %.1f dB, worst %.1f.' % (col, len(v), np.median(v), max(v)))
    import report_lib as RL
    print('wrote', RL.save('FwVocalChain', 'presets', L, 'Presets'))


if __name__ == '__main__':
    main(*(int(a) for a in sys.argv[1:2]))
