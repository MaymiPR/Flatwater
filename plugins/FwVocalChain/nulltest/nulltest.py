"""Phase 4 nulls: the reference CLA Vocals (cv_ref) against our Flatwater Vocal Chain VST3 (pedalboard), the same settings and
input, aligned (both report their latency); the null is the difference's energy under the reference output's (dB).

    python -I nulltest.py [linear | all]
    python -I nulltest.py full [part]       (the whole chain on held-out vocal part 30, or another)
    python -I nulltest.py report [jobs]     (every case at 44.1, 48 and 96 kHz: writes
                                                         measurements/fwvocalchain/nulls.json and NULLTESTS.md)
    python -I nulltest.py md                (NULLTESTS.md again from nulls.json and doubler.json)

WVC_RENDER in the environment names another ChRender (an older build, to compare).
"""
import os
import sys
import numpy as np
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.append(HERE)
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'scripts', 'nulltest'))   # scripts/nulltest/
import cv as C
import vocal_corpus as V

RENDER = os.environ.get('WVC_RENDER') or os.path.abspath(os.path.join(HERE, '..', '..', '..', 'build', 'ChRender_artefacts', 'Release', 'ChRender.exe'))
FS = 48000
STAGES = ('bass', 'treble', 'compress', 'reverb', 'delay', 'pitch')
CLEAR = {s + '_color': 0 for s in STAGES}
KEYS = {'sensitivity': 'sensitivity', 'output': 'output', 'balance': 'balance'}
for s in STAGES: KEYS[s] = s; KEYS[s + '_color'] = s + 'Color'


def render_ours(x, kw, fs=FS):
    """our plug-in's processor (plugins/FwVocalChain/tools/ChRender); the reference's defaults where kw says nothing (colours 1)"""
    import subprocess, tempfile
    x = np.asarray(x, np.float32)
    if x.ndim == 1: x = np.stack([x, x])
    d = tempfile.mkdtemp(prefix='wvc_')
    fin, fout = os.path.join(d, 'in.f32'), os.path.join(d, 'out.f32')
    x.T.astype(np.float32).tofile(fin)
    args = [RENDER, fin, fout, '2', str(fs), 'noise=0'] + ['%s=%g' % (KEYS[k], v) for k, v in kw.items()]
    subprocess.run(args, check=True, capture_output=True, timeout=600)
    return np.fromfile(fout, np.float32).reshape(-1, 2).T


def null(kw, x, values=None, fs=FS):
    y = C.render(x, fs, values or {C.NOISE: 0}, C.host(**kw))         # (the Noise off in both: it cannot null)
    z = render_ours(x, kw, fs)
    return 10 * np.log10(np.sum((y - z) ** 2) / max(np.sum(y ** 2), 1e-30)), y, z


def linear_cases():
    cases = [('clear', dict(CLEAR))]
    for st in ('bass', 'treble'):
        for c in (1, 2, 3):
            for v in (-10, -7.5, -3, 0, 2.5, 6, 10):
                cases.append(('%s c%d %+g' % (st, c, v), dict(CLEAR, **{st + '_color': c, st: v})))
    cases.append(('bass c2 +4, treble c3 -6', dict(CLEAR, bass_color=2, bass=4, treble_color=3, treble=-6)))
    for c in (1, 2, 3):
        for v in (0, 5, -7.3):
            cases.append(('reverb c%d %+g' % (c, v), dict(CLEAR, reverb_color=c, reverb=v)))
    cases.append(('reverb c2, sens +5, out -3, bal +3', dict(CLEAR, reverb_color=2, sensitivity=5, output=-3, balance=3)))
    cases.append(('reverb c3, bass c1 +6, treble c2 +4', dict(CLEAR, reverb_color=3, bass_color=1, bass=6, treble_color=2, treble=4)))
    return cases


def full_cases():
    """the whole chain: the defaults, a few of the factory presets, random settings (measure_campaign.py's)"""
    import measure_campaign as CV
    cases = [('defaults', {})]
    presets = CV.factory_presets()
    for name, kw in presets[:43:6]: cases.append((name, kw))
    rng = np.random.default_rng(2026)
    for i in range(6):
        kw = {st: round(float(rng.uniform(-10, 10)), 1) for st in STAGES}
        kw.update({st + '_color': int(rng.integers(0, 4)) for st in STAGES})
        kw['sensitivity'] = round(float(rng.uniform(-6, 6)), 1); kw['output'] = round(float(rng.uniform(-6, 6)), 1)
        kw['balance'] = round(float(rng.uniform(-6, 6)), 1)
        cases.append(('random %d' % i, kw))
    return cases


def full(part=30):
    """the whole chain on a held-out vocal at a working level (peaks -6 dBFS): 6 s of silence first in both, so the
    reference's delay glide has settled and the doubler's LFOs started together; the null over the vocal, and with the
    doubler left out (its time-varying voices)"""
    pad = 6 * FS
    v = V.part(part)[0][:FS * 10]; v = v / np.abs(v).max() * 10 ** (-6 / 20)
    x = np.concatenate([np.zeros(pad), v]).astype(np.float32)
    res = []
    for name, kw in full_cases():
        y = C.render(x, FS, {C.NOISE: 0}, C.host(**kw), preroll=0.0)[:, pad:]
        z = render_ours(x, kw)[:, pad:]
        n = 10 * np.log10(np.sum((y - z) ** 2) / np.sum(y ** 2))
        kw2 = dict(kw, pitch_color=0)
        y2 = C.render(x, FS, {C.NOISE: 0}, C.host(**kw2), preroll=0.0)[:, pad:]; z2 = render_ours(x, kw2)[:, pad:]
        n2 = 10 * np.log10(np.sum((y2 - z2) ** 2) / np.sum(y2 ** 2))
        res.append((n, n2))
        print('  %-10s null %6.1f dB; without the doubler %6.1f dB   %s' % (name, n, n2, ' '.join('%s=%s' % kv for kv in sorted(kw.items()))), flush=True)
    print('median %.1f dB (without the doubler %.1f dB)' % (np.median([r[0] for r in res]), np.median([r[1] for r in res])))


RATES = (44100, 48000, 96000)
FULL_PARTS = (30, 33)
OUT_JSON = os.path.join(HERE, '..', '..', '..', 'measurements', 'fwvocalchain', 'nulls.json')
OUT_MD = os.path.join(HERE, '..', '..', '..', 'plugins', 'FwVocalChain', 'NULLTESTS.md')
PITCH = {0: 'clear', 1: 'Stereo', 2: 'Wide', 3: 'Spreader'}


def at_rate(x, fs):
    from scipy.signal import resample_poly
    from math import gcd
    if fs == FS: return x
    d = gcd(fs, FS)
    return resample_poly(x, fs // d, FS // d)


def _linear_job(job):
    fs, name, kw = job
    x = V.part(30)[0][:FS * 12]; x = x * 10 ** (-16 / 20) / np.abs(x).max()
    n = null(kw, at_rate(x, fs), fs=fs)[0]
    return dict(set='linear', fs=fs, case=name, kw=kw, null=round(float(n), 2))


def _full_job(job):
    fs, part, name, kw = job
    pad = 6 * fs
    v = V.part(part)[0][:FS * 10]; v = at_rate(v / np.abs(v).max() * 10 ** (-6 / 20), fs)
    x = np.concatenate([np.zeros(pad), v]).astype(np.float32)
    out = dict(set='full', fs=fs, part=part, case=name, kw=kw, pitch=PITCH[kw.get('pitch_color', 1)])
    for key, k in (('null', kw), ('without_doubler', dict(kw, pitch_color=0))):
        y = C.render(x, fs, {C.NOISE: 0}, C.host(**k), preroll=0.0)[:, pad:]
        z = render_ours(x, k, fs)[:, pad:]
        out[key] = round(float(10 * np.log10(np.sum((y - z) ** 2) / np.sum(y ** 2))), 2)
    return out


def report(jobs=6):
    """every linear case at each rate, the whole chain on two held-out vocals at 48 kHz and on one at the other rates;
    the results to measurements/fwvocalchain/nulls.json, and NULLTESTS.md written from them"""
    from concurrent.futures import ProcessPoolExecutor
    work = [(_linear_job, (fs, n, kw)) for fs in RATES for n, kw in linear_cases()]
    work += [(_full_job, (fs, part, n, kw)) for fs in RATES for part in (FULL_PARTS if fs == FS else FULL_PARTS[:1])
             for n, kw in full_cases()]
    res = []
    with ProcessPoolExecutor(jobs) as ex:
        for r in ex.map(_run, work):
            res.append(r)
            print('  %(set)-6s %(fs)6d %(case)-38s' % r, r.get('null'), r.get('without_doubler', ''), flush=True)
    os.makedirs(os.path.dirname(OUT_JSON), exist_ok=True)
    import json
    json.dump(res, open(OUT_JSON, 'w', encoding='utf-8', newline='\n'), indent=1)
    write_md(res)


def _run(w):
    return w[0](w[1])


def write_md(res):
    import datetime
    med = lambda v: float(np.median(v)) if v else float('nan')
    L = ['# Flatwater Vocal Chain: null tests', '',
         'Residuals of Flatwater Vocal Chain against the reference CLA Vocals (Stereo), lower is better: the energy of the',
         "difference under the reference output's, in dB. Written by `plugins/FwVocalChain/nulltest/nulltest.py report` from",
         '`measurements/fwvocalchain/nulls.json` (%s); our processor through `ChRender`, the reference through'
         % datetime.date.today().isoformat(),
         "`plugins/FwVocalChain/nulltest/cv.py`, the Noise off in both (two noise sources cannot null). The vocals are held-out parts of the",
         'corpus (`scripts/nulltest/vocal_corpus.py`); how each stage was identified is in [MEASUREMENTS.md](MEASUREMENTS.md).', '',
         '## Summary', '',
         '| set | rate | cases | median (dB) | worst (dB) |', '|---|---:|---:|---:|---:|']
    for fs in RATES:
        v = [r['null'] for r in res if r['set'] == 'linear' and r['fs'] == fs]
        L.append('| Bass, Treble and Reverb, the others clear | %g kHz | %d | %.1f | %.1f |' % (fs / 1000, len(v), med(v), max(v)))
    for fs in RATES:
        f = [r for r in res if r['set'] == 'full' and r['fs'] == fs]
        v = [r['without_doubler'] for r in f]
        L.append('| The whole chain, the doubler clear | %g kHz | %d | %.1f | %.1f |' % (fs / 1000, len(v), med(v), max(v)))
        for col in ('Stereo', 'Wide', 'Spreader'):
            v = [r['null'] for r in f if r['pitch'] == col]
            if v: L.append('| The whole chain, doubler %s | %g kHz | %d | %.1f | %.1f |' % (col, fs / 1000, len(v), med(v), max(v)))
    L += ['', '## The whole chain', '',
          'A held-out vocal at -6 dBFS peaks after 6 s of silence (the delays settled, the doubler started in step in both):',
          'the defaults, every sixth factory preset and six random settings of every control; each case also with the',
          'Pitch stage clear.', '',
          '| rate | part | case | doubler | null (dB) | doubler clear (dB) |', '|---|---:|---|---|---:|---:|']
    for r in res:
        if r['set'] == 'full':
            L.append('| %g kHz | %d | %s | %s | %.1f | %.1f |' % (r['fs'] / 1000, r['part'], r['case'], r['pitch'], r['null'], r['without_doubler']))
    dbl = os.path.join(os.path.dirname(OUT_JSON), 'doubler.json')
    if os.path.exists(dbl):
        import json
        L += ['', '## The doubler in behaviour', '',
              "The Pitch stage's Wide and Spreader colours are pitch shifters that pick their splice points from the signal,",
              'so a voice parts from the reference at a splice where the two pick different steps; they are compared in',
              'behaviour (`plugins/FwVocalChain/nulltest/compare_doubler.py`, 48 kHz, Pitch +10, every other colour clear, the wet',
              "signal alone): the level, the third-octave spectrum (100 Hz .. 12 kHz) ours less the reference's, and on noise",
              "the voice's delay (5th / 50th / 95th percentile) and the strongest cycle of its movement. Stereo for scale.", '',
              '| colour | signal | side | level diff (dB) | spectrum rms / max (dB) | delay p5 / p50 / p95, ours (ref) (ms) | cycle, ours (ref) (s) | wet null (dB) |',
              '|---|---|---|---:|---|---|---|---:|']
        for r in json.load(open(dbl, encoding='utf-8')):
            for c in (0, 1):
                dl = cy = ''
                if 'delay' in r['ref']:
                    o, f = r['ours']['delay'][c], r['ref']['delay'][c]
                    dl = '%.1f / %.1f / %.1f (%.1f / %.1f / %.1f)' % (o['p5'], o['p50'], o['p95'], f['p5'], f['p50'], f['p95'])
                    cy = '%.2f (%.2f)' % (o['period'], f['period'])
                L.append('| %s | %s | %s | %+.2f | %.2f / %.2f | %s | %s | %.1f |' % (r['colour'], r['signal'], 'LR'[c],
                         r['level_diff'][c], r['band_rms'][c], r['band_max'][c], dl, cy, r['null'][c]))
    L += ['', "The factory presets against the reference's own: [NULLTESTS-presets.md](NULLTESTS-presets.md)."]
    L += ['', '## Bass, Treble and Reverb', '',
          'A held-out vocal at -16 dBFS peak (under the dynamics), every other colour clear.', '',
          '| case | ' + ' | '.join('%g kHz (dB)' % (fs / 1000) for fs in RATES) + ' |', '|---|' + '---:|' * len(RATES)]
    by = {}
    for r in res:
        if r['set'] == 'linear': by.setdefault(r['case'], {})[r['fs']] = r['null']
    for case, d in by.items():
        L.append('| %s | ' % case + ' | '.join('%.1f' % d[fs] if fs in d else '' for fs in RATES) + ' |')
    open(OUT_MD, 'w', encoding='utf-8', newline='\n').write('\n'.join(L) + '\n')


def main():
    x = V.part(30)[0][:FS * 12]; x = x * 10 ** (-16 / 20) / np.abs(x).max()    # (a held-out vocal, its peak under the dynamics' thresholds)
    print('input: peak %.1f dBFS, rms %.1f dBFS' % (20 * np.log10(np.abs(x).max()), 20 * np.log10(np.sqrt(np.mean(x ** 2)))))
    res = []
    for name, kw in linear_cases():
        n, y, z = null(kw, x)
        res.append(n)
        print('  %-38s null %6.1f dB' % (name, n), flush=True)
    print('median %.1f dB, worst %.1f dB' % (np.median(res), max(res)))


if __name__ == '__main__':
    if 'md' in sys.argv[1:]:
        import json
        write_md(json.load(open(OUT_JSON, encoding='utf-8')))
    elif 'report' in sys.argv[1:]:
        report(*(int(a) for a in sys.argv[2:3]))
    elif 'full' in sys.argv[1:]:
        full(*(int(a) for a in sys.argv[2:3]))
    else:
        main()
