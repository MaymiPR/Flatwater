"""The reference's settings that only its editor sets (the modulation slots, Fine, cents values, the modulators' own
settings), written into its saved state, for Flatwater Pitch Bend's measurements. A fresh instance's state is read once
(pedalboard), its preset array changed, and the state saved to a file that vb.render_ref loads (RefRender
state=). The array's places were found by comparing the reference's factory presets and states (RESEARCH.md,
Flatwater Pitch Bend, "The preset array").

    import editor_state_lib as S
    st = S.state_file({S.PITCH: 700, **S.slot(S.MIX, 0, S.M1, -100)}, 'm1_on_mix')
    y = H.render_ref(x, 48000, extra=('preroll=1', 'state=' + st))
"""
import os, re, struct, sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'scripts', 'nulltest'))   # scripts/nulltest/
import refhost as R
import ref_state_lib as W
import pb_ref as H

# the preset array: places of the controls (values in their real units: cents, %, ms, Hz)
PITCH, FORMANT, MIX = 1372, 1373, 1456          # cents, cents, %
SEMITONES = 1484                                  # 1: Pitch and Formant in whole semitones (Fine off), 0: cents (Fine on)
FLATTEN_NOTE, FLATTEN_ON = 1495, 1497             # 0 .. 23 (C2 .. B3), 0 / 1
PT_LEVEL, PT_SMOOTH, PT_OFFSET = 204, 206, 207
AM_LEVEL, AM_ATTACK, AM_RELEASE = 424, 425, 426
# the two LFO / step modulators (M1 at +0, M2 at +1)
# (M_SYNC: 0 synced to the tempo, 1 free in Hz; M_NOTE: the synced length, 0 1/64T .. 23 32 bars; M_HZ: the free rate)
M_LEVEL, M_SMOOTH, M_WARP, M_SYNC, M_NOTE, M_HZ, M_PHASE, M_STEPS, M_TYPE = 276, 280, 284, 304, 296, 300, 388, 416, 420

# the modulation sources as a slot holds them
NONE, M1, M2, AM, PT = 0, 1, 2, 3, 4

# targets with four slots: (first place, layout). 'scalar': slot s depth at +2s, source at +2s+1; 'mod k': the
# modulators' own controls, slot s depth at +8s+k, source at +8s+4+k
TARGETS = {'pitch': (1457, 'scalar'), 'formant': (1465, 'scalar'), 'mix': (1473, 'scalar'),
           'am_attack': (1224, 'scalar'), 'am_release': (1232, 'scalar'), 'am_level': (1240, 'scalar'),
           'pt_level': (1192, 'scalar'),
           'm_rate': (428, 'mod'), 'm_level': (460, 'mod'), 'm_warp': (492, 'mod'), 'm_phase': (524, 'mod')}


def slot(target, s, source, depth, k=0):
    """{place: value} for slot s (0 .. 3) of a target (a TARGETS key; k: which modulator's control, 0 M1, 1 M2)"""
    first, kind = TARGETS[target]
    if kind == 'scalar':
        return {first + 2 * s: depth, first + 2 * s + 1: source}
    return {first + 8 * s + k: depth, first + 8 * s + 4 + k: source}


_base = {}


def base(component='Vocal Bender Stereo'):
    if component not in _base:
        p = R.load(component)
        _base[component] = (p, p.raw_state)
    return _base[component]


def tables(raw):
    """the two modulators' shape tables in a state: 512 float32 each (an LFO's cycle or a sequence's steps)"""
    import base64, zlib
    import numpy as np
    text = W.get_inner(raw)[28:].decode('latin-1')
    out = []
    for k in (0, 1):
        m = re.search(r'TagName="SeqTable%d"[^>]*>\s*<Data>\s*<!\[CDATA\[(.*?)\]\]>' % k, text, re.S)
        out.append(np.frombuffer(zlib.decompress(base64.b64decode(re.sub(r'\s', '', m.group(1)))), '<f4').copy())
    return out


def with_values(raw, changes, shapes=None):
    """the state raw with its preset array changed ({place: value}) and optionally the shape tables ({0 or 1: 512
    values}); the inner chunk's two length fields follow the text"""
    import base64, zlib
    import numpy as np
    inner = W.get_inner(raw)
    l1, l2 = struct.unpack('>I', inner[0:4])[0], struct.unpack('>I', inner[20:24])[0]
    text = inner[28:].decode('latin-1')
    v = W.get_values(raw)
    for k, val in changes.items(): v[k] = '%.10g' % val
    arr = ' '.join(v) + ' '
    new = re.sub(r'(<Parameters Type="RealWorld">)([^<]*)(</Parameters>)', lambda m: m.group(1) + '\n' + arr + m.group(3), text)
    for k, table in (shapes or {}).items():
        data = base64.b64encode(zlib.compress(np.asarray(table, '<f4').tobytes())).decode('ascii')
        new = re.sub(r'(TagName="SeqTable%d"[^>]*>\s*<Data>\s*<!\[CDATA\[)(.*?)(\]\]>)' % k, lambda m: m.group(1) + '\n' + data + '\n' + m.group(3), new, flags=re.S)
    d = len(new) - len(text)
    chunk = struct.pack('>I', l1 + d) + inner[4:20] + struct.pack('>I', l2 + d) + inner[24:28] + new.encode('latin-1')
    return W.set_inner(raw, chunk)


def state_file(changes, tag, component='Vocal Bender Stereo', check=True, shapes=None):
    """a state file with the changes (and shape tables); check: load it, process a block and read the values back"""
    import numpy as np
    p, raw = base(component)
    st = with_values(raw, changes, shapes)
    if check:
        p.raw_state = st
        p(np.zeros((1 if component.endswith('Mono') else 2, 1024), np.float32), 48000, buffer_size=512, reset=False)
        got = W.get_values(p.raw_state)
        bad = {k: got[k] for k, val in changes.items() if k not in (M_HZ, M_HZ + 1) and (got[k] == '*' or abs(float(got[k]) - val) > 1e-6)}
        if bad: raise RuntimeError('the reference did not keep %s' % bad)
    path = os.path.join(H.TMP, 'state_%s.bin' % tag)
    open(path, 'wb').write(st)
    return path


def render(x, fs, changes=None, params=None, tag='tmp', bpm=120, extra=(), shapes=None):
    """The reference with the state changes (and shape tables); host parameters are applied before the state, so give
    a control either here or there (vb.render_ref's forms)"""
    st = state_file(changes or {}, tag, H.COMPONENTS[1 if H._f32(x).shape[0] == 1 else 2].split('@')[0], shapes=shapes)
    return H.render_ref(x, fs, params, extra=('preroll=1', 'state=' + st, 'bpm=%g' % bpm) + tuple(extra))
