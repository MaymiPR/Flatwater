"""Writes plugins/FwPitchBend/Source/Presets.h: Flatwater Pitch Bend's factory presets, the reference's 40 presets'
values (read by plugins/FwPitchBend/nulltest/read_ref_presets.py from the installed preset folder) under our own names and groups
(approved 2026-11-01). Each preset's settings that are not host parameters travel as extra values (the slots,
Fine, the modulators' shapes and steps), applied by the processor when the preset loads. A modulator's LFO shape is
one of ours: the same shape where the reference's is a standard one by its math, the nearest of ours where it is drawn
("shape substituted", listed in NULLTESTS-presets.md); a sequence keeps its step values (approved
2026-11-01). A rate stored in Hz without the reference's free-running flag (every factory preset's) is not run free
by the reference but snapped to the synced length nearest in time at the host's tempo (MEASUREMENTS.md, Phase 3), so
it becomes that synced length at 120 bpm.

    python -I plugins/FwPitchBend/scripts/make_presets.py
"""
import base64, glob, os, re, sys, zlib
HERE = os.path.dirname(os.path.abspath(__file__))
import numpy as np
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'nulltest'))   # plugins/FwPitchBend/nulltest/
import read_ref_presets as R

OUT = os.path.join(HERE, '..', '..', '..', 'plugins', 'FwPitchBend', 'Source', 'Presets.h')

# position in the reference's preset folder (sorted file names, as read_ref_presets.py lists them) -> (our group, our
# name); the position left out is its reset preset
NAMES = {
    0: ('Character', 'Deeper Voice'),
    1: ('Movement', 'Breathing Formant'),
    2: ('Character', 'Younger'),
    3: ('Harmony', 'Fourth Above'),
    4: ('Robot', 'Stepping Robot'),
    5: ('Movement', 'Shape Shifter'),
    6: ('Character', 'Tiny Voice'),
    7: ('Movement', 'Drawl'),
    8: ('Harmony', 'Octave Below Blend'),
    9: ('Movement', 'Deep Warble'),
    10: ('Double', 'Loose Double'),
    11: ('Character', 'Groggy Voice'),
    12: ('Movement', 'Wavering Voice'),
    14: ('Harmony', 'Fifth Above Blend'),
    15: ('Robot', 'Arcade Steps'),
    16: ('Movement', 'Pitch Follower Down'),
    17: ('Robot', 'Glide Robot'),
    18: ('Double', 'Stutter Twin'),
    19: ('Double', 'Tracked Double High'),
    20: ('Sequence', 'Plain Sequence'),
    21: ('Character', 'Pushy'),
    22: ('Double', 'Tracked Double Low'),
    23: ('Sequence', 'Lullaby Steps'),
    24: ('Character', 'Higher Register'),
    25: ('Harmony', 'Bass Backing'),
    26: ('Movement', 'Fan Wobble'),
    27: ('Character', 'Cartoon'),
    28: ('Robot', 'Alien Steps'),
    29: ('Sequence', 'Long Journey'),
    30: ('Sequence', 'Twin Sequences'),
    31: ('Double', 'Thin Double'),
    32: ('Double', 'Tape Double'),
    33: ('Robot', 'Vinyl Scrub'),
    34: ('Movement', 'Alarm'),
    35: ('Sequence', 'Rising Steps'),
    36: ('Movement', 'Warped Tape'),
    37: ('Sequence', 'Chopped Melody'),
    38: ('Movement', 'Shiver'),
    39: ('Movement', 'Submerged'),
    40: ('Movement', 'Wobble'),
}
GROUP_ORDER = ['Character', 'Harmony', 'Double', 'Movement', 'Robot', 'Sequence']

# our shapes (plugins/FwPitchBend/Source/dsp/Model.h Shape, in its order) by their math, for the nearest match
def our_shapes(n=256):
    ph = np.arange(n) / n
    sc = np.where(ph < 0.5, 2 * ph, 2 - 2 * ph)
    rnd = np.array([0.62, -0.48, 0.15, -0.91, 0.83, -0.12, 0.37, -0.70])
    return [('Sine', np.sin(2 * np.pi * ph)), ('Triangle', np.interp(ph, [0, .25, .75, 1], [0, 1, -1, 0])),
            ('Square', np.where(ph < .5, 1.0, -1.0)), ('Saw Up', 2 * ph - 1), ('Saw Down', 1 - 2 * ph),
            ('Ramp', 2 * (np.exp(3 * ph) - 1) / (np.exp(3) - 1) - 1), ('S-Curve', 2 * sc * sc * (3 - 2 * sc) - 1),
            ('Half Sine', 2 * np.abs(np.sin(np.pi * ph)) - 1), ('Pulse 25', np.where(ph < .25, 1.0, -1.0)),
            ('Pulse 75', np.where(ph < .75, 1.0, -1.0)), ('Four Steps', 1 - 2 * np.floor(ph * 4) / 3),
            ('Random Steps', rnd[np.minimum(7, (ph * 8).astype(int))]), ('Flat', np.zeros(n))]


def tables(path):
    t = open(path, encoding='utf-8', errors='replace').read()
    out = []
    for k in (0, 1):
        d = re.search(r'TagName="SeqTable%d"[^>]*>\s*<Data>\s*<!\[CDATA\[(.*?)\]\]>' % k, t, re.S)
        out.append(np.frombuffer(zlib.decompress(base64.b64decode(re.sub(r'\s', '', d.group(1)))), '<f4').astype(float))
    return out


def nearest(table):
    """our shape for an LFO cycle: the same one where it is one of ours by its math (rms difference under 0.02), else
    the one that moves most like it (the highest correlation; Flat only for a flat cycle); and whether it is the same"""
    t = table[:256]
    shapes = our_shapes()
    for i, (name, s) in enumerate(shapes):
        if np.sqrt(np.mean((s - t) ** 2)) < 0.02: return i, name, True
    if np.std(t) < 1e-3: return len(shapes) - 1, 'Flat', True
    corr = [(float(np.corrcoef(s, t)[0, 1]), i, name) for i, (name, s) in enumerate(shapes) if name != 'Flat']
    c, i, name = max(corr)
    return i, name, False


TARGET_KEYS = {'pitch': 'pitch', 'formant': 'formant', 'mix': 'mix', 'am_attack': 'amAttack', 'am_release': 'amRelease',
               'am_level': 'amLevel', 'pt_level': 'ptLevel'}
SOURCES = {'M1': 1, 'M2': 2, 'AM': 3, 'PT': 4}


def target_key(name):
    if name in TARGET_KEYS: return TARGET_KEYS[name]
    m = re.match(r'm([12])_(rate|level|warp|phase)$', name)
    return 'm%s%s' % (m.group(1), m.group(2).capitalize())


SYNCED_BEATS = [128.0, 64.0, 32.0, 16.0, 8.0, 4.0, 3.0, 2.0, 1.5, 4 / 3, 1.0, 0.75, 2 / 3, 0.5, 0.375, 1 / 3, 0.25,
                0.1875, 1 / 6, 0.125, 0.09375, 1 / 12, 0.0625, 1 / 24]       # plugins/FwPitchBend/Source/dsp/Mod.h syncedBeats, our index order


def snapped(hz, bpm=120.0):
    """our synced index for a rate in Hz as the reference runs it: the length nearest in time at the tempo"""
    beats = bpm / 60.0 / hz
    return min(range(len(SYNCED_BEATS)), key=lambda i: abs(SYNCED_BEATS[i] - beats))


def values(p, tabs):
    v = [('pitch', p['pitch_cents'] / 100.0), ('formant', p['formant_cents'] / 100.0), ('mix', p['mix']),
         ('flatten', p['flatten']), ('flattenNote', p['flatten_note']),
         ('amAttack', p['am']['attack']), ('amRelease', p['am']['release']), ('amLevel', p['am']['level']),
         ('ptSmooth', p['pt']['smooth']), ('ptOffset', p['pt']['offset']), ('ptLevel', p['pt']['level']),
         ('fine', p['fine'])]
    notes = []
    for k, m in enumerate(p['mods']):
        pre = 'm%d' % (k + 1)
        free = 1 if m['free'] else 0                              # Sync off: a free rate in Hz (approved 2026-11-01)
        synced = snapped(m['hz']) if (m['hz_shown'] and not free) else 23 - m['note']
        v += [(pre + 'Type', 1 if m['type'] == 'Seq' else 0), (pre + 'Steps', m['steps']), (pre + 'Sync', free),
              (pre + 'Rate', m['hz']), (pre + 'RateSynced', synced), (pre + 'Phase', m['phase']),
              (pre + 'Warp', m['warp']), (pre + 'Smooth', m['smooth']), (pre + 'Level', m['level'])]
        i, name, same = nearest(tabs[k])
        v.append((pre + 'Shape', i))
        if not same: notes.append('%s LFO: %s' % (pre.upper(), name))
        for s in range(16):
            v.append(('%sStep%d' % (pre, s), int(round(tabs[k][256 + s] * 24))))
    for t, lst in p['slots'].items():
        for s, (src, depth) in enumerate(lst):
            v += [('slot.%s.%d.src' % (target_key(t), s), SOURCES[src]), ('slot.%s.%d.depth' % (target_key(t), s), depth)]
    return v, notes


def fmt(x):
    x = float(x)
    return ('%.6g' % x) + ('.0f' if re.fullmatch(r'-?\d+', '%.6g' % x) else 'f')


def main():
    rows, subst = [], []
    for k, path in enumerate(sorted(glob.glob(os.path.join(R.PRESETS, '*.xps')))):
        p = R.read(path)
        if p is None: continue                                    # the reset preset: our Default
        if k not in NAMES: raise SystemExit('a preset without our name: position %d' % k)
        group, ours = NAMES[k]
        vals, notes = values(p, tables(path))
        used = [k for k, m in enumerate(p['mods']) if any(src == 'M%d' % (k + 1) for lst in p['slots'].values() for src, _ in lst)]
        notes = [n for n in notes if int(n[1]) - 1 in used and p['mods'][int(n[1]) - 1]['type'] == 'LFO']
        if notes: subst.append((group, ours, notes))
        rows.append((GROUP_ORDER.index(group), ours, '    { "%s", "%s", { %s } },' % (group, ours, ', '.join('{ "%s", %s }' % (k, fmt(x)) for k, x in vals))))
    assert len(rows) == 40 and len({r[1] for r in rows}) == 40
    rows.sort()
    with open(OUT, 'w', encoding='utf-8', newline='\n') as f:
        f.write('#pragma once\n// Factory presets of Flatwater Pitch Bend, generated by plugins/FwPitchBend/scripts/make_presets.py (do not edit by hand).\n'
                '// Setting values carried over from the reference\'s presets; names and groups are Flatwater\'s own. Values that\n'
                '// are not host parameters (the slots, Fine, the shapes and steps) are applied by the processor as the preset loads.\n'
                '#include "shared/Presets.h"\n\nnamespace fwpitchbend\n{\ninline const std::vector<fw::FactoryPreset>& factoryPresets()\n{\n'
                '    static const std::vector<fw::FactoryPreset> list {\n')
        for _, _, line in rows: f.write(line + '\n')
        f.write('    };\n    return list;\n}\n}\n')
    print('wrote %d presets; shape substituted in %d:' % (len(rows), len(subst)))
    for g, n, notes in subst: print('  %s / %s: %s' % (g, n, '; '.join(notes)))


if __name__ == '__main__':
    main()
