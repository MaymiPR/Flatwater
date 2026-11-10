# Flatwater Pitch Bend: measurements

The current values of every stage of Flatwater Pitch Bend, measured on the original it recreates (Vocal Bender), with
the method and the residual or confidence of each. The data is in
[measurements/FwPitchBend/](../../measurements/FwPitchBend); the generated case tables are in
[NULLTESTS.md](NULLTESTS.md) and [the presets null test](NULLTESTS.md#presets).

## Setup

| Item | Value |
|---|---|
| Reference | Vocal Bender (Waves), the Mono and Stereo components, from a licensed install |
| Host | `tools/RefRender` through `plugins/FwPitchBend/nulltest/pb_ref.py`, a fresh instance per render; the settings only the editor sets are written into the reference's saved state (`plugins/FwPitchBend/nulltest/editor_state_lib.py`) |
| Rates | 48 kHz unless noted; 44.1 and 96 kHz checks |
| Hosting check | `plugins/FwPitchBend/nulltest/first_null.py`: latency 0 on both components; the host Bypass is the input (-282 dB); two renders of the same input and settings null to -280 dB (deterministic, so every stage compares render against render); our renderer equals the VST3 (-282 dB) |
| Modulator readout | Mix is an exact crossfade (section 1), so m(t), the Mix the reference applies, reads any modulator's output sample by sample: Pitch +12 for a wet far from the dry, noise input, 16-sample windows |
| Shifter battery | `plugins/FwPitchBend/nulltest/measure_shifter.py`: about 2,600 renders of the Mono component at Mix 100; per case in `measurements/FwPitchBend/shifter/SUMMARY.md` and its JSON files |
| Modulator battery | `plugins/FwPitchBend/nulltest/measure_mods.py` (`measurements/FwPitchBend/mods/`); the nulls `plugins/FwPitchBend/nulltest/null_mods.py` |
| Scorecard | `plugins/FwPitchBend/nulltest/check_shifter.py` (`measurements/FwPitchBend/shifter/ours.json`); thresholds M1 to M8 in `SUMMARY.md` |

A shifter does not null sample for sample once it shifts, so the shifter is judged on the scorecard (behaviour) and
the modulators, which are exact or fitted laws, are nulled through Mix: an exact law away from the jumps against
0.003 rms, a fitted one over the whole curve against 0.01.

## Stage classes

| Stage | Class | Target |
|---|---|---|
| Mix crossfade, Bypass | exact | the Mix law below -90 dB with the same wet |
| Modulation routing: slots, depth in the target's units, clip to the range | exact | modulated Mix within 0.002 of the reference's |
| LFO: 256-value cycle, rate, Phase, Warp, Level | exact | within 0.003 rms of the reference's curve |
| Sequencer | exact | within 0.003 |
| Modulator timing | exact | per sample, matched |
| Smooth (M1, M2, PT), AM follower | fittable | within 0.01 rms |
| PT follower: scale exact, detector timing fittable | exact and fittable | scale exact; timing <= the reference + 10 ms |
| No-shift path: the fractional delay and its interpolator | exact | below -55 dB on voice at 48 and 96 kHz |
| Pitch ratio, β, Flatten target | exact | scorecard M1, M2, M6 |
| Grain length, cross-fade, window; level per setting; detector timing and gate | fittable | scorecard M3, M4, M7 |
| Pitch marks and detector decisions on hard material (octave hysteresis, chords, pulse trains, narrow resonances), noise output | random or proprietary | behaviour only: scorecard M1b, M5, M8 |

## 1. Mix

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Mix law | an exact crossfade at the output: dry + m (wet - dry) with the same wet render, so the wet path does not depend on Mix | renders at several Mix values, m held over 64-sample windows | -54 dB (the window is the limit) |
| Mix 0 | the dry input | | -179 dB |
| Dry and wet at mid Mix | a comb: the wet is delayed (section 7) and no latency is reported; first notch near 227 Hz at Mix 50 | derived | exact |

## 2. Modulation routing and depth

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Sources | none, M1, M2, AM, PT; four slots per modulatable control | state written and read back | exact |
| Depth on Mix | in the target's own units, clipped at its range: on Mix (0 ... 100) at base 50, depth 25 swings 25 ... 75, depth 50 and more 0 ... 100; negative depth inverts | m(t) through Mix | exact |
| Depth on Pitch | in the knob's units: depth 25, 50, 75, 100 of a +1 source give +6.1, +12.0, +17.9, +24.0 st; the sum of the slots clips at +-24, twice the knob's range (two slots adding past +24 give +24; base +5 with +12 gives +17) | pitch of the output | exact at the clip |
| Depth on Formant | clips at the knob's +-12 (depth 75 and 100 both give +12.05) | the same | exact |
| Depth curve of a modulated Pitch | not quite linear: with constant sources +6.122 for depth 25 x 1 and 50 x 0.5 but +6.148 for 100 x 0.25; +12.000 for 50 x 1 and 100 x 0.5; +9.075 for 37.5 x 1 and 75 x 0.5, +9.096 for 100 x 0.375, +9.040 for 50 x 0.75; at depth 100 the excess rises to +15 cents near +5 st, is 0 at +12 and -20 cents near +19; two slots on one source add as one slot of their summed depth; static knob settings are exact | the same | open: no law fits every case; ours is linear (at most about 20 cents off) |
| Knobs against modulation | the output's pitch and the Formant part of the envelope follow the modulated totals; the envelope's share of Pitch (β) follows the Pitch knob only (knob -5 with +5 modulated: -2.0 st, pitch unshifted); the knobs also decide the level (section 9) | `plugins/FwPitchBend/nulltest/measure_control.py`, `shifter/control.json` | the envelope within 0.5 st on 38 of 43 cells |

## 3. M1 and M2: LFO and sequencer

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Shape | one cycle of 256 values (the defaults: a sine for M1, a triangle for M2), each value held for its 256th of the cycle, not interpolated | synced sine curves | held 0.0001 ... 0.0004 rms against 0.0013 ... 0.0030 interpolated; the table read at 256 per cycle follows to 0.006 rms with the default Smooth 1.5 |
| Synced rate | exact: 1/32, 1/8T, 1/8, 1/4, 1/2, 1 bar at 90 and 120 bpm give 0.1250, 0.3333, 0.5000, 1.0000, 2.0000, 3.99 beats per cycle | m(t) through Mix | exact |
| Free rate | the period is a whole number of 8-sample ticks at 48 kHz, rounded down: 3.7 Hz runs at 12968 samples (3.7014 Hz), 7.3 Hz at 6568 (7.3082 Hz), 21 Hz at 2280 (21.053 Hz), 13.1 Hz at 3664, 1 Hz at 48000 | the same | exact |
| Rate in Hz with only Hz display | snapped to the synced length nearest in time at the host's tempo (what the host's Sync On/Off parameter sets, and every factory preset's state): 3.7 Hz at 100 bpm runs at 3.333 Hz; at 120 bpm 2.27 Hz runs at 1 beat and 2.30 Hz at 3/4 beat (the period midpoint is 2.286 Hz); 5.003 Hz runs at 0.375 beat at 120 bpm, 1/3 beat at 90 and 1/2 beat at 140; the reference rewrites the stored note to the snapped one | the same | exact |
| No tempo from the host | the free and the synced rates both run at 22.37 Hz | the same | measured (a host that always plays with a tempo does not meet this) |
| Phase | synced, the cycle is locked to the host's song position (Phase 0 starts the cycle on the beat; 0.25 and 0.5 add a quarter and a half cycle); free, it counts from the start of processing (0.7 Hz after a 1 s pre-roll: 0.7616, 0.0424, 0.3906 at 0.1, 0.5, 1 s against 0.77, 0.05, 0.40) | the same | exact |
| Timing | per sample, the cycle late by 0.1 ms synced and 8.1 ms free | the same | exact |
| Warp | maps the phase within a cycle as φ' = w φ / (1 + (w - 1) φ) | a ramp table at Warp 0.1, 0.5, 2, 10, nine points per cycle (Warp 0.1 at φ 0.489: 0.0873 predicted, 0.086 measured; Warp 10 at 0.089: 0.494 and 0.498) | within 0.004 |
| Level | scales the modulator by L / (2 - L): Level 0.5 gives 1/3 (0.3333 ... 0.6667 on base 50 depth 50), 0.25 gives 1/7 (0.4286 ... 0.5714) | the same | exact |
| Smooth | a one-pole low-pass with a time constant of Smooth ms, starting from 0 (Smooth 1000 still shows the start after the 1 s pre-roll: 0.052 to 0.0008) | a square table at 0.5 Hz: about 10 ms at Smooth 1, 20 ... 50 ms at 10, a time constant near 97 ms at 100, no settling at 1000 | Smooth 100 on a square 0.0002 rms |
| Sequencer | step values are whole steps -24 ... +24, read as value / 24; one step lasts one Rate period, counted from the start of processing; each value is held (4 steps 0.5, -0.25, 1, -1 read back 0.7500, 0.3744, 1.0000, 0.0000 at base 50 depth 50; 16 steps follow their values in order) | the same | exact |

Method notes: read through 16-sample windows, the modulators seem to lag by about a host block plus 1 ms (12 ms at
block 512, 6.4 ms at 256, 0.7 Hz) and Smooth 1 seems to turn a square table's step within one block; that reading is
the held values and the fixed delay above seen through the window. Per sample with the fixed delay the curves null.

## 4. AM follower

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Linearity | follows the input's amplitude linearly: 1 kHz bursts at -40, -20, -6 dBFS give 0.0165, 0.165, 0.827 at depth 100 (factors 10.0 and 5.01, as the amplitudes) | bursts | exact |
| Read | the input's magnitude once every 8 samples, counted from the start of processing: a 3 kHz sine at 48 kHz, read at its zero crossings, gives almost nothing; 1 kHz and 8 kHz read 0.866 of their peak, 100 and 300 Hz all of it | tones | exact |
| Ballistics | one-poles: attack time constant Attack ms, release Release + Attack / 85 ms; the value is twice the result, times Level (Attack 1 ms reads 5 % higher than 10 ms at the end of a 0.5 s burst; Attack 100 / Release 10 reads a fifth) | sines whose level steps but never stops (bursts leave the release unread): Attack / Release 10 / 100 and 30 / 300 fit at 1.000 and 1.002 of the nominal times; the Attack / 85 term fitted on 100 / 10 and 1000 / 1, held on 500 / 2, 1000 / 20, 300 / 1, 50 / 1 and 200 / 50 | 0.0006 ... 0.0030 rms |
| Gate | after 100 ... 120 ms (110 used) with the input under 5e-4 ... 6e-4 (5.5e-4 used, about -65 dBFS) the follower reads nothing and keeps releasing, until about 92 samples (1.92 ms) after the input returns | gaps of 2 ms ... 0.3 s, floors of 3e-5 ... 1e-3 | within target |
| On real voice | sung and spoken, depth -26 and 100 | through Mix | 0.0003 ... 0.0021 rms |

## 5. PT follower

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Scale | log2 (f / 261.63 Hz) / 2 (+-1 = +-2 octaves about C4; a free fit gives 0.496 per octave about 262.4 Hz): sines at 110, 165, 220, 330, 440 Hz read -0.631, -0.325, -0.117, +0.157, +0.369 against -0.625, -0.333, -0.125, +0.167, +0.375 | sines, Smooth 1 | within 0.010 |
| Offset | adds to the scale (0.5 at depth 50: +0.25 on Mix) | the same | exact |
| Detector | our own detector; on speech it holds its last pitch where its aperiodicity is over 0.15, as the reference holds through unclear stretches (octave disagreements on a spoken take 3.2 % up and 1.6 % down of the voiced frames without the hold, 0.4 % and 0.8 % with it; the spoken take's rms 0.062 without, 0.031 with) | a glide 80 Hz ... 1 kHz, a sung and a spoken take | glide and sung take 0.005 ... 0.006 rms; the spoken take reported |
| Reference quirks not copied | the reference holds the lower octave on an octave jump (220 to 440 Hz takes it 182 ms; it reads 880 Hz as 440); its readings sit about +-1 % off the scale (110 Hz reads as 109.1, 220 as 222.2: a period counted in whole samples at a reduced rate) | | measured |
| Speech | on a phrase's falling, creaky end the reference's PT goes 150 ... 500 cents lower than ours; on voiced speech elsewhere the two differ by a median 18 cents | the spoken take | open: a detector's call on rough voice |

## 6. Modulation of the modulators' own controls

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Scaling law | the controls that scale are multiplied by ((a + s) / (a - s))^p of the slots' sum s (depth / 100 x source), clipped to the control's range | M2 a constant +1 or a two-step +1 / -1 sequence on a control, read through M1 (or AM, PT) on Mix; 19 cases | 0.0001 ... 0.0036 rms |
| Warp | a 1, p 1 (s 0.1 gives x1.221, 0.5 gives x3.0; s = +-1 reaches 0.01 and 100) | the same | exact |
| AM and PT Level | a 0.5, p 1 (s 0.1 gives x1.5, -0.25 gives x1/3, -0.5 gives 0) | the same | fitted |
| AM Attack and Release | a 1, p 1 | the same | fitted |
| Rate | a 1.5, p 3.905 (s 0.05 gives x1.30, 0.1 x1.68, 0.25 x3.72, 0.5 x15.6, clipped at 30 Hz); the same on a synced rate (1 beat at 120 bpm with s 0.1 runs at 3.37 Hz, not at a synced length); independent of the base rate (x1.6845 at 0.2, 1 and 4 Hz) | the same | exact |
| Phase, Level (M1, M2) | Phase adds and wraps (-0.25 is 0.75); Level adds | the same | exact |
| Timing | a modulator's own controls follow their sources sample by sample (stepped modulation lands without a block's lag); a free rate's count starts its delay late rather than subtracting it, so a moving rate does not jump the cycle; the free period is made whole ticks before the modulation scales it (1 Hz x 1.6845 runs at 1.6843 Hz) | the same | exact |
| Pitch and Formant | follow their slots sample by sample: on a 220 Hz sine the reference's output frequency crosses a step's midpoint 2.9 ms after the step (spread 0.8 ms, the same at every place in the host block, free or synced), on a vowel 1.4 ms after; ours runs the shifter in steps of 16 samples at the totals there: 2.8 ms on the sine, 3.4 on the vowel (an output period is decided as its grain starts, a period ahead); a 2 Hz LFO on Pitch lines up with no lag | `plugins/FwPitchBend/nulltest/measure_modpitch.py`: a 0 / +6 st sequence, 250 ms steps, at all 16 places in the host block | measured |

Method notes: a pitch tracker's crossings on a vowel suggest a rule of one block late; that is the tracker's own lag.
The case table is generated in [NULLTESTS.md](NULLTESTS.md), the modulators section.

## 7. The shifter: no-shift path

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| No shift | a fixed fractional delay of 105.531 samples at 48 kHz (2.199 ms; 211.063 at 96 kHz, 120.438 at 44.1 kHz), the same for every f0, material and level; 105.53125 is the best 1/32-sample delay on a vibrato vowel | the shifter battery | -57 ... -68 dB on voice after it; scorecard M4 delay error 0.0002 samples |
| Silence | comes out as exactly 0 | the same | exact |
| Grain latch | at a total of 0 from the start the output is the pass-through bit for bit; once Pitch has moved (by the knob, +3 and back to 0, or by modulation, a +3 step among 0 steps) it never returns: 4.5 ... 7 s later the residual against the pass-through is still +1.3 ... +2.2 dB, at the input's level (0.00 dB on a vowel, +0.01 on the sung take); ours keeps its grains running once they have run | the same | the latched level matches (-0.00 and +0.03 dB) |
| Reset on a stopped input | once the input has played, a run of digital silence or of any constant (a DC of 1e-5 ... 1e-3 too, but not a sine of 1e-9, nor a voice at -80 dBFS) of about 22 ms stops the grains and the latch; from then on the pass-through is the input exactly 39 samples late (residual -143 dB) where before it is 105.53 samples late. The run: 1030 samples at 48 kHz from a place 0 mod 8 of the processing (1029 does not reset), 1029 from 3 mod 8, 976 ... 980 at 44.1 kHz, 2101 ... 2400 at 96; the delay 39 samples at 44.1 and 48 kHz, 78 at 96. The pre-roll's silence before anything played does not count; every phrase gap of the takes (0.25 s of zeros) and the silence between a DAW's regions resets it | `plugins/FwPitchBend/nulltest/measure_modpitch.py silence` | ours: the run 1024 samples at 48 kHz on an 8-sample grid, scaled with the rate; the delay 39 x the rate's multiple of 48 kHz |

## 8. The shifter: pitch, formant and grains

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Pitch ratio on steady input | exact: 147 vowel cases (80 ... 600 Hz, -12 ... +12) within +-0.41 c median, p90 <= 1.01 c, no gross errors; cents, 44.1 and 96 kHz within +-0.13 c; sung phrases p90 6 c | the shifter battery | measured |
| Settling | shifted onsets settle in 15 ... 41 ms (slower at lower f0), 3-semitone jumps in 29 ... 40 ms; an octave-down jump (400 to 200 Hz) never recovers (the period found first is kept) | the same | measured |
| Formant | moves the spectral envelope by its setting | vowels | within 0.07 st median |
| Pitch's share of the envelope | β = clip(0.418 P, +-2.51) semitones; Formant and β add (F + β); the grain time scale F + β stops at +-12 semitones (+12 / +12 acts as 12, not 14.5: the tone difference on the Tiny Voice preset 3.9 dB without the stop, 1.8 dB with it) | 16 settings | 0.012 st rms |
| Structure | a pitch-synchronous overlap-add whose grains are time-scaled by F + β, about 1.4 input periods long, cross-faded over about 0.7 period; the grain placement depends on the signal (a 0.02 click moved it in 38 of 80 probes at -12) | click probes on a steady vowel | inferred |
| Grain read point | each grain is read a set delay behind its centre on a lattice one input period apart, the grains spaced by the input period there (from the marks' spacings), so at no shift the output is the input delayed; the delay 0.3 ms (more under a time scale below 1, what the half grain drifts), from Formant +2 one input period and 1.2 periods per unit of time scale over 1 (not for the time scale Pitch alone moves); shifted (any Pitch, Flatten, or the time scale away from 1) the read point is the nearest mark (off it, neighbouring grains put one pulse into the output twice and the envelope ripples: M2's envelope error goes to +5.5 dB) | vibrato and pulse-train probes (section 10) | within 2 ms of the reference on 27 of 34 grid settings |
| A voice's first grain | moves to the mark the delay before it (its fade-in shorter, down to half a grain); the level loop waits for 20 ms of voiced output after the gate opens before it corrects (with the shorter fade-in it overshoots by up to 10 dB without the wait) | onsets | scorecard onsets 0 ms over the reference; Flatten's lock 49 ms (the reference 54) |
| Flatten | note 0 is 130.81 Hz (MIDI 48: the editor's C2 is the C an octave below middle C, middle C being its C3); steady input lands within +-0.16 c, the lock takes 26 ... 54 ms; Pitch adds on top | the shifter battery | measured |
| Flatten's grains | four output periods long, still no more than one input period each side: on the spoken take flattened to C3 (261.6 Hz) one output period against the next correlates 0.952 (the reference 0.95 median) and 1.5 % of the voiced frames part by more than 50 cents; tone and level unchanged (2.3 dB, +0.1 dB) | period correlation, a pitch tracker | measured |
| Stereo | both sides share one pitch analysis on the reference too (a quieter second voice on the other side is moved by the first's pitch: 220 Hz at -12 dB on the right comes out at 65.5 Hz on both); two different voices at the same level are handled as a chord, neither shifted cleanly | stereo probes | measured, not tuned further |
| Block size | every path runs per sample: blocks of 1, 512 and 4096 give the same output as 64 | renders | bit for bit |

## 9. The shifter: level

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Input level | the output level does not depend on the input's level | -40 ... -6 dBFS | <= 0.3 dB |
| Per setting | moves -3.5 ... +1.8 dB with the setting; noise -0.6 dB | the shifter battery | measured |
| Per material | within a setting the level follows the input's peakiness (kurtosis over 20 ms windows, energy weighted): a straight line per setting in ln kurtosis, -1 ... -2.7 dB per unit, a peakier voice coming out quieter; the spread across materials, 0.76 dB rms per setting (median), leaves 0.40 after the fit (f0 as a second variable takes off at most 0.13 more); the grid replaces an additive Pitch + Formant law (off by up to about 2 dB in combination: +12 / +12 is -3.4 dB on the reference) | `plugins/FwPitchBend/nulltest/measure_level_grid.py`, `shifter/levelgrid.json`, `SUMMARY.md` section 8: a 13 x 11 Pitch x Formant grid (142 settings) over 20 materials (vowels 80 ... 300 Hz, two bandwidths, sung and spoken phrases) | scorecard M3 140 of 143 cells |
| Near no shift | the reference's level rises from 0 dB at no shift through +0.16, +0.47, +0.94, +1.9 and +2.9 dB at 0.02, 0.05, 0.1, 0.25 and 0.5 semitone to about +3.5 at +-1 (both signs alike); ours applies its +-1 level times tanh(abs(shift) / 0.36 st) | a vowel, free mode, every knob at 0 | measured |
| Under Flatten and past +-12 | the level target follows the shift each grain really makes (under Flatten, the note against the voice's pitch), with a Flatten level model fitted from -36 to +30 st of real shift and the Pitch table carried to +-24; a high sung voice flattened to C2 comes out within 0.1 ... 0.8 dB of the reference (-5.1 ... -7.9 dB) | `plugins/FwPitchBend/nulltest/measure_level_flatten.py`, `shifter/level_flatten.json` | 188 of 220 judged cells within 1 dB |
| With the knobs at 0 | with every knob at 0 (and no Flatten) modulated Pitch is not level-corrected (+1 ... +5 dB, its own measured table) and noise passes as the delayed input; with a knob set, the Pitch table applies at the total; the shifter takes both the knobs' values and the totals | `plugins/FwPitchBend/nulltest/measure_control.py`, `shifter/control.json` | 301 of 331 judged cells within 1 dB |
| Not modelled | a 4.5 ... 7 dB dip of the reference at +12 with Formant -3 ... -5 on 300 Hz voices only, and a -3 dB ridge where F + β equals Pitch, both only at the grid points | the level grid | open |

## 10. The shifter: a moving pitch

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Reference lag | about 1.5 ms behind the input's pitch at Formant -12 ... +1; from Formant +2 one input period more, rising with the time scale (+6: 12 ms at 120 Hz, 7 at 200; +12: 19.5 and 10.5); with Pitch set, 1 ... 7 ms at 200 Hz (11 at 120, Pitch +12); it keeps the vibrato's full swing (rms 1 ... 7 cents) | `plugins/FwPitchBend/nulltest/measure_modpitch.py vibrato`: a vowel with 100 cents of vibrato at 5 Hz, static settings, Fine on; a pulse train with the same vibrato shows the same lags in its output pulses, so they are the grains' spacing | measured |
| Ours | the read point and spacing of section 8; on the grid (Formant -12 ... +12, Pitch -12 ... +12, 120 and 200 Hz) within 2 ms of the reference on 27 of 34 settings (every one at 200 Hz), tracking as closely or closer on Formant, 1 ... 2 times its rms on Pitch; at Formant -12 ... +1 within 1 ms | the same | one note's delay is set once at its first grain, so at Formant +4 ... +12 and Pitch +12 ours lies 1 ... 4 ms off, as far as the reference's own 120 and 200 Hz lags are from one rule |

## 11. Sample nulls with a shift

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Grain centres | on the voice's pulses (about 21 samples after each source pulse on the vowel, where its waveform peaks) | click probes on steady vowels and pulse trains at 150 and 200 Hz (each input instant's copies in the output) | measured |
| Read delay | the no-shift delay (105.53 samples) less F / 12 of an input period behind the grain's centre (Formant -6 ... +1 at both pitches, to +2.5 at 150 Hz; at a time scale of 1, the no-shift delay itself), locked to the input's pulses (the same for a note started 0 ... 160 samples later); past that the reference reads further back by an amount that depends on the voice (+0.58 ... +0.68 periods at 150 Hz from Formant +3, one period at 200 Hz from +2; a pulse train and a vowel differ by a period at +6); ours reads the same delay up to Formant +1.5, a voice's first grain moved to it | the same | within 1 ... 7 samples |
| Steady 150 Hz vowel | raw null -27 dB at a time scale of 1, -13 ... -19 dB at Formant +-0.5 ... +2 (200 Hz: -31, -8 ... -9); -26 ... -47 dB once ours is moved by a fraction of a sample and matched in level (the reference is 1 ... 1.5 dB louder at Formant +-0.5 ... +-1, a bump our level table does not have) | the same | [NULLTESTS.md](NULLTESTS.md) |
| A voice | on the sung take, even at Formant +0.01, the reference's output against its own input delayed is +0.3 dB, no sample relation (on the vowel that setting gives -27 dB): it resynthesises the voice from its own period estimates, grain by grain, and the voiced part is 99.8 % of the energy; ours against it is about -7 dB per 100 ms window at best alignment (the best tenth -25 ... -34) | the sung take | a null on a voice needs the reference's period estimates to a fraction of a sample on every period, which measurement from outside does not reach |

## 12. Hard material (behaviour)

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Noise | re-granulated (uncorrelated, its spectrum moved by F + β, not by the pitch) | the shifter battery | scorecard M5 |
| Chords | mostly left unshifted (shifted share 0.17 ... 0.24 on a triad, the reference's 0.18 ... 0.21) | the same | scorecard M8 |
| Gate | nothing is shifted below about -51 dBFS rms; the shift holds to 10 dB SNR | the same | scorecard M7 |
| Pulse trains and single narrow resonances | mistracked on the reference | the same | behaviour only |

The scorecard rows, the reference's numbers and ours are generated in [NULLTESTS.md](NULLTESTS.md). The M3 miss is
the cells where the reference's own level is more than 1 dB from its median for that setting (/a/ and /u/ at +5, /a/
at -7, +5 / -5, F -7, /i/ at F -12), which one level per setting cannot meet. M4's 150 to 160 Hz jump at +5 passes
since the grains are spaced by the period where they read.

## Deliberate differences

| Behaviour of the reference | Flatwater Pitch Bend |
|---|---|
| The factory presets' step sequences | kept as preset values |
| The modulators' timing | matched (section 3), so modulated Mix nulls |
| The 2.2 ms wet delay with its comb at mid Mix; chords left mostly unshifted | copied |
| The pitch detector's octave-down hysteresis (an octave drop 400 to 200 Hz never followed) | not copied: ours follows the drop in 26 ... 28 ms |
| The +1.8 dB on noise at Pitch +-1 | not copied: ours -0.2 ... -1.1 dB (noise correlation with the delayed input <= 0.07) |
| A rate in Hz with Sync off snapped to a synced length | not copied: Sync off is a free rate in Hz; the factory presets carry the length the reference plays at 120 bpm as a synced rate (`plugins/FwPitchBend/scripts/make_presets.py`) |
| The original's drawn LFO shapes | not shipped: presets that use one get the nearest of our own shapes (shape substituted) |

## Declined changes

| Change | Measured effect |
|---|---|
| Flatten's longer grains (four output periods) everywhere | the sung phrases' worst gross error rises to 0.32 (limit 0.15) and the onset settling by 9 ms; kept under Flatten only |

## Factory presets

`plugins/FwPitchBend/nulltest/null_presets.py` (`measurements/FwPitchBend/presets_null.json`): 62 of the 80 preset and take pairs
meet every target (level, pitch, tone); 67 of 80 with the reference's own shapes loaded into ours. The median pitch
difference is 2.3 cents on the sung take and 6.5 on the spoken one; the median sample null on the sung take is +1.5 dB
(no cancellation on a voice, section 11). Where the pairs miss:

| Cause | Presets | Measured |
|---|---|---|
| The spoken take's pitch track | Vinyl Scrub, Glide Robot, Pitch Follower Down (AM or PT moving the pitch) | 11 ... 25 % of frames more than 50 cents apart against a 10 % target; PT on Pitch at depth 25 alone passes (3 % gross): the difference comes with Flatten or the full depth |
| Shape substituted | Wobble | passes on both takes with the reference's table loaded into ours |
| Blends at Mix about 35 % with AM or PT | Bass Backing, Tracked Double Low | the pitch column measures which of the two voices a tracker hears; with the AM slot removed both sides pass, and AM itself matches on that voice to 0.0003 rms |
| Fast sweeps | Arcade Steps | where the tracks part, they part at whole-harmonic ratios, the tracker reading a harmonic of one output |

Every pair: [the presets null test](NULLTESTS.md#presets).

## Numerics on every platform

The shifter decides on near-equal values (voicing, its marks, which period a grain reads), so a unit in the last place
from a C library's exp, log or sin could move a grain on another system. Its transcendental functions and the
modulators' come from `plugins/FwPitchBend/Source/dsp/Math.h` (on `shared/dsp/Math.h`, with sin and cos of its own), which give the
same bits wherever doubles round to nearest. The knobs' exponential tapers (Rate, Warp, Smooth, AM's times, PT's
Smooth) round their value to 1e-5, so a value set comes back as itself (in double precision a free rate set to 4 Hz
otherwise comes back a hair over and runs a whole 8-sample tick short, which fails four modulator null cases). The
render regression has 20 cases (`tests/regress/FwPitchBend/jobs.txt`: no shift, Pitch and Formant, Fine, Mix,
Flatten, noise, each modulator on a control, three presets, 44.1 and 96 kHz), run on Linux, Windows and macOS.

## CPU

| Configuration | Cost |
|---|---|
| Stereo at 48 kHz | about 1 % of one core |

## Open

- The depth curve of a modulated Pitch (section 2); M3's last 3 cells; the level dips and ridges (section 9); PT on
  speech (section 5).
- Where a shifted output's grains sit after a reset: the reference's first onset after silence differs from its later
  ones (at Pitch +5, 152 samples, then 184 ... 262; ours 102 throughout), which sets how dry and wet interfere in a
  blend.
- Noise when a knob is set and its total is back at 0: the reference re-granulates to +2 ... +3 dB (as the +1.8 dB at
  Pitch +-1, not copied).
- Undo after Default Preset brings back the parameters but not the slots, shapes and Fine.

## Provenance

The reference is the original named in Setup, run from a licensed install.

- Measured: the reference's VST3 in a host (its audio through `tools/RefRender`), its settings read from and written
  to its saved state, its factory presets read in our terms, its editor captured on screen (kept in the git-ignored
  `reference/` folder).
- Fitted from the reference's output: the constants of our own methods (a YIN detector, pitch-synchronous overlap-add,
  the modulators by their laws): the no-shift delay, β, the level per setting and per material, the gate, the
  modulators' timing and ranges.
- Redrawn: the editor, drawn in code in Flatwater's own look (`shared/ui/Theme.h`) on the original's measured
  layout (the positions and sizes of the controls, the knobs' angle ranges, the drag speeds); our own fluted caps, arcs,
  icons and lettering; no table of the original's pixels ships. The modulators' shape library is our own (shapes by
  their math and our own step patterns).
- Presets: the original's factory preset values under our own names, the suite's standing rule; the preset files
  themselves are not copied. Presets that use one of the original's drawn shapes get one of our own shapes (shape
  substituted).
- Shipped on recorded approval: none beyond the above.
- None of the original's code, artwork, preset files, shapes, fonts or other files ships with Flatwater Pitch Bend.
