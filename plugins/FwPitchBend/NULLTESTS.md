# Flatwater Pitch Bend: null tests

Flatwater Pitch Bend against the reference (its Stereo component), both at 48 kHz unless said. With every control at 0
both pass the voice through with the same delay, and that is null-tested sample for sample. Once anything shifts,
Flatwater Pitch Bend is its own pitch shifter (a pitch detector and overlap-added grains, not a copy), so the shifter
is judged by what is heard (pitch, envelope, level, timing) on the scorecard below, and the modulators, which are exact
laws, are null-tested through the Mix control. How each part was identified is in [MEASUREMENTS.md](MEASUREMENTS.md).

## Summary

| part | cases | result |
|---|---:|---|
| No shift (every control at 0): the delayed voice | sung and spoken takes, vowels | −67 dB on the sung take; −57.9 dB worst on voice, the delay within 0.0002 samples (scorecard M4) |
| The modulators (M1, M2, AM, PT; their laws, timing and modulation of each other), read through Mix | 38 judged, 1 reported | 38 of 38 within their targets (0.003 rms for an exact law away from jumps, 0.01 for a fitted one) |
| The shifter's scorecard: pitch accuracy, formant envelope, level, onsets and jumps, noise, Flatten, the gate | 12 judged measures (one more reported) | 11 of 12 within their thresholds (the miss: a few level cells where the reference's own level varies by more than 1 dB within one setting) |
| The 40 factory presets on a sung and a spoken take: level, pitch track and tone | 80 pairs | 62 of 80 meet every target (67 with the reference's own LFO tables loaded into ours) |
| Sample nulls with a shift, a steady vowel | Formant 0.01 … ±6 | −27 dB at a time scale of 1 (−31 at 200 Hz), −13 … −19 dB at Formant +0.5 … +2 |
| Sample nulls with a shift, a voice | sung take | no cancellation (the reference resynthesises the voice from its own period estimates) |

## The modulators

`nulltest/null_pitchbend_mods.py` (`measurements/fwpitchbend/mods/null.json`): each case's control value over time,
read through Mix on both sides, ours against the reference; rms after 0.1 s, over the whole curve and away from its
jumps (more than 600 samples from a step or a wrap). The cases are described in MEASUREMENTS.md, sections 3 to 6.

| case | class | rms | away from jumps | max | result |
|---|---|---|---|---|---|
| depth25 | exact | 0.0000 | 0.0000 | 0.0006 | pass |
| shape | exact | 0.0001 | 0.0001 | 0.0017 | pass |
| free3.7Hz | exact | 0.0004 | 0.0004 | 0.0026 | pass |
| beat90 | exact | 0.0002 | 0.0002 | 0.0018 | pass |
| phase.25 | exact | 0.0020 | 0.0001 | 0.1156 | pass |
| warp0.1 | exact | 0.0016 | 0.0001 | 0.1016 | pass |
| level0.5 | exact | 0.0001 | 0.0001 | 0.0005 | pass |
| seq4 | exact | 0.0032 | 0.0007 | 0.1160 | pass |
| smooth100 | fitted | 0.0002 | 0.0002 | 0.0018 | pass |
| am | fitted | 0.0018 | 0.0018 | 0.0639 | pass |
| am10_100 | fitted | 0.0006 | 0.0006 | 0.0091 | pass |
| am1_1000 | fitted | 0.0059 | 0.0059 | 0.0312 | pass |
| am100_10 | fitted | 0.0022 | 0.0022 | 0.0091 | pass |
| am1000_1 | fitted | 0.0019 | 0.0019 | 0.0142 | pass |
| am30_300 | fitted | 0.0005 | 0.0005 | 0.0032 | pass |
| am300_1 | fitted | 0.0008 | 0.0008 | 0.0077 | pass |
| am_gate | fitted | 0.0011 | 0.0009 | 0.0241 | pass |
| pt_glide | fitted | 0.0049 | 0.0049 | 0.0096 | pass |
| pt_sung | fitted | 0.0058 | 0.0058 | 0.1369 | pass |
| pt_spoken | report | 0.0309 | 0.0309 | 0.2537 | report |
| mod_level | exact | 0.0001 | 0.0001 | 0.0010 | pass |
| mod_ratefree | exact | 0.0003 | 0.0003 | 0.0020 | pass |
| mod_ratesync | exact | 0.0004 | 0.0004 | 0.0022 | pass |
| mod_warp | exact | 0.0016 | 0.0001 | 0.1018 | pass |
| mod_phase | exact | 0.0020 | 0.0001 | 0.1156 | pass |
| mod_levelstep | exact | 0.0001 | 0.0001 | 0.0010 | pass |
| mod_ratestep_sync | exact | 0.0027 | 0.0003 | 0.1097 | pass |
| mod_ratestep_free | exact | 0.0007 | 0.0007 | 0.0054 | pass |
| mod_warpstep | exact | 0.0016 | 0.0001 | 0.1019 | pass |
| mod_phasestep | exact | 0.0021 | 0.0003 | 0.1363 | pass |
| mod_amlevel | fitted | 0.0004 | 0.0004 | 0.0061 | pass |
| mod_ptlevel | fitted | 0.0033 | 0.0033 | 0.0064 | pass |
| seq_warp | exact | 0.0019 | 0.0005 | 0.0770 | pass |
| smooth10 | fitted | 0.0007 | 0.0007 | 0.0164 | pass |
| smooth1000 | fitted | 0.0008 | 0.0008 | 0.0021 | pass |
| mod_amattack | fitted | 0.0008 | 0.0008 | 0.0057 | pass |
| mod_amrelease | fitted | 0.0009 | 0.0009 | 0.0090 | pass |
| pt_smooth300 | fitted | 0.0036 | 0.0036 | 0.0159 | pass |
| am_spoken | fitted | 0.0030 | 0.0030 | 0.0174 | pass |

PT on the spoken take is reported, not judged: two pitch detectors on speech disagree where the voice is rough (the
falling, creaky end of a phrase), which neither reproduces from the other.

## The shifter's scorecard

`nulltest/score_pitchbend_shifter.py` (`measurements/fwpitchbend/shifter/ours.json`; the reference's numbers in
`measurements/fwpitchbend/shifter/`). The same signals and measures on both shifters:

| metric | reference | ours | threshold | result |
|---|---|---|---|---|
| M1 vowel grid: worst median / worst p90 (c) / gross | 0.41 / 1.01 / 0 | 0.41 / 0.70 / 0 | ≤ 1 / ≤ 3 / 0 | pass |
| M1 at 44.1 and 96 kHz: worst median, worst p90 (c) | 0.06, 0.13 | 0.04, 0.05 | ≤ 1, ≤ 3 | pass |
| M1b sung phrases: median p90 (c) / max gross | 5.96 / 0.118 | 6.29 / 0.099 | ≤ 10 / ≤ 0.15 | pass |
| M2 envelope against F + β: worst error (st) / envelope error against the reference (dB) | 0.34 / n/a | 0.24 / +0.34 | ≤ 0.5 / ≤ +1 | pass |
| M3 level cells within ±1 dB | 143 cells | 139 of 143 (worst +1.35) | all | **miss** |
| M4 no-shift delay error (samples) | n/a | 0.0002 | ±0.05 | pass |
| M4 no-shift residual on voice (dB) | −57 … −68 | −57.9 worst (−82 … −92 sung) | < −55 | pass |
| M4 onset settle, worst against the reference | n/a | +4 ms | ≤ +10 | pass |
| M4 jump settle, worst against the reference | n/a | +5 ms | ≤ +10 | pass |
| M4 onset delay (report only, samples) | 69 … 131 | 105 … 106 | ±20 | 37 off |
| M5 noise level against −0.6 dB / centroid against F + β | n/a | 0.5 dB / 0.08 st | ±1 / ±0.5 | pass |
| M6 Flatten steady / vibrato p90 / lock | 0.16 c / 8.5 c / 54 ms | 0.23 c / 6.5 c / 45 ms | ≤ 1 / ≤ 10 / ≤ 60 | pass |
| M7 gate / SNR | −50 dBFS / 10 dB | −51 dBFS / 10 dB | −51 ± 3 / ≤ 10 | pass |

## The factory presets

The reference's 40 presets loaded into it against Flatwater Pitch Bend's (Flatwater names): level, the pitch track
where both outputs are clearly voiced, and the long-term spectrum, on a sung and a spoken take. Median pitch
difference 2.3 cents on the sung take and 6.5 on the spoken one. Every pair, and why the misses miss:
[NULLTESTS-presets.md](NULLTESTS-presets.md).

## Sample nulls with a shift

Click probes and pulse trains (MEASUREMENTS.md, section 11, Sample nulls with a shift) show where the reference's grains sit and what they read; ours read the same for Formant up to +1.5. On a
steady 150 Hz vowel the residual (raw, then with ours moved by a fraction of a sample and matched in level):

| setting | raw (dB) | lined up (dB) |
|---|---:|---:|
| Formant +0.01 | −27.2 | −46.9 |
| Formant +0.5 | −14.4 | −31.7 |
| Formant +1 | −12.9 | −25.5 |
| Formant −1 | −14.6 | −26.7 |
| Formant −3 | −6.6 | −13.7 |
| Formant −6 | −9.4 | −16.1 |
| Pitch +5 | −0.6 | −9.4 |

On a sung take, even at Formant +0.01, the reference's output against its own input, delayed, is +0.3 dB: it
rebuilds a moving voice grain by grain from its own pitch-period estimates, so no recreation that does not share its
detector's exact estimates nulls it; ours reaches about −7 dB per 100 ms window at best alignment.
