"""Null test: Flatwater Bass Lift (offline renderer of the plug-in DSP) against the reference's render, residual in dB.

    python -I bf_ref.py [--quick] [--save | --from-json] [key=value ...]     (keys go to BfRender)

For every signal x setting both render the same stereo input at 48 kHz. The residual is
20 log10( rms(ours - reference) / rms(reference) ) after the first 0.2 s, for the whole output and, separately, for
the generated (harmonic) part alone: the reference's generated part is split off with a second render at Intensity
-24 dB (Intensity is an exact gain on it), so the bass enhancer itself is judged, not only the dry signal that the
original path passes. Numbers are reported as they come: no case is labelled a null unless the data shows it.
"""
import sys, os, subprocess, json
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'scripts', 'nulltest'))   # scripts/nulltest/
import numpy as np
from scipy.signal import butter, sosfilt
import refhost as R

HERE = os.path.dirname(os.path.abspath(__file__))
EXE = os.environ.get("FWBASSLIFT_RENDER", os.path.join(HERE, "..", "..", "..", "build", "BfRender_artefacts", "Release", "BfRender.exe"))
TMP = os.path.join(HERE, "renders"); os.makedirs(TMP, exist_ok=True)
FS = 48000


def ours(x, fs, freq, intensity, gain, orig_in, tune=()):
    x = np.atleast_2d(np.asarray(x, np.float32))
    ch, n = x.shape
    fi, fo = os.path.join(TMP, "bin%d.f32" % os.getpid()), os.path.join(TMP, "bout%d.f32" % os.getpid())
    x.T.astype(np.float32).tofile(fi)
    args = [EXE, fi, fo, str(ch), str(fs), str(freq), str(intensity), str(gain), "1" if orig_in else "0", *tune]
    subprocess.run(args, check=True, capture_output=True)
    return np.fromfile(fo, np.float32).reshape(-1, ch).T


def residual_db(a, b, skip=0):
    a = np.asarray(a, np.float64)[..., skip:]; b = np.asarray(b, np.float64)[..., skip:]
    num = np.sqrt(np.mean((a - b) ** 2)); den = np.sqrt(np.mean(b ** 2))
    return 20 * np.log10(max(num, 1e-12) / max(den, 1e-12))


def signals(fs=FS, quick=False):
    rng = np.random.default_rng(5)
    n = int(3.0 * fs); t = np.arange(n) / fs
    S = {}
    for f, L in ((30, -20), (50, -12), (80, -20), (120, -12), (40, -40), (60, -3)):
        S["sine %d Hz %d dBFS" % (f, L)] = R.tone(fs, f, 3.0, L)
    f0, f1 = 20.0, 2000.0
    S["log sweep 20-2k -20"] = (0.1 * np.sin(2 * np.pi * f0 * (3.0 / np.log(f1 / f0)) * (np.exp(t / 3.0 * np.log(f1 / f0)) - 1))).astype(np.float32)
    S["bass steps -50/-10/-30"] = np.concatenate([R.tone(fs, 55, 1.0, -50), R.tone(fs, 55, 1.0, -10), R.tone(fs, 55, 1.0, -30)])
    z = rng.standard_normal(n)
    S["pink-ish noise -20 rms"] = (sosfilt(butter(1, 200, fs=fs, output='sos'), z) * 0.4).astype(np.float32)
    S["white noise -20 rms"] = (z / z.std() * 0.1).astype(np.float32)
    # bass-guitar-like notes: decaying harmonic tones
    notes = np.zeros(n);
    for k, f in enumerate((41.2, 55.0, 73.4, 49.0, 61.7, 82.4)):
        s0 = int(k * 0.5 * fs); m = int(0.5 * fs); tt = np.arange(m) / fs
        notes[s0:s0 + m] += 0.5 * np.exp(-tt / 0.3) * sum(np.sin(2 * np.pi * f * h * tt) / h ** 1.5 for h in range(1, 8))
    S["bass notes"] = notes.astype(np.float32)
    # kick-like: pitch-dropping sine bursts
    kick = np.zeros(n)
    for k in range(6):
        s0 = int(k * 0.5 * fs); m = int(0.35 * fs); tt = np.arange(m) / fs
        kick[s0:s0 + m] += 0.8 * np.exp(-tt / 0.08) * np.sin(2 * np.pi * (45 * tt + 60 * 0.03 * (1 - np.exp(-tt / 0.03))))
    S["kick-like"] = kick.astype(np.float32)
    if quick:
        keep = ["sine 50 Hz -12 dBFS", "log sweep 20-2k -20", "bass steps -50/-10/-30", "pink-ish noise -20 rms", "bass notes", "kick-like"]
        S = {k: v for k, v in S.items() if k in keep}
    return S


# (Freq, Intensity, Gain, Original Bass In)
SETTINGS = [(80, 0, 0, False), (80, 0, 0, True), (80, 12, 0, False), (80, -12, 0, True), (40, 0, 0, False), (160, 0, 0, False),
            (256, 6, -6, True), (32, 0, 0, False)]

DATA = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "measurements", "FwBassLift", "nulltests-settings.json")


def save_part(rows):
    """the Settings part of NULLTESTS.md and its rows (measurements/FwBassLift/nulltests-settings.json)"""
    tots = np.array([r["total_db"] for r in rows]); gens = np.array([r["generated_db"] for r in rows])
    import report_lib as RL
    with RL.Part("FwBassLift", "settings") as f:
        f.write("| Freq | Intensity | Gain | Original Bass | Signal | Total residual (dB) | Generated part (dB) |\n|---|---|---|---|---|---|---|\n")
        for r in rows:
            f.write("| %d | %+d | %+d | %s | %s | %.1f | %.1f |\n" % (r["freq"], r["intensity"], r["gain"], "In" if r["orig_in"] else "Out", r["signal"], r["total_db"], r["generated_db"]))
        f.write("\nMedian total %.1f dB (worst %.1f); median generated part %.1f dB (worst %.1f); %d cases.\n" % (np.median(tots), tots.max(), np.median(gens), gens.max(), len(rows)))
    json.dump(rows, open(DATA, "w", newline="\n"), indent=1)



def main():
    args = sys.argv[1:]
    quick = "--quick" in args
    tune = tuple(a for a in args if "=" in a and not a.startswith("--"))
    save = "--save" in args
    if "--from-json" in args:
        return save_part(json.load(open(DATA)))
    S = signals(quick=quick)
    settings = SETTINGS[:4] if quick else SETTINGS
    rows = []
    for (freq, inten, gain, orig_in) in settings:
        for name, x in S.items():
            xs = np.stack([x, x]).astype(np.float32)
            ref = render_ref(xs, FS, freq=freq, intensity=inten, gain=gain, hp=0 if orig_in else 1)
            ref_lo = render_ref(xs, FS, freq=freq, intensity=inten - 24 if inten - 24 >= -24 else -24, gain=gain, hp=0 if orig_in else 1)
            y = ours(xs, FS, freq, inten, gain, orig_in, tune)
            skip = int(0.2 * FS)
            tot = residual_db(y, ref, skip)
            # generated part alone (reference split off with the Intensity render; ours from a render at -inf intensity)
            g_hi = 10 ** (inten / 20); g_lo = 10 ** (max(inten - 24, -24) / 20)
            gen_ref = (ref.astype(np.float64) - ref_lo) / (1 - g_lo / g_hi)
            y0 = ours(xs, FS, freq, -200, gain, orig_in, tune)
            gen_ours = y.astype(np.float64) - y0
            gen = residual_db(gen_ours, gen_ref, skip)
            rows.append(dict(freq=freq, intensity=inten, gain=gain, orig_in=orig_in, signal=name, total_db=round(tot, 1), generated_db=round(gen, 1)))
            print("Freq %3d Int %+3d Gain %+3d %s  %-26s total %7.1f dB  generated %7.1f dB" % (freq, inten, gain, "In " if orig_in else "Out", name, tot, gen), flush=True)
    tots = np.array([r["total_db"] for r in rows]); gens = np.array([r["generated_db"] for r in rows])
    print("\nmedian total %.1f dB (worst %.1f), median generated %.1f dB (worst %.1f), %d cases" % (np.median(tots), tots.max(), np.median(gens), gens.max(), len(rows)))
    if save:
        save_part(rows)


if __name__ == "__main__":
    main()

# ---- the hosts of first_null.py and nulltest.py --vst3 ----------------------------------------------------------------
import local_config_lib as L
PLUGIN, PRODUCT = 'FwBassLift', 'Flatwater Bass Lift'
REF_NAME = L.get('FwBassLift.ref_name', required=False) or 'RBass Stereo'      # the reference's stereo component in the shell
_BUILD = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'build')
VST3 = os.path.join(_BUILD, PLUGIN + '_artefacts', 'Release', 'VST3', PRODUCT + '.vst3')
RENDER = os.path.join(_BUILD, 'BfRender_artefacts', 'Release', 'BfRender.exe')


def render_ref_host(x, fs=48000, params=None, extra=()):
    """The reference through tools/RefRender (latency removed); params by display text, 'Name:' for a normalised value"""
    return R.render_ref(x, fs, REF_NAME, params=params, extra=extra)[0]


def render_vst3(x, fs=48000, params=None, extra=()):
    """our built VST3 through tools/RefRender"""
    return R.render_ref(x, fs, PRODUCT, params=params, extra=extra, vst3=VST3)[0]


def render_model(x, fs=48000, **keys):
    """our processor (keys: renderer keys) through BfRender at the plug-in's defaults"""
    return R.render_exe(RENDER, x, fs, 80, 0, 0, 1, **keys)


# ---- the reference, every parameter set (the non-automatable ones through its state) ----------------------------
# preset-array indices (the reference's own order): 0 Input, 1 Freq, 2 Response, 3 Ratio, 4 Clip, 5 Intensity, 6 Hi Pass
# (the Original Bass switch), 7 Out gain, 8 Decay, 13 SmoothTc, 14 Tc, 15 XFadeTc, 16 Smooth
_base = {}


def render_ref(x, fs, freq=80.0, intensity=0.0, gain=0.0, hp=0, extra=None, name=None, buffer=512):
    """Render through the reference with every parameter (also the non-automatable ones) set through its state."""
    import ref_state_lib as W
    name = name or REF_NAME
    p = R.load(name)
    if name not in _base:
        _base[name] = p.raw_state
    base = _base[name]
    # hidden values (Original Bass switch, Response, Ratio, ...) through the state, visible ones as parameters
    want_hidden = {6: hp, **(extra or {})}
    cur = W.get_values(p.raw_state)
    if any(abs(float(cur[k]) - float(val)) > 1e-6 for k, val in want_hidden.items()):
        v = W.get_values(base)
        for k, val in want_hidden.items(): v[k] = '%g' % val
        W.apply_values(p, v, base)
    p.frequency = float(freq); p.intensity = float(intensity); p.out_gain = float(gain)
    got = W.get_values(p.raw_state)
    assert all(abs(float(got[k]) - float(val)) < 1e-6 for k, val in want_hidden.items()), (want_hidden, got)
    return R.render_with(p, x, fs, buffer, stereo=name.endswith('Stereo'))

