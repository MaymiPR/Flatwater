"""The suite's reference host: renders audio through the installed references (the shell VST3 named by local.yaml's
ref_vst3, each reference a component of it) with pedalboard or tools/RefRender, and through our own builds, for local
analysis only. Nothing rendered here is committed (see .gitignore). Each plug-in's harness (plugins/<Id>/nulltest/
<ab>_ref.py) holds what is particular to its reference: the component, the parameter names and the defaults.

    import refhost as R
    p = R.load(name); y = R.render_with(p, x, fs)          # pedalboard, a fresh render with dropout retries
    y, log = R.render_ref(x, fs, name, params={...})        # tools/RefRender: side-chain, host tempo, state files
    y, log = R.render_ref(x, fs, product, vst3=path)        # the same host on one of our built VST3 bundles
    y = R.render_exe(exe, x, fs, *positional, **keys)       # one of our offline renderers (<Ab>Render)
"""
import os, sys
import numpy as np
import pedalboard as pb
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import local_config_lib as L

REF_SHELL = L.get('ref_vst3')
ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
_cache = {}
STATS = {"silent_retries": 0}


def _named(name):
    """a component's name, or stop with the keys that set it (a harness reads them with required=False)"""
    if not name:
        raise SystemExit('local.yaml: set the reference component <Id>.ref_name (and <Id>.ref_mono_name and kin where a '
                         'script uses one; scripts/nulltest/README.md lists the keys)')
    return name


def fresh(name):
    """forget the cached instance: the next render loads a new one (a reference can keep state from the settings
    of earlier renders that its parameters do not show)"""
    _cache.pop(name, None)


def load(name):
    if name not in _cache:
        _cache[name] = pb.load_plugin(REF_SHELL, plugin_name=_named(name))
    return _cache[name]


def dropout(x, y, fs, win=0.01):
    """The shell now and then returns blocks of pure silence while the input is not silent; detect them."""
    n = max(1, int(win * fs))
    m = x.shape[1] // n
    xi = np.abs(x[:, : m * n]).reshape(x.shape[0], m, n).max(axis=2)
    yo = np.abs(y[:, : m * n]).reshape(y.shape[0], m, n).max(axis=2)
    skip = 8   # the first windows hold the latency / start-up
    return bool(np.any((xi[:, skip:] > 1e-5) & (yo[:, skip:] < 1e-9)))


def render_with(p, x, fs, buffer=512, stereo=False):
    """x: (channels, n) or (n,) float; stereo=True: a mono input goes to both channels (a stereo component).
    Renders from a reset state, again while the output has dropouts (up to eight times)."""
    x = np.asarray(x, np.float32)
    if x.ndim == 1: x = x[None, :]
    if stereo and x.shape[0] == 1: x = np.concatenate([x, x])
    for attempt in range(8):
        y = p(x, fs, buffer_size=buffer, reset=True)
        if not dropout(x, y, fs):
            break
        STATS["silent_retries"] += 1
    return y


def tone(fs, f, secs, db):
    t = np.arange(int(secs * fs)) / fs
    return (10 ** (db / 20) * np.sin(2 * np.pi * f * t)).astype(np.float32)


def rms_db(x):
    x = np.asarray(x, np.float64)
    return 10 * np.log10(max(float(np.mean(x * x)), 1e-30))


REFRENDER = os.path.join(ROOT, "build", "RefRender_artefacts", "Release", "RefRender.exe")


def render_ref(x, fs, name, sc=None, bpm=0.0, params=None, block=512, extra=(), vst3=None):
    """Render offline through tools/RefRender (a JUCE host): the component `name` of the shell (or the plug-in `name`
    in the VST3 bundle `vst3`, one of our builds), the side-chain bus fed with sc (stereo or mono), a host tempo,
    parameters by display text (name:=x for a normalised value); extra: more RefRender arguments. Latency removed.
    Returns ((2, n), RefRender's log)."""
    import subprocess, tempfile
    x = np.asarray(x, np.float32)
    if x.ndim == 1: x = np.stack([x, x])
    d = tempfile.mkdtemp(prefix="refr_")
    fin, fout, fsc = os.path.join(d, "in.f32"), os.path.join(d, "out.f32"), os.path.join(d, "sc.f32")
    x.T.astype(np.float32).tofile(fin)
    cmd = [REFRENDER, vst3 or REF_SHELL, _named(name), fin, fout, "fs=%d" % fs, "block=%d" % block]
    if sc is not None:
        sc = np.asarray(sc, np.float32)
        if sc.ndim == 1: sc = np.stack([sc, sc])
        sc.T.astype(np.float32).tofile(fsc); cmd.append("sc=" + fsc)
    if bpm: cmd.append("bpm=%g" % bpm)
    for k, v in (params or {}).items(): cmd.append("%s=%s" % (k, v))
    cmd += list(extra)                          # e.g. preroll=1, at=0.5:Delay Sec=100
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
    if not os.path.exists(fout): raise RuntimeError(r.stdout + r.stderr)
    y = np.fromfile(fout, np.float32).reshape(-1, 2).T
    return y, r.stdout


def render_exe(exe, x, fs, *positional, **keys):
    """one of our offline renderers (<Ab>Render in.f32 out.f32 channels fs [positional ...] [key=value ...]) on x,
    (channels, n); returns (channels out, n), the renderer's own channel count (always stereo for ChRender)"""
    import subprocess, tempfile
    x = np.atleast_2d(np.asarray(x, np.float32))
    if not os.path.isfile(exe):
        raise RuntimeError('%s is not built (cmake --build build --config Release --target %s)'
                           % (exe, os.path.splitext(os.path.basename(exe))[0]))
    d = tempfile.mkdtemp(prefix="ours_")
    fin, fout = os.path.join(d, "in.f32"), os.path.join(d, "out.f32")
    x.T.astype(np.float32).tofile(fin)
    cmd = [exe, fin, fout, str(x.shape[0]), str(fs)] + [str(v) for v in positional] + ['%s=%s' % kv for kv in keys.items()]
    subprocess.run(cmd, check=True, capture_output=True)
    y = np.fromfile(fout, np.float32)
    ch = 2 if y.size == 2 * x.shape[1] else x.shape[0]
    return y.reshape(-1, ch).T


EQUAL_DB = -120.0      # nulltest.py --vst3: the built VST3 equals the renderer when the residual is below this


def vst3_main(h, argv, cases=None):
    """nulltest.py --vst3 [--fs rate] [--only text] [--threshold dB]: our built VST3 (h.render_vst3, through
    tools/RefRender) against our renderer (h.render_model), case by case, without the reference; h is a plug-in's
    harness. A case is (name, x, params, extra, model): params are the VST3's settings by display text ('Name:' for
    a normalised value), extra more RefRender arguments, model the renderer's keys or a function of (x, fs) that
    renders it. A sixth item is the signal for the VST3's side-chain bus (h.render_vst3 then takes sc=). cases(fs) gives a plug-in's own cases; without it the plug-in's defaults on noise, a low tone and
    drums. Both sides get h.CHECK, the settings that make the plug-in deterministic (an Analog stage off). Prints one
    line per case; the residual is over the whole render, in dB re the renderer's output. Returns the exit code: 0
    every case below the threshold, 1 a case above it, 2 the check could not run (the VST3, the renderer or RefRender
    is not built, or no case matches)."""
    import argparse, subprocess
    ap = argparse.ArgumentParser(prog='nulltest.py --vst3', description='Our built VST3 against our renderer; no reference needed.')
    ap.add_argument('--vst3', action='store_true'); ap.add_argument('--only', default=None)
    ap.add_argument('--fs', type=int, default=48000); ap.add_argument('--threshold', type=float, default=EQUAL_DB)
    v = ap.parse_args(argv)
    missing = [q for q in (REFRENDER, getattr(h, 'VST3', None), getattr(h, 'RENDER', None)) if q and not os.path.exists(q)]
    if missing:
        print('not built: ' + ', '.join(os.path.relpath(q, ROOT).replace(os.sep, '/') for q in missing))
        return 2
    if cases is None:
        import nulltest_lib as nl
        cases = lambda fs: [(name, x, {}, (), {}) for name, x in (
            ('noise -18 dBFS', nl.noise(3.0, -18.0, seed=7, fs=fs)), ('tone 100 Hz -12 dBFS', nl.tone(100.0, 3.0, -12.0, fs=fs)),
            ('drums', nl.drums(3.0, -6.0, fs=fs)))]
    rows = [c for c in cases(v.fs) if not v.only or v.only.lower() in c[0].lower()]
    if not rows:
        print('no case matches --only ' + str(v.only))
        return 2
    params0, keys0 = getattr(h, 'CHECK', ({}, {}))
    worst, failed = -400.0, 0
    for name, x, params, extra, model, *more in rows:
        x = np.asarray(x, np.float32)
        if x.ndim == 1: x = np.stack([x, x])
        try:
            a = h.render_vst3(x, v.fs, {**params0, **params}, extra=tuple(extra), **({'sc': more[0]} if more else {}))
            b = model(x, v.fs) if callable(model) else h.render_model(x, v.fs, **{**keys0, **model})
        except (RuntimeError, OSError, subprocess.CalledProcessError) as e:
            print('could not render %s: %s' % (name, str(e).strip().splitlines()[-1] if str(e).strip() else type(e).__name__))
            return 2
        n = min(a.shape[1], b.shape[1])
        d = np.asarray(a[:, :n], np.float64) - b[:, :n]
        res = 10 * np.log10(max(float(np.sum(d * d)), 1e-30) / max(float(np.sum(np.asarray(b[:, :n], np.float64) ** 2)), 1e-30))
        worst = max(worst, res); failed += res >= v.threshold
        print('%s  %-58s %7.1f dB' % ('ok  ' if res < v.threshold else 'FAIL', name, res), flush=True)
    print('%d case(s) at %d Hz, the VST3 against the renderer: worst %.1f dB, limit %g dB, %d above it'
          % (len(rows), v.fs, worst, v.threshold, failed))
    return 1 if failed else 0
