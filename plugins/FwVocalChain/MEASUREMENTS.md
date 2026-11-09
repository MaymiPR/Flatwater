# Flatwater Vocal Chain: measurements

The current values of every stage of Flatwater Vocal Chain, measured on the original it recreates (CLA Vocals), with
the method and the residual or confidence of each. The data is in [measurements/](../../measurements); the generated
case tables are in [NULLTESTS.md](NULLTESTS.md) and [NULLTESTS-presets.md](NULLTESTS-presets.md).

## Setup

| Item | Value |
|---|---|
| Reference | CLA Vocals (Waves), the Stereo and Mono/Stereo components, from a licensed install |
| Hosts | `plugins/FwVocalChain/nulltest/cv.py` (pedalboard, aligned for the 193-sample latency) and `tools/RefRender` (a host tempo, the Mono/Stereo component, changes during playback) |
| Probes | `plugins/FwVocalChain/nulltest/measure_stages.py` (each finding names its subcommand); the campaign `plugins/FwVocalChain/nulltest/measure_campaign.py` (pedalboard, 12 parallel instances, about 1,500 renders) and `plugins/FwVocalChain/nulltest/measure_campaign_rr.py` (RefRender); `plugins/FwVocalChain/nulltest/analyse_campaign.py` prints every table; data in the git-ignored `reference/fwvocalchain/meas/` |
| Rate | 48 kHz unless noted; the Stereo component unless noted |
| Reaching a stage | the per-stage bypass and colour settings cannot be written into the reference's state (it reads them back as 0); the colours and faders work as host parameters, so each stage is measured against the chain with every colour clear, where the wet stages are muted and the others hold their fixed settings |
| Noise | the Noise setting does take in the state; it is off in every measurement but its own and in every null (two noise sources cannot null) |
| Determinism | two renders of the same input null to -200 dB, the reverb, delays and doubler included, so every stage can be nulled |
| Interactions | scored against the stages acting independently: the product of their responses for the linear stages, the sum of their contributions for the wet stages |
| Nulls | `plugins/FwVocalChain/nulltest/nulltest.py`: our processor through `plugins/FwVocalChain/tools/ChRender`, the reference through `plugins/FwVocalChain/nulltest/cv.py`; held-out vocal parts of the corpus (`scripts/nulltest/vocal_corpus.py`) at -16 dBFS peak for the linear stages (under the dynamics' thresholds) and at -6 dBFS peaks after 6 s of silence for the whole chain |

## 1. Input: Sensitivity and Balance

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Sensitivity | an exact gain before every stage (+-5, +-10 give +-5.00, +-10.00 dB at a linear level); it moves every dynamic stage's threshold by its own dB (the de-esser and Compress curves shift by exactly the fader's value) and the wet sends with it | `gains`, `single`, `levels` | exact; on the delays +9.36 dB at +10 (their input is limited at that level) |
| Balance | turns down the far channel only, exactly the value: -6 gives L 0, R -6.00 dB; +3 gives L -3.00, R 0 (-6.0 ... +6.0 in 0.5 steps) | `gains`, `single` | exact |
| Balance into the wet stages | the reverb and the delays take the mono sum after Balance (Balance +-3 gives -1.28 dB on both sides, +-6 gives -2.08); the doubler stays per side (-6 gives R -6.00, L 0) | `pairs` | exact |
| Sensitivity LED | the LED's frame (of 24) is round(23 (Y + 144) / 144), Y the LED's level curve (X -144 / -20 / -12 / -4 / 0 dB to Y -144 / -109.56 / -72 / -34.43 / 0; the stored defaults are not the live ones) applied piecewise linearly to the input's rms level (a sine's peak -3 dB); it follows within a frame and falls to off within about 60 ms of silence | `tools/MeterCapture gl=1`, a 1 kHz tone stepping -40 ... +8 dBFS | every step matches |

## 2. The fixed path (every colour clear)

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Linear part | L = R; the impulse response is mostly polarity-inverted (taps +0.278, -0.721, -0.130, then a positive tail decaying by about 0.78 per sample), +0.55 dB at DC; the magnitude +0.54 dB at 30 Hz, +0.37 at 1 kHz, -0.12 at 10 kHz, -0.90 at 16 kHz, -1.71 at 20 kHz | `base` | measured |
| Model | an order-8 IIR per rate (`plugins/FwVocalChain/nulltest/fit_fixed.py`); its two all-passes (about 1.95 and 6.4 kHz) are analog-defined, a section near the top of the band is not, so it is kept per rate | the reference's impulse responses | -85 ... -92 dB (order 6 leaves -55 dB below 120 Hz) |
| Static distortion | the 2nd harmonic rises 10 dB per 10 dB (-82.5 dB at -20 dBFS), the 3rd 20 dB per 10 dB; placed before the fixed filter: y = F(x - 0.00145 x² + 0.00146 x³) | a regression on tones and a vocal | the clear null -75.4 dB without it, -76.5 dB with it; tone cases up to 12 dB better |
| Stereo | the dry path keeps L and R apart (nothing from L reaches R with every colour clear) | `stereo` | -240 dB |
| Mono/Stereo component | the Stereo component fed the same signal on both inputs: L and R identical with every colour clear | `monostereo` (RefRender) | identical to the noise floor (-74 dB) for every colour at fader +5, the defaults and clear, except Pitch Wide, whose R voice differs (-1.6 dB residual) |
| What stays in the path | the two EQs and both de-essers stay in the path with every colour clear (the fixed filter and the high-frequency compression) | the chain's own presets, checked against the audio | measured |

## 3. Bass and Treble

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Bass Sub / Lower | a low shelf / bump at about 50 to 100 Hz (Lower reaching higher): fader +10 gives +11.8 / +12.6 dB at 50 Hz, -10 gives -11.2 / -11.8; plus a subsonic high-pass (-2.2 dB at 30 Hz) at every setting, fader 0 included | `eq`: each colour and fader -10 ... +10 against every colour clear, -40 dBFS noise; full curves in `eq.npz` | measured |
| Bass Upper | at 0 and above a steeper high-pass (-10 dB at 30 Hz, -2.9 at 50), switched in at 0 (30 Hz: -1.16 dB at -1, -9.21 at 0); boosts peak near 100 Hz (+10.1 at fader +10) with a -1.3 dB dip at 1 kHz; cuts are a broad low shelf (-11.7 at 30 to 100 Hz) with a +1.4 dB bump at 1 kHz | the same, `single` | measured |
| Treble Bite | a bell at about 5 kHz, +12 / -10.4 dB at +-10 | the same | measured |
| Treble Top | a high shelf from about 2 kHz, +12.6 / -9.9 at 16 kHz | the same | measured |
| Treble Roof | a high shelf from about 3 to 5 kHz, +11.3 / -13.5, its boosts with a -1.3 dB dip at 500 Hz | the same | measured |
| Fader 0 | Treble's colours change nothing; Bass's set the subsonic high-pass | the same | measured |
| Fader law | near 1.2 dB per step at the largest point but not a straight line; cut and boost differ (dB per step, cut / boost: Bass Sub 1.17 / 1.20, Lower 1.21 / 1.26, Treble Bite 1.04 / 1.16, Top 0.94 / 1.23, Roof 1.29 / 1.14), departures from a line up to 1.6 dB (the shelves' shapes change with gain); Bass Sub at 50 Hz: +2.5 gives +2.8, +5 gives +4.8, +10 gives +11.8; -2.5 gives -2.3, -10 gives -11.2 | `single`: all 21 steps, 1/12 octave from 20 Hz (`single.json`) | measured |
| Structure | each fader sets one setting of a stage on a straight line (slope x fader + offset), per colour. Sub / Lower: the first EQ's LF band at 1.2 dB per step, its high-pass at 45 Hz, the LF frequency 60 Hz (Sub) or 100 Hz (Lower). Upper: the second EQ's LF gain, its high-pass switched in from fader 0 up. Bite: the first EQ's mid-high bell (4.5 kHz in the chain's preset; measured near 5 kHz). Top: the first EQ's high shelf. Roof: the second EQ's high band | the chain's own presets, each point checked against the audio | measured |
| Bass x Treble | independent: the combination equals the sum of each alone in dB | `pairs`: 225 combinations | within 0.01 dB (median 0.001) |
| Order | the EQ comes before the dynamics: an EQ boost at a tone's frequency is pulled back once the compressor or the de-esser acts (Treble Top +10 at 10 kHz: +12.9 dB at -45 dBFS; the de-esser takes it back from -27 dBFS, -26 dB by +6; with Spank from -30 dBFS); Bass boosts pass the de-esser untouched | `levels` | measured |
| Sample rates | 44.1, 88.2 and 96 kHz give the same responses as 48 within +-0.5 dB (30 Hz ... 16 kHz), except Treble Roof, +0.9 dB at 16 kHz at 88.2 and 96 kHz (its response is cramped near the Nyquist frequency at 44.1 and 48) | `rates` | measured |
| Changes during playback | the Bass and Treble faders glide: 90 % in about 120 ms (Treble Bite -10 to +10 at 5 kHz: 124 ms); a Bass colour change glides over about 60 to 100 ms | `changes`: a control jumped at 1.5 s, 1 ms frames | measured |
| Model | every one of the fader's 201 positions, per colour and rate, is a low-order digital filter, fitted through the clear chain's own response (`plugins/FwVocalChain/nulltest/fit_eq_tables.py`); Bass Upper's subsonic high-pass has a notch near 10 Hz (zeros on the unit circle) and poles crowded near DC, refined with the poles and zeros themselves as the parameters (least squares on the response, an analytic Jacobian, several starts; `plugins/FwVocalChain/nulltest/fit_eq_refine.py`, 696 positions); the tables have 10 sections; other rates use the fitted shapes | the reference's responses per position | Treble -114 dB or better at every position and rate; Bass Sub and Lower median -114 ... -146 dB, every position -86 dB or better at every rate; Bass Upper median -98.5 dB at 48 kHz, -97.4 at 44.1, -86 at 88.2 and 96, worst -59 (13 of its 101 positions from 0 up above -80 at 48 kHz) |

## 4. Compress

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Push | a makeup gain and a ceiling: the gain below -9 dBFS is -3.9 / -2.1 / +1.4 / +5.5 / +8.9 dB at fader -10 / -5 / 0 / +5 / +10, and the output is held near the same level above (at 0 dBFS in, -6.2 ... -4.1 dB); at fader 0 a -30 to -6 dBFS step settles in about 72 ms and recovers within 1 ms after the drop | `compress`: a 1 kHz tone stepping -45 ... 0 dBFS against every colour clear | measured |
| Spank | more makeup (+1.5 / +3.3 / +7.0 / +10.7 / +13.7) and a softer knee from about -27 dBFS at fader +10; attack t63 6 ms, release t63 104 ms | the same | measured |
| Wall | makeup +1.5 / +3.3 / +6.7 / +10.8 / +14.2, a harder knee; attack 10 ms, release 17 ms | the same | measured |
| Makeup per step | Push -3.87, -3.48, -3.11, -2.75, -2.41, -2.08, -1.26, -0.51, +0.17, +0.81, +1.41, +2.40, +3.29, +4.09, +4.83, +5.51, +6.30, +7.02, +7.68, +8.30, +8.88 (-10 ... +10; kinks at -5 and at 0); Spank +1.52 ... +6.99 ... +13.69, Wall +1.47 ... +6.74 ... +14.21 (the same kinks); 100 Hz tones get 0.2 dB less makeup; 6 kHz tones meet the de-esser after the compressors | `single`: the low-level gain over clear at every step | measured |
| Highest output (1 kHz) | rises with the fader: Push -5.82 ... -3.71 dBFS, Spank -8.20 ... -5.83, Wall -5.99 ... -3.94 | the same | measured |
| Sensitivity | each Compress curve is the Sensitivity-0 one shifted by the Sensitivity fader | `levels`: noise steps, -42 ... 0 dB rms | measured |
| With the EQ | in series, the EQ first: below the threshold the two add; above it the EQ's boost or cut is taken back by up to the compressor's whole reduction (Bass Lower +10 into Spank: -11.3 dB at +6 dBFS) | `pairs` | below the threshold within 0.07 dB |
| Timing | depends on colour, fader and level (a limiter and a compressor in series): attack t63 from 1 to 71 ms (Push at fader 0, -30 to -6: 71 ms; Wall +10: 3 to 4 ms), release from 1 ms to about 1 s (Push +10 after 0 dBFS: 958 ms; Spank 0: 264 ms; Wall 0, -30 to -6: 18 ms); every curve in `timing.json` | `timing`: tone bursts -40 to -20, -30 to -6, -20 to 0 dBFS of 50, 300, 1500 ms, every colour, faders -10, 0, 10 | measured |
| Changes during playback | the Compress fader changes within 1 ms (no smoothing); a Compress colour change glides over 49 ms | `changes` | measured |
| Structure | Clear bypasses the limiter and the compressor; a colour loads their settings; the fader moves the compressor's input | the chain's own presets, checked against the audio | measured |
| Colour filters | each colour adds a fixed linear filter whatever the fader (the fader moves only the gain): Push and Wall share one (-0.36 dB at 40 Hz, -1.19 at 20 kHz), Spank has its own (-7.8 dB at 20 kHz); captured per colour and rate, order 8 | `plugins/FwVocalChain/nulltest/fit_compress.py` | -112 ... -137 dB (Spank at 96 kHz -73.5) |
| Distortion | the colours distort: the 2nd harmonic 18 dB over the clear chain's, the odd ones rising 10 dB per 10 dB (an x abs(x) term, as an analog-modelled compressor gives) | tones | measured |
| Dynamics | a compressor (Push: a slow attack, the gain sinking over about 100 ms; a slow release, 300 ... 900 ms; Spank a fast first stage and a slower second) and a smooth ceiling (-4.86 dBFS out for 0 ... +6 dBFS in on Push, steady to 0.1 dB within a cycle: no sample-level clipping) | tones, bursts | measured |
| Low tones | the reference compresses 100 and 250 Hz tones less than 1 kHz ones and its gain does not ripple within a cycle (Spank's 3rd harmonic at -24 dBFS -65 dB; the 2nd harmonic -55 dB on Spank and Wall) | 100 / 250 Hz tones at -24 ... -6 dBFS | measured |
| Model | an input gain, a compressor with a two-stage release and a first-order high-pass on its detector (settling at about 30 ... 40 Hz), a static shaper, a look-ahead limiter (its 64 samples taken out of the input delay, so the latency stays 193; its sliding minimum drops entries by age) and an output gain, with a static correction of the reduction at the tone ladder's 18 levels (kept only where the mean null improves); Push and Wall with the compressor first and a feed-forward detector, Spank from +2 up with the limiter first and a feedback detector, from +1 down as Push; fitted per colour and fader step (`plugins/FwVocalChain/nulltest/fit_compress.py refit`, `refit_low`) on a tone ladder, two 100 / 250 Hz ladders (-36 ... 0 dBFS), two bursts and two loud vocals, with the main de-esser in the model; where two neighbouring steps differ in structure the plug-in takes the nearer step's parameters | the fit cases | mean null per step: Push -10 ... 0 -47.9 ... -35.2, +1 ... +10 -33.8 ... -27.8 (+2 -31.5); Spank -10 ... +1 -33.5 ... -29.0, +2 ... +10 -26.6 ... -21.8; Wall -43.0 ... -27.0; the low ladders -27 ... -33 dB; the loud vocals Spank +5 / +10 -23.6 / -21.6, Wall +10 -22.3 |
| Port | the plug-in equals the model on every colour | `plugins/FwVocalChain/nulltest/comp.c` against the renderer | -142 ... -147 dB |
| Where the error is | on loud vocals at fader +5 ... +10, 79 ... 97 % of Push's and Wall's envelope error falls in steady loud stretches, the reference there 0.1 ... 0.5 dB louder than ours; on steady noise (150 Hz ... 4 kHz, -36 ... -6 dBFS rms) the reference is 0.4 ... 1.1 dB quieter at -18 ... -6 dBFS (noise nulls -18 ... -24 dB): the miss depends on the crest factor, which a correction learned from sines cannot follow; on a loud vocal Spank +10's error is mostly gain movement on a 1 ... 10 ms scale (a per-2 ms gain correction takes its null from -17.8 to -24.8 dB; a 64-tap linear filter changes nothing) | vocals, noise | open: the limiter's and the compressor's detectors on a dense signal against a sine; the learned correction of section 9 covers part of it |

## 5. De-essers

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| The always-on de-esser, from outside | nothing below 3 kHz; above about 4 kHz a tone is turned down from about -13 dBFS peak, by nearly 1 dB per dB above that (6 kHz: -0.9 at -12, -6.2 at -6, -11.1 at 0 dBFS; 8 to 10 kHz down 13 dB at 0 dBFS); noise at -12 dBFS rms already 1.8 dB down at 5 kHz | `deess` | measured |
| Thresholds | no action at 2 kHz at any Sensitivity; 4 kHz from -9 dBFS (Sensitivity 0), 6 and 10 kHz from -12, nearly 1 dB per dB above (0.9 at 6 kHz, 0.97 at 10 kHz); the threshold moves 1:1 with Sensitivity | `levels` | measured |
| Timing, from outside | attack within 1 ms at every level; after a drop it holds about 5 ms, then releases (-5.0 dB at 10 ms, -2.9 at 20, -0.8 at 50, 0 by 100 ms); release 2 to 104 ms, longer after deeper reduction and longer bursts (6 kHz: 9 ms after -2.6 dB, 104 ms after -11 dB) | `deess`, `timing` | measured |
| Band | zero-phase: with a loud 12 or 16 kHz tone holding the de-esser down, a quiet noise probe's response over the clear chain's is 1 - (1 - g) H(f) with H real (phase 0.000 rad at every frequency, the same shape at every depth and drive): H 0.009 at 200 Hz, 0.215 at 1 kHz, 0.544 at 2, 0.749 at 3, 0.919 at 5, 0.993 at 10 kHz, 1 above; a de-essed signal is x - (1 - g) H x, so loud sibilance pulls the band down from about 1 kHz (the chain's preset names a 5 kHz crossover; there is no 5 kHz high-pass) | `plugins/FwVocalChain/nulltest/probe_deess.py` | a symmetric FIR of +-24 taps at 48 kHz (designed per rate) holds it to -62 dB |
| Behaviour | split band (a 500 Hz tone under a heavily reduced 6 kHz moves 0.3 dB); stereo-linked (a loud L takes R down 3.6 dB); adaptive: loud low content raises its threshold (a 300 Hz tone at -10 dBFS takes 6 kHz's reduction from 6.1 to 2.2 dB, at -3 dBFS to none), not by a plain power sum | tone mixes | measured |
| Gain | two stereo-linked detectors on the signal after the fixed filter: a second-order high-pass one (4.7 kHz, Q 0.79; attack 0.5 ms, release 0.7 ms) against a threshold that rises with a low-pass one in linear amplitude (1.4 kHz, Q 0.76, 44 / 254 ms; the preset's low band 24.9 / 248.7): thr = T + 0.40 low. A 16 kHz tone at -3 dBFS loses 9.4 dB alone, 8.5 with a 100 Hz tone at -24, 5.6 at -12, none from 0 dBFS (the same law at 1 and 2 kHz through the low-pass). The reduction is the high level over the threshold at 1.02 dB per dB (an unlimited ratio: a 16 kHz tone comes out at -12.77 dBFS from -12 to +6 in), smoothed: attack 0.07 ms, a 5.1 ms hold, release 17 ms; computed 28 samples (48 kHz) ahead of the audio it turns down | `plugins/FwVocalChain/nulltest/deess.c`, `plugins/FwVocalChain/nulltest/fit_deess.py`: tone steps, low-tone mixes, bursts of 2 ms ... 1 s and loud training vocals | fit mean -40.9 dB; held-out vocals: the sibilant ones -35 ... -44 dB (-29 ... -37 without the de-esser), the others unchanged (-71 ... -77) |
| Not matched | white noise at -4.5 dBFS rms (-8 dB: the reference turns it down 4 dB, ours more) and a loud L with a quiet R (-6 dB): the stereo link is not a plain maximum; neither is vocal material | | open |
| The reverb's de-esser | the same, 8 dB lower (the presets' -28 against -20), after the main one, on the reverb's send only: a 10 kHz burst at 0 dBFS reaches every reverb 8.0 dB lower than through the main one alone, the same on every colour; the reverbs' responses start with at least 162 silent samples, so its delay comes out of them | 10 kHz bursts | reverb wet nulls on 10 kHz bursts from -20 dBFS up -34 ... -47 dB (+3.6 ... -14 without it); on loud vocals -24 ... -59 dB |

## 6. Reverb

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Colours | Tight T60 0.94 s, first arrival 28 ms, L 1.5 dB over R; Large T60 1.69 s, first 3.7 ms; Chamber T60 3.48 s, first 193 ms; L / R nearly uncorrelated (+0.01 ... +0.08); energy at fader 0: -19.9 / -20.4 / -15.4 dB | `reverb`: a -20 dBFS impulse, the output less the output with the colour clear | measured |
| Input | the sum of L and R, spread to both sides (L-only input: Reverb Large on R at -16 dB at 100 Hz) | `stereo` | measured |
| With the EQ | Treble changes the send (Top +-10: -3.8 / +8.4 dB, its T60 1.57 ... 1.72 s as the spectrum tilts); Bass reaches it (Sub +10: +2.0 dB) | `pairs` | measured |
| Changes during playback | the reverb colour changes within 1 ms | `changes` | measured |
| Model | convolution (chosen on approval after the model-first attempt): linear, exactly time-invariant, true stereo (L and R inputs give uncorrelated responses) and designed per rate (one rate's response resampled is unrelated to another's): the four paths per colour and rate, 16-bit (-73 dB), 15.5 MB, run by zero-latency convolutions that keep running while Reverb is clear; prepare waits for each engine's full length (a background load finishing after prepare starts a render with a partial reverb: nulls -0.8 ... -33 dB under load) | impulse responses per colour and rate | -67 ... -75 dB |

## 7. Delay

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Pattern | synced to the host's tempo; ping-pong at about 6 dB lower per bounce, R first (the side of the first echo changes with the tempo) | `delay`, `tempo` | measured |
| Slap | one echo at a 1/16 note at every tempo (60 / 90 / 120 / 140 / 180 bpm: 249.2 / 166.7 / 125.0 / 107.1 / 83.4 ms; 62.5 ... 375 ms over 40 ... 240 bpm, 369 at 40 bpm, 297.9 at 50) | `tempo`: RefRender at 40 ... 240 bpm in steps of 10 | measured |
| Eight, Quarter | echoes every 1/8 and 1/4 note (Eight at 120 bpm: 249, 507, 749 ms) | the same | measured |
| Glide | the line starts at the preset's time and glides to the tempo's note, so early in a render long notes read short through one shared curve: 250 to 249.2 ms, 300 to 297.8, 333 to 329.7, 375 to 368.9, 429 to 418, 500 to 481.5, 600 to 566, 750 to 686 (Eight at 40 bpm and Quarter at 80 bpm alike), 857 to 766, 1000 to 868, 1200 to 1001, 1500 to about 1190 (Quarter at 120 bpm: 481 ms); an impulse 4 s or more into a render gives exactly the note (1/8 at 60 bpm: 481.4 ms at 0.02 s, 497.5 at 1 s, 500.0 at 4 and 8 s) | impulses at several times | exact |
| Input | the sum of L and R after Balance, high-passed (Bass changes the delays by <= 0.3 dB); taken after the fixed filter (its polarity-inverting response is in the echoes) and after the compressors | `pairs`, `stereo` | measured |
| With the EQ | Treble Top +-10: -7.2 / +9.6 dB | `pairs` | measured |
| Model | Flatwater Delay's model, set from each colour's stored settings; the tap delay's two taps each turn the stereo image by 45 degrees: the first at once, L = g (L + R), R = g (R - L); the second 0.004 bar later (8 ms at 120 bpm), L = g (L - 0.995 R), R = g (0.995 L + R); g at fader 0 -16.1 dB (the send's gain stage, the tap level and the pan law); the tap delay's LFO is off | clicks | measured |
| Echo shape | a low shelf (132 Hz, +0.16 dB), a bell (430 Hz, Q 1.0, -0.15) and a high shelf (1.93 kHz, +0.35) on the delays' output; without them our echoes over the reference's are -0.15 dB at 60 Hz, +0.15 at 500 Hz, -0.3 above 3 kHz, the phase within 0.06 rad, the same on every colour and side within 0.05 dB | white noise at -30 dBFS after 6 s of silence, the wet output less the output with Delay clear | the ratio's error rms 0.015 (0.033 without); echo nulls Slap -36 dB, Eight -31, Quarter -33 (-29, -26, -27 without) |
| Loop level | the delay's gain recovery compresses the echoes: our model is fed the send 4.4 dB lower (x 0.6, made up after it) | held-out vocal at -6 and -20 dBFS peaks | echo nulls -35 ... -36 dB at -6 dBFS (-21 ... -23 without), -35 at -20 (-30 ... -32 without); x 0.5 and x 0.7 both worse |

## 8. Pitch (the doubler)

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| From outside | one voice per side, delayed and slowly pitch-modulated: Stereo 9.2 ms L / 17.4 ms R, +-7 cents; Wide 11.7 / 39.7 ms; Spreader 3.9 / 9.4 ms; its level at fader +10 about the dry level; the doubler works per side | `pitch`: a -20 dBFS impulse | measured |
| With the EQ | Treble Top +-10: -8.9 / +12.2 dB; Bass <= 0.12 dB (its input is high-passed) | `pairs` | measured |
| Read head | one moving read head per voice, linearly interpolated: each click comes back as one tap, two neighbouring samples sharing it, then a tail common to every click | `plugins/FwVocalChain/nulltest/fit_doubler2.py`: click trains through the Pitch stage alone (fader +10, every other colour clear, the output less the output with Pitch clear), the send response divided out; a shared response plus a per-click delay (alternating least squares, `track`) | -51 dB median per click |
| Delay law | the speed swings as a triangle: the delay's second difference is constant within each half of the LFO and flips sign at its peaks, so the delay is a chain of parabolas (a cosine misses it by up to +-5 samples, which on broadband material leaves no correlation); just after each delay maximum the delay jumps +1 sample, just after each minimum -1 (from the first whole-sample crossing after a turn down, one sample longer, until the first crossing after the next turn up) | the same, `tri` | the Stereo tracks to 0.007 samples rms |
| Stereo | +-7 cents (0.0040434 in natural-log units) at 0.19988 and 0.29992 Hz at 48 kHz, starting at 0 and rising; start delays 359.01 and 719.01 samples; a steady drift of -8.15e-6 samples per sample on both voices (-0.39 samples a second; 60 s of clicks follow it, with no reset); the LFO's clock leads the aligned output by about 130 samples | the same | 0.01 samples rms over 60 s |
| Stereo's response | an RBJ low shelf 200.47 Hz, Q 0.500, -8.61 dB and a high shelf 5539 Hz, Q 0.500, +4.50 dB; -10.0 dB at fader 0 (1.2 dB per step) | the same | -81 dB from the measured response |
| Spreader | a pitch shifter: +-15 cents at 0.1501 Hz, the two voices opposite, a drift of about -4e-5 samples per sample, both voices starting at 320 samples; when a voice's delay leaves 64 ... 576 samples (320 +- 256), a tap one of 20 steps nearer the middle fades in linearly over 256 samples as the old one fades out | `stagger`, `grains`: twelve click trains 100 samples apart | on clicks our model follows the reference to 0.05 samples through its jumps |
| Wide | the same pitch shifter: +-18 cents (0.0104 in natural-log units) at 0.2 and 0.3 Hz, a drift of about -5.5e-5 samples per sample, the shelves as Stereo's; its left voice starts at 349 samples, the right at 1717.5, each the middle of its window (start +- 256) | clicks 100 ms apart, one voice per side | measured |
| One-sample steps | after each turn on Wide and Spreader too; the fitted drifts are close to -swing² / 2 on all three colours (inferred; an exponential speed law does not fit better) | click tracks | 0.007 (Wide L), 0.018 (Spreader R), 0.026 samples rms (Spreader L) with them; 0.24 without |
| Step choice | the jump is always one of 20 steps: round(256 x 2^(k / 20)), k = 0 ... 19 (256, 265, 274 ... 461, 478, 495), every probe step and every step measured on noise one of them; a single click never changes the step, a pair does (a product correlation); the step is the one where the send's (the fixed filter's output) autocorrelation is highest, which reproduces all 15 pair spacings tried (150 ... 600, including 230 to 284, 400 to 446, 480 to 431, 520 to 461; on the raw input or low-passed versions 1 ... 12 of them); in silence every step ties and the longest (495) is taken; the shelves come before the pitch shifter (its step search hears them) | `plugins/FwVocalChain/nulltest/probe_splice.py`: silent input but for a pair of clicks some samples apart and a probe after the jump | 11 of 12 first jumps on six noise renders (a decision 100 samples earlier or later: 7 of 12) |
| Step memory | exponential with a time constant of 960 samples (anything from 930 to 990 the same): of two pairs, an older one needs (0.5 / a)² weight 0.31 ... 0.39 to win 1000 samples back and 0.028 ... 0.063 3000 back | the pair probes | inside both bounds |
| Rates | Stereo's start delays are whole samples, 7.5 and 15 ms rounded down less one (329 / 660 at 44.1 kHz, 359 / 719 at 48, 719 / 1439 at 96; the fits 328.999 / 659.993 and 718.94 ... 719.05 / 1438.8 ... 1439.1); the swing (7.00 cents) and the drift (-8.15e-6 samples per sample at 44.1 and 48) are the same at every rate; the LFO's rate differs by under 0.1 % (L 0.20000 / 0.19988 / 0.20008 Hz at 44.1 / 48 / 96 kHz), kept per rate; the LFO's lead is 130 samples at 44.1 and 48 kHz and 260 at 96 (samples times the latency's multiple, 193 to 385, not the rate); at 96 kHz Stereo is a pitch shifter too (its delay stays within its start +-256 samples and jumps by 388, one of the 20 steps; at 48 kHz Stereo's swing, +-121 samples, never reaches the edge); the window, the steps and the 256-sample crossfade are in samples at every rate; Spreader's start delay and lead are samples times the latency's multiple (320 at 44.1), Wide's start delay whole samples as Stereo's | `plugins/FwVocalChain/nulltest/fit_doubler2.py` with `DBL_FS`, `plugins/FwVocalChain/nulltest/measure_doubler_rates.py`, `plugins/FwVocalChain/nulltest/fit_rates_lead.py` (the per-click error against the delay's slope: +202 / +194 samples on a first fit's 70 / 67) | with a lead of 260 the right voice tracks to 0.017 samples rms; Stereo's click delays follow the reference to 0.05 ... 0.14 samples rms at 44.1 kHz and 0.02 ... 0.3 at 96; Spreader's jumps match in size and time at both rates |

Method notes: the doubler's Wide and Spreader colours pick their splice points from the signal, so a voice parts from
the reference at a jump where the rule picks another step or the one-sample steps start differently, and stays apart
after it (remaining partings are near-ties in the search: Wide's right voice at 7.7 s, 315 against 326 samples, 1 %
apart; Spreader's left voice takes its one-sample step one turn differently from 3.5 s). They are compared in
behaviour in [NULLTESTS.md](NULLTESTS.md). Wide at 44.1 kHz picks a neighbouring step at its jumps (446 against 431
samples).

| Doubler null on a held-out vocal | Wet output | Whole output |
|---|---|---|
| Stereo, every fader, 48 kHz | -45 / -48 dB (L / R); white noise -31 / -29 (the one-sample steps' timing near a turn is the limit) | -59 dB at fader 0 |
| Spreader, 48 kHz | right voice -39.2 dB, left -20.6 dB; -43 ... -47 dB for the first 3 ... 4.5 s | |
| Wide, 48 kHz | left voice -36.9 dB over the whole 10 s; right -20 ... -55 dB | |
| The defaults (Stereo), whole chain | | -42.5 dB at 48 kHz, -40.6 at 44.1, -18.1 at 96 (the splices part as the voices run on) |

## 9. The wet sends

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Fader law | every wet stage's level follows its fader exactly 1.2000 dB per step (all 21 steps, all nine colours, zero departure from the line); the fader leaves the shape alone (T60, first arrival and L / R correlation never change) | `single`, `reverb`, `delay`, `pitch` | exact |
| Structure | the wet faders set only the gain stage after their effect; a wet colour's Clear mutes that gain stage while the effect keeps running | the chain's own presets, checked against the audio | measured |
| Level | linear in level (-40 ... 0 dBFS impulses: energy per unit input unchanged), apart from what the dynamic stages before them do | `levels` | measured |
| After Compress | every send follows Compress's makeup (Push -10: -3.8 dB; Spank +10: +13.3 dB on every wet stage): the sends are taken after the compressors | `pairs` | measured |
| Together | parallel sends: any combination equals the sum of its stages alone | `pairs`: 54 combinations | -77 dB or better |
| Changes during playback | the wet faders change within 1 ms (no smoothing) | `changes` | measured |

## 10. Output and noise

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Output | an exact gain at the very end, the same at every input level (-10 ... +10 gives -10.00 ... +10.00 dB, levels -42 ... 0 dB rms), applied to the wet stages too; not limited: factory presets peak at up to +5.7 dBFS; changes within 1 ms | `single`, `changes` | exact |
| Noise | on by default: white, uncorrelated L / R, -108.3 dBFS rms with every colour clear (-106.6 with the defaults, the Compress makeup lifting it); flat within +-2 dB from 30 Hz to 20 kHz | `noise` | measured |
| Mono/Stereo | L and R identical with every colour clear, the same response as the Stereo component's | `gains` (RefRender) | exact |

## Learned blocks

Compress keeps its fitted model exact; a small GRU per colour scales the compressor's output (after its limiter,
before the colour's filter), so the compressor's state never sees it and everything after it is linear (the
de-esser's gain held). The procedure is [docs/learned-blocks.md](../../docs/learned-blocks.md).

| Item | Value |
|---|---|
| Ceiling | the model's output scaled per block by its least-squares gain (`plugins/FwVocalChain/nulltest/null_compress_baseline.py`), held-out vocals at +5 ... +10: Push -24.3 dB to -38 (2 ms blocks) or -34.5 (10 ms), Spank -22 ... -23.7 to -30 ... -33 or -26 ... -28.5, Wall -23.5 to -33 or -29.5; between the steps the model nulls as at the steps beside them; Push's needed gain is a smooth wander (2 ms blocks correlate 0.98 with the next) with no mean (+0.01 dB): it depends on the history, not the level |
| Target | the reference with every other colour clear; the reference's gain (2 ms blocks smoothed over 10 ms), then the null itself |
| Dataset | `plugins/FwVocalChain/nulltest/render_trainset.py`: per colour and step 48 four-second examples at +1 ... +10 and 16 at -10 ... 0 (corpus vocals, vocals over noise, band noise with and without envelopes, tones, level steps, synthetic voices; peaks -24 ... +6 dBFS), 14 positions between the steps, fader moves (the fader automated per 512-sample block; a held fader equals a static render exactly), and held-out vocals (parts 24 ... 31, 8 s, peaks -3 ... +6); `plugins/FwVocalChain/nulltest/comp.c` gains `comp_dump` (its output unchanged on all 63 steps); Wall adds 48 vocals at each of steps -8, -7, -6, -5, -1, 0 and +1 (`plugins/FwVocalChain/nulltest/render_trainset.py wallx`: peaks -12 ... +6 dBFS, 30 % of them over a noise bed; `plugins/FwVocalChain/nulltest/prep_compress.py` with `WALLX=1`) |
| Network | inputs per 4 samples (12 kHz): the detector level, the compressor's and the limiter's reduction, the input's peak and power, and the fader position; hidden 32; the correction bounded to +-2 dB, each sample taking the last two finished steps' values (causal) |
| Training | `plugins/FwVocalChain/nulltest/prep_compress.py`, `plugins/FwVocalChain/nulltest/train_compress.py`: supervised on the gain first, then on the null plus 3 x the amount any piece gets worse (each null floored at -50 dB); on the null alone Push learns nothing; net A is that recipe; net B the same with 48 examples at every step; net C floors the loss at -40, weighs the no-worse term 10 x and keeps the checkpoint with the best held-out mean plus 3 x its regressions (`SEL`); net D is net A's recipe (SUP=2000 FLOOR=-50 LAMBDA=3, 8000 steps) on Wall's added examples, checkpoint at step 7250 (held-out mean -30.1 dB) |
| Shipped | two nets per colour, their corrections averaged: Push and Spank A + C, Wall B + D |
| Export | `plugins/FwVocalChain/scripts/export_net.py` to `plugins/FwVocalChain/Source/dsp/CompressNet.h`; `CompressNet` (`plugins/FwVocalChain/Source/dsp/Engine.h`) runs the nets per sample in the compressor's loop on the 12 kHz clock |
| C++ equals PyTorch | `plugins/FwVocalChain/nulltest/check_compress_net.py`, full float32, a held-out vocal at five fader positions per colour: -107 ... -133 dB (Wall -112 ... -133) |
| Validation set | `plugins/FwVocalChain/nulltest/render_trainset.py val`: parts 24 ... 31 from 8 s to 16 s, which neither training nor checkpoint choice saw, at peaks -12, -6 and 0 dBFS, 24 per step; each null floored at -40 dB |
| Unseen vocals | parts 32 ... 35, Compress alone, the plug-in's model in Python, median and worst of 4 vocals at 0 dBFS and +6 ... +10: Push -24.3 ... -24.6 to -29.8 ... -29.9, Spank -22.0 ... -23.5 to -26.7 ... -27.8, Wall -22.1 ... -23.2 to -28.4 ... -29.3 (net A alone: -28.8 ... -29.7, -27.0 ... -28.0, -27.8 ... -29.3); net B's worst cases, Push -2 at -6 dBFS (+8.2 dB) and Spank -2 at -12 dBFS (+6.4), are +0.2 and +1.2 with the pairs; worse by more than 1 dB where it can be heard: Wall 0 at 0 dBFS, one vocal +1.5 (median -26.1 to -29.0), Spank -2 at -12 dBFS +1.2 (median -35.3 to -35.9); Push -10 ... -6 at -12 and -6 dBFS lose up to 6 dB from -60 ... -69 dB, inaudible |
| Other rates | `plugins/FwVocalChain/nulltest/null_compress_grid.py rates`, against the renderer without the nets: at 44.1 and 96 kHz every median 2 ... 7 dB better, no vocal worse |
| CPU | a minute of stereo, Compress alone: 1.81 s without the nets, 2.73 s with one per colour, 3.75 s with two (the nets about 3 % of a core) |

Validation set, per colour: the mean change over the 21 steps and the worst single vocal (dB):

| Colour | Net A | Net B | Net C | Shipped pair |
|---|---|---|---|---|
| Push | -1.53 / +0.96 | -1.65 / +1.83 | -1.82 / +2.18 | A + C: -1.88 / +0.27 |
| Spank | -3.88 / +2.02 | -4.13 / +5.18 | -3.55 / +2.05 | A + C: -4.01 / +0.76 |
| Wall | -3.92 / +2.50 | -3.97 / +1.32 | -3.65 / +2.19 | B + D (below) |

Each net regresses on a different single vocal at a few steps: variance between trainings, which averaging two nets'
corrections removes. Wall's regressions at steps -7, -6 and 0 sit where the de-esser is idle (`plugins/FwVocalChain/nulltest/check_wall.py`)
on steps with few examples; net D adds examples there:

| Wall | mean | worst | worst at -7 / -6 / 0 | worse > 0.5 dB |
|---|---:|---:|---|---:|
| A + B | -4.10 | +1.46 | +1.46 / +1.29 / +1.24 | 3 |
| A + D | -4.21 | +1.38 | +1.05 / +0.34 / +1.38 | 2 |
| B + D (shipped) | -4.18 | +1.37 | +0.40 / +0.03 / +1.37 | 2 |

Fit cases with net A, held-out vocals median without and with the net: Push +5 ... +10 -24.3 ... -24.5 to -27.4 ...
-27.7, -10 ... +4 -32.5 ... -24.4 to -35.3 ... -27.1; Spank +5 ... +10 -21.6 ... -23.7 to -26.3 ... -27.7, -10 ... +4
-24 ... -25 to -27.2 ... -30.0; Wall +5 ... +10 -23.4 ... -24.4 to -27.1 ... -30.7, -10 ... +4 -23.9 ... -33.8 to -25.7
... -34.0. The fit cases' mean improves at every step but Wall -10 (-39.3 to -38.7). Of 336 cases per colour, worse by
more than 0.5 dB: Push 14 (all but one already -46 ... -66 dB, a vocal at -6 dBFS losing 3 ... 5 dB at -10 ... -3; one
held-out vocal at -2 -26.5 to -25.0), Spank none, Wall 5 (a burst at -10 -56.4 to -50.3; two held-out vocals at 0
-23.9 to -22.9 and -24.7 to -24.2). The held-out vocals also chose each net's checkpoint, so these figures lean
slightly their way; the validation set above did not.

The whole chain with the doubler clear (`plugins/FwVocalChain/nulltest/nulltest.py full`), without and with the nets:

| Case | Without | With |
|---|---:|---:|
| Part 33, preset 06 (every stage at its most) | -20.7 | -24.3 |
| Part 33, preset 12 | -23.9 | -29.8 |
| Part 33, preset 18 | -23.5 | -27.8 |
| Part 33, preset 24 | -23.2 | -30.2 |
| Part 33, preset 30 | -24.4 | -27.3 |
| Part 33, preset 36 | -30.4 | -33.0 |
| Part 33, preset 42 | -23.5 | -27.7 |
| Part 33, random 0 (Spank -0.7) | -28.6 | -28.2 |
| Part 30, median | -32.1 | -36.2 |
| Part 30, random 5 (Push at 0) | -41.4 | -40.1 |

With Wall's B + D pair the chain's medians with the doubler clear are -32.1 dB at 44.1 kHz (-32.4 with A + B), -33.2 at
48 (-33.1) and -35.5 at 96; the 43 presets' median stays -30.1 dB (Pitch clear -31.2, -31.4 with A + B).

## Declined changes

| Change | Measured effect |
|---|---|
| Fitting each EQ step with generic shelves and interpolating between steps | -20 dB between steps (neighbouring steps' fits are different equivalent shapes) |
| Steiglitz-McBride direct form for Bass Upper | -46 ... -67 dB (it cannot resolve the 10 Hz notch and the poles near DC) |
| Compress: the noise ladder as an eighth fit case per step with the detector's window free (`noise_signals`) | the window stays at its 0.05 ms floor; Push +10 mean -26.7 to -27.0 dB, its vocal 1.7 dB worse |
| Compress: Wall in the limiter-first structure (`restructure 3 1 0`) | +0.6 ... +1.1 dB better mean from +4 up on the fit cases (mostly noise and bursts); Wall's presets on the held-out vocal through the whole chain slightly worse (06 -21.2 to -20.4, 12 -25.4 to -24.9) |
| Shrinking the learned correction (`plugins/FwVocalChain/nulltest/fit_compress_shrink.py`: scaled by 0 ... 1 per step) | the best scale 0.875 ... 1 nearly everywhere, worth under 0.2 dB |
| Doubler: per-grain step state; a grain inheriting the direction's step | both fit worse than one step state per voice |
| Doubler: a cosine delay law (f = the preset's LFO rate / 2, amplitude 0.1494 ms x abs(depth) / rate) | uncorrelated with the reference on broadband material (+3 dB); replaced by the triangle law |

## The whole chain

[NULLTESTS.md](NULLTESTS.md) holds every case at 44.1, 48 and 96 kHz; the linear stages with the others clear have a
median of -75.7 dB at 48 kHz (worst -67.9, Bass Upper from 0 up -69.7 ... -75.1); the whole chain with the doubler
clear -33.2 dB, with Stereo -36.0 dB. Compress at high faders is the largest error after the doubler's Wide and
Spreader colours.

## Factory presets

The reference's 43 presets store the six faders, their colours and Balance; their Sensitivity and Output are ignored
when a preset loads (RESEARCH.md) and their per-stage bypass follows the colours. Flatwater Vocal Chain's 43 factory
presets carry the same values under Flatwater names (`scripts/make_presets.py`, approval recorded), leaving
Sensitivity, Output and the In/Out link alone. `plugins/FwVocalChain/nulltest/null_presets.py` sets the reference from its preset
values as host parameters against ours loading the preset, on three held-out vocals:
[NULLTESTS-presets.md](NULLTESTS-presets.md).

## CPU

| Configuration | Cost |
|---|---|
| A minute of stereo at 48 kHz, Compress alone, with the two nets per colour | 3.75 s (1.81 s without the nets) |

## Open

- The latency's split across the chain (it adds up to 193 samples) and the Noise's source: not settled.
- The main de-esser's stereo link and its reading of loud noise (section 5).
- Compress's detectors on dense material (section 4) and Wall's remaining bias at steps -7, -6 and 0 (smaller, not
  gone).
- The doubler's Wide and Spreader splices on dense material (section 8).

## Provenance

The reference is the original named in Setup, run from a licensed install.

- Measured: renders of the reference's output per stage and for the whole chain (tones, sweeps, impulses, clicks,
  vocal parts) at its fader positions, colours, tempos and sample rates.
- Fitted or learned from the reference's output: the EQ curves and their tables, the compressor, the de-essers, the
  delays and the doubler identified from those renders; two small recurrent nets per Compress colour (our own
  weights, `plugins/FwVocalChain/Source/dsp/CompressNet.h`) trained on captures of its output (Learned blocks).
- Redrawn: the editor, drawn in code in Flatwater's own dark look (`shared/ui/Theme.h`) on the original's
  measured layout (positions and sizes of the controls), with our own colours and shading; no table of the
  original's pixels ships.
- Presets: the original's factory preset values under our own names and groups, the suite's standing rule; the
  preset files themselves are not copied.
- Shipped on recorded approval: the reverb's measured impulse responses (`plugins/FwVocalChain/resources/ir/*.bin`), after the
  model-first attempt (allowed in `.provenance`).
- None of the original's code, artwork, preset files, fonts or other files ships with Flatwater Vocal Chain.
