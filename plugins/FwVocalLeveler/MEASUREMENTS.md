# Flatwater Vocal Leveler: measurements

The current values of every stage of Flatwater Vocal Leveler, measured on the original it recreates (Vocal Rider),
with the method and the residual or confidence of each. With this structure every constant is one of the reference's
own hidden default or preset values; none is fitted. The data is in [measurements/](../../measurements).

## Setup

| Item | Value |
|---|---|
| Reference | Vocal Rider (Waves), the Mono, Stereo, Live Mono and Live Stereo components, from a licensed install |
| Host | the reference's VST3 through `plugins/FwVocalLeveler/nulltest/measure_reference.py` (a fresh instance per render) and `tools/RefRender`, 48 kHz unless noted |
| Gain readout | the applied gain as output over input, sample by sample: a least-squares ratio over 1 to 20 ms; a -80 dBFS pilot at 3001 Hz exposes the gain in silences; no latency, so nothing is aligned |
| Probes | 26 probes (steps, gaps, stops, tones, syllable trains) and targeted drops; each subcommand of `plugins/FwVocalLeveler/nulltest/measure_reference.py` is named in the tables |
| Corpus | `scripts/nulltest/vocal_corpus.py`: 84 sung and spoken phrases from the DAW's own vocal packs strung into 36 parts of 20 s; parts 0 to 23 identify the model, 24 to 35 are held out |
| Reference renders | `plugins/FwVocalLeveler/nulltest/render_reference.py`: the probes and the 36 parts at ten settings each, 620 renders |
| Model | `plugins/FwVocalLeveler/nulltest/model.c`, identical line for line to `plugins/FwVocalLeveler/Source/dsp/Ride.h`; `plugins/FwVocalLeveler/nulltest/fit_model.py report` scores it over the cached renders |

## 1. The level read

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Weighting (re 1 kHz) | -13.4 dB at 50 Hz, -8.55 at 100, -3.52 at 200, -0.48 at 500, flat to 2 kHz, -1.44 at 4 kHz, -2.62 at 6.5 kHz, -4.02 at 10 kHz, -5.74 at 16 kHz | steady sines at -21 dBFS rms through the law (`weighting`) | measured |
| Weighting as filters | two shelves, each two identical first-order sections with half the gain: a low shelf at 100 Hz / -18 dB and a high shelf at 6.5 kHz / -6 dB (the hidden high-pass and low-pass settings are shelves, not pass filters) | fit to the curve above | 0.15 dB rms, 0.50 max at exactly those values; freed, 102.4 Hz / -17.4 dB and 6.92 kHz / -6.1 dB give 0.12 dB rms |
| Follower | the peak power of the weighted signal, as a fast-attack, slow-release power follower reads (a sine's peak is +3.01 dB over its rms) | `static`, `bias` | exact |
| Floor | a power floor at -42 dB in the follower's own units (the hidden vocal bias), added to the level: below about -45 dBFS the level read flattens; at 1 kHz the floor comes out at -44.6 dBFS rms after c, +-0.2 over -30 ... -54 dBFS | `bias` | exact |
| Follower timing | starts at the floor; up by at most the attack's rate (50 ms at Attack Slow, 10 ms at Fast); down by at most the release's (1009 ms) on the samples the detector passes and the hold's (5000 ms) on the others; all multiplicative per sample with squared coefficients; since it rises only on samples above it, it slows near a sine's peak, which gives the gain-down shape of section 3 | level steps (`steps`), gaps (`gaps`) | exact (the reference's hidden values) |
| Stereo | the Stereo component reads the mid, (L + R) / 2: one channel alone reads 6 dB under both, anti-phase reads nothing (parked at Idle), L with R 6 dB lower reads -2.5 dB; both channels get the same gain | `stereo` | exact |

Method notes: the weighting is read through the static law of section 3 from steady sines; the floor from the level where the law flattens.

## 2. The vocal detector

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Decision | per sample (the hidden detector attack and release are 0.01 ms), no debounce: a sample passes when its short-term power exceeds 10^((-Vocal Sensitivity - 14) / 10) times the follower's level plus its floor, plus the Spill power on the Live components; the ride moves only on the samples that pass | drops of 1 to 12 dB from -15 and -27 dBFS at Range +-12 on a 1 kHz sine (`steps`) | exact: the release rates below are multiples of one step |
| Onset of riding on a steady tone | -42 / -50 / -58 / -64 / -70 dBFS for Vocal Sensitivity -12 / -6 / 0 / +6 / +12 dB (a 2 dB grid), independent of Target; the rule predicts -42.3 / -51.9 / -58.4 / -64.5 / -70.6 (for a steady tone the short- and long-term powers are equal, so the onset sits that far under the floor); below it the gain parks at the Idle value | `onset` | within the 2 dB grid |
| Spill (Live components) | an absolute level added to the threshold; the steady onset sits about 2 dB (rms) under the Spill value (-45 gives -47, -40 gives -42, -18 gives -20 dBFS: the same 2.6 dB as c); after a drop from -18 to -30 dBFS at Spill -30 nothing is ridden for 0.9 s (to -31 dBFS: 2.2 s) until the follower has fallen, which a separate gate cannot do; Spill -48 (its minimum) is off; the Studio components ignore it | `spill`, targeted drops | exact; the Live drops match to 0.003 ... 0.06 dB |
| Consequence on a voice | the peaks are brief, so few samples pass and the gain rises slowly (about 2 dB/s on the corpus) | the corpus | explains the slow rise the first model missed |

Method notes: on a 1 kHz sine at 48 kHz the three samples nearest each zero crossing never clear a threshold 14 dB
under the peak; just after a 12 dB drop 10 of each 24-sample half period pass, settled 21.

## 3. The ride

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Static law | in linear gain, gain = 1/2 gMin + c Target / level, clamped to [gMin, gMax] (Range Min and Max as gains); the 1/2 gMin offset is why the output is not held at Target: a 3 dB louder input leaves the variable part exactly 3 dB lower (ratios 1.403 ... 1.416 per 3 dB; 10^(3/20) = 1.4125) while the total gain falls by less, more so near Min | `explore`, `settle`, `static`, `ranges`: Target -12 / -21 / -30, Range +-12, +-6, -3 ... +9, -12 ... 0, +-1, levels -36 ... -6 dBFS | 0.1 dB at every setting with c = 0.738 on a 1 kHz sine (rms level) |
| c | the weighting at 1 kHz (-0.42 dB) and +3.06 dB (the peak read); as a hidden value the law scale is 0 dB | `static` | exact |
| Settling | a steady tone settles within about a second and stays (checked to 30 s) | `settle` | measured |
| Start | the gain starts at 1/2 gMin + 1 (+1.94 dB at Range -6 ... +6) and the park hold counts from the first sample (the reference holds through an initial silence shorter than the hold) | the start of every render | exact |
| Gain down (the level rises) | time to 63 % of the change 39 to 58 ms at Attack Slow, 7.7 to 11.4 ms at Fast (the follower's attack of 50 / 10 ms, which Attack sets) | `steps` | exact through the follower |
| Gain up (the level falls) | stepped rates: every rate is a multiple of 0.362 dB/s, k / 24 of 8.69 dB/s (the ride release, 1000 ms per neper), k the samples of each 24-sample half period that pass; just after a 12 dB drop 3.62 dB/s (k 10), settled 7.61 dB/s (k 21); as seen from outside 6 dB down takes 555 ms to 63 %, 12 dB 1373 ms (t90 764 / 1789 ms), the same at Slow and Fast; the steps depend on the drop, not on the level before it | `steps` | the level drops match to 0.002 dB rms |
| Ride attack | 0.55 ms at Slow, 0.35 ms at Fast (hidden values) | | exact |
| Park (the voice stops) | the gain holds for 500 ms at Slow, 50 ms at Fast, then glides to the Idle value at 1205 ms per neper (7.21 dB/s; as seen from outside about -6.1 dB/s steady coming down from +6, +4 ... +5 dB/s and getting faster going up from -5.8); the Idle value is Min + Idle x (Max - Min) | `idle` | exact |
| Gaps between words | shorter than the hold, the gain does not move; longer, it glides (about 6.6 dB/s) until the voice returns; back at the same level the gain recovers in about 0.3 s (the follower held the level through the gap, its hold 5 s); back louder it comes down in the attack time | `gaps` | exact |
| Output | a plain gain after the ride (-6 gives -6.00, +3 gives +3.00 dB relative); its smoother's attack and release are not identified (they smooth the Output control only, and Output moves are not measured) | `static` | exact |

Method notes: the constants are the reference's hidden defaults and preset values: law scale 0 dB, floor -42 dB,
threshold 14 dB; the follower's attack 50 / 10 ms, release 1009 ms and hold 5000 ms, squared; the detector's attack
and release 0.01 ms; the ride's attack 0.55 / 0.35 ms and release 1000 ms; the park glide 1205 ms; the park hold 500 /
50 ms. A local CMA-ES from these values over 17 constants (`plugins/FwVocalLeveler/nulltest/fit_model.py fit`, 500 cases) found no better point
in 176 candidates (mean null -63.8 dB at the start, the best other candidate -44.5); single nudges (the follower's
attack at Fast 8 or 12 ms, the detector's attack and release 0.1 ms, the ride's attack at Fast 0.2 or 0.6 ms, the park
hold at Fast 45 or 55 ms) all make Fast worse. Steady tones at 150 Hz ... 8 kHz are within 0.022 dB.

## 4. Music side-chain

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Effect of the side-chain | none observed: with the side-chain bus enabled and fed (`tools/RefRender`, `tools/MeterCapture` with `sc=`), the reference's Music knob keeps its disabled picture, its LED stays dark and Music Sensitivity changes nothing; the reference uses the music only while its host reports the side-chain active, which these hosts do not achieve | offline hosts | open: needs a host with a real side-chain send; the plug-in rides without the music |

## 5. The model against the reference

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| C++ against the model | 25 cases | `plugins/FwVocalLeveler/nulltest/check_port.py` | worst difference 0.0000 dB |
| Median null over the 620 renders | probes -70.5 dB, training parts -60.8, held-out parts -62.3 (per setting -50 ... -69 dB; Attack Fast the weakest, about -50, at word onsets) | `plugins/FwVocalLeveler/nulltest/fit_model.py report` | |
| The plug-in on fresh renders | -47 ... -74 dB, Stereo, Live, 44.1 and 96 kHz included | [NULLTESTS.md](NULLTESTS.md) | |
| The factory presets | the reference's 13 presets loaded into its own state against ours: median -60.9 dB, worst -37.9 | [NULLTESTS-presets.md](NULLTESTS-presets.md) | |

## Declined changes

| Change | Measured effect |
|---|---|
| A debounced vocal detector with every constant fitted by CMA-ES | -22 dB on the probes, -12 dB on the corpus: the reference rises much more slowly on real vocals than on tones; replaced by the per-sample detector of section 2 with the hidden values |
| Single nudges of the hidden values (section 3, Method notes) | every nudge makes Attack Fast worse |

## CPU

| Configuration | Cost |
|---|---|
| Stereo at 48 kHz through the offline renderer | about 0.6 % of one core |

## Provenance

The reference is the original named in Setup, run from a licensed install.

- Measured: level steps, tones and held-out vocal parts through the reference's VST3 in a host, its riding read from
  its output.
- Fitted or learned from the reference's output: the weighted level read, the vocal detector, the hold and the glide
  to the Idle value, identified on those measurements (`plugins/FwVocalLeveler/Source/dsp/Fit.h`); every constant
  is a hidden value of the reference confirmed by the measurements, none a fit.
- Redrawn: the editor, drawn in code in Flatwater's own dark look (`shared/ui/Theme.h`) on the original's
  measured layout (the positions and sizes of the controls), with our own colours and shading; no table of the
  original's pixels ships.
- Presets: the original's factory preset values under our own names and groups, the suite's standing rule; the
  preset files themselves are not copied.
- Shipped on recorded approval: none beyond the above.
- None of the original's code, artwork, preset files, fonts or other files ships with Flatwater Vocal Leveler.
