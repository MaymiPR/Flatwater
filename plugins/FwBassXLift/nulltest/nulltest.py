"""Null test: Flatwater BassX Lift (offline renderer of the plug-in DSP) against the reference's render, residual in dB.

    python -I nulltest.py [--quick] [--save | --from-json] [--fs 48000]

For every signal x setting both render the same stereo input. The residual is
20 log10( rms(ours - reference) / rms(reference) ) after the first 0.2 s, for the whole output (Monitor: Audio) and
for the generated part alone (Monitor: Harmonics, which plays it pre-fader on both). Numbers are reported as they
come: no case is labelled a null unless the data shows it.
    python -I nulltest.py --report     NULLTESTS.md again from measurements/FwBassXLift/nulltests.json, rendering nothing
    python -I nulltest.py --vst3       our built VST3 against our renderer (the check before a number is published)
"""
import sys, os, subprocess, json
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'FwBassLift', 'nulltest'))   # plugins/FwBassLift/nulltest/
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'scripts', 'nulltest'))   # scripts/nulltest/
import numpy as np
import refhost as R
import bx_ref as BX
import bf_ref as NW

HERE = os.path.dirname(os.path.abspath(__file__))
EXE = os.environ.get("FWBASSXLIFT_RENDER", os.path.join(HERE, "..", "..", "..", "build", "BxRender_artefacts", "Release", "BxRender.exe"))
TMP = os.path.join(HERE, "renders"); os.makedirs(TMP, exist_ok=True)
NINF = float("-inf")

# (Freq, Ratio, Response, Decay, High-pass 0/1/2, Original Bass dB, Harmonics dB)
SETTINGS = [(80, 1.0, 20.0, -15.0, 1, 0.0, 0.0), (80, 4.0, 20.0, -15.0, 1, NINF, 0.0), (80, 2.0, 10.0, -9.0, 2, NINF, 6.0),
            (50, 1.0, 30.0, -24.0, 0, 0.0, -6.0), (160, 1.41, 20.0, -15.0, 1, 0.0, 0.0), (256, 4.0, 15.0, -12.0, 1, -6.0, 3.0),
            (32, 1.0, 20.0, -15.0, 1, 0.0, 12.0), (120, 2.5, 25.0, -18.0, 2, NINF, 0.0)]


def ours(x, fs, F, ratio, resp, decay, hp, orig, harm, monitor):
    x = np.atleast_2d(np.asarray(x, np.float32)); ch = x.shape[0]
    fi, fo = os.path.join(TMP, "xin%d.f32" % os.getpid()), os.path.join(TMP, "xout%d.f32" % os.getpid())
    x.T.astype(np.float32).tofile(fi)
    db = lambda v: "-200" if v == NINF else "%g" % v
    args = [EXE, fi, fo, str(ch), str(fs), "freq=%g" % F, "ratio=%g" % ratio, "response=%g" % resp, "decay=%g" % decay,
            "highpass=%d" % hp, "orig=" + db(orig), "harm=" + db(harm), "monitor=%d" % monitor]
    subprocess.run(args, check=True, capture_output=True)
    return np.fromfile(fo, np.float32).reshape(-1, ch).T


def theirs(x, fs, F, ratio, resp, decay, hp, orig, harm, monitor):
    return BX.render_ref(x, fs, frequency=F, ratio=ratio, response=resp, harmonics_decay=decay, highpass=hp,
                       orig_bass_gain=orig, harmonics_gain=harm, monitor=monitor)

DATA = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "measurements", "FwBassXLift", "nulltests-settings.json")


def save_part(rows):
    """the Settings part of NULLTESTS.md and its rows (measurements/FwBassXLift/nulltests-settings.json)"""
    tots = np.array([r["total_db"] for r in rows]); gens = np.array([r["generated_db"] for r in rows])
    import report_lib as RL
    with RL.Part("FwBassXLift", "settings") as f:
        f.write("| Freq | Ratio | Response | Decay | High-pass | Original Bass | Harmonics | Signal | Total residual (dB) | Generated part (dB) |\n"
                "|---|---|---|---|---|---|---|---|---|---|\n")
        for r in rows:
            f.write("| %d | %.2f | %.0f | %+.0f | %s | %s | %+.0f | %s | %.1f | %.1f |\n" % (
                r["freq"], r["ratio"], r["response"], r["decay"], ("6", "12", "24")[r["highpass"]],
                "-inf" if r["orig"] == NINF else "%+.0f" % r["orig"], r["harm"], r["signal"], r["total_db"], r["generated_db"]))
        f.write("\nMedian total %.1f dB (worst %.1f); median generated part %.1f dB (worst %.1f); %d cases.\n"
                % (np.median(tots), tots.max(), np.median(gens), gens.max(), len(rows)))
    json.dump(rows, open(DATA, "w", newline="\n"), indent=1, default=str)



def main():
    if '--report' in sys.argv:
        import report_lib as RL
        print('wrote', os.path.relpath(RL.write('FwBassXLift')))
        return
    if '--vst3' in sys.argv:
        sys.exit(0 if vst3_check() is not False else 1)
    args = sys.argv[1:]
    quick = "--quick" in args
    save = "--save" in args
    if "--from-json" in args:
        return save_part(json.load(open(DATA)))
    fs = int(args[args.index("--fs") + 1]) if "--fs" in args else 48000
    S = NW.signals(fs, quick=quick)
    settings = SETTINGS[:4] if quick else SETTINGS
    rows = []
    for st in settings:
        for name, x in S.items():
            xs = np.stack([x, x]).astype(np.float32)
            skip = int(0.2 * fs)
            tot = NW.residual_db(ours(xs, fs, *st, 0), theirs(xs, fs, *st, 0), skip)
            gen = NW.residual_db(ours(xs, fs, *st, 1), theirs(xs, fs, *st, 1), skip)
            F, ratio, resp, decay, hp, orig, harm = st
            rows.append(dict(freq=F, ratio=ratio, response=resp, decay=decay, highpass=hp, orig=orig, harm=harm, signal=name,
                             total_db=round(tot, 1), generated_db=round(gen, 1)))
            print("F %3d R %.2f Resp %2.0f Dec %+3.0f HP %d Orig %5s Harm %+4.0f  %-26s total %7.1f dB  generated %7.1f dB"
                  % (F, ratio, resp, decay, hp, "-inf" if orig == NINF else "%+.0f" % orig, harm, name, tot, gen), flush=True)
    tots = np.array([r["total_db"] for r in rows]); gens = np.array([r["generated_db"] for r in rows])
    print("\nmedian total %.1f dB (worst %.1f), median generated %.1f dB (worst %.1f), %d cases"
          % (np.median(tots), tots.max(), np.median(gens), gens.max(), len(rows)))
    if save:
        save_part(rows)


def vst3_check():
    """--vst3: our built VST3 against our renderer at the defaults, on noise, a low tone and drums"""
    import refhost as R, nulltest_lib as nl, bx_ref as h
    S = {'noise -18 dBFS': nl.noise(3.0, -18.0, seed=7), 'tone 100 Hz -12 dBFS': nl.tone(100.0, 3.0, -12.0),
         'drums': nl.drums(3.0, -6.0)}
    return R.vst3_equals_renderer(h, S)


if __name__ == "__main__":
    main()
