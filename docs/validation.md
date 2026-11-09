# Validation

The current validation state of every Flatwater plug-in. The null tests against the originals are in
[results.md](results.md) and each plug-in's NULLTESTS.md.

## State

| Plug-in | CHECK sites | ctest | pluginval 10 | FocusCheck | VST3 equals renderer (48 / 96 kHz) | Render regression cases | CPU per 60 s of stereo at 48 kHz |
|---|---:|---|---|---|---|---:|---|
| Flatwater Vocal Comp | 39 | pass | pass | pass | not recorded | 4 | about 2.1 s (3.5 % of one core, the learned slow path on) |
| Flatwater Bass Lift | 28 | pass | pass | pass | not recorded | 3 | the learned block about 3.6 s (6 %) |
| Flatwater BassX Lift | 26 | pass | pass | pass | not recorded | 4 | the learned block about 4.2 s (7 %) |
| Flatwater Comp | 66 | pass | pass | pass | identical (worst -85.8 dB over 30 cases) | 5 | 1.99 s (1.39 s without the learned correction) |
| Flatwater Delay | 69 | pass | pass | pass | identical (-400 dB) / identical | 3 | about 0.96 s (1.6 %, Feedback 60, Analog 2) |
| Flatwater Bass Leveler | 69 | pass | pass | pass | identical / identical | 3 | about 0.42 s (0.7 %) |
| Flatwater Vocal Leveler | 65 | pass | pass | pass | identical / identical | 3 | about 0.36 s (0.6 %) |
| Flatwater Vocal Chain | 37 | pass | pass | pass | not recorded | 19 | Compress alone 3.75 s with its two nets per colour (1.81 s without) |
| Flatwater Pitch Bend | 118 | pass | pass | pass | identical (-282 dB) / not recorded | 20 | about 0.6 s (1 %) |

The command behind each column:

| Column | Command |
|---|---|
| CHECK sites | `(Select-String -Path tests\<Stem>Tests.cpp -Pattern '^\s*CHECK(_NEAR)?\s*\(').Count` (each site may run many checks: the test prints its run count) |
| ctest | `ctest --test-dir build -C Release --output-on-failure` |
| pluginval 10 | pluginval 1.0.4 with `--strictness-level 10 --skip-gui-tests --timeout-ms 900000 --validate <Plug-in Name>.vst3`, as the release workflow runs it (on Windows through `Start-Process -Wait -PassThru` for its exit code) |
| FocusCheck | `FocusCheck <path to .vst3> [x,y ...]` (`tools/FocusCheck.cpp`, Windows): the space bar reaches the host after clicks into the editor |
| VST3 equals renderer | the built VST3 hosted by `tools/RefRender` (or pedalboard) against the plug-in's offline renderer: `plugins/FwBassLeveler/nulltest/check_vst3.py`, `plugins/FwVocalLeveler/nulltest/check_vst3.py`, `plugins/FwComp/nulltest/check_vst3.py`, `plugins/FwVocalComp/nulltest/check_vst3.py` (Flatwater Vocal Comp), `plugins/FwPitchBend/nulltest/first_null.py` |
| Render regression cases | `python scripts/render_regress.py --build build` over `tests/regress/<PluginTarget>/jobs.txt` |
| CPU | the offline renderer on 60 s of stereo at 48 kHz; percentages of one core x 60 s |

## Ports and learned blocks

| Plug-in | Check | Result |
|---|---|---|
| Flatwater Vocal Leveler | the C++ ride against the model (`plugins/FwVocalLeveler/nulltest/check_port.py`, 25 cases) | identical (0.0000 dB) |
| Flatwater Bass Leveler | the C++ ride against the fitted model (`plugins/FwBassLeveler/nulltest/check_port.py`, 7 cases) | within 0.09 dB at 99 % |
| Flatwater Comp | the Analog stage, C++ against the fitted model | the difference is the added noise alone (-82.9 ... -109.3 dBFS) |
| Flatwater Comp | the learned correction on 200 unseen cases (random settings and signals, `plugins/FwComp/nulltest/train_net.py` TEST) | the weak cases (worse than -35 dB without it) -25.8 to -36.6 dB, all -48.3 to -52.6; 106 better, 15 worse by more than 1 dB (the worst -96.3 to -73.4) |
| Flatwater Comp | sample rates: the quick null test at 44.1 / 48 / 96 kHz (5 settings x 7 signals) | median -43.1 / -42.9 / -41.8 dB with the learned correction (-40.4 / -40.2 / -38.6 without); at 96 kHz the correction improves 14 cases by more than 1 dB and worsens none |
| Flatwater Vocal Comp | the learned slow path, C++ against PyTorch on the null-test signals | -70 dB or lower |
| Flatwater Vocal Comp | the learned slow path at 44.1 / 96 kHz (5 signals x 3 settings) | the same gains as at 48 kHz; steady tones unchanged |
| Flatwater BassX Lift | the learned level control, C++ against PyTorch | -71 ... -117 dB on most examples (-43 dB on a kick) |
| Flatwater BassX Lift | null at 96 kHz against 48 kHz (24 cases) | within 1.8 dB |
| Flatwater Bass Lift | the learned gain block, C++ against PyTorch on held-out renders | -74 ... -91 dB |
| Flatwater Bass Lift | null at 44.1 / 96 kHz against 48 kHz (5 signals) | within 0.2 dB |
| Flatwater Vocal Chain | the Compress nets, C++ against PyTorch (`plugins/FwVocalChain/nulltest/check_compress_net.py`) | -107 ... -133 dB |

## Editors against the originals

| Plug-in | Check | Result |
|---|---|---|
| Flatwater Comp | knob feel (`tools/MeterCapture` drag and wheel scripts on every knob, plain and Ctrl, mouse steps 1 ... 30 px) | the reference's values on every script (the wheel within the display's last digit) |
| Flatwater Comp | VU needle (both filmed with MeterCapture with the same probe: the reference from its OpenGL frames, ours by screen grab; threshold -30, ratio 4) | GR mean reading drums 11.5 / 11.5 dB, voice 5.6 / 5.9, tones and bursts 5.3 / 5.4; In: mean frame within 1.4 of 70, 90 % of frames within 1.5 steps; the laws alone against the films: 97 % (GR) and 97.5 % (In / Out) of frames within one step |
| Flatwater Vocal Comp | meters (`tools/MeterCapture`, bass line and kicks, Compression -12 dB) | Energy, Output and Gain reduction within 2.0 / 2.3 / 2.0 px on 90 % of frames |
| Flatwater BassX Lift | value entry (MeterCapture double-click and type on a knob, a fader and Freq; single click on the value; Ctrl- and Alt-click) | the same parameter values after every step; entry boxes match to the pixel |
| Flatwater BassX Lift | output meters (MeterCapture, -6 / -20 dBFS bursts) | bar height within 1 px on 88 % of frames, 3 px on 93 %; steady levels and the silence dot exact |
| Flatwater BassX Lift | factory presets (22): 16 converted by the reference itself, the other 6 by the same scales, which reproduce all 16 exactly; Input untouched on preset load | unit tests pass |
| Flatwater Bass Lift | meters (`tools/MeterCapture`, bass line and kicks) | Original Bass within 3.5 px (0.7 dB) on 90 % of frames; output and harmonics within 8 px, the rest being the generated level |
| Flatwater Vocal Chain | the editor's marks, value box and values against the reference's (MeterCapture click, hover and drag scripts) | frame for frame |

## Listening tests

| Plug-in | Result |
|---|---|
| Flatwater Bass Leveler | good in FL Studio |
| Flatwater Vocal Leveler | the Music side-chain is untested: the reference uses it in no offline host; it needs FL Studio or another host with a real side-chain send |

## What the unit tests cover

| Plug-in | Coverage |
|---|---|
| Flatwater Vocal Comp | latency, static law, gate, gate and compressor together, attack and release, finite output on extreme input, block-size invariance, stereo link, processor parameters, state round trip, Mono mode, factory presets, the learned slow path leaving steady tones and Compression 0 exactly as without it |
| Flatwater Bass Lift | original path In and Out, latency 0, Intensity exact on the generated part, low-level generated gain, Decay switching the harmonic product, output clip, Gain trim, block-size invariance, silence, stereo, processor parameters and state round trip |
| Flatwater BassX Lift | the identified structure against the reference's own measurements (generated part at rest, 4 settings, 48 / 44.1 / 96 kHz, within 0.15 dB; Decay feedback h2 / h3 within 0.3 / 0.6 dB), original path and its all-pass, Original Bass monitor pre-fader, Input trim stopping at 0 dB, Audio = original + Harmonics x G, Harmonics capped at +10.96 dB, Decay on the even harmonics, High-pass on the generated part, output clip, block-size invariance, channels independent, 96 kHz, processor and state round trip |
| Flatwater Comp | the learned correction leaving steady high tones within 0.1 dB and moving a low note under a long attack toward the reference; the needle laws against filmed values; the 26 factory presets loading and leaving Mix alone; typed Threshold values read as negative; latency at 44.1 ... 192 kHz, idle identity, static curve and makeup, block-size invariance, stereo link, limiter Off / On ceiling, Mix 0, tempo sync and the long-note fallback, Analog Off identity, Analog noise levels and independent channels, Analog 4 crossover offset; the processor ignoring the side-chain, state round trip, Mode Mono |
| Flatwater Delay | a preset's exact values reaching the DSP until the parameter is moved and surviving a project reload; the parameter texts and tapers, Rate at its shown resolution, the display's note and Dual / Ping Pong layouts and clicks, the DSP laws (interpolator, glide, gain stage, LFO, filters, Ping Pong, Phase), an impulse through the processor at the default 1/8D, the 88 factory presets loading and leaving Mix alone, state round trip |
| Flatwater Bass Leveler | the parameters as the reference's (ranges, defaults, texts, the Idle arrow's place in the range), the components' latency (42 / 5 ms), the audio path, the ride against the reference's measured law, look-ahead, hold, glide and stereo mid, the Idle value before and after notes, the state, the editor and its selection, the 17 factory presets and Target left alone |
| Flatwater Vocal Leveler | the parameters as the reference's, the buses (a side-chain), no latency, the audio path, Automation Read and Write, the editor's Studio and Live layouts and the Music knob's disabled look, the 13 factory presets (their values, Attack, Automation Off in Default, an Idle of 0 or 1 kept exactly), and the riding against the reference's measured values: the start, the law, the rise after 12 and 4 dB drops, the hold and glide to Idle at Slow and Fast, Spill on Live and off at -48 |
