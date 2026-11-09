# Flatwater Bass Leveler: measurements

The current values of every stage of Flatwater Bass Leveler, measured on the original it recreates (Bass Rider), with
the method and the residual or confidence of each. The data is in [measurements/](../../measurements).

## Setup

| Item | Value |
|---|---|
| Reference | Bass Rider (Waves), the Mono, Stereo, Live Mono and Live Stereo components, from a licensed install |
| Host | the reference's VST3 through pedalboard in `nulltest/measure_bassleveler.py`, 48 kHz unless noted; a fresh instance per render (the reference keeps note state between renders); the reported latency compensated, so the output lines up with the input (cross-correlation lag 0) |
| Gain readout | the applied gain as output over input, sample by sample: a least-squares ratio over 2 ms; a -54 dBFS 3 kHz pilot exposes the gain in silences |
| Probes | about 1,200 short signals with the reference's gain at set times (`nulltest/br_probes.py`: steps, decays, re-attacks, gaps, legato drops, decay-and-step); each subcommand of `measure_bassleveler.py` is named in the tables |
| Traces | `nulltest/br_traces.py`: the reference's gain on a bass line and on synthetic parts (ghost notes at a tenth of the level, random gaps) at several settings |
| Corpus | `nulltest/bass_corpus.py`: 36 parts of 12 s from the DAW's own bass packs (acoustic, 80s pick, Ibanez, slap, Temptated, a picked 5-string, a pluck multisample; Ogg Vorbis in WAV decoded by `tools/OggDecode.cpp`, cached in the git-ignored `reference/`) and three synthetic voices (808 sub, filtered saw, slow swells); parts 0 to 23 identify the model, 24 to 35 are held out |
| Corpus renders | `nulltest/br_corpus_ref.py`: the reference's gain on every part at 10 settings (default, Fast, Sensitivity 0 / 75 / 100, Fast with Sensitivity 25 / 100, Artifacts 15 / 85, Target -24); `nulltest/br_corpus_eval.py` compares (`--events` and per-note tables) |
| Model | `nulltest/brmodel.c`, identical line for line to `plugins/FwBassLeveler/Source/dsp/Ride.h`; `nulltest/br_fit.py` (CMA-ES over probes, traces and the training corpus, corpus weight `CW`), `nulltest/br_polish.py` (each per-setting value on a grid in turn), `nulltest/br_export.py` (writes the constants into the C++) |
| Editor | `tools/MeterCapture gl=1`: the OpenGL frames read back, with `drag=...,hover` and `click` scripts and `in=` audio; `nulltest/read_br_film.py` reads the films |

## 1. The level read

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Weighting | two identical first-order low shelves of half the gain each: the hidden high-pass 30 Hz / -30 dB as two -15 dB sections, built as Flatwater Vocal Leveler's | the level read against frequency, 12 Hz to 1.8 kHz, 35 points, linear in level (`weighting`) | an RBJ low shelf at 31.35 Hz / -28.90 dB / Q 0.5075 fits to 0.023 dB rms, 0.06 max; with 29.9 Hz / -30 dB and Q free, 0.065 dB rms; Q 0.5 is what the pair gives; with the pair the steady 80 Hz null -41.5 -> -48.9 dB and the probe cost 969 -> 925 |
| Weighting values | flat above the shelf; -4.81 dB at 80 Hz, -11.5 at 40 Hz, -19.7 at 20 Hz | `weighting` | measured |
| Envelope | neither peak nor rms: the power of the analytic (quadrature) envelope through a one-pole of 2 ms attack and 100 ms release (the hidden level attack and release) | against a 1 kHz sine: two equal tones read +5.62 dB (rms would say +3.01, peak +5.93), three tones +8.91, a low-passed square +4.75 (its own peak +2.55), band noise of the same rms +5.50 (`envelope`) | 0.18 dB rms over the four; on the raw waveform the square misses by 2.4 dB |
| Quadrature pair | an IIR all-pass pair, 12 Hz to 20 kHz, 0.23 degrees at 48 kHz, one set per rate (`nulltest/design_hilbert.py`); it adds no delay of its own | the gain's ramp starts exactly at the look-ahead | exact |
| Stereo | one gain for both channels, read from the mid, (L + R) / 2: L -6 / R -30 dBFS in phase rides at the mid's -11.5; R = -L reads silence (gain at Idle); L 1 kHz with R 1.2 kHz reads as the two-tone sum | `stereo` | exact |

Method notes: the level read is taken through the static law of section 2 from steady sines and multi-tone signals.

## 2. The gain law

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Static law | gain = Target - E + 0.68 dB, clamped to Range Min / Max, E the level read of section 1; the output settles at Target + 0.68 dB of E; for an 80 Hz sine, Target + 5.48 - peak | Targets -14, -24, -4 from -60 to 0 dBFS (`static`) | 0.01 dB everywhere off the clamps |
| Attack phase | from a note's start the gain follows the law both ways while the note level N stays over the follower F plus a margin (section 3), and for the settle time after it falls under; then the gain holds | probes | exact on the probes |
| Hold | outside the attack phase the gain holds where it is, whatever the level read does: a 71 Hz note at -4.4 dBFS followed legato by a 122 Hz note at -4.5 (3.5 dB louder through the weighting) keeps the first note's gain to the hundredth; a legato step down is held unchanged for 8 s; a legato step up is an attack over S | `steps`, probes | exact |
| Held gain on decaying notes | single 200 Hz notes at -6 dBFS with decays from 0.05 to 2 s and none: held gain +1.38 / +0.97 / +0.70 / +0.45 / +0.18 / 0 dB over a steady note's, from the shortest decay to none; a fixed settle time or a smoothed law cannot give that curve, the attack-phase rule does, and it takes the quadrature pair's onset overshoot out of the held gain | probes | 0.12 dB before the fit |
| End of the attack phase | the gain follows until the note is a margin under F, a one-pole follower of N's power: a slow fade is followed throughout (at Sensitivity 75 and 100, 9 dB/s and slower never stop); a faster decay holds after a fixed lag in dB (Sensitivity 75: 1.8 dB at 15 to 45 dB/s, the stop at 346 / 231 / 176 / 120 / 63 / 36 ms for 10 / 11 / 12 / 15 / 30 / 60 dB/s); at Sensitivity 0 it holds after about 20 ms, at 25 after 20 to 40 ms, at 50 after 26 to 70 ms; the same at Slow and Fast and at 50 / 100 / 200 Hz | `followstop` probes: 100 Hz notes from silence decaying 6 to 60 dB/s (53 probes) | F and the margins fitted (section 3) |
| The lock moment | at the reference's lock the windowed note level has not yet fallen from its peak; the energy read is 0.3 to 0.5 dB under its peak | corpus notes | open: no fixed time, window, energy-window mean or follower on N or E explains the held values better than the model (median 0.28 dB) |
| After a deep drop | the new note's gain is limited by the energy follower still falling at 43 dB/s from the note before, so re-attacks soon after a drop read partway between the old gain and the new note's | probes | modelled |

## 3. The note detector

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Detector band | below 300 Hz the threshold is flat; above, it rises like a low-pass: +0.5 dB at 500 Hz, +1.9 at 700, +4.8 at 1 kHz, +8.0 at 1.4 kHz, +10.9 at 1.8 kHz (the hidden detector low-pass, 998 Hz); above about 1.85 kHz nothing is ridden (the hidden high-pass for out-of-band content, 2 kHz) | `gate` | a Butterworth-like second-order low-pass at 830 to 1020 Hz fits to 0.15 to 0.26 dB |
| Note level N | the peak of the low-passed input over the last 1024 / 2048 / 4096 samples, the window picked by pitch (the reference keeps a hidden pitch index and a power-of-two window length); no onset overshoot (the reference's R reaches a burst's true level within 6 ms at 50 Hz and never above it); R and S follow N | the release timer: the shortest dip that releases a note is 23.3 ms at 200 Hz to 1 kHz, 42.7 ms (2048 samples) at 80 to 140 Hz, 85.3 ms (4096) at 60 Hz, 96.9 ms at 40 Hz; a note ends about 30 ms after a 42 dB drop at 175 Hz to 1 kHz, 50 ms at 90 to 150 Hz, 93 ms at 50 to 80 Hz | measured |
| Window bands | 1024 samples over 172 Hz (175 Hz up; 165 Hz is 2048), 2048 over 78 Hz (86 to 165 Hz), 4096 at 74 Hz and under, intermediate 50 to 58 ms at 78 to 82 Hz; the model picks the shortest window spanning 3.3 periods of a zero-crossing period estimate, pinned from these timings; a zero crossing counts only when both halves of its cycle reach within 12 dB of N, so a stopped note's filter ringing does not shrink the window | the release timer, measured finely | pinned, not fitted |
| Slow envelope S | a peak follower: about a 20 ms rise, falling at most a few dB/s, tracking anything slower exactly | probes | fitted (constants below) |
| Long reference R | a peak follower; falls 1.74 dB/s after about 0.1 s: the hidden 5000 ms release, on amplitude | measured 0.3 to 6 s after a drop, two floors | fitted (constants below) |
| Start gate | a note starts when its level is over the Artifacts gain times (R plus the Spill level), as amplitudes; on the Live components, steady notes after silence are ridden from Spill - 8.69 dB peak at Artifacts 50 and Spill - 18.88 dB at Artifacts 0 (predicted -8.82 / -19.08: from silence R equals the note); the Studio components use the same gate with Spill fixed at -40, which gives the floor of -48.75 dBFS peak below 300 Hz; on Studio, Spill -50 to -3 changes nothing | steady 40 / 80 / 200 Hz notes after silence on Live Mono at Spill -40 / -25 / -10, 0.25 dB bisection (`gate`, `live`) | 0.1 dB from silence at Artifacts 0 and 50 |
| Artifacts gate | a re-attack is a new note only if it comes within about 2 / 6 / 10 / 14 / 18 dB of the earlier note's peak at Artifacts 100 / 75 / 50 / 25 / 0, whatever the floor it rises from (the hidden note-on gate, -20 to -3 dB, 50 at -11.5) | `artifacts` | measured |
| Release | before a re-attack counts, the level must dip under S by a depth for 20 to 30 ms (2 ms at Sensitivity 100); measured depths in the table below; the model's depth is set by Response only, Slow 12 dB and Fast 4 dB at every Sensitivity | `release`, probes | fit cost 2965 -> 2932 before the refit |
| New note while sounding | a rise over S by a threshold set by Sensitivity | probes | fitted (constants below) |
| New note on a decaying note | a rise of about 1.7 to 1.9 dB over a slow envelope falling at most about 8.7 dB/s (release about 1000 ms); Slow and Fast agree except after a deep decay (Fast takes +2.6 dB after 12 dB, Slow nothing up to +9); at 200 Hz a released note needs 3 to 4.5 dB at Sensitivity 0, 2 to 3 at 50, 1 to 2 at 100; the model reads the rise on N (`block_rise` 0) and gates it on the block level, the highest positive peak in each quarter-window block (`gate_block` 2), so a quiet note right after a gap, while N still holds the note before, stays out | 288 decay-and-step probes | fitted (constants below) |
| End of a note | the level stays 38 dB under its peak (the hidden note-off gate, -38 dB) for about 60 ms: a legato drop of 36 dB keeps the note, 38 ends it; a note at -30 dBFS that drops to -60 never ends; a 21 ms gap between notes does not end one; Response does not move this | `noteoff` | measured; open: in the reference a fading note ends about 42 dB under its peak, in the model 38 (about 60 ms earlier) |
| After a note ends | the gain glides to the Idle value (section 5); while it glides, any legal note within 38 dB of the last note's peak is a new note (no floor, no Artifacts gate): after a -20 dBFS note the gate stays at -57.6 dBFS for 300 ms, the glide's length, and is back to the Artifacts gate by 400 ms; an ignored note (outside the gates, within 38 dB of the last note) freezes the gain where it is | probes | measured |
| A held note following again | only within the gates (`hold_latch` 2) | the bass line at Artifacts 100 and 68.2 | Artifacts 100 -12.5 -> -37.6 dB, 68.2 -18.6 -> -24.8; corpus Artifacts 85, training parts -19.1 -> -20.5 |

Measured release depths (the dip before a re-attack counts):

| Response | Sensitivity 0 | 50 | 75 | 100 |
|---|---|---|---|---|
| Slow | 12 to 13 dB | 12 to 13 dB | 1 to 2 dB | under 1 dB |
| Fast | 4 to 5 dB | 1 to 2 dB | | under 1 dB |

The fitted constants (`measurements/fwbassleveler/detector_fit.json`, CMA-ES over probes, traces and the training
corpus, 25 generations, cost 2891):

| Constant | Value |
|---|---|
| S rise / release | 9.85 ms / 1351 ms |
| R release | 5678 ms |
| F rise / release | 10.4 ms / 345 ms |
| Attack-phase margin over F, Sensitivity 0 / 25 / 50 / 75 / 100 | +1.28 / +1.78 / +0.38 / 0.00 / -1.00 dB |
| Rise over S for a new note while sounding (`thr_u`), same steps | 1.58 / 0.33 / 1.39 / 3.27 / -0.83 dB |
| Rise of a released note (`on_rise`), same steps | 2.07 / 2.50 / 1.88 / 0.96 / 1.17 dB |
| Release depth | Slow 12 dB, Fast 4 dB |
| Detector timers (`rel_ms`, `settle`, `off_ms`, `off_rel_ms`, `win_ms`; their roles in `nulltest/brmodel.c`) | 1.32, 53.7, 26.6, 6.61, 1.15 ms |
| Fast re-ride band (`rr_lo1`, `rr_hi1`) | 36.8 to 49.3 Hz |
| Weighting (`sh_fc`, `sh_g`) | 30 Hz, -30 dB |

Method notes: some per-Sensitivity thresholds are not monotonic in Sensitivity where the probes cannot tell two paths
apart; a straight line in Sensitivity for the note-on threshold fits worse, so the per-Sensitivity tables stay fitted
(their mapping to the reference's threshold is not known).

## 4. Response Fast re-rides

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Fast against Slow | the same over most of a part; Fast adds a few re-rides as notes decay, and holds between them | the corpus | measured |
| Re-rides inside a smooth decay | only at 40 to 48 Hz (and 85 to 88 Hz at some rates and Sensitivities), never at 34 to 38, 50 to 80 or 90 to 198 Hz; at 40 to 44 Hz the re-ride comes exactly when the note is released (within +-7 ms of the model's release at Fast 0 / 50 / 100: the note 4.3 to 4.8 dB under S), 12 to 30 ms later at 46 to 48 Hz, 30 to 90 ms later at 85 to 88 Hz (5.0 to 5.6 dB under S) | 52 probes: sines decaying 10 to 45 dB/s at 34 to 198 Hz, Fast at Sensitivity 0 / 50 / 100 and Slow | measured |
| Legato onto a quieter note | ridden afresh at Fast only when the new note is at 40 to 45 or 80 to 90 Hz (from 60, 100 and 150 Hz alike; 35, 50 to 75 and 95 Hz up hold); at Slow the same once the drop passes the release depth (100 -> 85 Hz, 14 dB down); these are the bands just over each window boundary, where a period is longer than a quarter window | probes | measured |
| The band rule | at Fast and Sensitivity 50 and up, a held note in the band of section 3 is ridden afresh once it is 4.4 dB under S for the release time, released or not, and S restarts from the note; the band re-ride is not gated by Artifacts at 50 and under (at 41 Hz every drop re-rides, 20 dB under R too) and gated above 50 (`rr_band` 7) | probes | the bass line at Fast, Artifacts 68.2: -17.8 -> -24.6 dB |
| The 42 Hz note of the bass line at 5.15 s | ridden afresh at Fast 50 / 75 / 100 (the reference's gain to +12 dB), not at Fast 0 / 25, as the 20 dB/s probe at Fast 0 is not | the bass line | modelled |
| Decays of 10 dB/s | reach the depth only once the note is more than the Artifacts gap under R: no re-ride (the gate applies) | probes | modelled |
| Legato drops onto lower notes at Fast | from 100 Hz onto 50 to 78 Hz (the window doubles) a deep enough drop re-rides (10 dB at 78 Hz, 14 at 69, 20 at 50 / 62), always moving the gain +3.0 to 3.3 dB; from 55 Hz onto 50 to 78 Hz never; at 90 Hz, from both, +0.7 to 3 dB | 64 probes: a held note at 55 or 100 Hz, then one at 41 to 130 Hz, 6 to 20 dB lower | not modelled |
| Overlapping notes | when the louder of two overlapping notes stops, the quieter one left is ridden afresh if it is lower in pitch (100 -> 69 / 41, 55 -> 41, 78 -> 41 Hz) and not if higher; two overlapping notes beating (a 4 dB rise) are not a new note in the reference, though they are in the model | probes: A fading, B from 0.8 s at another pitch, -6 to +4 dB, A stopping at 1.6 s | not modelled |
| Events on the corpus at Fast | onset decisions right on 93 % of 616 notes; re-rides inside notes: the reference 74, the model 59, 24 coincide | `br_corpus_eval.py --events` | open |

Method notes: the rise of a released note was read on the block level before it was read on N; on the sampled basses
(not the 808, saw and swell voices) at Fast and Sensitivity 100 the false re-rides that block level produced carried
66 % of the held error, 9 % once read on N. Open: at 35 Hz the model re-rides and the reference does not; the band at
85 to 88 Hz; the reference's second re-ride in a long 42 Hz decay comes 20 to 40 % sooner than the model's (its slow
envelope seems to hold after a re-ride).

## 5. Timing, Idle and the start

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Look-ahead | Studio 42 ms: the gain starts to move exactly 42 ms before a louder part reaches the output; Live about 5 ms (4 to 5 ms before the step; reported 221 samples) | `steps`, `live` | exact |
| Gain smoothing | settles in about 20 ms (at 2 / 4 / 6 / 10 / 22 ms: 17 / 41 / 59 / 81 / 98 % of the way, in linear gain): the 2 ms energy attack in series with a 5 ms gain smoother (the hidden gain smoothing time) | `steps` | measured |
| Live | the same smoothing and the same note detection as Studio (a dip and re-attack gives the same new note) | `live` | measured |
| Idle glide | after a note ends the gain moves to the Idle value in a straight line at 32 dB/s, starting about 14 ms before the drop reaches the output | `idle` | measured |
| Idle and Range changes | the arrow keeps its place in the range (Range Max 12 -> 7.5 moves it from 0 to -2.2 dB); a sounding note keeps its gain whatever the arrow does; when the note ends, or in silence after a note, an Idle or Range change glides the gain to the new value at 32 dB/s, so the Rider follows the arrow as it is dragged; an ignored note holds the gain | films with `in=` audio; renders with Idle and Range changed between blocks | within 0.1 dB of the reference's glides |
| Before any note | the gain starts at 0 dB and, once the look-ahead has filled, glides to the Idle value at 32 dB/s and follows it while the input is silent; silence is the level read E under -77.7 dB (an 80 Hz tone at -72.75 dBFS holds the gain at 0 whatever Idle does, -73 lets it glide; 40 Hz glides at -72, 200 Hz holds at -74; Target and Spill change nothing); model options `pre_silence`, `pre_hold` | renders with a -54 dBFS 3 kHz pilot | within 0.1 dB of the reference's glides |
| Rider display | shows the gain being applied (within its 1 px reading, 0.1 dB); at a note's start the shown gain overshoots and settles within 0.1 s (the audio, 42 ms later, is already at the settled gain); after a note it waits about 50 ms and returns to the Idle value within about 0.1 s; a level change inside a held tone moves it only partly (-10 -> -30 dBFS: -1.5 -> +1.6 dB) | films of steady 80 Hz tones and a bass line (`read_br_film.py`) | 0.1 dB |
| Energy meter | reads a steady 80 Hz sine 1.9 dB under its rms (-20 -> -21.9), low notes lower (45 Hz about -6 dB, the weighting); rises at once, falls 37 dB per second at most | the same films | measured |
| Clip LED | latches at the first clip and stays lit until clicked | the same films | measured |

## 6. Learned blocks

None ships. A learned lock was tried (`nulltest/br_lock.py`; `brmodel.c` lock modes, off by default; data in
`measurements/fwbassleveler/lock_net.json`):

| Item | Value |
|---|---|
| Labels | on each note the model starts and the reference rides, the reference's lock is where the law last meets the value it then holds |
| Inputs | the detector state every attack millisecond, 19 inputs: time since the start, N and E against their peaks and followers, slopes, window, Sensitivity, Response, the rule's own test; the lock does not feed back into them, so they are exact |
| Ceiling | an oracle locking at the labelled moments, training corpus: default -23.6 -> -26.5 dB, Sensitivity 0 -23.3 -> -28.6, Artifacts and Target settings +1 to +3 dB; Fast, Fast 25 and Sensitivity 100 barely gain (there the reference rarely holds 30 ms between notes: the error is which notes are ridden, not when they lock) |
| Follow / hold network | 16 hidden units, every millisecond; 93.0 % of held-out milliseconds right against the rule's 87.8 %; after its first hold it follows again only where the rule does |

## 7. Declined changes

| Change | Measured effect |
|---|---|
| The follow / hold network in place of the rule | held-out corpus: default -24.7 -> -26.1 dB, Sensitivity 75 -21.6 -> -23.5, Artifacts 85 -21.1 -> -24.2, Target -24 -23.2 -> -24.3, Fast unchanged, Sensitivity 100 -22.0 -> -21.5, Fast 100 -23.7 -> -22.8; the bass line loses about 6 dB at Fast 0 / 25 (-30 -> -24): no clear win |
| A one-shot lock network (a hazard per millisecond) | worse than the rule everywhere: lock times miss by a median 10 to 15 ms, mean about 40 ms, and it cannot resume following |
| Refitting on the corpus (`br_fit.py` with weight `CW`, `br_polish.py globals`) | held-out corpus Fast 25 -18.4 -> -23.5 dB, Sensitivity 0 -24.4 -> -27.1, Sensitivity 100 -21.8 -> -22.7, the worst at Fast -6.1 -> -13.4; but the bass line at Fast -30 -> -16 (one release at 5.16 s whose outcome flips on tiny parameter changes) and 1 to 3 dB lost on the synthetic seeds |
| A pitch read on the input low-passed again (4 poles near 200 to 250 Hz, `pitch_lp`) | with a rise threshold for a released note, the corpus at Fast -14 -> -19 dB; kept as a model option, off in the plug-in |
| Hysteresis on the band switch (`band_hyst`) | synthetic part 7 -21 -> -27.5 dB at 2 %; synthetic part 6 unchanged; the bass line at Fast breaks from 3 % |
| Polish moves of the attack margins, the rises over S and the Slow depths (up to 1.5 dB; Slow 100's depth 6.2 -> 12.4) | 2 to 3 dB won on the training parts, 1 to 2 dB lost on held-out ones (Artifacts 85 -21.1 -> -18.9, Target -24 -23.2 -> -21.6, Fast 100 -1.3) |
| Reading the re-attack gate on N as well as the rise | 3 dB lost on synthetic part 6 |
| A straight line in Sensitivity for the note-on threshold | the fit got worse |

## 8. The model against the reference

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| C++ against the model | 0.08 dB at 99 % (also on a 42 Hz decay at Fast, a corpus part at Fast 25 and at Artifacts 100) | `nulltest/check_fwbassleveler_port.py` | |
| The plug-in against the reference | the fixed cases, the corpus, the rates, the components and stereo | [NULLTESTS.md](NULLTESTS.md) (`nulltest/nulltest_fwbassleveler.py`, `nulltest/validate_fwbassleveler.py`, through `BlRender`) | |
| Where the corpus error is | the level read is right (on 173 of 175 notes the reference's held value lies on the law's path over the attack); per 10 ms block, 66 to 77 % of the error energy is both sides holding more than 1 dB apart (typically 0.3 to 0.5 dB, 1 to 4 dB at the 90th percentile), and 81 to 99 % of that follows a different ride decision, not a different lock on the same ride; at Fast what remains is mostly re-rides the reference takes and the model does not (at onsets 49 %, inside notes 33 %) | `br_corpus_eval.py` | |
| Synthetic part 6 | turns on one note at 2.38 s, 13 dB over a quiet passage, which the reference takes as a new note; the gate's form is the reference's, the long reference R's exact fall is not | the fixed cases | open |
| Part 6 at Fast | -16 dB at every Sensitivity from one passage: a 77 Hz note fading after its release, where the pitch read wobbles over the 78 Hz band edge and the window switch reads as a rise (the reference holds) | the fixed cases | open |
| Notes right after a note-off | -15 to -20 dB at every setting (part 13) | fresh synthetic parts | open |

## 9. Factory presets

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Presets | the reference's 17, with Flatwater names (`nulltest/read_ref_presets_br.py`, `tools/make_presets.py` -> `plugins/FwBassLeveler/Source/Presets.h`); the 15 that store Target as a wildcard leave it where it is (`FactoryPreset::leave`) | each array loaded into the reference's own state and its parameters read back (Artifacts 77.1, 100, 85.3, 74.1, 60, 0 as converted) | exact positions |
| Null | each preset on the bass line and held-out corpus parts 30 and 25 | `nulltest/nulltest_fwbassleveler_presets.py`, [NULLTESTS-presets.md](NULLTESTS-presets.md) | median -25.2 dB, worst -9.0; the Fast presets on the bass line are weakest (the drops onto lower notes at Fast, not modelled) |

## 10. The editor

Measured on the reference's frames (the skin in the git-ignored `reference/fwbassleveler/`); every value below is a
measurement the redrawn editor uses, not a copied pixel.

| Quantity | Value |
|---|---|
| Layout | 320 x 560 including the two-row toolbar (36 px); the top panel y 36 to 162, x 6 to 313; the cream panel y 162 to 510, a trapezoid (x 5 to 314 at its top, 0 to 318 at its foot); both with a gentle shading and grain (rms about 3 and 2 levels), measured as a colour per row times a factor per column after removing the lines and wells |
| Scales | +12 dB at y 222, 0 at 327, -12 at 432 (8.75 px per dB) for the Rider, the Output and the Range column; the Rider's marks every 3 dB (numbered every 6, 2 px thick at +-12 and 0); five half-pixel minor ticks (grey 101) at 5, 9, 13, 17, 21 px below each 3 dB mark, not on a dB grid; the Output's 3 dB marks 6 px long (11 at +-12 and 0); numbers centred at x 117.5 / 202 / 291, Arial about 10 px |
| Faders | Range Max's cap ends at its value, Range Min's starts there; the lit band between them, a warm light (255, 253, 209) at 0.21, reaches the Rider's scale; the Idle arrow and a light red line sit at the Idle value; each cap has a 1 px dark outline and a dark lip under it (Rider x 133 to 186, y 294 to 362 at 0 dB; Range Max x 41 to 79, y 200 to 219 at +12; Output x 238 to 276, y 319 to 338 at 0), measured on the frame itself (`tools/fit_bassleveler_parts.py`) since the 2x skin reduced to 1x shifts the ridges by a row |
| Knobs | +-132 degrees of travel in 71 positions (0 at -132 degrees, 35 at -3, 70 at +131); centres x 46 / 200 / 275, y 111.5; the dome graded about 196 to 110 top to bottom, a black line at its edge, a bright arc at the top and a reflection near the bottom (a grey per radius and two angular harmonics); the pointer a 2 px core from radius 2.5 to 17.5 (grey 87 to 104); only the dome turns |
| Knob scales | Sensitivity: 12 dashes at +-(11.3 + 22.67 k) degrees, 16 degrees long and 4 px deep, from radius 20 at the top drifting out to 22.3 at the ends (grey 131, the last pair 100); Spill and Artifacts: 13 ticks at 0, +-23, +-46, +-69, +-91, +-115, +-135 degrees, each with its own length (radius 20.3 to 27.3) and grey (100 to 147) |
| Response switch, clip LED | the switch's block is identical in both states, 15 px apart, the slot's gradient mirrored (darkest by the block), and it flips in one frame; the LED is a dark 15 x 10 frame whose inside changes by row (off 55 to 87, lit red-orange 173 to 245 with a red glow beside and below) |
| Selection | hovering changes no pixel; a click marks the control in yellow (a ring on knobs, a frame on caps and the switch) until another control is clicked; the selected control's value shows while the mouse is on it (knobs in their value slot under the label; Range Max / Min in a dark box with a 1 px white frame right of the cap); the Response switch's click toggles it and marks the preset modified |
| Cursor | over Sensitivity, Artifacts and Output an arrow with a small up / down arrow (hotspot 6, 7), elsewhere the system arrow; hidden during every drag, it comes back where the thumb went |
| Drags | the faders and the Idle arrow 8.75 px per dB, the Target 4.31 px per dB, the knobs 0.1 per px on slow moves (the suite's knob feel); the Range box and the band do not drag |
| The Rider grabbed | it follows the mouse, outlined in blue (its own selected look, as the Idle arrow's); on release it returns to riding at once |
| Text entry | a double-click opens it on the knobs, Target, Output and Range Max / Min (in their value slots); a value outside the range is ignored (Target 25); Spill reads a typed positive number as negative (30 -> -30); the Rider, the Idle arrow and the Range box take none; a single click and typing does nothing |
| Idle | dragging the arrow shows its value in a box fixed at the left edge (3, 323); the host's text for Idle maps it over a fixed +-12 dB (0.5 shows 0.0 whatever the range), while the arrow sits at its place in the current range |
| Lasso | a background drag draws a 1 px white rectangle and selects the controls it touches (in the order Spill, Rider, Range Max, Range Min, Output, Sensitivity, Target, Idle, Response, Artifacts): the first yellow, the others blue; clicking a member makes it the yellow one; dragging a member moves nothing; with the mouse on the yellow one every member's value shows |

## Provenance

The reference is the original Bass Rider, run from a licensed install.

- Measured: about 1,200 probe signals through the reference's VST3 in a host and its riding on a corpus of 36 bass
  parts, its applied gain read from its output.
- Fitted or learned from the reference's output: the level read, the gain law, the note detector, the release,
  re-attack and Idle rules, identified on those measurements (`measurements/fwbassleveler/detector_fit.json`); no
  learned block ships.
- Redrawn: the editor, drawn in code in Flatwater's own dark look (`shared/ui/Theme.h`) on the original's
  measured layout (the positions and sizes of the controls), with our own colours and shading; no table of the
  original's pixels ships.
- Presets: the original's factory preset values under our own names and groups, the suite's standing rule; the
  preset files themselves are not copied.
- Shipped on recorded approval: none beyond the above.
- None of the original's code, artwork, preset files, fonts or other files ships with Flatwater Bass Leveler.
