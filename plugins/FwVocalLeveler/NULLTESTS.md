# Flatwater Vocal Leveler: null test against the reference

Residuals of Flatwater Vocal Leveler's output against the reference's, after the first 0.1 s, in dB (lower is better),
and the error of the applied gain where the input is loud enough to read it. The plug-in runs through its own
processor offline (`VlRender`); the reference plug-in through its VST3 (a fresh instance per case). The vocal parts are held-out parts of the corpus (`scripts/nulltest/vocal_corpus_lib.py` 24 .. 35: FL Studio's own vocal
phrases strung into 20 s parts with wandering levels, gaps and room noise); they were not used to identify the model.
How the model was found: [MEASUREMENTS.md](MEASUREMENTS.md).

## Settings

### The fixed cases (`plugins/FwVocalLeveler/nulltest/nulltest.py`)

| case | residual (dB) | gain error median (dB) | 90 % (dB) |
|---|---|---|---|
| steps 1 kHz | -74.0 | 0.001 | 0.003 |
| syllable train | -50.8 | 0.025 | 0.033 |
| vocal part 24 | -64.2 | 0.002 | 0.009 |
| vocal part 25, Fast | -56.9 | 0.002 | 0.088 |
| vocal part 26, range -12 .. +12 | -62.0 | 0.003 | 0.008 |
| vocal part 27, Target -30 | -63.0 | 0.003 | 0.009 |
| vocal part 28, Vocal Sensitivity +6 | -65.6 | 0.002 | 0.007 |
| vocal part 29, range -3 .. +9, Idle 0.67 | -48.3 | 0.004 | 0.105 |
| vocal part 30, Live, Spill -30 | -60.4 | 0.003 | 0.017 |
| vocal part 31, Live Fast, Spill -36 | -46.6 | 0.004 | 0.101 |
| vocal part 32, stereo (R 4 dB lower) | -60.3 | 0.003 | 0.009 |
| vocal part 33, 44.1 kHz | -63.5 | 0.003 | 0.008 |
| vocal part 34, 96 kHz, Fast | -53.2 | 0.005 | 0.037 |

### The model over all cached renders (`plugins/FwVocalLeveler/nulltest/fit_model.py report`)

The same model (`plugins/FwVocalLeveler/nulltest/native/RideModel.c`, identical to the plug-in's C++ to 0.0000 dB) against the 620 reference renders
of `plugins/FwVocalLeveler/nulltest/render_reference.py`: 26 probes (steps, gaps, stops, tones, syllable trains) and the 36 corpus parts, each at ten
settings. Median null in dB per setting:

| set | all | default | Fast | VS +6 | VS -6 | Target -30 | Target -12 | ±12 | -3 .. +9 | Fast ±12 | Live, Spill -30 |
|---|---|---|---|---|---|---|---|---|---|---|---|
| probes | -70.5 | -76.1 | -56.7 | -76.0 | -71.9 | -74.9 | -76.4 | -69.3 | -74.7 | -55.0 | -72.7 |
| corpus 0 .. 23 | -60.8 | -61.1 | -49.5 | -62.0 | -62.8 | -64.7 | -64.2 | -59.5 | -60.5 | -52.3 | -63.6 |
| corpus 24 .. 35 (held out) | -62.3 | -64.2 | -50.3 | -63.4 | -64.2 | -64.9 | -68.7 | -60.2 | -61.6 | -54.9 | -63.8 |

The worst case over all 620 is -40 dB (a training part at Fast). The residual left is mostly at word onsets at
Attack Fast, where the gain drops in about 10 ms and the two differ by a fraction of a dB for a few milliseconds.

### Not covered

- The **Music side-chain**: the reference used it in no host available offline (its Music knob stays disabled), so
  Music Sensitivity is not modelled and the plug-in rides without the music. It needs a host with a real side-chain
  send.
- Automation Read and Write, and grabbing the Rider fader, are host interactions; the unit tests check them, not
  this null test.

## Presets

Each of the reference's 13 presets loaded into its state (its stored values, hidden ones included), against
Flatwater Vocal Leveler's preset of the same settings (Flatwater names), both Mono, on held-out vocal parts of the corpus
(`scripts/nulltest/vocal_corpus_lib.py`); the output residual after the first 0.1 s, dB (lower is better). Made by
`plugins/FwVocalLeveler/nulltest/null_presets.py`. The Music Sensitivity a preset stores has no effect on either
side here (no side-chain).

| preset | part 24 | part 27 | part 33 |
|---|---|---|---|
| Default | -64.2 | -59.3 | -62.3 |
| Gentle Ride | -68.6 | -67.9 | -70.2 |
| Steady Lead | -70.7 | -71.7 | -81.2 |
| Quick Ride | -55.3 | -70.7 | -77.8 |
| Floor Ride | -60.9 | -58.5 | -52.6 |
| Upper Ride | -45.7 | -55.4 | -52.2 |
| Big Swings | -70.7 | -65.7 | -70.9 |
| Loud Mix | -42.5 | -65.2 | -49.1 |
| Loud Lead | -41.6 | -70.7 | -78.4 |
| Auto Fader | -50.4 | -55.4 | -50.2 |
| Level Tamer | -54.7 | -64.2 | -62.2 |
| Ceiling Ride | -37.9 | -39.6 | -60.0 |
| Open Up | -59.8 | -37.9 | -61.4 |

Median -60.9 dB, worst -37.9 dB over 39 renders.
