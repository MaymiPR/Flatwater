# Flatwater Bass Leveler: research

What the vendor documents for Bass Rider, the original Flatwater Bass Leveler recreates, and what the original shows in
a host: its behaviour, parameters, editor and presets. Nothing of the original is copied into this repository. The
measured behaviour is in [MEASUREMENTS.md](MEASUREMENTS.md).

## Sources

| Source | What it is |
|---|---|
| B1 | The public manual, [bass-rider.pdf](https://assets.wavescdn.com/pdf/plugins/bass-rider.pdf); the product page [waves.com/plugins/bass-rider](https://www.waves.com/plugins/bass-rider) |
| B2 | The original plug-in observed in a host: its parameters (ranges, steps, defaults) and its 17 factory presets, each loaded into the original and read back |
| B3 | The original's VST3 as a host sees it (`plugins/FwBassLeveler/nulltest/probe_parameters.py`): Bass Rider Mono, Bass Rider Stereo, Bass Rider Live Mono, Bass Rider Live Stereo; no side-chain bus |
| B4 | Frames of the real editor read back from its OpenGL drawing (`tools/MeterCapture gl=1`, 320 x 560, git-ignored `reference/fwbassleveler/`) |

## Documented behaviour (B1)

- Rides bass levels automatically, note by note: it detects each note's attack, sustain and release and sets a gain
  per note towards a target, leaving the note's own envelope alone (unlike a compressor).
- Target 0 ... -42 dBFS (default -14), a slider over the Target Energy Meter (the input's energy, 0 ... -42 dBFS); it
  also calibrates the Rider fader's 0. Meant to sit near the input's average level.
- Note detection: Sensitivity 0 ... 100 (default 50; higher detects more notes), Response Slow / Fast (default Slow;
  when the detector resets for the next note: Slow for long releases, Fast for short ones).
- Ignore: Spill -50 ... -3 dBFS (default -40; signal below it is not detected as notes), Artifacts 0 ... 100 (default
  50; tolerance to buzz, crackles, squeaks and modulation within a note that would trigger detection).
- Range: Max -11 ... +12 dB (default +12), Min -12 ... +11 dB (default -12), the Effective Range shown between them
  (1 ... 24 dB); the Idle arrow (-12 ... +12 dB, default 0) is the Rider fader's value when no bass plays.
- Rider fader -12 ... +12 dB: the automatic gain; it can be grabbed for manual gain, and riding resumes on release.
- Output -12 ... +12 dB (default 0) after the riding, with a clip LED.
- Components: Bass Rider (Mono, Stereo), latency 42 ms, described as round and transparent; Bass Rider Live (Mono,
  Stereo), latency 5 ms, described as adding a little punch to note starts.

## Specifications (B3, measured)

| Item | Value |
|---|---|
| Latency | reported and compensated: 42.00 ms (1852 samples at 44.1 kHz, 2016 at 48, 4032 at 96); Live 5.00 ms (221 / 240 / 480); with it removed the output lines up with the input (cross-correlation lag 0) |
| Static level at the defaults | an 80 Hz sine settles at -11.53 dBFS rms out for any input from -20 to -3 dBFS rms (gain +8.47 ... -8.53 dB), and at +12 dB gain (the Max range) for -30 and -40; so Target -14 aims at an energy measure that reads an 80 Hz sine 2.47 dB below its rms (the weighting and the envelope of MEASUREMENTS.md section 1) |
| VST3 parameters | 10 plus bypass: spill (471 steps of 0.1 dB), range_max, range_min, output (0.1 dB), sensitivity (1001 steps), target (421 steps of 0.1 dB), idle (shown in dB), response (Slow / Fast), artifacts (1001 steps); the Rider fader is not a host parameter (no ride automation out) |

## Parameters (B2, B3), in preset-array order

Flatwater Bass Leveler's parameters (`plugins/FwBassLeveler/Source/PluginProcessor.cpp`), in the order the original stores them in its
presets. The meters, the Rider fader, the Effective Range and the Idle value in dB are display only, not parameters.

| ID | Label | Range, step | Default | Notes |
|---|---|---|---|---|
| output | Output | -12 ... +12 dB, 0.1 | 0 | gain after the riding |
| rangeMin | Range Min | -12 ... +11 dB, 0.1 | -12 | the lowest gain the riding may set |
| rangeMax | Range Max | -11 ... +12 dB, 0.1 | +12 | the highest gain the riding may set |
| target | Target | -42 ... 0 dB, 0.1 | -14 | the level the riding aims at; left unchanged by all but two factory presets |
| sensitivity | Sensitivity | 0 ... 100, 0.1 | 50 | note detection |
| response | Response | Slow / Fast | Slow | when the detector resets for the next note |
| idle | Idle | 0.02 ... 0.98, 0.01 | 0.5 | the Idle arrow as its place in the range (0.5 is 0 dB at +-12); the host shows it in dB over a fixed +-12 dB |
| artifacts | Artifacts | 0 ... 100, 0.1 | 50 | shown 0 ... 100; inside, a -20 ... -3 dB note-on gate (50 is -11.5 dB) |
| spill | Spill | -50 ... -3 dB, 0.1 | -40 | a typed positive value is read as negative |
| mode | Mode | Stereo, Mono, Live Stereo, Live Mono | Stereo | the original's four components; Mono / Stereo by the bus layout, Studio / Live as this switch; Live looks ahead 5 ms instead of 42 |

Besides its controls, the original keeps fixed internal settings that sketch the design, shared by all presets: a
note detector on a low-passed (about 1 kHz) signal with note, slow and reference envelopes (attack / release pairs),
note-on and note-off gates and a sustain threshold; a weighted energy (high-pass 30 Hz at -30 dB, high shelf 5.1 kHz)
for the level; a gain smoother (attack 2 ms, release 100 ms); an idle park glide (271 ms); the 42 ms direct delay as
look-ahead.

## GUI geometry (B2, B4)

The view is 320 x 560 px including the two-row toolbar (its preset row 316 px wide, narrower than the suite's other
editors). The layout of every control was measured on the editor's frames (B4,
`reference/fwbassleveler/shot_default_running.png`):

- Dark top panel: the Target slider over the Energy meter; a Detect group with the Sensitivity knob (a dashed ring
  scale) and the Response Slow / Fast toggle; an Ignore group with the Spill and Artifacts knobs.
- Cream lower panel: the Range column (Max and Min thumbs, the Idle arrow marker, the Range value box), the Rider
  fader on a +-12 dB scale (6 dB marks, 1 dB ticks) with its value box, and the Output fader with its value box and a
  clip LED. Faders have shadows; the Range column shades the band between Min and Max, and the Idle value is drawn as
  a line across to the Rider scale. The Rider fader moves by itself and can be grabbed.
- Bottom: a bass clef and the product name in large type on dark leather.
- Text entry and Alt-reset on the knobs and faders; the fader text follows the thumb.
- The Live components differ in the editor only by a badge.

## Presets (B2)

17 presets (Full Reset, PA Default, and 15 for bass styles: slow / fast, fretless, upright, picked, slap, fret noise,
stage, a suppressor with Max 0). They change only Output, Range Min / Max, Sensitivity, Response, Idle, Artifacts and
Spill (and Target in the first two). Flatwater's presets carry them over by their values under Flatwater names; their
null test loads the reference's own preset states (`RefRender state=`, [NULLTESTS-presets.md](NULLTESTS-presets.md)).
