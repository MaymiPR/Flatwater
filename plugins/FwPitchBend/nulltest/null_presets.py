"""Flatwater Pitch Bend's 40 factory presets against the reference's own: the reference with its preset loaded into its
state (the stored values, hidden ones included, and its two shape tables), Flatwater Pitch Bend with its factory preset
(PbRender preset=k), both stereo at 48 kHz and 120 bpm from song position 0, on a sung and a spoken take of
the vocal corpus. A shifter that is not a copy cannot null sample by sample, so each preset is compared by what the
ear hears (after the first 0.1 s):
  level   the output's rms, ours minus the reference's (dB), target within 1.5
  pitch   the output's pitch track (YIN, 5 ms hop) where both are clearly voiced (aperiodicity under 0.15 on both: the
          tracker's own errors on speech stay out): the median |difference| (cents), target 15, and the share of
          frames more than 50 cents apart (gross), target 10 %
  tone    the long-term spectra (1/6 octave, 100 Hz .. 8 kHz, levels matched): the rms difference (dB), target 3
  null    the residual (dB), for the record
A preset whose LFO is one of ours in place of a drawn one ("shape substituted") is run twice: as it ships, and with
the reference's own table loaded into ours (PbRender table=), which tells the shape apart from the rest.
Writes plugins/FwPitchBend/NULLTESTS-presets.md and measurements/FwPitchBend/presets_null.json.

    python -I -B null_presets.py [our preset name ...]
"""
import sys, os, re, json, glob, subprocess, tempfile
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.append(HERE)
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'scripts'))   # plugins/FwPitchBend/scripts/
import numpy as np
from concurrent.futures import ThreadPoolExecutor
import vb as H
import editor_state as S
import measure_shifter as M
import read_ref_presets as R
import make_presets as MP

FS = 48000
BPM = 120
SKIP = FS // 10
OUT_MD = os.path.join(HERE, '..', '..', '..', 'plugins', 'FwPitchBend', 'NULLTESTS-presets.md')
OUT_JSON = os.path.join(HERE, '..', '..', '..', 'measurements', 'FwPitchBend', 'presets_null.json')
HEADER = os.path.join(HERE, '..', '..', '..', 'plugins', 'FwPitchBend', 'Source', 'Presets.h')
TARGETS = dict(level=1.5, pitch=15.0, gross=0.10, tone=3.0)


def takes():
    """('sung', x) and ('spoken', x): the longest phrases of each kind strung to about 8 s, -18 dBFS rms, stereo"""
    out = []
    for name, group in (('sung', 'sung'), ('spoken', 'rap')):
        ph = M.corpus_phrases(1.0, group=group)
        if not ph: raise SystemExit('the vocal corpus is not decoded (scripts/nulltest/vocal_corpus.py)')
        parts, n = [], 0
        for _, _, p in ph:
            parts += [p, np.zeros(FS // 4)]; n += len(p) + FS // 4
            if n > 8 * FS: break
        x = np.concatenate(parts)[:8 * FS]
        x *= 10 ** (-18 / 20) / np.sqrt(np.mean(x[np.abs(x) > 1e-4] ** 2))
        out.append((name, np.tile(x.astype(np.float32), (2, 1))))
    return out


def our_order():
    """our preset names in the program list's order (program k = index + 1)"""
    return re.findall(r'^    \{ "[^"]+", "([^"]+)", \{', open(HEADER, encoding='utf-8').read(), re.M)


def presets():
    """[(our name, program k, the reference's {place: value}, its tables, the substituted modulators)]"""
    order = our_order()
    out = []
    for path in sorted(glob.glob(os.path.join(R.PRESETS, '*.xps'))):
        p = R.read(path)
        if p is None: continue
        group, ours = MP.NAMES[os.path.basename(path)[:-4]]
        text = open(path, encoding='utf-8', errors='replace').read()
        vals = re.search(r'<Parameters Type="RealWorld">(.*?)</Parameters>', text, re.S).group(1).split()
        changes = {i: float(a) for i, a in enumerate(vals) if a != '*'}
        tabs = MP.tables(path)
        used = [k for k in (0, 1) if any(src == 'M%d' % (k + 1) for lst in p['slots'].values() for src, _ in lst)]
        subst = [k for k in used if p['mods'][k]['type'] == 'LFO' and not MP.nearest(tabs[k])[2]]
        out.append((ours, order.index(ours) + 1, changes, tabs, subst))
    return out


def ref(x, changes, tabs, tag):
    st = S.state_file(changes, tag, check=False, shapes={0: tabs[0], 1: tabs[1]})
    return H.render_ref(x, FS, extra=('preroll=1', 'state=' + st, 'bpm=%g' % BPM))


def ours(x, k, tables=None):
    d = tempfile.mkdtemp(prefix='pbp_', dir=H.TMP)
    fi, fo = os.path.join(d, 'in.f32'), os.path.join(d, 'out.f32')
    x.T.astype(np.float32).tofile(fi)
    args = [H.RENDER, fi, fo, '2', str(FS), 'preset=%d' % k, 'preroll=1', 'bpm=%g' % BPM]
    for m, t in (tables or {}).items():
        f = os.path.join(d, 'table%d.f32' % m)
        np.asarray(t, '<f4').tofile(f)
        args.append('table=%d:%s' % (m, f))
    r = subprocess.run(args, capture_output=True, text=True)
    if r.returncode: raise RuntimeError(r.stderr)
    return np.fromfile(fo, np.float32).reshape(-1, 2).T


def compare(a, b):
    """a: the reference's output, b: ours (2 x n)"""
    a = np.asarray(a, np.float64)[:, SKIP:]; b = np.asarray(b, np.float64)[:, SKIP:]
    level = H.rms_db(b) - H.rms_db(a)
    _, fa, pa = M.yin(a.mean(0), hop=240)
    _, fb, pb = M.yin(b.mean(0), hop=240)
    both = np.isfinite(fa) & np.isfinite(fb) & (pa < 0.15) & (pb < 0.15)
    c = np.abs(M.cents(fb[both], fa[both])) if both.any() else np.array([])
    _, la = M.ltas_db(a.mean(0)); _, lb = M.ltas_db(b.mean(0))
    dl = (lb - lb.mean()) - (la - la.mean())
    res = dict(level=round(float(level), 2), pitch=round(float(np.median(c)), 1) if c.size else None,
               gross=round(float(np.mean(c > 50)), 3) if c.size else None, voiced=int(both.sum()),
               tone=round(float(np.sqrt(np.mean(dl * dl))), 2), null=round(float(H.residual_db(a, b)), 1))
    res['pass'] = bool(abs(res['level']) <= TARGETS['level'] and res['tone'] <= TARGETS['tone']
                       and (res['pitch'] is None or (res['pitch'] <= TARGETS['pitch'] and res['gross'] <= TARGETS['gross'])))
    return res


def run(item, xs):
    name, k, changes, tabs, subst = item
    row = dict(program=k, substituted=['M%d' % (m + 1) for m in subst], takes={})
    for tname, x in xs:
        tag = 'preset%d_%s_%d' % (k, tname, os.getpid())
        a = ref(x, changes, tabs, tag)
        row['takes'][tname] = compare(a, ours(x, k))
        if subst:
            row['takes'][tname]['own_shape'] = compare(a, ours(x, k, {m: tabs[m] for m in subst}))
    return name, row


def cell(r):
    p = 'n/a' if r['pitch'] is None else '%g c / %d %%' % (r['pitch'], round(100 * r['gross']))
    return '%+.1f | %s | %.1f | %.1f | %s' % (r['level'], p, r['tone'], r['null'], 'pass' if r['pass'] else '**miss**')


def write(rows):
    order = our_order()
    names = sorted(rows, key=order.index)
    groups = re.findall(r'^    \{ "([^"]+)", "([^"]+)", \{', open(HEADER, encoding='utf-8').read(), re.M)
    group = {n: g for g, n in groups}
    tk = list(next(iter(rows.values()))['takes'])
    npass = sum(r['takes'][t]['pass'] for r in rows.values() for t in tk)
    L = ['# Flatwater Pitch Bend: the factory presets against the reference\'s own', '',
         'Each of the reference\'s 40 presets loaded into its state (its stored values, hidden ones included, and its two',
         'shape tables), against Flatwater Pitch Bend\'s preset of the same settings (Flatwater names), both stereo at',
         '48 kHz and %d bpm from song position 0, on a sung and a spoken take of the vocal corpus (about 8 s each). Made by' % BPM,
         '`plugins/FwPitchBend/nulltest/null_presets.py`. Our shifter is our own method, so the outputs are compared by what is heard,',
         'not sample by sample (after the first 0.1 s):', '',
         '- **level**: the output\'s rms, ours minus the reference\'s, dB (target within %g);' % TARGETS['level'],
         '- **pitch**: where both outputs are clearly voiced, the median difference of their pitch tracks in cents (target %g),' % TARGETS['pitch'],
         '  and the share of frames more than 50 cents apart (target %d %%);' % round(100 * TARGETS['gross']),
         '- **tone**: the rms difference of the long-term spectra, 1/6 octave from 100 Hz to 8 kHz, levels matched, dB',
         '  (target %g);' % TARGETS['tone'],
         '- **null**: the output residual, dB, for the record.', '',
         'A preset marked *shape substituted* uses one of our own LFO shapes where the reference\'s is a drawn one (we do',
         'not ship those); its second line is the same preset with the reference\'s table loaded into ours, which shows how',
         'much of the difference is the shape.', '',
         'In a blend (Mix well under 100 %) the pitch column measures which of the two voices a tracker hears frame by',
         'frame, which is decided where they are close in level; MEASUREMENTS.md (Phase 3) checks those presets\' modulation',
         'directly.', '',
         '%d of %d preset and take pairs meet every target.' % (npass, len(rows) * len(tk)), '']
    for t in tk:
        L += ['## The %s take' % t, '', '| preset | level | pitch | tone | null | |', '|---|---|---|---|---|---|']
        for n in names:
            r = rows[n]
            label = '%s / %s' % (group[n], n) + (' (shape substituted: %s)' % ', '.join(r['substituted']) if r['substituted'] else '')
            L.append('| %s | %s |' % (label, cell(r['takes'][t])))
            if 'own_shape' in r['takes'][t]:
                L.append('| &nbsp;&nbsp;with the reference\'s shape | %s |' % cell(r['takes'][t]['own_shape']))
        L.append('')
    open(OUT_MD, 'w', encoding='utf-8', newline='\n').write('\n'.join(L))


def main():
    want = set(sys.argv[1:])
    items = [p for p in presets() if not want or p[0] in want]
    xs = takes()
    S.base()                                                       # (the reference's default state, loaded once, before the threads)
    rows = json.load(open(OUT_JSON)) if os.path.exists(OUT_JSON) else {}
    with ThreadPoolExecutor(4) as ex:
        for name, row in ex.map(lambda it: run(it, xs), items):
            rows[name] = row
            print('%-24s %s' % (name, '  '.join('%s: %s' % (t, cell(v)) for t, v in row['takes'].items())), flush=True)
    json.dump(rows, open(OUT_JSON, 'w'), indent=1)
    if len(rows) == 40: write(rows)


if __name__ == '__main__':
    main()
