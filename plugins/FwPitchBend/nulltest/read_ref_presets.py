"""Flatwater Pitch Bend, Phase 1: the reference's factory presets read from their files (the installed preset folder),
in our terms: the controls, the modulators' settings, the modulation slots, and what kind of shape each modulator holds
(a standard waveform by its math, or a drawn one). Shape points are not written anywhere in the repository: a drawn
shape is only named "drawn" (the original's shape library is not shipped; presets using a drawn shape get one of
ours, "shape substituted"). The preset folder is the reference folder under local.yaml's ref_presets_dir.

    python -I read_ref_presets.py            -> measurements/FwPitchBend/presets.json (by position) and a table on the console
"""
import base64, glob, json, os, re, sys, zlib
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'scripts', 'nulltest'))   # scripts/nulltest/
import numpy as np
import editor_state_lib as S
import local_config_lib as L

PRESETS = os.path.join(L.get('ref_presets_dir'), 'Vocal Bender')
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'measurements', 'FwPitchBend', 'presets.json')
SOURCES = {0: None, 1: 'M1', 2: 'M2', 3: 'AM', 4: 'PT'}


def standard_shapes(n=256):
    ph = np.arange(n) / n
    return {'sine': np.sin(2 * np.pi * ph), 'triangle': np.interp(ph, [0, 0.25, 0.75, 1], [0, 1, -1, 0]),
            'square': np.where(ph < 0.5, 1.0, -1.0), 'saw up': 2 * ph - 1, 'saw down': 1 - 2 * ph,
            'flat': np.zeros(n)}


def classify(table, kind, steps):
    if kind == 1:                                            # a sequence: its step values (integers -24 .. 24)
        return 'sequence of %d steps' % steps
    t = table[:256].astype(np.float64)
    best, err = None, 9.0
    for name, s in standard_shapes().items():
        for sign in (1, -1):
            for shift in range(0, 256, 4):
                e = float(np.max(np.abs(np.roll(sign * s, shift) - t)))
                if e < err: best, err = ('%s%s%s' % ('-' if sign < 0 else '', name, ' +%d/256' % shift if shift else '')), e
    return best if err < 0.02 else 'drawn'


def read(path):
    t = open(path, encoding='utf-8', errors='replace').read()
    m = re.search(r'<Parameters Type="RealWorld">(.*?)</Parameters>', t, re.S)
    if not m: return None
    v = [float('nan') if a == '*' else float(a) for a in m.group(1).split()]
    tables = []
    for k in (0, 1):
        d = re.search(r'TagName="SeqTable%d"[^>]*>\s*<Data>\s*<!\[CDATA\[(.*?)\]\]>' % k, t, re.S)
        tables.append(np.frombuffer(zlib.decompress(base64.b64decode(re.sub(r'\s', '', d.group(1)))), '<f4') if d else None)
    p = dict(pitch_cents=v[S.PITCH], formant_cents=v[S.FORMANT], mix=v[S.MIX], fine=int(v[S.SEMITONES] == 0),
             flatten=int(v[S.FLATTEN_ON]), flatten_note=int(v[S.FLATTEN_NOTE]),
             am=dict(attack=v[S.AM_ATTACK], release=v[S.AM_RELEASE], level=v[S.AM_LEVEL]),
             pt=dict(smooth=v[S.PT_SMOOTH], offset=v[S.PT_OFFSET], level=v[S.PT_LEVEL]), mods=[], slots={})
    for k in (0, 1):
        kind, steps = int(v[S.M_TYPE + k]), int(v[S.M_STEPS + k])
        p['mods'].append(dict(type='Seq' if kind else 'LFO', steps=steps, free=int(v[288 + k]), hz_shown=int(v[S.M_SYNC + k]),
                              note=int(v[S.M_NOTE + k]), hz=v[S.M_HZ + k], phase=v[S.M_PHASE + k], warp=v[S.M_WARP + k],
                              smooth=v[S.M_SMOOTH + k], level=v[S.M_LEVEL + k],
                              shape=classify(tables[k], kind, steps) if tables[k] is not None else 'none'))
    for target, (first, layout) in S.TARGETS.items():
        for k in ((0, 1) if layout == 'mod' else (None,)):
            for s in range(4):
                d = S.slot(target, s, 0, 0, k or 0)
                (di, depth), (si, src) = list(d.items())
                src, depth = int(v[si]), v[di]
                if src:
                    name = target if k is None else target.replace('m_', 'm%d_' % (k + 1))
                    p['slots'].setdefault(name, []).append([SOURCES.get(src, str(src)), depth])
    return p


def main():
    res, names = {}, {}
    for k, f in enumerate(sorted(glob.glob(os.path.join(PRESETS, '*.xps')))):
        p = read(f)
        res[str(k)] = p if p is not None else 'the defaults (a reset preset)'   # keyed by the position in the folder
        names[str(k)] = os.path.basename(f)[:-4]                                # (the console table only)
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    json.dump(res, open(OUT, 'w'), indent=1)
    for key, p in res.items():
        name = names[key]
        if isinstance(p, str): print('%-22s %s' % (name, p)); continue
        sl = ' '.join('%s<-%s' % (t, ','.join('%s%+g' % (a, b) for a, b in L)) for t, L in p['slots'].items())
        print('%-22s P%+5g F%+6g mix %5.1f%s%s | M1 %s %s | M2 %s %s | %s' % (
            name, p['pitch_cents'], p['formant_cents'], p['mix'], ' fine' if p['fine'] else '',
            ' flat%d' % p['flatten_note'] if p['flatten'] else '', p['mods'][0]['type'], p['mods'][0]['shape'],
            p['mods'][1]['type'], p['mods'][1]['shape'], sl))


if __name__ == '__main__':
    main()
