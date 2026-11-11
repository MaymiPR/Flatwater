"""Phase 1 probe of the reference (the shell VST3, the local install named by local.yaml's ref_vst3): the components the shell offers,
each one's VST3 parameters (name, default text, the values a host can set), the reported latency, the real delay of
the audio (cross-correlation of a bass-like signal in and out) and the gain it applies to a steady tone at a few
levels (the default Target -14).

    python -I probe_parameters.py
"""
import sys, os
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'scripts', 'nulltest'))   # scripts/nulltest/
import numpy as np
import pedalboard as pb
import refhost as R
import local_config_lib as L


def main():
    names = pb.VST3Plugin.get_plugin_names_for_file(R.REF_SHELL)
    br = [n for n in names if L.ref_base('FwBassLeveler') in n]
    print("the reference's components in the shell:", br)
    for name in br:
        R.fresh(name); p = R.load(name)
        print("\n==", name, "latency (reported)", getattr(p, "reported_latency_samples", "?"))
        for k, q in p.parameters.items():
            if q.string_value == "": continue                 # the shell's MIDI controller slots
            vals = getattr(q, "valid_values", None)
            vs = (" values %s" % (vals[:6] + (["..."] if len(vals) > 6 else []) if len(vals) <= 400 else "%d values" % len(vals))) if vals else ""
            print("  %-22s %-14s %s%s" % (k, repr(q.string_value), getattr(q, "label", ""), vs))
    fs = 48000
    rng = np.random.default_rng(3)
    t = np.arange(int(6 * fs)) / fs
    # a bass line: notes of 0.25 s at random pitches (40 .. 160 Hz) and levels (-30 .. -6 dBFS), with decays
    x = np.zeros_like(t)
    for k in range(24):
        a = 10 ** (rng.uniform(-30, -6) / 20); f = rng.uniform(40, 160); s = int(k * 0.25 * fs)
        n = np.arange(int(0.25 * fs)); x[s:s + len(n)] += a * np.sin(2 * np.pi * f * n / fs) * np.exp(-n / fs / 0.4)
    x = x.astype(np.float32)
    for name in br:
        R.fresh(name); p = R.load(name)
        for bs in (512,):
            xin = x[None] if "Mono" in name else np.stack([x, x])
            y = p(xin, fs, buffer_size=bs)[0]
            c = np.correlate(y[:int(4 * fs)], x[:int(4 * fs)], 'full')[len(x[:int(4 * fs)]) - 1:]
            lag = int(np.argmax(np.abs(c[:int(0.2 * fs)])))
            print("%-26s real delay %d samples (%.2f ms) at %d Hz" % (name, lag, lag / fs * 1000, fs))
    # the gain on steady 80 Hz tones (after 4 s), at the default settings
    name = [n for n in br if "Stereo" in n and "Live" not in n][0]
    for lev in (-40, -30, -20, -14, -8, -3):
        R.fresh(name); p = R.load(name)
        xs = (10 ** (lev / 20) * np.sqrt(2) * np.sin(2 * np.pi * 80 * t)).astype(np.float32)
        y = p(np.stack([xs, xs]), fs, buffer_size=512)[0]
        seg = slice(int(4.5 * fs), int(5.5 * fs))
        g = 20 * np.log10(np.sqrt(np.mean(y[seg] ** 2)) / np.sqrt(np.mean(xs[seg] ** 2)))
        print("80 Hz at %+d dBFS rms: gain %+.2f dB (ride %s)" % (lev, g, p.parameters['ride'].string_value if 'ride' in p.parameters else '?'))


if __name__ == "__main__":
    main()
