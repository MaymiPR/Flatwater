# Flatwater Bass Leveler: null test against the reference

Residuals of Flatwater Bass Leveler's output against the reference's, over the loud parts, in dB (lower is better). The plug-in
runs through its own processor offline (`BlRender`; the built VST3 gives the same samples); the reference
through its VST3 at 48 kHz unless stated. How the model was found: [MEASUREMENTS.md](MEASUREMENTS.md).

## Settings

### Spill, the Artifacts gate, the weighting and the release depths

What changed (MEASUREMENTS.md, sections 1 and 3): Spill acts on the Live components; a note starts
over the Artifacts factor times (the long reference + Spill), summed as amplitudes (Studio: Spill fixed at -40); the
weighting is two first-order -15 dB shelves at 30 Hz; the release depths are Slow 12 / Fast 4 dB; the rest refitted.
Before -> now:

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

Held-out real bass parts (12), median per setting:

| setting | defaults | Fast | Sens. 0 | Sens. 100 | Fast, Sens. 25 | Fast, Sens. 100 | Sens. 75 | Artifacts 15 | Artifacts 85 | Target -24 |
|---|---|---|---|---|---|---|---|---|---|---|
| before | -24.7 | -24.0 | -24.4 | -22.9 | -19.6 | -23.2 | -21.6 | -24.7 | -21.2 | -23.2 |
| now | -25.6 | -23.7 | -25.1 | -22.4 | -24.2 | -22.9 | -21.9 | -25.6 | -25.1 | -23.0 |

Synthetic part 6 turns on one note (2.38 s, 13 dB over a quiet passage): it clears the old gate by 0.04 dB and misses
the additive one by 0.2 dB, after which the two hold apart; the gate's form is the reference's (measured from
silence at Artifacts 0 and 50 to 0.1 dB), the long reference R's exact fall is not. The tables below predate this
change.

### The fixed cases (`plugins/FwBassLeveler/nulltest/nulltest.py`)

| case | residual (dB) | gain error median (dB) | 90 % (dB) |
|---|---|---|---|
| steady 80 Hz -20 dBFS | -41.5 | 0.074 | 0.074 |
| steps 1 kHz | -48.6 | 0.029 | 0.030 |
| bass line | -30.2 | 0.073 | 0.160 |
| bass line, Target -24 | -30.2 | 0.103 | 0.200 |
| bass line, Sensitivity 80 | -28.1 | 0.196 | 0.622 |
| bass line, Artifacts 20 | -30.3 | 0.103 | 0.185 |
| synthetic part 6 (ghost notes, gaps) | -34.5 | 0.097 | 0.388 |
| synthetic part 7, Fast | -18.8 | 0.123 | 4.430 |
| synthetic part 8, range -6 .. +6 | -34.5 | 0.050 | 0.347 |

### The bass line at every Sensitivity x Response

| Sensitivity | 0 | 25 | 50 | 75 | 100 |
|---|---|---|---|---|---|
| Slow | -30.2 | -30.6 | -30.2 | -28.0 | -25.3 |
| Fast | -30.3 | -30.5 | -29.3 | -28.0 | -26.2 |

Artifacts on the bass line: 0 / 30 / 50 -30.3; 68.2 -24.8; 100 -37.6 (Slow).

### A corpus of real bass parts (`plugins/FwBassLeveler/nulltest/compare_corpus.py`)

36 twelve-second parts from FL Studio's own bass samples (acoustic, 80s pick, Ibanez, slap, Temptated, a picked
5-string, a pluck) and three synthetic voices (808 sub, filtered saw, slow swells): rhythms, accents, ghost notes,
rests, staccato, legato and ringing notes. 24 parts were used in the fit, 12 are held out. Median residual:

| setting | held-out parts | fitted parts |
|---|---|---|
| defaults | -24.7 | -23.6 |
| Sensitivity 0 | -24.4 | -23.9 |
| Sensitivity 75 | -21.6 | -21.4 |
| Sensitivity 100 | -22.9 | -18.3 |
| Fast | -24.0 | -19.6 |
| Fast, Sensitivity 25 | -19.6 | -19.4 |
| Fast, Sensitivity 100 | -23.2 | -18.5 |
| Artifacts 15 | -24.7 | -23.6 |
| Artifacts 85 | -21.2 | -20.5 |
| Target -24 | -23.2 | -24.1 |

The worst parts are -10 .. -17 dB. Nearly all of the remaining error is a note ridden by one side and not the
other (then the two hold 1 .. 4 dB apart until the next note), not a different gain law.

### Sample rates, components, stereo (`plugins/FwBassLeveler/nulltest/nulltest.py --vst3`)

| case | residual (dB) |
|---|---|
| bass line at 44.1 kHz | -30.8 |
| bass line at 96 kHz | -30.4 |
| Live Mono (5 ms look-ahead), bass line | -26.2 |
| Live Mono, Fast | -25.7 |
| Stereo, two different bass parts on L / R (one gain, from the mid) | -27.0 |
| Live Stereo, the same | -26.8 |

### Factory presets

The reference's 17, set on both: median -24.8 dB, see [plugins/FwBassLeveler/the Presets section of NULLTESTS.md](the Presets section of NULLTESTS.md).

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
