"""Null test: Flatwater Delay (offline renderer of the plug-in DSP, plugins/FwDelay/tools/DeRender) against the reference
(de_ref.py), residual in dB. Settings by de_ref.py's short names; both get the same silent pre-roll (the
reference starts at 375 ms and glides to the settings during it; its first block runs at the default Analog
Mod2, whose noise enters the delay line: 3 s let it die away). Analog Off unless a case names a mode.

    python -I nulltest.py [--quick] [--save] [--fs 48000] [--band 20000]

Residual = 20 log10( rms(ours - reference) / rms(reference) ) over the input (the pre-roll is not written). Numbers
are reported as they come.
    python -I nulltest.py --report     NULLTESTS.md again from measurements/FwDelay/nulltests.json, rendering nothing
    python -I nulltest.py --vst3       our built VST3 against our renderer (the check before a number is published)
"""
import sys, os, subprocess
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'FwComp', 'nulltest'))   # plugins/FwComp/nulltest/
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'scripts', 'nulltest'))   # scripts/nulltest/
import numpy as np
import de_ref as H
import co_ref as NC

HERE = os.path.dirname(os.path.abspath(__file__))
EXE = os.environ.get("FWDELAY_RENDER", os.path.join(HERE, "..", "..", "..", "build", "DeRender_artefacts", "Release", "DeRender.exe"))
TMP = os.path.join(HERE, "renders"); os.makedirs(TMP, exist_ok=True)
EXTRA = []                                               # more DeRender arguments (fitting)
_T = 1.0 - 3.0e-9                                        # The reference's triplets, a hair short (as plugins/FwDelay/Source/PluginProcessor.cpp)
BEATS = [_T / 24, 1 / 16, 3 / 32, _T / 12, 1 / 8, 3 / 16, _T / 6, 1 / 4, 3 / 8, _T / 3, 1 / 2, 3 / 4, 2 * _T / 3, 1, 1.5, 4 * _T / 3, 2, 3, 4, 8]


EXACT = False                                            # values as a preset gives them: no rounding (exact=1)


def rate_res(v):
    """Rate at the reference's resolution: three significant digits, at least 0.01 (as plugins/FwDelay/Source/PluginProcessor.cpp)"""
    if EXACT: return v
    step = 10.0 if v >= 1000 else 1.0 if v >= 100 else 0.1 if v >= 10 else 0.01
    return round(v / step) * step


class State:
    """The reference's time from Sync, the note, the tempo and Delay (as the plug-in computes it)"""
    def __init__(self, host_bpm):
        self.v = dict(sync="Host", note="1/8D", note_r="1/8D", bpm=120.0, delay=375.0, delay_r=375.0, rate=0.1)
        self.host = host_bpm

    def set(self, k, v):
        self.v[k] = v
        if k == "delay_r": self.v["delay"] = v           # linked without Dual: the last set wins (measured)
        if k == "delay": self.v["delay_r"] = v

    def times(self):
        def t(note, ms):
            s = self.v["sync"]
            if s == "ms": return float(ms if EXACT else round(ms))  # The reference's resolution: 1 ms (and 1 BPM)
            beats = BEATS[H.NOTES.index(note)]
            return beats * 60000.0 / ((float(self.v["bpm"]) if EXACT else float(round(float(self.v["bpm"])))) if s == "BPM" else self.host)
        return t(self.v["note"], float(self.v["delay"])), t(self.v["note_r"], float(self.v["delay_r"]))

    def rate(self):
        """the LFO's rate: in Host / BPM sync Rate counts cycles per beat at 60 BPM (measured)"""
        s = self.v["sync"]
        return rate_res(float(self.v["rate"])) * (1.0 if s == "ms" else (float(round(float(self.v["bpm"]))) if s == "BPM" else self.host) / 60.0)


def our_args(k, v):
    if k in ("sync", "note", "note_r", "bpm", "delay", "delay_r", "rate"): return []
    if k == "hipass": return ["hipass=%g" % (0 if v == "Off" else float(v))]
    if k == "lopass": return ["lopass=%g" % (0 if v == "Off" else float(v))]
    if k == "lofi": return ["lofi=%d" % (v == "LoFi")]
    if k == "pingpong": return ["pingpong=%d" % (v == "PingPong")]
    if k == "start": return ["start=%d" % (v == "Right")]
    if k in ("phase_l", "phase_r"): return ["%s=%d" % (k, v == "-")]
    if k == "analog": return ["analog=%d" % (0 if v == "Off" else int(str(v)[-1]))]
    return ["%s=%g" % (k, float(v))]


def ours(x, fs=48000, comp="Stereo", preroll=3.0, host_bpm=120.0, changes=(), block=512, **settings):
    x = np.atleast_2d(np.asarray(x, np.float32))
    if x.shape[0] == 1: x = np.concatenate([x, x])
    st = State(host_bpm)
    args = (["exact=1"] if EXACT else []) + ["analog=2", "rate0=%.9f" % State(host_bpm).rate()]   # The reference's defaults (its first block at 375 ms)
    for k, v in settings.items():
        st.set(k, v); args += our_args(k, v)
    tl, tr = st.times()
    args += ["time=%.17g" % tl, "time_r=%.17g" % tr, "rate=%.9f" % st.rate(), "mode=%d" % {"Stereo": 0, "Mono": 1, "Mono/Stereo": 2}[comp]]
    if settings.get("dual"): args.append("dual=1")
    for t, k, v in sorted(changes, key=lambda c: c[0]):
        st.set(k, v)
        a = our_args(k, v)
        if k in ("sync", "note", "note_r", "bpm", "delay", "delay_r", "rate"):
            tl, tr = st.times(); a = ["time=%.17g" % tl, "time_r=%.17g" % tr, "rate=%.9f" % st.rate()]
        args += ["at=%.9f:%s" % (t, s) for s in a]
    fi, fo = os.path.join(TMP, "din%d.f32" % os.getpid()), os.path.join(TMP, "dout%d.f32" % os.getpid())
    x.T.astype(np.float32).tofile(fi)
    cmd = [EXE, fi, fo, "2", str(fs), "preroll=%g" % preroll, "block=%d" % block] + args + EXTRA
    subprocess.run(cmd, check=True, capture_output=True)
    return np.fromfile(fo, np.float32).reshape(-1, 2).T


def theirs(x, fs=48000, **kw):
    return H.render(x, fs, **kw)


BAND = None                                              # Hz: the residual below this frequency only (--band)


def residual_db(a, b, seg=None, fs=None):
    seg = seg or slice(0, a.shape[-1])
    a, b = a[..., seg], b[..., seg]
    if BAND and fs and BAND < fs / 2:                    # brick-wall: what is heard of a 96 kHz render
        def lp(v):
            V = np.fft.rfft(v, axis=-1); V[..., np.fft.rfftfreq(v.shape[-1], 1.0 / fs) >= BAND] = 0
            return np.fft.irfft(V, v.shape[-1], axis=-1)
        a, b = lp(a), lp(b)
    e = a - b
    return 20 * np.log10(np.sqrt((e ** 2).mean()) / max(np.sqrt((b ** 2).mean()), 1e-20) + 1e-20)


CASES = [  # (name, settings); times are set inside the pre-roll, after Sync (a time set with Sync is not taken)
    ("default (Host 1/8D, Feedback 60)", dict(analog="Off")),
    ("ms 100, Feedback 0", dict(analog="Off", feedback=0, changes=[(-2.5, "sync", "ms"), (-2.45, "delay", 100)])),
    ("ms 250, Feedback 80", dict(analog="Off", feedback=80, changes=[(-2.5, "sync", "ms"), (-2.45, "delay", 250)])),
    ("Host 1/4 at 97 BPM, Mix 50", dict(analog="Off", mix=50, note="1/4", host_bpm=97, preroll=4.0)),
    ("ms 40, Feedback 150", dict(analog="Off", feedback=150, changes=[(-2.5, "sync", "ms"), (-2.45, "delay", 40)])),
    ("ms 120, HiPass 300, LoPass 4000", dict(analog="Off", hipass=300, lopass=4000, changes=[(-2.5, "sync", "ms"), (-2.45, "delay", 120)])),
    ("ms 120, LoFi", dict(analog="Off", lofi="LoFi", changes=[(-2.5, "sync", "ms"), (-2.45, "delay", 120)])),
    ("Ping Pong, ms 150, Feedback 70", dict(analog="Off", pingpong="PingPong", feedback=70, changes=[(-2.5, "sync", "ms"), (-2.45, "delay", 150)])),
    ("Ping Pong right, Phase R", dict(analog="Off", pingpong="PingPong", start="Right", phase_r="-")),
    ("Depth 40, Rate 0.8 Hz, ms 30", dict(analog="Off", depth=40, rate=0.8, changes=[(-2.5, "sync", "ms"), (-2.45, "delay", 30)])),
    ("glide: ms 375 -> 120 in the input", dict(analog="Off", changes=[(-2.5, "sync", "ms"), (-2.45, "delay", 375), (1.0, "delay", 120)])),
    ("Output -6, Mix 70, Analog 4", dict(analog="Mod4", output=-6, mix=70)),
    ("Analog 1 (noise -103 dBFS)", dict(analog="Mod1")),
    ("Analog 2, the default (noise -86 dBFS)", dict(analog="Mod2")),
    ("Analog 3 (noise -106 dBFS)", dict(analog="Mod3")),
    ("Feedback 200, ms 60, LoPass 3000", dict(analog="Off", feedback=200, lopass=3000, changes=[(-2.5, "sync", "ms"), (-2.45, "delay", 60)])),
]


def signals(fs, quick=False):
    S = NC.signals(fs, quick)
    keep = ["drum loop", "bass line"] + [k for k in S if "voice" in k][:1]
    return {k: S[k] for k in keep if k in S}


def main():
    if '--report' in sys.argv:
        import report_lib as RL
        print('wrote', os.path.relpath(RL.write('FwDelay')))
        return
    if '--vst3' in sys.argv:
        sys.exit(0 if vst3_check() is not False else 1)
    a = sys.argv[1:]
    quick = "--quick" in a
    fs = int(a[a.index("--fs") + 1]) if "--fs" in a else 48000
    save = "--save" in a
    global BAND
    BAND = float(a[a.index("--band") + 1]) if "--band" in a else None
    S = signals(fs, quick)
    lines = ["| case | " + " | ".join(S) + " |", "|---|" + "---|" * len(S)]
    for name, kw in (CASES[:4] if quick else CASES):
        row = []
        for sname, x in S.items():
            x2 = np.stack([x, x * 0.8]) if "Ping" not in name else np.stack([x, 0.5 * x])
            kw2 = dict(kw); kw2.setdefault("preroll", 3.0)
            # Feedback 0 through the pre-roll (the reference's first block, at the default Analog Mod2, puts noise in its
            # delay line, which Feedback would keep), the case's Feedback from the input's start
            kw2["changes"] = list(kw2.get("changes", ())) + [(0.0, "feedback", kw2.pop("feedback", 60))]
            kw2["feedback"] = 0
            r = theirs(x2, fs, **kw2); o = ours(x2, fs, **kw2)
            row.append("%.1f" % residual_db(o, r, fs=fs))
        lines.append("| %s | %s |" % (name, " | ".join(row)))
        print(lines[-1], flush=True)
    if save:
        import report_lib as RL
        print("wrote", RL.save("FwDelay", "settings" if fs == 48000 else "settings-96k", lines))


def vst3_check():
    """--vst3: our built VST3 against our renderer at the defaults, on noise, a low tone and drums"""
    import refhost as R, nulltest_lib as nl, de_ref as h
    S = {'noise -18 dBFS': nl.noise(3.0, -18.0, seed=7), 'tone 100 Hz -12 dBFS': nl.tone(100.0, 3.0, -12.0),
         'drums': nl.drums(3.0, -6.0)}
    return R.vst3_equals_renderer(h, S)


if __name__ == "__main__":
    main()
