"""Flatwater Bass Leveler's riding model (RideModel.c, the current fit) against the reference's riding on the bass corpus
(render_corpus.py): per part and setting the output residual and the gain error; with --events, the ride events: the
moments either side starts moving its gain after holding (a note taken), matched within 15 ms, and each unmatched one
with its context (the input's level step, the time since the last event, the gain before and after on both sides).

    python -I compare_corpus.py [train|held|all] [setting ...] [--events] [--fit path] [--save]

--save (with every part and setting: no other argument) writes the corpus part of NULLTESTS.md.
"""
import sys, os, json
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'scripts', 'nulltest'))   # scripts/nulltest/
import numpy as np
import bass_corpus_lib as K, render_corpus as CR, fit_detector as F, model_lib as B

FS = 48000


def model(x, st, g, td):
    resp = st.get('response', 'Slow'); sens = float(st.get('sensitivity', 50.0))
    return B.ride(x, trace=True, response=resp, sensitivity=sens, artifacts=float(st.get('artifacts', 50.0)),
                  target=float(st.get('target', -14.0)), **dict(g, **F.setting_params(td, resp, sens)))


def loud(x):
    ok = np.convolve(x.astype(float) ** 2, np.ones(96) / 96, 'same') > 10 ** (-5.5); ok[:4800] = False
    return ok


def residual(x, gm, gr, ok):
    ym, yr = x * 10 ** (gm / 20), x * 10 ** (gr / 20)
    return 10 * np.log10(np.sum(((ym - yr)[ok]) ** 2) / np.sum((yr[ok]) ** 2) + 1e-30)


def ride_starts(g, ok):
    """the moments the gain starts moving after holding (>= 15 ms within 0.05 dB), read on 1 ms steps: a rise or a fall
    of over 0.4 dB within the next 20 ms that is not the 32 dB/s glide"""
    h = 48; d = g[::h]; okd = ok[::h]
    ev = []; still = 0
    for i in range(1, len(d) - 21):
        if abs(d[i] - d[i - 1]) < 0.003: still += 1; continue
        if still >= 15 and okd[i] and abs(d[i + 20] - d[i]) > 0.4:
            slope = (d[i + 20] - d[i]) / 0.02
            if abs(abs(slope) - 32.0) > 6.0: ev.append(i * h)
        still = 0
    return np.array(ev, int)


def main():
    args = [a for a in sys.argv[1:]]
    if '--fit' in args: i = args.index('--fit'); F.OUT = args[i + 1]; del args[i:i + 2]
    events = '--events' in args; args = [a for a in args if a != '--events']
    save = '--save' in args; args = [a for a in args if a != '--save']
    if save and (args or events): raise SystemExit('--save takes every part and setting: give no other argument')
    which = args[0] if args and args[0] in ('train', 'held', 'all') else 'all'
    sets = [a for a in args if a in CR.SETTINGS] or list(CR.SETTINGS)
    parts = {'train': list(K.TRAIN), 'held': list(K.HELD), 'all': list(K.TRAIN) + list(K.HELD)}[which]
    g, td = F.load_fit()
    table = {s: [] for s in sets}
    split = {s: {'held': [], 'train': []} for s in sets}
    for k in parts:
        x, desc = K.part(k); ok = loud(x)
        row = []
        for s in sets:
            if not os.path.exists(CR.path(k, s)): row.append('   -  '); continue
            gr = CR.load(k, s); gm, tr = model(x, CR.SETTINGS[s], g, td)
            r = residual(x, gm, gr, ok); table[s].append(r); row.append('%6.1f' % r)
            split[s]['train' if k in K.TRAIN else 'held'].append(r)
            if events:
                er, em = ride_starts(gr, ok), ride_starts(gm, ok)
                er, em = er[er < len(x) - 2400], em[em < len(x) - 2400]
                miss = [e for e in er if not len(em) or np.min(np.abs(em - e)) > 720]
                extra = [e for e in em if not len(er) or np.min(np.abs(er - e)) > 720]
                env = 10 * np.log10(np.convolve(x.astype(float) ** 2, np.ones(960) / 960, 'same') * 2 + 1e-30)
                print('part %2d %-8s %s: ref %d rides, ours %d; missed %d, extra %d' % (k, s, desc['voice'], len(er), len(em), len(miss), len(extra)))
                for kind, lst in (('MISS ', miss), ('EXTRA', extra)):
                    for e in lst:
                        prev = [p for p in er if p < e - 720]
                        print('   %s %6.3f s  level %+6.1f (from %+6.1f 50 ms before)  ref %+6.2f -> %+6.2f  ours %+6.2f -> %+6.2f  since ref ride %s'
                              % (kind, e / FS, env[e + 960], env[max(0, e - 2400)], gr[e - 240], gr[e + 2400], gm[e - 240], gm[e + 2400],
                                 '%.0f ms' % ((e - prev[-1]) / 48) if prev else '-'))
        print('part %2d %-5s %-10s %s' % (k, 'train' if k in K.TRAIN else 'held', desc['voice'], ' '.join(row)), flush=True)
    print('settings          ' + ' '.join('%6s' % s[:6] for s in sets))
    print('median residual   ' + ' '.join('%6.1f' % (np.median(table[s]) if table[s] else np.nan) for s in sets))
    print('worst             ' + ' '.join('%6.1f' % (np.max(table[s]) if table[s] else np.nan) for s in sets))
    if save: report(split)


LABELS = {'default': 'defaults', 's0': 'Sensitivity 0', 's75': 'Sensitivity 75', 's100': 'Sensitivity 100', 'fast': 'Fast',
          'fast25': 'Fast, Sensitivity 25', 'fast100': 'Fast, Sensitivity 100', 'art15': 'Artifacts 15', 'art85': 'Artifacts 85',
          'tgt24': 'Target -24'}


def report(split):
    """the corpus part of NULLTESTS.md: the median per setting over the held-out and the fitted parts, and the worst"""
    held = np.array([v for s in split for v in split[s]['held']]); train = np.array([v for s in split for v in split[s]['train']])
    import report_lib as RL
    with RL.Part('FwBassLeveler', 'corpus') as f:
        f.write("36 twelve-second parts from FL Studio's own bass samples (acoustic, 80s pick, Ibanez, slap, Temptated, a picked "
                '5-string, a pluck) and three synthetic voices (808 sub, filtered saw, slow swells): rhythms, accents, ghost notes, '
                'rests, staccato, legato and ringing notes. %d parts were used in the fit, %d are held out. The riding model '
                "(`native/RideModel.c` with the current fit, the plug-in's ride: `check_port.py`) against the reference's gain on "
                'each part (`render_corpus.py`); the residual over the loud parts, the median per setting (dB). Produced by '
                '`plugins/FwBassLeveler/nulltest/compare_corpus.py`.\n\n' % (len(K.TRAIN), len(K.HELD)))
        f.write('| setting | held-out parts | fitted parts |\n|---|---|---|\n')
        for s in LABELS:
            if s in split: f.write('| %s | %.1f | %.1f |\n' % (LABELS[s], np.median(split[s]['held']), np.median(split[s]['train'])))
        f.write('\nOver the %d held-out renders: median %.1f dB, worst %.1f dB, %d above -20 dB. Over the %d fitted renders: median '
                '%.1f dB, worst %.1f dB, %d above -20 dB. Nearly all of the remaining error is a note ridden by one side and not the '
                'other (then the two hold 1 .. 4 dB apart until the next note: `--events` lists them), not a different gain law.\n' % (len(held), np.median(held), held.max(), (held > -20).sum(),
                                                               len(train), np.median(train), train.max(), (train > -20).sum()))


if __name__ == '__main__':
    main()
