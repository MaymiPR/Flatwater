"""Null test of Flatwater Vocal Leveler with music on its side-chain bus: our renderer (VlRender sc=) against the
reference, both fed the same voice and the same music. The reference renders through tools/RefRender, which enables
the side-chain bus and feeds it; pedalboard cannot. Stereo components, 48 kHz.

    python -I null_sidechain.py [--save] [--quick] [--tune key=value ...]

The cases: the probes of measure_music_law.py (a steady 1 kHz voice with a tone, a step of the music, noise, bursts
and two tones as the music), and held-out vocal parts of the corpus (scripts/nulltest/vocal_corpus_lib.py) with a
dense synthetic mix as the music. Residual = 20 log10( rms(ours - reference) / rms(reference) ) after the first
0.1 s; the gain error is the largest difference of the two gains over 50 ms windows. Numbers are reported as they
come: no case is labelled a null unless the data shows it. --tune passes constants of Source/dsp/Tuning.h to the
renderer (for a fit); --save needs none.
"""
import os, sys, subprocess
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.append(HERE)
sys.path.append(os.path.join(HERE, '..', '..', '..', 'scripts', 'nulltest'))   # scripts/nulltest/
import numpy as np
import nulltest_lib as nl
import measure_music_law as M

FS = M.FS
EXE = os.environ.get('FWVOCALLEVELER_RENDER', os.path.join(HERE, '..', '..', '..', 'build', 'VlRender_artefacts', 'Release', 'VlRender.exe'))
TMP = os.path.join(HERE, 'renders'); os.makedirs(TMP, exist_ok=True)


def ours(x, music, s, tune=()):
    """our renderer: x the voice (mono, on both channels), music (mono or (2, n)); s: target, range_min, range_max,
    sens (dB, or 'Off'), fast"""
    fi, fo, fk = [os.path.join(TMP, 'sc%s%d.f32' % (k, os.getpid())) for k in ('in', 'out', 'key')]
    nl.stereo(x).T.astype(np.float32).tofile(fi)
    nl.stereo(music).T.astype(np.float32).tofile(fk)
    sens = -12.1 if s.get('sens', 0) == 'Off' else float(s.get('sens', 0))
    args = [EXE, fi, fo, '2', str(FS), 'mode=0', 'target=%g' % s.get('target', -21.0), 'rangeMin=%g' % s.get('range_min', -6.0),
            'rangeMax=%g' % s.get('range_max', 6.0), 'musicSensitivity=%g' % sens, 'attack=%d' % (1 if s.get('fast') else 0),
            'sc=' + fk, 'scch=2'] + ['tune:' + t for t in tune]
    r = subprocess.run(args, capture_output=True, text=True)
    if r.returncode: raise RuntimeError(r.stdout + r.stderr)
    return np.fromfile(fo, np.float32).reshape(-1, 2).T


def theirs(x, music, s, tag):
    p = {'Target': s.get('target', -21.0), 'Range Min': s.get('range_min', -6.0), 'Range Max': s.get('range_max', 6.0)}
    if s.get('sens', 0) == 'Off': p['Music Sensitivity:'] = 0
    else: p['Music Sensitivity'] = s['sens'] if isinstance(s.get('sens', 0), str) else '%g' % s.get('sens', 0)
    if 'fast' in s: p['Attack:'] = 1 if s['fast'] else 0   # (by its normalised value: the choice's text is not taken)
    return M.ref(x, music, p, tag)


def cases(quick):
    """(name, voice, music, settings, the cache tag of measure_music_law.py)"""
    wide = dict(range_min=-12.0, range_max=12.0)
    out = []
    for mdb, sens, target, rng in ((-18, '0', -21.0, wide), (-6, '0', -21.0, wide), (-18, '6', -21.0, wide), (-18, '0', -30.0, wide),
                                   (-18, '0', -21.0, dict(range_min=-6.0, range_max=6.0))):
        out.append(('steady voice, tone %d dBFS, Sensitivity %s, Target %g, range %g .. %g' % (mdb, sens, target, rng['range_min'], rng['range_max']),
                    M.sine(1000.0, 16.0, -21), M.sine(1000.0, 16.0, mdb), dict(rng, sens=sens, target=target), 'settled'))
    x = M.sine(1000.0, 18.0, -21)
    m = M.sine(1000.0, 18.0, -40); m[int(4 * FS):int(10 * FS)] *= 10 ** (34 / 20)
    out.append(('steady voice, the music steps -40 / -6 / -40 dBFS', x, m, dict(wide, sens='0', fast=False), 'time'))
    out.append(('the same, Attack Fast', x, m, dict(wide, sens='0', fast=True), 'time'))
    if quick: return out
    x = M.sine(1000.0, 20.0, -21)
    for name, m in M.kinds(20.0).items():
        if name != 'tone 1 kHz': out.append(('steady voice, music: ' + name, x, m, dict(wide, sens='0'), 'kind'))
    import vocal_corpus_lib as V
    mix = nl.music(30.0, -10.0)
    for k, s, label in ((24, dict(sens=0.0), 'defaults'), (27, dict(sens=6.0), 'Sensitivity 6'), (33, dict(sens=0.0, fast=True), 'Attack Fast'),
                        (25, dict(sens=-6.0, **wide), 'Sensitivity -6, range -12 .. 12'), (26, dict(sens='Off'), 'Sensitivity Off')):
        v = V.part(k)[0]; n = min(len(v), mix.shape[1])
        out.append(('vocal part %d with a mix at -10 dBFS peak, %s' % (k, label), v[:n], mix[:, :n], s, 'null'))
    return out


def gain_error(a, b, x):
    """the largest difference of the two gains (dB) over 50 ms windows where the voice is above -50 dBFS"""
    w = int(0.05 * FS); worst = 0.0
    for k in range(2, len(x) // w):
        s = slice(k * w, (k + 1) * w)
        if nl.rms_db(x[s]) > -50.0: worst = max(worst, abs(nl.rms_db(a[0, s]) - nl.rms_db(b[0, s])))
    return worst


def main():
    args = sys.argv[1:]
    tune = args[args.index('--tune') + 1:] if '--tune' in args else []
    rows = []
    for name, x, m, s, tag in cases('--quick' in args):
        b = theirs(x, m, s, tag)
        a = ours(x, m, s, tune)
        res = nl.residual_db(b[0], a[0], int(0.1 * FS))
        ge = gain_error(a, b, np.asarray(x))
        rows.append((name, res, ge))
        print('%-78s %7.1f dB   gain error %5.2f dB' % (name, res, ge), flush=True)
    v = np.array([r[1] for r in rows])
    print('median %.1f dB, best %.1f, worst %.1f (%d cases)' % (np.median(v), v.min(), v.max(), len(v)))
    if '--save' in args and not tune:
        import report_lib as RL
        with RL.Part('FwVocalLeveler', 'sidechain') as f:
            f.write('## With music on the side-chain\n\n')
            f.write('Flatwater Vocal Leveler against the reference, both fed the same voice on the main input and the same music on the '
                    'side-chain bus (Stereo, %d Hz; the residual after the first 0.1 s, dB re the reference output; the gain error is the '
                    'largest difference of the two gains over 50 ms). The steady voice is a 1 kHz sine at -21 dBFS rms; the vocal parts '
                    'are held-out parts of the corpus, with a dense synthetic mix as the music. Produced by '
                    '`plugins/FwVocalLeveler/nulltest/null_sidechain.py`.\n\n' % FS)
            f.write('| case | residual (dB) | gain error (dB) |\n|---|---:|---:|\n')
            for name, res, ge in rows: f.write('| %s | %.1f | %.2f |\n' % (name, res, ge))
            f.write('\nmedian %.1f dB, best %.1f, worst %.1f; %d cases\n' % (np.median(v), v.min(), v.max(), len(v)))


if __name__ == '__main__':
    main()
