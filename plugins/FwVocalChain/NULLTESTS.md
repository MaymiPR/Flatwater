# Flatwater Vocal Chain: null tests

Residuals of Flatwater Vocal Chain against the reference (its Stereo component), lower is better: the energy of the
difference under the reference output's, in dB. Written by `nulltest/nulltest_fwvocalchain.py report` from
`measurements/fwvocalchain/nulls.json`; our processor through `FwVocalChainRender`, the reference through
`nulltest/cv_ref.py`, the Noise off in both (two noise sources cannot null). The vocals are held-out parts of the
corpus (`nulltest/vocal_corpus.py`); how each stage was identified is in [MEASUREMENTS.md](MEASUREMENTS.md).

## Summary

| set | rate | cases | median (dB) | worst (dB) |
|---|---:|---:|---:|---:|
| Bass, Treble and Reverb, the others clear | 44.1 kHz | 55 | -77.7 | -67.8 |
| Bass, Treble and Reverb, the others clear | 48 kHz | 55 | -75.7 | -67.9 |
| Bass, Treble and Reverb, the others clear | 96 kHz | 55 | -71.0 | -60.3 |
| The whole chain, the doubler clear | 44.1 kHz | 15 | -32.1 | -24.1 |
| The whole chain, doubler Stereo | 44.1 kHz | 4 | -35.6 | -28.2 |
| The whole chain, doubler Wide | 44.1 kHz | 4 | -13.8 | -10.8 |
| The whole chain, doubler Spreader | 44.1 kHz | 4 | -18.3 | -4.6 |
| The whole chain, the doubler clear | 48 kHz | 30 | -33.2 | -24.2 |
| The whole chain, doubler Stereo | 48 kHz | 8 | -36.0 | -27.2 |
| The whole chain, doubler Wide | 48 kHz | 8 | -24.2 | -4.7 |
| The whole chain, doubler Spreader | 48 kHz | 8 | -17.6 | -4.8 |
| The whole chain, the doubler clear | 96 kHz | 15 | -35.5 | -25.4 |
| The whole chain, doubler Stereo | 96 kHz | 4 | -18.1 | -13.8 |
| The whole chain, doubler Wide | 96 kHz | 4 | -9.0 | -3.4 |
| The whole chain, doubler Spreader | 96 kHz | 4 | -9.6 | -4.5 |

## The whole chain

A held-out vocal at -6 dBFS peaks after 6 s of silence (the delays settled, the doubler started in step in both):
the defaults, every sixth factory preset and six random settings of every control; each case also with the
Pitch stage clear.

| rate | part | case | doubler | null (dB) | doubler clear (dB) |
|---|---:|---|---|---:|---:|
| 44.1 kHz | 30 | defaults | Stereo | -40.6 | -41.0 |
| 44.1 kHz | 30 | preset 00 | Stereo | -40.6 | -41.0 |
| 44.1 kHz | 30 | preset 06 | Spreader | -4.6 | -24.1 |
| 44.1 kHz | 30 | preset 12 | clear | -31.9 | -31.9 |
| 44.1 kHz | 30 | preset 18 | Spreader | -18.1 | -26.2 |
| 44.1 kHz | 30 | preset 24 | clear | -29.4 | -29.4 |
| 44.1 kHz | 30 | preset 30 | Stereo | -28.2 | -28.3 |
| 44.1 kHz | 30 | preset 36 | Stereo | -30.5 | -30.5 |
| 44.1 kHz | 30 | preset 42 | Spreader | -18.5 | -26.2 |
| 44.1 kHz | 30 | random 0 | Wide | -10.8 | -35.2 |
| 44.1 kHz | 30 | random 1 | Wide | -15.2 | -42.4 |
| 44.1 kHz | 30 | random 2 | Spreader | -29.8 | -37.8 |
| 44.1 kHz | 30 | random 3 | clear | -42.1 | -42.1 |
| 44.1 kHz | 30 | random 4 | Wide | -23.7 | -45.7 |
| 44.1 kHz | 30 | random 5 | Wide | -12.3 | -32.1 |
| 48 kHz | 30 | defaults | Stereo | -42.5 | -42.5 |
| 48 kHz | 30 | preset 00 | Stereo | -42.5 | -42.5 |
| 48 kHz | 30 | preset 06 | Spreader | -17.1 | -26.0 |
| 48 kHz | 30 | preset 12 | clear | -31.9 | -31.9 |
| 48 kHz | 30 | preset 18 | Spreader | -16.1 | -27.3 |
| 48 kHz | 30 | preset 24 | clear | -29.5 | -29.5 |
| 48 kHz | 30 | preset 30 | Stereo | -28.5 | -28.5 |
| 48 kHz | 30 | preset 36 | Stereo | -33.5 | -33.6 |
| 48 kHz | 30 | preset 42 | Spreader | -16.6 | -27.1 |
| 48 kHz | 30 | random 0 | Wide | -29.5 | -36.2 |
| 48 kHz | 30 | random 1 | Wide | -30.0 | -57.0 |
| 48 kHz | 30 | random 2 | Spreader | -18.2 | -39.0 |
| 48 kHz | 30 | random 3 | clear | -54.5 | -54.5 |
| 48 kHz | 30 | random 4 | Wide | -40.3 | -57.2 |
| 48 kHz | 30 | random 5 | Wide | -31.1 | -40.1 |
| 48 kHz | 33 | defaults | Stereo | -38.5 | -39.0 |
| 48 kHz | 33 | preset 00 | Stereo | -38.5 | -39.0 |
| 48 kHz | 33 | preset 06 | Spreader | -4.8 | -24.2 |
| 48 kHz | 33 | preset 12 | clear | -29.5 | -29.5 |
| 48 kHz | 33 | preset 18 | Spreader | -23.3 | -27.8 |
| 48 kHz | 33 | preset 24 | clear | -30.2 | -30.2 |
| 48 kHz | 33 | preset 30 | Stereo | -27.2 | -27.2 |
| 48 kHz | 33 | preset 36 | Stereo | -32.6 | -33.0 |
| 48 kHz | 33 | preset 42 | Spreader | -23.6 | -27.7 |
| 48 kHz | 33 | random 0 | Wide | -7.0 | -28.1 |
| 48 kHz | 33 | random 1 | Wide | -18.9 | -30.2 |
| 48 kHz | 33 | random 2 | Spreader | -32.1 | -33.4 |
| 48 kHz | 33 | random 3 | clear | -47.7 | -47.7 |
| 48 kHz | 33 | random 4 | Wide | -11.6 | -41.2 |
| 48 kHz | 33 | random 5 | Wide | -4.7 | -42.1 |
| 96 kHz | 30 | defaults | Stereo | -18.1 | -42.2 |
| 96 kHz | 30 | preset 00 | Stereo | -18.1 | -42.2 |
| 96 kHz | 30 | preset 06 | Spreader | -4.5 | -25.4 |
| 96 kHz | 30 | preset 12 | clear | -31.0 | -31.0 |
| 96 kHz | 30 | preset 18 | Spreader | -9.3 | -26.9 |
| 96 kHz | 30 | preset 24 | clear | -28.8 | -28.8 |
| 96 kHz | 30 | preset 30 | Stereo | -13.8 | -27.9 |
| 96 kHz | 30 | preset 36 | Stereo | -32.1 | -33.4 |
| 96 kHz | 30 | preset 42 | Spreader | -9.9 | -26.8 |
| 96 kHz | 30 | random 0 | Wide | -6.5 | -35.5 |
| 96 kHz | 30 | random 1 | Wide | -27.7 | -56.8 |
| 96 kHz | 30 | random 2 | Spreader | -19.9 | -38.5 |
| 96 kHz | 30 | random 3 | clear | -54.3 | -54.3 |
| 96 kHz | 30 | random 4 | Wide | -11.5 | -57.1 |
| 96 kHz | 30 | random 5 | Wide | -3.4 | -39.8 |

## The doubler in behaviour

The Pitch stage's Wide and Spreader colours are pitch shifters that pick their splice points from the signal,
so a voice parts from the reference at a splice where the two pick different steps; they are compared in
behaviour (`nulltest/behaviour_vocalchain_doubler.py`, 48 kHz, Pitch +10, every other colour clear, the wet
signal alone): the level, the third-octave spectrum (100 Hz .. 12 kHz) ours less the reference's, and on noise
the voice's delay (5th / 50th / 95th percentile) and the strongest cycle of its movement. Stereo for scale.

| colour | signal | side | level diff (dB) | spectrum rms / max (dB) | delay p5 / p50 / p95, ours (ref) (ms) | cycle, ours (ref) (s) | wet null (dB) |
|---|---|---|---:|---|---|---|---:|
| Stereo | noise | L | +0.00 | 0.00 / 0.01 | 7.4 / 9.9 / 12.4 (7.4 / 9.9 / 12.4) | 4.98 (4.98) | -27.2 |
| Stereo | noise | R | +0.00 | 0.00 / 0.01 | 14.9 / 16.5 / 18.2 (14.9 / 16.5 / 18.2) | 3.32 (3.32) | -24.7 |
| Stereo | vocal | L | +0.00 | 0.01 / 0.03 |  |  | -48.0 |
| Stereo | vocal | R | +0.00 | 0.01 / 0.04 |  |  | -41.4 |
| Wide | noise | L | +0.00 | 0.01 / 0.03 | 3.4 / 8.2 / 19.5 (3.0 / 8.0 / 22.9) | 4.43 (4.55) | -1.9 |
| Wide | noise | R | +0.00 | 0.00 / 0.01 | 29.1 / 34.7 / 40.6 (29.4 / 34.8 / 40.7) | 3.47 (3.39) | -2.0 |
| Wide | vocal | L | +0.00 | 0.02 / 0.04 |  |  | -17.7 |
| Wide | vocal | R | -0.01 | 0.01 / 0.04 |  |  | -14.1 |
| Spreader | noise | L | -0.05 | 0.04 / 0.07 | 2.4 / 6.6 / 11.6 (3.0 / 6.9 / 11.4) | 7.25 (6.93) | 2.7 |
| Spreader | noise | R | -0.04 | 0.04 / 0.06 | 2.1 / 6.6 / 10.5 (1.9 / 6.0 / 10.6) | 6.93 (6.93) | -0.3 |
| Spreader | vocal | L | -0.04 | 0.05 / 0.12 |  |  | -8.2 |
| Spreader | vocal | R | -0.04 | 0.04 / 0.09 |  |  | -15.4 |

The factory presets against the reference's own: [NULLTESTS-presets.md](NULLTESTS-presets.md).

## Bass, Treble and Reverb

A held-out vocal at -16 dBFS peak (under the dynamics), every other colour clear.

| case | 44.1 kHz (dB) | 48 kHz (dB) | 96 kHz (dB) |
|---|---:|---:|---:|
| clear | -78.9 | -76.5 | -71.1 |
| bass c1 -10 | -77.5 | -75.6 | -72.7 |
| bass c1 -7.5 | -77.6 | -75.6 | -72.3 |
| bass c1 -3 | -77.8 | -75.8 | -71.5 |
| bass c1 +0 | -77.9 | -75.8 | -71.0 |
| bass c1 +2.5 | -77.7 | -75.7 | -70.4 |
| bass c1 +6 | -77.3 | -75.5 | -69.8 |
| bass c1 +10 | -73.8 | -72.9 | -68.1 |
| bass c2 -10 | -77.6 | -75.7 | -72.9 |
| bass c2 -7.5 | -77.7 | -75.7 | -72.5 |
| bass c2 -3 | -77.9 | -75.8 | -71.6 |
| bass c2 +0 | -77.9 | -75.8 | -71.0 |
| bass c2 +2.5 | -77.5 | -75.6 | -70.3 |
| bass c2 +6 | -76.9 | -75.2 | -69.7 |
| bass c2 +10 | -72.7 | -71.9 | -68.1 |
| bass c3 -10 | -77.5 | -75.6 | -73.4 |
| bass c3 -7.5 | -78.1 | -75.9 | -73.7 |
| bass c3 -3 | -79.0 | -76.5 | -70.2 |
| bass c3 +0 | -77.0 | -75.1 | -70.4 |
| bass c3 +2.5 | -70.7 | -69.7 | -60.3 |
| bass c3 +6 | -75.3 | -73.9 | -69.0 |
| bass c3 +10 | -73.8 | -73.0 | -61.0 |
| treble c1 -10 | -78.5 | -76.2 | -70.9 |
| treble c1 -7.5 | -78.7 | -76.2 | -71.0 |
| treble c1 -3 | -78.8 | -76.4 | -71.0 |
| treble c1 +0 | -78.9 | -76.5 | -71.1 |
| treble c1 +2.5 | -79.0 | -76.5 | -71.1 |
| treble c1 +6 | -79.2 | -76.7 | -71.2 |
| treble c1 +10 | -79.2 | -77.0 | -71.5 |
| treble c2 -10 | -78.2 | -75.9 | -70.6 |
| treble c2 -7.5 | -78.4 | -76.1 | -70.8 |
| treble c2 -3 | -78.7 | -76.3 | -71.0 |
| treble c2 +0 | -78.9 | -76.5 | -71.1 |
| treble c2 +2.5 | -79.2 | -76.7 | -71.2 |
| treble c2 +6 | -79.5 | -77.0 | -71.4 |
| treble c2 +10 | -75.7 | -74.8 | -71.9 |
| treble c3 -10 | -78.6 | -76.2 | -71.2 |
| treble c3 -7.5 | -78.6 | -76.2 | -71.2 |
| treble c3 -3 | -78.7 | -76.3 | -71.1 |
| treble c3 +0 | -78.9 | -76.5 | -71.1 |
| treble c3 +2.5 | -79.2 | -76.7 | -71.1 |
| treble c3 +6 | -79.4 | -77.0 | -71.2 |
| treble c3 +10 | -76.8 | -75.7 | -71.0 |
| bass c2 +4, treble c3 -6 | -77.0 | -75.2 | -70.1 |
| reverb c1 +0 | -72.5 | -72.1 | -69.3 |
| reverb c1 +5 | -67.8 | -68.0 | -66.7 |
| reverb c1 -7.3 | -68.1 | -67.9 | -65.8 |
| reverb c2 +0 | -76.9 | -75.4 | -70.9 |
| reverb c2 +5 | -74.0 | -73.5 | -70.4 |
| reverb c2 -7.3 | -69.1 | -69.3 | -67.0 |
| reverb c3 +0 | -78.2 | -76.1 | -71.0 |
| reverb c3 +5 | -76.7 | -75.3 | -71.0 |
| reverb c3 -7.3 | -70.4 | -69.8 | -67.3 |
| reverb c2, sens +5, out -3, bal +3 | -76.8 | -75.5 | -70.9 |
| reverb c3, bass c1 +6, treble c2 +4 | -76.7 | -75.2 | -69.9 |
