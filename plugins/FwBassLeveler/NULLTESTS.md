# Flatwater Bass Leveler: null test against the reference

Residuals of Flatwater Bass Leveler's output against the reference's, over the loud parts, in dB (lower is better). The plug-in
runs through its own processor offline (`BlRender`; the built VST3 gives the same samples); the reference
through its VST3 at 48 kHz unless stated. How the model was found: [MEASUREMENTS.md](MEASUREMENTS.md).

## Settings

### Spill, the Artifacts gate, the weighting and the release depths

What changed (MEASUREMENTS.md, sections 1 and 3): Spill acts on the Live components; a note starts
over the Artifacts factor times (the long reference + Spill), summed as amplitudes (Studio: Spill fixed at -40); the
weighting is two first-order -15 dB shelves at 30 Hz; the release depths are Slow 12 / Fast 4 dB; the rest refitted.
Before -> now, as measured when the change was made, before the 1.0.0 release and not reproduced by a script in this tree (the scripted tables below hold the current numbers):

| case | before (dB) | now (dB) |
|---|---|---|
| steady 80 Hz -20 dBFS | -41.5 | -48.9 |
| steps 1 kHz | -48.6 | -48.3 |
| bass line | -30.2 | -29.8 |
| bass line, Target -24 / Sensitivity 80 / Artifacts 20 | -30.2 / -28.1 / -30.3 | -29.7 / -28.2 / -29.9 |
| synthetic part 6 (ghost notes, gaps) | -34.5 | -13.5 |
| synthetic part 7, Fast | -18.8 | -18.8 |
| synthetic part 8, range -6 .. +6 | -34.5 | -28.2 |
| Live Mono, bass line / Fast | -26.2 / -25.7 | -26.0 / -24.3 |
| Live Mono, bass line, Spill -25 | -17.9 | -21.2 |
| Live Mono, bass line, Spill -15 | -14.3 | -24.2 |
| Stereo (different bass on L / R) / Live Stereo | -27.0 / -26.8 | -27.7 / -26.8 |
| bass line at 44.1 / 96 kHz | -30.8 / -30.4 | -30.2 / -30.0 |
| factory presets, median | -24.8 | -25.2 |

Held-out real bass parts (12), median per setting (the same comparison, measured before the 1.0.0 release and not reproduced by a script in this tree):

| setting | defaults | Fast | Sens. 0 | Sens. 100 | Fast, Sens. 25 | Fast, Sens. 100 | Sens. 75 | Artifacts 15 | Artifacts 85 | Target -24 |
|---|---|---|---|---|---|---|---|---|---|---|
| before | -24.7 | -24.0 | -24.4 | -22.9 | -19.6 | -23.2 | -21.6 | -24.7 | -21.2 | -23.2 |
| now | -25.6 | -23.7 | -25.1 | -22.4 | -24.2 | -22.9 | -21.9 | -25.6 | -25.1 | -23.0 |

Synthetic part 6 turns on one note (2.38 s, 13 dB over a quiet passage): it clears the old gate by 0.04 dB and misses the additive one by 0.2 dB, after which the two hold apart; the gate's form is the reference's (measured from silence at Artifacts 0 and 50 to 0.1 dB), the long reference R's exact fall is not. The tables below are written by their scripts from the current source.

### The fixed cases (`plugins/FwBassLeveler/nulltest/nulltest.py`)

| case | residual (dB) | gain error median (dB) | 90 % (dB) |
|---|---|---|---|
| steady 80 Hz -20 dBFS | -48.9 | 0.031 | 0.031 |
| steps 1 kHz | -48.3 | 0.031 | 0.031 |
| bass line | -29.8 | 0.110 | 0.207 |
| synthetic part 6 | -13.5 | 0.163 | 6.354 |
| synthetic part 7, Fast | -18.8 | 0.123 | 4.459 |
| bass line, Target -24 | -29.7 | 0.130 | 0.266 |
| bass line, Sensitivity 80 | -28.2 | 0.176 | 0.675 |
| bass line, Artifacts 20 | -29.9 | 0.130 | 0.217 |
| synthetic part 8, range -6 .. +6 | -28.2 | 0.042 | 2.457 |

Reproduce: `python -I plugins/FwBassLeveler/nulltest/nulltest.py --save`

### The bass line at every Sensitivity x Response

The bass line of the fixed cases, the residual as there (dB). Produced by `plugins/FwBassLeveler/nulltest/nulltest.py --grid`.

| Sensitivity | 0 | 25 | 50 | 75 | 100 |
|---|---|---|---|---|---|
| Slow | -30.5 | -30.5 | -29.8 | -28.7 | -25.5 |
| Fast | -30.5 | -30.3 | -26.1 | -26.1 | -25.1 |

| Artifacts (Slow, Sensitivity 50) | 0 | 30 | 50 | 68.2 | 100 |
|---|---|---|---|---|---|
| residual (dB) | -29.9 | -29.8 | -29.8 | -24.6 | -35.8 |

Reproduce: `python -I plugins/FwBassLeveler/nulltest/nulltest.py --grid --save`

### A corpus of real bass parts (`plugins/FwBassLeveler/nulltest/compare_corpus.py`)

36 twelve-second parts from FL Studio's own bass samples (acoustic, 80s pick, Ibanez, slap, Temptated, a picked 5-string, a pluck) and three synthetic voices (808 sub, filtered saw, slow swells): rhythms, accents, ghost notes, rests, staccato, legato and ringing notes. 24 parts were used in the fit, 12 are held out. The riding model (`native/RideModel.c` with the current fit, the plug-in's ride: `check_port.py`) against the reference's gain on each part (`render_corpus.py`); the residual over the loud parts, the median per setting (dB). Produced by `plugins/FwBassLeveler/nulltest/compare_corpus.py`.

| setting | held-out parts | fitted parts |
|---|---|---|
| defaults | -25.6 | -25.9 |
| Sensitivity 0 | -25.1 | -23.4 |
| Sensitivity 75 | -21.9 | -21.2 |
| Sensitivity 100 | -22.4 | -20.0 |
| Fast | -23.7 | -20.5 |
| Fast, Sensitivity 25 | -24.2 | -20.7 |
| Fast, Sensitivity 100 | -22.9 | -19.7 |
| Artifacts 15 | -25.6 | -25.1 |
| Artifacts 85 | -25.1 | -21.6 |
| Target -24 | -23.0 | -24.8 |

Over the 120 held-out renders: median -23.8 dB, worst -10.0 dB, 34 above -20 dB. Over the 240 fitted renders: median -22.0 dB, worst -6.2 dB, 93 above -20 dB. Nearly all of the remaining error is a note ridden by one side and not the other (then the two hold 1 .. 4 dB apart until the next note: `--events` lists them), not a different gain law.

Reproduce: `python -I plugins/FwBassLeveler/nulltest/compare_corpus.py --save`

### Sample rates, components, stereo (`plugins/FwBassLeveler/nulltest/nulltest.py --checks`)

The residual over the loud parts (dB); the bass line of the fixed cases, resampled for the other rates. Produced by `plugins/FwBassLeveler/nulltest/nulltest.py --checks`.

| case | residual (dB) |
|---|---|
| bass line at 44.1 kHz | -30.2 |
| bass line at 96 kHz | -30.0 |
| Live Mono, bass line | -26.0 |
| Live Mono, bass line, Fast | -24.3 |
| Live Mono, bass line, Spill -25 | -21.2 |
| Live Mono, bass line, Spill -15 | -24.2 |
| Stereo, two different bass parts (L / R) | -27.7 |
| Live Stereo, the same | -26.8 |

Reproduce: `python -I plugins/FwBassLeveler/nulltest/nulltest.py --checks --save`

### Factory presets

The reference's 17, set on both, on 3 signals: median -25.2 dB, worst -9.0 dB over the 51 renders. Every preset: [Presets](#presets).

Reproduce: `python -I plugins/FwBassLeveler/nulltest/null_presets.py`

## Presets

Each of the reference's 17 presets set on both plug-ins (Flatwater names; Target at its default where the preset
leaves it), output residual over the loud parts in dB (lower is better). Generated by
`plugins/FwBassLeveler/nulltest/null_presets.py`.

| preset | bass line | corpus part 30 (picked) | corpus part 25 (slap) |
|---|---|---|---|
| Default | -29.8 | -22.6 | -26.7 |
| Live PA Start | -23.2 | -23.9 | -27.8 |
| Soft Touch | -29.6 | -33.3 | -59.0 |
| Fingerstyle Slow | -24.6 | -24.8 | -26.0 |
| Fretless Slow | -14.0 | -25.3 | -26.4 |
| Unplugged Bass Ease | -24.3 | -19.8 | -27.2 |
| Upright Bass | -35.8 | -25.1 | -11.1 |
| Fast Fingers | -35.2 | -16.5 | -33.1 |
| Wide Dynamics | -19.3 | -16.8 | -29.4 |
| Clean and Lively | -23.4 | -15.7 | -23.8 |
| Fast, Artifacts Ignored | -9.9 | -23.9 | -26.7 |
| Picked Bass Fast | -23.1 | -16.5 | -26.1 |
| Fast Eighths | -26.0 | -16.8 | -25.3 |
| Squeak Proof | -25.2 | -28.4 | -57.9 |
| Slap Through the Amp | -9.0 | -20.6 | -27.2 |
| Bass Tamer | -34.8 | -78.0 | -61.1 |
| Gig Night | -22.3 | -16.9 | -33.1 |

Median -25.2 dB, worst -9.0 dB.

Reproduce: `python -I plugins/FwBassLeveler/nulltest/null_presets.py`
