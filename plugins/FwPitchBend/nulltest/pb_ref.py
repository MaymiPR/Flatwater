"""Renders for Flatwater Pitch Bend's null tests and measurements: the reference (the reference, the reference sub-plugin,
through tools/RefRender: a fresh instance per render, parameters by host name and display text or normalised value),
our processor (PbRender) and our built VST3 (RefRender again). Every function takes and returns (channels, n)
float32 at fs; none has latency. Nothing rendered here is committed (nulltest/renders is git-ignored).

    import pb_ref as H
    y = H.render_ref(x, fs, {'Pitch': '-5'})              # the reference (display text), or {'Pitch:': 0.25} (normalised)
    y = H.render_ours(x, fs, {'pitch': -5})               # our processor, parameter IDs in real units
    y = H.render_vst3(x, fs, {'Mix': '50'})               # our VST3, as the reference
"""
import os, subprocess, sys, tempfile
import numpy as np
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'scripts', 'nulltest'))   # scripts/nulltest/
import local_config_lib as L

HERE = os.path.dirname(os.path.abspath(__file__))
BUILD = os.environ.get('FW_BUILD', os.path.join(HERE, '..', '..', '..', 'build'))
REFRENDER = os.path.join(BUILD, 'RefRender_artefacts', 'Release', 'RefRender.exe')
RENDER = os.path.join(BUILD, 'PbRender_artefacts', 'Release', 'PbRender.exe')
VST3 = os.path.join(BUILD, 'plugins', 'Flatwater Pitch Bend.vst3')
REF_SHELL = L.get('ref_vst3')
COMPONENTS = {2: 'Vocal Bender Stereo@185582434', 1: 'Vocal Bender Mono@184106338'}
TMP = os.path.join(HERE, 'renders'); os.makedirs(TMP, exist_ok=True)


def _f32(x):
    x = np.asarray(x, np.float32)
    return x[None, :] if x.ndim == 1 else x


def _refrender(plugin, name, x, fs, params, block, extra):
    """RefRender writes stereo: a mono input is fed to both sides and the left output returned"""
    x = _f32(x)
    ch = x.shape[0]
    st = np.concatenate([x, x]) if ch == 1 else x
    d = tempfile.mkdtemp(prefix='pb_', dir=TMP)
    fin, fout = os.path.join(d, 'in.f32'), os.path.join(d, 'out.f32')
    st.T.astype(np.float32).tofile(fin)
    cmd = [REFRENDER, plugin, name, fin, fout, 'fs=%d' % fs, 'block=%d' % block]
    for k, v in (params or {}).items():
        cmd.append('%s:=%r' % (k[:-1], float(v)) if k.endswith(':') else '%s=%s' % (k, v))
    cmd += list(extra)
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=900)
    if not os.path.exists(fout) or 'no parameter' in r.stdout: raise RuntimeError(' '.join(cmd) + '\n' + r.stdout + r.stderr)
    y = np.fromfile(fout, np.float32).reshape(-1, 2).T
    for f in (fin, fout): os.remove(f)
    os.rmdir(d)
    return (y[:1] if ch == 1 else y), r.stdout


def render_ref(x, fs, params=None, block=512, extra=('preroll=1',), log=False):
    """The reference (its Mono component for a mono input); preroll=1: its parameters are set after a first block"""
    x = _f32(x)
    y, out = _refrender(REF_SHELL, COMPONENTS[x.shape[0]], x, fs, params, block, extra)
    return (y, out) if log else y


def render_vst3(x, fs, params=None, block=512, extra=(), log=False):
    y, out = _refrender(VST3, 'Flatwater Pitch Bend', x, fs, params, block, extra)
    return (y, out) if log else y


def render_ours(x, fs, params=None, block=512):
    x = _f32(x)
    ch = x.shape[0]
    fi, fo = os.path.join(TMP, 'pbin%d.f32' % os.getpid()), os.path.join(TMP, 'pbout%d.f32' % os.getpid())
    x.T.astype(np.float32).tofile(fi)
    args = [RENDER, fi, fo, str(ch), str(fs), 'block=%d' % block] + ['%s=%r' % (k, float(v)) for k, v in (params or {}).items()]
    subprocess.run(args, check=True, capture_output=True)
    return np.fromfile(fo, np.float32).reshape(-1, ch).T


# ---- signals and measures ----------------------------------------------------------------------------------------
def noise(secs, db, fs=48000, seed=7, ch=2):
    rng = np.random.default_rng(seed)
    return (rng.standard_normal((ch, int(secs * fs))) * 10 ** (db / 20)).astype(np.float32)


def vowel(secs, f0, db, fs=48000, ch=2, vib=0.0):
    """a sung vowel stand-in: a pulse train at f0 (vibrato vib cents at 5 Hz) through three formant resonators (an /a/)"""
    from scipy.signal import lfilter
    n = int(secs * fs)
    t = np.arange(n) / fs
    f = f0 * 2 ** (vib / 1200 * np.sin(2 * np.pi * 5 * t))
    ph = np.cumsum(f) / fs
    src = np.diff(np.floor(ph), prepend=0.0)
    y = np.zeros(n)
    for fc, bw, g in ((800, 80, 1.0), (1150, 90, 0.5), (2900, 120, 0.25)):
        r = np.exp(-np.pi * bw / fs)
        a = [1, -2 * r * np.cos(2 * np.pi * fc / fs), r * r]
        y += g * lfilter([1 - r], a, src)
    y *= 10 ** (db / 20) / (np.sqrt(np.mean(y ** 2)) + 1e-30)
    return np.tile(y.astype(np.float32), (ch, 1))


def rms_db(x):
    x = np.asarray(x, np.float64)
    return 10 * np.log10(max(float(np.mean(x * x)), 1e-30))


def residual_db(a, b, skip=0):
    """20 log10 (rms (b - a) / rms (a)) after `skip` samples"""
    a = np.asarray(a, np.float64)[:, skip:]; b = np.asarray(b, np.float64)[:, skip:]
    n = min(a.shape[1], b.shape[1])
    return rms_db(b[:, :n] - a[:, :n]) - rms_db(a[:, :n])

# ---- the names first_null.py and nulltest.py --vst3 use in every harness -----------------------------------------
PLUGIN, PRODUCT = 'FwPitchBend', 'Flatwater Pitch Bend'
REF_NAME = COMPONENTS[2]


def render_ref_host(x, fs=48000, params=None, extra=()):
    return render_ref(x, fs, params, extra=extra)


def render_model(x, fs=48000, **keys):
    return render_ours(x, fs, keys or None)

