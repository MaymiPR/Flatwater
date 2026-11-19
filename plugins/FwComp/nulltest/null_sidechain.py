"""Null test of Flatwater Comp with a key on its side-chain bus: our renderer against the reference, both keyed by
the same signal, beside the same case without a key. The reference renders through tools/RefRender, which enables
the side-chain bus and feeds it; pedalboard cannot. Analog Off.

    python -I null_sidechain.py [--save] [--fs 48000]

Residual = 20 log10( rms(ours - reference) / rms(reference) ) after the first 0.2 s. Numbers are reported as they
come: no case is labelled a null unless the data shows it.
"""
import os, sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'FwVocalComp', 'nulltest'))   # plugins/FwVocalComp/nulltest/
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'scripts', 'nulltest'))   # scripts/nulltest/
import numpy as np
import refhost as R
import vc_ref as NV
import co_ref as N

PAIRS = (('pad', 'drum loop'), ('bass line', 'drum loop'), ('pad', 'bass line'))   # (the main signal, the key)


def signals(fs):
    """the main signals and the keys: a sustained three-tone pad, and the harness's drum loop and bass line"""
    S = N.signals(fs, quick=True)
    n = len(S['drum loop']); t = np.arange(n) / fs
    pad = 0.25 * (np.sin(2 * np.pi * 220.0 * t) + np.sin(2 * np.pi * 277.2 * t) + np.sin(2 * np.pi * 329.6 * t))
    return {'pad': pad.astype(np.float32), 'drum loop': S['drum loop'], 'bass line': S['bass line'][:n]}


def ref_params(thr, rat, att, rel, pu, mix, out, lim):
    """a setting of co_ref.SETTINGS as the reference's parameters for tools/RefRender: the values by display text, the
    choices (Analog Off, Sync ms, Limiter) by their normalised value, because a choice's text is not taken"""
    return {'Analog:': 0, 'Sync:': 1, 'Threshold': thr, 'Ratio': rat, 'Attack': att, 'Release': rel, 'Punch': pu, 'Mix': mix,
            'Output': out, 'Limiter:': 1 if lim == 'On' else 0}


def theirs(x, fs, st, key=None):
    return R.render_ref(x, fs, N.REF_NAME, sc=key, params=ref_params(*st))[0]


def main():
    args = sys.argv[1:]
    fs = int(args[args.index('--fs') + 1]) if '--fs' in args else 48000
    S = signals(fs)
    two = lambda x: np.stack([x, x]).astype(np.float32)
    a0 = int(0.2 * fs)
    rows = []
    for st in N.SETTINGS[:8]:
        for mname, kname in PAIRS:
            m, k = two(S[mname]), two(S[kname])
            keyed = NV.residual_db(N.ours(m, fs, *st, key=k)[0], theirs(m, fs, st, k)[0], a0)
            plain = NV.residual_db(N.ours(m, fs, *st)[0], theirs(m, fs, st)[0], a0)
            rows.append((st, mname, kname, keyed, plain))
            print('thr %5.1f r %4.1f att %5.1f rel %6.1f punch %4.1f mix %5.1f out %+5.1f lim %-3s  %-10s key %-10s %7.1f dB   (no key %7.1f dB)'
                  % (*st[:8], mname, kname, keyed, plain), flush=True)
    v = np.array([r[3] for r in rows]); w = np.array([r[4] for r in rows])
    print('with a key: median %.1f dB, best %.1f, worst %.1f (%d cases); the same cases without a key: median %.1f dB, worst %.1f'
          % (np.median(v), v.min(), v.max(), len(v), np.median(w), w.max()))
    weak = max((r for r in rows if r[1] == 'pad'), key=lambda r: r[4])      # the pad's weakest setting without a key
    lower = []
    for db in (-6, -12, -18):
        m = two(S['pad'] * 10 ** (db / 20))
        lower.append(NV.residual_db(N.ours(m, fs, *weak[0])[0], theirs(m, fs, weak[0])[0], a0))
    print('the pad without a key at threshold %g, ratio %g, release %g: %.1f dB; 6, 12 and 18 dB lower: %.1f, %.1f, %.1f dB'
          % (weak[0][0], weak[0][1], weak[0][3], weak[4], *lower), flush=True)
    if '--save' in args:
        import report_lib as RL
        with RL.Part('FwComp', 'sidechain') as f:
            f.write('## With a key on the side-chain\n\n')
            f.write('The main signal against the reference, both keyed by the same signal on the side-chain bus, and the same case '
                    'without a key (%d Hz, residual after the first 0.2 s, dB re the reference output; Analog Off). The pad is a sustained '
                    'chord of three tones. Produced by `plugins/FwComp/nulltest/null_sidechain.py`.\n\n' % fs)
            f.write('| threshold | ratio | attack | release | punch | mix | output | limiter | signal | key | residual (dB) | without a key (dB) |\n')
            f.write('|---:|---:|---:|---:|---:|---:|---:|---|---|---|---:|---:|\n')
            for st, mname, kname, keyed, plain in rows:
                f.write('| %g | %g | %g | %g | %g | %g | %g | %s | %s | %s | %.1f | %.1f |\n' % (*st[:8], mname, kname, keyed, plain))
            f.write('\nwith a key: median %.1f dB, best %.1f, worst %.1f; without a key: median %.1f dB, worst %.1f\n'
                    % (np.median(v), v.min(), v.max(), np.median(w), w.max()))
            f.write('\nThe pad without a key at its weakest setting (threshold %g, ratio %g, attack %g, release %g, punch %g): %.1f dB at '
                    'full level (peak 0.75), and %.1f, %.1f and %.1f dB with the pad 6, 12 and 18 dB lower.\n'
                    % (*weak[0][:5], weak[4], *lower))


if __name__ == '__main__':
    main()
