# Flatwater Pitch Bend: the factory presets against the reference's own

Each of the reference's 40 presets loaded into its state (its stored values, hidden ones included, and its two
shape tables), against Flatwater Pitch Bend's preset of the same settings (Flatwater names), both stereo at
48 kHz and 120 bpm from song position 0, on a sung and a spoken take of the vocal corpus (about 8 s each). Made by
`nulltest/null_pitchbend_presets.py`. Our shifter is our own method, so the outputs are compared by what is heard,
not sample by sample (after the first 0.1 s):

- **level**: the output's rms, ours minus the reference's, dB (target within 1.5);
- **pitch**: where both outputs are clearly voiced, the median difference of their pitch tracks in cents (target 15),
  and the share of frames more than 50 cents apart (target 10 %);
- **tone**: the rms difference of the long-term spectra, 1/6 octave from 100 Hz to 8 kHz, levels matched, dB
  (target 3);
- **null**: the output residual, dB, for the record.

A preset marked *shape substituted* uses one of our own LFO shapes where the reference's is a drawn one (we do
not ship those); its second line is the same preset with the reference's table loaded into ours, which shows how
much of the difference is the shape.

In a blend (Mix well under 100 %) the pitch column measures which of the two voices a tracker hears frame by
frame, which is decided where they are close in level; MEASUREMENTS.md (Factory presets) checks those presets'
modulation directly.

62 of 80 preset and take pairs meet every target.

## The sung take

| preset | level | pitch | tone | null | |
|---|---|---|---|---|---|
| Character / Cartoon | -0.0 | 0.6 c / 0 % | 2.0 | 1.7 | pass |
| Character / Deeper Voice | +0.2 | 0.6 c / 0 % | 1.0 | 1.1 | pass |
| Character / Higher Register | +0.0 | 1.2 c / 0 % | 1.6 | 3.0 | pass |
| Character / Groggy Voice | +0.1 | 1.5 c / 0 % | 1.3 | 2.4 | pass |
| Character / Pushy | +0.1 | 1.2 c / 0 % | 1.2 | 4.3 | pass |
| Character / Tiny Voice | -0.1 | 0.7 c / 0 % | 2.5 | 3.1 | pass |
| Character / Younger | +0.4 | 0.7 c / 0 % | 1.5 | 4.8 | pass |
| Harmony / Bass Backing | +0.0 | 0.2 c / 1 % | 1.0 | -6.3 | pass |
| Harmony / Fifth Above Blend | +0.1 | 0.5 c / 0 % | 1.0 | -1.0 | pass |
| Harmony / Fourth Above | -0.0 | 0.7 c / 0 % | 1.7 | 0.8 | pass |
| Harmony / Octave Below Blend | +0.0 | 1 c / 0 % | 1.0 | 0.5 | pass |
| Double / Stutter Twin (shape substituted: M1) | +0.1 | 15.2 c / 5 % | 2.2 | 0.3 | **miss** |
| &nbsp;&nbsp;with the reference's shape | +0.0 | 9.2 c / 1 % | 2.2 | -0.2 | pass |
| Double / Loose Double | -0.1 | 3.4 c / 1 % | 0.8 | -5.7 | pass |
| Double / Tape Double | +0.0 | 3.1 c / 1 % | 1.3 | -3.6 | pass |
| Double / Thin Double | +0.8 | 0.8 c / 1 % | 2.7 | -1.2 | pass |
| Double / Tracked Double High | -0.1 | 3.6 c / 1 % | 1.2 | -5.3 | pass |
| Double / Tracked Double Low | +0.2 | 3.1 c / 1 % | 1.0 | -3.6 | pass |
| Movement / Wavering Voice | +0.4 | 2.3 c / 1 % | 0.7 | 2.0 | pass |
| Movement / Alarm | -0.5 | 8.9 c / 1 % | 1.5 | 2.7 | pass |
| Movement / Breathing Formant | +0.1 | 0.7 c / 0 % | 1.0 | 0.7 | pass |
| Movement / Deep Warble | -0.1 | n/a | 2.4 | 3.2 | pass |
| Movement / Drawl | -0.1 | 4.7 c / 1 % | 1.6 | 2.6 | pass |
| Movement / Fan Wobble | +0.3 | 0 c / 0 % | 1.6 | -3.5 | pass |
| Movement / Pitch Follower Down (shape substituted: M2) | +0.1 | 10.1 c / 0 % | 2.1 | 2.9 | pass |
| &nbsp;&nbsp;with the reference's shape | +0.1 | 10.2 c / 0 % | 1.2 | 2.7 | pass |
| Movement / Shape Shifter | -0.2 | 0.9 c / 0 % | 1.5 | 1.0 | pass |
| Movement / Shiver | +0.8 | 2.6 c / 0 % | 3.9 | 1.1 | **miss** |
| Movement / Submerged (shape substituted: M1) | -0.2 | 1.2 c / 1 % | 1.5 | 0.4 | pass |
| &nbsp;&nbsp;with the reference's shape | -0.1 | 1 c / 0 % | 0.9 | 1.3 | pass |
| Movement / Warped Tape (shape substituted: M1) | +0.3 | 1.9 c / 17 % | 3.6 | 1.2 | **miss** |
| &nbsp;&nbsp;with the reference's shape | +0.3 | 1.3 c / 1 % | 2.0 | 3.3 | pass |
| Movement / Wobble (shape substituted: M1) | -0.1 | 48 c / 46 % | 2.6 | 3.0 | **miss** |
| &nbsp;&nbsp;with the reference's shape | +0.0 | 3.1 c / 0 % | 1.6 | 1.5 | pass |
| Robot / Alien Steps (shape substituted: M2) | -0.1 | 3.7 c / 2 % | 2.9 | 1.4 | pass |
| &nbsp;&nbsp;with the reference's shape | -0.1 | 3.6 c / 2 % | 1.7 | 3.5 | pass |
| Robot / Arcade Steps (shape substituted: M2) | -0.6 | 35.4 c / 20 % | 3.3 | 3.0 | **miss** |
| &nbsp;&nbsp;with the reference's shape | -0.5 | 10 c / 3 % | 3.1 | 3.6 | **miss** |
| Robot / Glide Robot | +1.4 | 10.3 c / 0 % | 1.8 | 3.9 | pass |
| Robot / Vinyl Scrub | +0.4 | 8.9 c / 0 % | 1.0 | 3.1 | pass |
| Robot / Stepping Robot (shape substituted: M2) | -1.1 | 3.6 c / 3 % | 3.9 | 3.0 | **miss** |
| &nbsp;&nbsp;with the reference's shape | -0.8 | 4.3 c / 0 % | 2.2 | 3.3 | pass |
| Sequence / Chopped Melody (shape substituted: M2) | +0.8 | 6.3 c / 1 % | 2.0 | 3.5 | pass |
| &nbsp;&nbsp;with the reference's shape | +1.2 | 6 c / 1 % | 1.7 | 3.4 | pass |
| Sequence / Long Journey (shape substituted: M2) | +0.4 | 11.1 c / 3 % | 2.9 | 2.9 | pass |
| &nbsp;&nbsp;with the reference's shape | -0.0 | 11.6 c / 4 % | 1.7 | 2.9 | pass |
| Sequence / Lullaby Steps | -0.3 | 11.8 c / 0 % | 1.0 | 2.3 | pass |
| Sequence / Plain Sequence | -0.7 | 0.9 c / 23 % | 3.6 | -1.2 | **miss** |
| Sequence / Rising Steps | -0.3 | 8.7 c / 1 % | 0.9 | 2.7 | pass |
| Sequence / Twin Sequences | -0.5 | 1.1 c / 1 % | 1.1 | 1.0 | pass |

## The spoken take

| preset | level | pitch | tone | null | |
|---|---|---|---|---|---|
| Character / Cartoon | +0.3 | 2.3 c / 0 % | 1.4 | 2.2 | pass |
| Character / Deeper Voice | +0.7 | 2.4 c / 0 % | 0.8 | 1.4 | pass |
| Character / Higher Register | +0.7 | 6.5 c / 1 % | 2.2 | 3.7 | pass |
| Character / Groggy Voice | -1.1 | 2.8 c / 1 % | 1.1 | 2.6 | pass |
| Character / Pushy | -0.2 | 6.4 c / 0 % | 0.8 | 3.6 | pass |
| Character / Tiny Voice | -0.2 | 5.8 c / 1 % | 1.8 | 2.7 | pass |
| Character / Younger | +0.4 | 1.6 c / 0 % | 0.6 | 0.6 | pass |
| Harmony / Bass Backing | +0.0 | 1.9 c / 20 % | 0.3 | -8.8 | **miss** |
| Harmony / Fifth Above Blend | -0.0 | 2.1 c / 3 % | 0.4 | -3.2 | pass |
| Harmony / Fourth Above | +0.0 | n/a | 0.7 | 0.4 | pass |
| Harmony / Octave Below Blend | -0.8 | 2 c / 0 % | 1.1 | -0.9 | pass |
| Double / Stutter Twin (shape substituted: M1) | +0.4 | 16.9 c / 8 % | 1.2 | 0.5 | **miss** |
| &nbsp;&nbsp;with the reference's shape | +0.2 | 15.3 c / 5 % | 0.7 | 0.4 | **miss** |
| Double / Loose Double | +0.2 | 3.9 c / 1 % | 0.4 | -5.1 | pass |
| Double / Tape Double | -0.4 | 4.1 c / 12 % | 1.2 | -3.8 | **miss** |
| Double / Thin Double | -0.6 | 1.6 c / 3 % | 0.7 | -5.5 | pass |
| Double / Tracked Double High | -0.1 | 2.4 c / 2 % | 0.6 | -5.3 | pass |
| Double / Tracked Double Low | -0.3 | 3.5 c / 10 % | 0.9 | -4.3 | pass |
| Movement / Wavering Voice | +0.0 | 8.1 c / 0 % | 1.0 | 3.8 | pass |
| Movement / Alarm | -0.7 | 14.5 c / 6 % | 1.1 | 2.8 | pass |
| Movement / Breathing Formant | +0.4 | 4.1 c / 0 % | 0.5 | 1.2 | pass |
| Movement / Deep Warble | +0.4 | n/a | 2.0 | 3.2 | pass |
| Movement / Drawl | +0.3 | 15.9 c / 7 % | 0.8 | 3.5 | **miss** |
| Movement / Fan Wobble | -0.0 | 0 c / 0 % | 0.3 | -4.4 | pass |
| Movement / Pitch Follower Down (shape substituted: M2) | -0.7 | 34.5 c / 24 % | 2.4 | 2.7 | **miss** |
| &nbsp;&nbsp;with the reference's shape | -0.9 | 35.5 c / 24 % | 2.0 | 2.4 | **miss** |
| Movement / Shape Shifter | -0.3 | 12.8 c / 6 % | 1.2 | 3.0 | pass |
| Movement / Shiver | -0.0 | 10.3 c / 5 % | 2.3 | 1.7 | pass |
| Movement / Submerged (shape substituted: M1) | +0.6 | 4.4 c / 2 % | 1.0 | 0.9 | pass |
| &nbsp;&nbsp;with the reference's shape | +0.5 | 4.5 c / 2 % | 0.7 | 0.7 | pass |
| Movement / Warped Tape (shape substituted: M1) | +0.0 | 3.2 c / 6 % | 1.2 | 3.3 | pass |
| &nbsp;&nbsp;with the reference's shape | -0.0 | 3.6 c / 0 % | 0.6 | 3.3 | pass |
| Movement / Wobble (shape substituted: M1) | +0.4 | 51.2 c / 54 % | 1.0 | 3.4 | **miss** |
| &nbsp;&nbsp;with the reference's shape | +0.5 | 7.7 c / 2 % | 0.7 | 2.0 | pass |
| Robot / Alien Steps (shape substituted: M2) | +0.5 | 9.3 c / 8 % | 2.0 | 3.4 | pass |
| &nbsp;&nbsp;with the reference's shape | +0.8 | 9 c / 9 % | 1.4 | 3.5 | pass |
| Robot / Arcade Steps (shape substituted: M2) | -1.1 | 24 c / 9 % | 0.9 | 2.4 | **miss** |
| &nbsp;&nbsp;with the reference's shape | -1.0 | 18.6 c / 4 % | 0.9 | 2.5 | **miss** |
| Robot / Glide Robot | +1.1 | 24.5 c / 25 % | 1.2 | 3.5 | **miss** |
| Robot / Vinyl Scrub | +0.2 | 22.5 c / 23 % | 0.9 | 3.1 | **miss** |
| Robot / Stepping Robot (shape substituted: M2) | -0.2 | 10.5 c / 3 % | 2.4 | 3.5 | pass |
| &nbsp;&nbsp;with the reference's shape | -0.1 | 10.5 c / 6 % | 1.6 | 3.0 | pass |
| Sequence / Chopped Melody (shape substituted: M2) | +0.3 | 12 c / 6 % | 1.8 | 2.7 | pass |
| &nbsp;&nbsp;with the reference's shape | +0.7 | 10.9 c / 8 % | 1.5 | 3.0 | pass |
| Sequence / Long Journey (shape substituted: M2) | +0.7 | 15.3 c / 9 % | 2.2 | 3.2 | **miss** |
| &nbsp;&nbsp;with the reference's shape | +0.2 | 15.2 c / 7 % | 1.0 | 3.0 | **miss** |
| Sequence / Lullaby Steps | +0.5 | 10.8 c / 4 % | 1.6 | 4.0 | pass |
| Sequence / Plain Sequence | -0.9 | 61.3 c / 75 % | 1.5 | -1.3 | **miss** |
| Sequence / Rising Steps | -0.3 | 7.4 c / 1 % | 0.8 | 2.9 | pass |
| Sequence / Twin Sequences | -0.0 | 2.6 c / 2 % | 0.8 | 3.4 | pass |
