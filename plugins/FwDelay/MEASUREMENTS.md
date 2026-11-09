# Flatwater Delay: measurements

The current values of every stage of Flatwater Delay, measured on the original it recreates (H-Delay), with the
method and the residual or confidence of each. The data is in [measurements/FwDelay/](../../measurements/FwDelay).

## Setup

| Item | Value |
|---|---|
| Reference | H-Delay (Waves), the H-Delay Stereo, Mono and Mono/Stereo components, from a licensed install |
| Host | `tools/RefRender` offline (`plugins/FwDelay/nulltest/hd.py`); `nulltest/measure_hdelay.py` writes `measurements/FwDelay/results.json` and the echo kernels (`kernel_*.csv`); 48 kHz unless noted |
| Defaults of the probes | Analog Off, Feedback 0, Mix 100, Delay 375 ms, small impulses (0.001) |
| Pre-roll | the reference shows but does not process values set before its first block, so RefRender renders a silent pre-roll (`preroll=`) and sets the values after its first block; both sides get the same pre-roll in whole blocks (a 3 s pre-roll is 144 384 samples, not 144 000, which matters while a glide or the LFO runs) |
| First block | the reference's first block runs at its defaults, Analog mode 2 included, and puts that noise into its delay line; cases start with Feedback 0 and set the case's Feedback at the input's start |
| Parameter changes | a delay time set in the same block as Sync is not taken, so times are set as timed changes (`at=`, also inside the pre-roll, sample exact); choices are set by their normalised value (the reference does not parse its own choice texts); rates of 1 kHz and above are typed in its own form (1.00k: 1000 is not read as 1000 Hz) |
| Model | `plugins/FwDelay/Source/dsp/Engine.h`, shared by the plug-in and `plugins/FwDelay/tools/DeRender`, against the reference through the same host (`plugins/FwDelay/nulltest/nulltest.py`) |

## 1. The kernel

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Interpolator | a Kaiser-windowed sinc cut off at Nyquist, half-length 511/32 and beta 4.968, its edge value kept; 31 taps around ceil(d); a fit of the echo reads half-length 15.89, beta 4.93 (`kernel_48k.csv`), the same at 44.1 kHz | the echo of an impulse | impulses null at -95 dB; the fit within 7e-6 |
| Window | not quite Kaiser (up to 4e-4 off); the sinc hides that in the normal kernel, LoFi shows it | LoFi impulses | LoFi nulls at -70 dB |
| Read position | quantised to 1/32 sample, one phase above the true position: (floor(32 d) + 1) / 32; a whole d is read 1/32 late, about the tap d itself, so at a whole-sample delay the kernel is the 1/32 phase; a static time is a whole number of samples (Host and BPM notes at any tempo give the same kernel) | a sweep of 688 echoes during a slow glide | fractions within 0.0005 of a phase; delays within 0.01 samples rms, a few one phase off |
| LoFi | swaps the interpolator for one cut off at 6277 Hz (the original's fixed 6280 Hz setting), the same window (a fit reads half-length 15.80, beta 4.87) and 1/32 phase; the delay range stays; flat to 4 kHz, -1.05 dB at 5 kHz, -4.5 dB at 6 kHz, then -56 to -68 dB above 10 kHz; no aliasing products (a 1 kHz tone gives only odd harmonics below -78 dB) | impulses, tones | -70 dB |
| Latency | 0 samples, reported and real (RESEARCH.md) | impulse | exact |

## 2. The glide

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Law | after a change of time the delay moves as d' = max(-1, (T - d) / (T + 16)) per sample, each sample's step taken before its read: an exponential approach with a time constant equal to the new time; the playback runs at most twice its normal speed; no limit when the delay grows; with the LFO the glide runs on D = d + 16 (section 5) | a 500 Hz tone's phase on 375 -> 100, 50, 365, 1000 and 3500 ms | the law without the offset within 0.013 to 0.092 ms rms; with it, glides inside the input null at -60 to -83 dB |
| Offset | 16 samples, sharp: 15.5 or 16.5 lose 30 dB on a falling glide | falling glides | exact |
| Slow tail | the delay drifts across a 1/32 phase every many samples, and a sample's difference in when that happens leaves -40 to -50 dB on broadband material | broadband material | known limit |
| State load | loading a state with a new time jumps at once | RESEARCH.md | measured |

## 3. Loop gain and feedback

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Gain stage | in the loop, before the delay line: the wet gain is 1 / (1 + peak); an impulse of a comes back at a / (1 + a) (1.0 -> 0.500, 0.5 -> 0.667, 0.25 -> 0.800, 0.1 -> 0.909) with the kernel unchanged; it takes the peak at once, the sample itself included; linked across the channels (a 1.0 impulse on L brings R's 0.01 down to 0.5 as well); a steady sine of amplitude A comes back at 1 / (1 + A), with odd harmonics (h3 -48 dB at A 0.5, -59 at 0.1); it keeps Feedback above 100 % bounded | impulses, tones | exact |
| Recovery | a one-pole of exactly 10 ms toward 1 (per sample 0.997919) | impulse pairs | nulls -50 -> -100 dB with the exact value |
| Feedback | each echo is Feedback / 100 of the one before, exactly at small levels (0.1, 0.3, 0.6, 1.0 less the gain stage's 1/(1 + 0.001), 1.5, 2.0); the kernel is applied again on each pass (the second echo is the kernel convolved with itself); each pass round the loop adds 1 sample of delay | impulses | exact |
| Without the filters | Feedback above 100 % nulls at -62 to -73 dB | loops | measured |
| Open | whether the gain stage reads the loop's sum or the input; how Feedback 200 % settles | | open |

## 4. Filters

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Type | RBJ biquads in the loop with fixed Qs, HiPass Q 0.8 and LoPass Q 0.95, on the line's output (after the read; with a moving delay the two places differ, and only the output nulls); the second echo is filtered twice | impulses at HiPass 100 / 1000 Hz and LoPass 1000 Hz | within 0.05 dB; nulls -90 to -100 dB |
| Frequency warping | w0 = 2 atan(pi f / fs), so LoPass 4000 sits at 3912 Hz and 15000 at 11 859 Hz (this is why LoPass 5000 reads 0.3 to 0.7 dB under an unwarped biquad near its cutoff) | impulses | exact |
| Off | HiPass Off at the bottom, LoPass Off at the top; set from outside LoPass is Off from about 20 099 up; loaded from a preset only at exactly 20 100, its reset value (section 6) | presets, parameter sweeps | exact |
| Self-oscillation | Feedback above 100 % with HiPass and / or LoPass on (as in the preset Quick Alarm) is chaotic: in the model a 1e-6 change of the input grows to -4 dB within 3 s, so such a loop nulls for its first fraction of a second and then cannot, by any model short of a bit-exact one | the model | known limit |

## 5. LFO

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Waveform | a float triangle between +-1, from 0 rising, a step of (float)(4 rate / fs) a sample, clamped at each end where it turns (the overshoot is lost); each half-cycle is a whole number of steps, ceil(2 / step), so rates whose half-period falls just under a whole number are the sensitive ones; the same on both channels | rate scans (`plugins/FwDelay/nulltest/probe_mod.py`), the delay trace of a steady sine (`plugins/FwDelay/nulltest/probe_lfo_trace.py`, the period over 85 cycles) | exact |
| Target | Tm = (T + 16)(1 + 0.5 depth tri), and the glide runs on D = d + 16: D' = max(-1, (Tm - D) / Tm), the step before the read; linear in Depth (25 gives exactly a quarter of the triangle's span) | the delay trace; `plugins/FwDelay/nulltest/fit_lfo.py` | the earlier form T (1 + 0.5 Depth / 100 tri) fitted within +-0.3 ms |
| Swing | the glide smooths the target, so the swing shrinks as the rate rises: Depth 100 at 0.25 / 1 / 4 Hz gives 93.5 / 74.1 / 31.5 ms peak to peak around 100 ms; at audio rates it gives FM sidebands (Delay 5 ms, Rate 100 Hz: first sideband +9 dB) | tones | measured |
| Rate resolution | Rate is taken as shown, to three significant digits and at least 0.01 (3074 acts as 3070, 1537.24 as 1540, 281.7 as 282, 47.84 as 47.8, 0.537 as 0.54), unless loaded from a preset (section 6) | rate scans, the delay trace | 3074 Hz: -10 to -54 dB without the rounding, -76 to -84 dB with it |
| Sync | in Host / BPM sync Rate counts cycles per beat at 60 BPM: the LFO runs at Rate x tempo / 60 | the delay trace | exact |
| Nulls | modulated material -67 to -84 dB (Depth 39 / 100, Rate 1 to 6000 Hz, delays 30 / 750 ms) | `plugins/FwDelay/nulltest/nulltest.py` | |

## 6. Sync, tapers and parameter resolutions

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Sync | Host and BPM note values land on the note's time (500.0, 444.44, 300.0 and 225.0 ms), less what the glide has not yet covered; triplet notes are a hair short (x (1 - 3e-9): 1/8T at 120 BPM is 7999 samples, not 8000); a note whose time at the tempo passes 3500 ms is not taken (at 120 BPM the knob stops at 1 bar; 2 bars are 4 s), and Flatwater Delay holds the longest that fits | echo positions | exact |
| Linked times | without Dual the two times are linked: setting Delay R moves both (the last one set wins) | renders | exact |
| Delay taper | 1 ms per 1/1000 of the range up to 501 ms at the middle (exact), then a curve to 3500 ms that no simple law fits within the display's 1 ms (a power law is 57 ms off, a cubic 14 ms); Flatwater Delay keeps the reference's values at 201 points and interpolates between them | each parameter set to 101 normalised values, its text read back | exact at the points |
| Other tapers | Rate 0.1 to 6000 Hz and both filters 19 to 20 138.8 Hz logarithmic (the middle at 24.5 Hz and 619 Hz, the geometric means); HiPass Off at the bottom, LoPass Off at the top (from 20 100 Hz); BPM 30 to 300, Feedback, Mix, Depth and Output linear; the reference keeps fractional values (its presets store Depth 0.69 %, Delay 243.9 ms), so Flatwater Delay's values are continuous and only their texts are rounded | the same | exact |
| Set from outside | a value from a host, automation or a knob is used at the original's parameter resolution: Delay, Feedback, Mix, BPM, Depth and the filters in whole units (Depth 0.69 acts as 1), Output 0.1 dB, Rate to three significant digits and at least 0.01 | `plugins/FwDelay/nulltest/null_presets.py`, which loads the reference's own preset arrays into its state | exact |
| Loaded from a preset | a value is used exactly as stored: Depth 0.69, Rate 281.7 and 47.84 (the delay trace's period: 1004 samples, not 47.8's 1006); nine factory presets store LoPass 20 139, which the reference shows as Off but runs as a filter at 20.1 kHz; a stored Delay is still taken in whole ms (6.6 acts as 6); Flatwater Delay keeps a factory preset's values in the state (`fw::presetExactTag`) and uses them while a parameter stays where the preset put it, moved it is used at its resolution | the same | impulses null at -100 dB against a 20 139 Hz filter, +0.9 dB against Off |

## 7. Routing, mix and output

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Phase L / R | invert their side's input to the delay line; the dry signal stays | impulses | exact |
| Ping Pong | (+-L +- R) / sqrt(2) goes into the start side's line (Phase R gives (L - R) / sqrt(2)); the first echo comes on the start side, the second on the other side at the same level, then every further pass at Feedback^2 (Feedback 30: 0.707, 0.706, 0.064, 0.064, 0.0057 ...): the start side's line feeds the other side's at gain 1, which feeds back at Feedback^2; each line adds its sample | impulses | -97 to -100 dB, either start side |
| Mono | behaves as the stereo component; Mono/Stereo feeds its single input into Ping Pong at sqrt(2) | impulses | exact |
| Mix | equal-power: dry cos(pi/2 m), wet sin(pi/2 m) (25 %: 0.924 / 0.383) | impulses | exact |
| Output | an exact dB gain on both; nothing limits the output (+18 dB passes a 0.9 sine at 7.15) | tones | exact |

## 8. Analog

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| Position | colours the input before the dry / wet split, so the dry and the delayed signal both carry it (with Mix 50 its noise comes back in the echo at a correlation of 0.50 at the delay's lag) | noise correlation | measured |
| Mode 4 | Flatwater Comp's mode 4: its fitted model applied to the reference's Analog-Off output | renders | -100 dB (peaks 0.55), -79 dB (peaks 0.06) |
| Modes 1 to 3 | not Flatwater Comp's, although the original's Analog settings carry the same numbers (no Comp mode nulls them better than -31 dB); fitted with Flatwater Comp's model on the reference's own data (`plugins/FwDelay/nulltest/measure_analog.py`, `plugins/FwComp/nulltest/fit_analog2.py` with `ANALOG_PREFIX=hd`), exported by `plugins/FwDelay/scripts/export_analog_tables.py` to `plugins/FwDelay/Source/dsp/AnalogTables.h`: mode 1 with its low dip as a band-pass before H1, mode 3 with a band-pass at 71 Hz, Q 0.41 | the 54-case probe set | median mode 1 -53.0 dB, mode 2 -41.6, mode 3 -41.3; the quietest probes limited by the modes' own noise |
| Small-signal responses | at -30 dBFS (noise in, `results.json` `analog`): mode 1 a low cut (-3.6 dB at 30 Hz, -0.3 dB in the mids); mode 2 a high shelf (-0.67 dB below 200 Hz, -1.55 dB above 4 kHz); mode 3 a deep dip near 100 Hz (-13.9 dB) that depends on level; mode 4 within 0.9 dB | noise | measured |
| Low-level transfer | modes 1 and 2 agree with the reference at every frequency; mode 3 is off below 1 kHz (the reference 1 dB lower below 20 Hz, up to 1 dB higher around 100 Hz), and the ratio is not a smooth filter: mode 3 is still non-linear at these levels (its single-sine response at -50 dBFS differs from the multitone's) | `plugins/FwDelay/nulltest/probe_analog_tf.py`, a multitone at -60 dBFS per tone, Mix 0 | modes 1 and 2 -50 to -85 dB; mode 3 -18 to -40 dB |
| Noise | with silence in: -100.9 / -83.7 / -104.0 / -106.8 dBFS for modes 1 to 4, and -122 dBFS even with Off; with Mix 0 -102.7 / -85.5 / -106.0 / -108.9 dBFS; independent per channel | silence | measured |
| In the null suite | Analog 1 -53 to -56 dB, 2 -51 to -55, 3 -40 to -42 (the noise included), 4 -83 to -90 | `plugins/FwDelay/nulltest/nulltest.py` | |

## 9. Other rates

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| 44.1 kHz | behaves as 48 kHz | the suite | -95 dB |
| 96 kHz kernel | the same filter in time: 62 taps, cut off at 24 kHz (a fit reads half-length 31.06, beta 4.71), the read 1/16 sample late; the delay settles one sample short (an echo at T - 1 + 1/16) | impulses | first echo -78 dB |
| 96 kHz LFO | runs twice as fast: its step is 4 rate x factor / fs (factor 2, the interpolator's) | the delay trace (a 0.6 s period at Rate 0.8) | the modulated case +2.6 -> -48 dB with it |
| 96 kHz nulls | dominated by the band above 20 kHz, round the kernel's 24 kHz cutoff, where the kernels differ; `plugins/FwDelay/nulltest/nulltest.py --fs 96000 --band 20000` measures below 20 kHz | the suite | the delay path -46 to -61 dB below 20 kHz, -28 to -60 dB over the whole band; loops at high Feedback -31 to -44 dB |

## 10. The model against the reference

| Quantity | Value | Method | Residual or confidence |
|---|---|---|---|
| The delay path | the defaults, ms and synced times, the filters, Ping Pong from either side and Feedback up to 200 % | [NULLTESTS.md](NULLTESTS.md) | -84 to -104 dB |
| Modulation | modulated cases | the same | -60 to -84 dB |
| Factory presets | the reference's 88 presets loaded as it loads them, against ours with the values as stored | [NULLTESTS-presets.md](NULLTESTS-presets.md) | median -53.2 dB; Analog mode 3 the weakest part (about -41 dB on every material) |

## 11. Declined changes

| Change | Measured effect |
|---|---|
| Glide offset 15.5 or 16.5 samples | 30 dB lost on a falling glide |
| Double- or float-precision variants of the glide update | do not reproduce the reference's 1/32 phase steps in a glide's slow tail |
| The filters on the line's input | do not null with a moving delay |
| A correction EQ for Analog mode 3 | cannot fix it: the mismatch is non-linear at low level, not a smooth filter; mode 3 needs a different structure ([docs/improvements.md](../../docs/improvements.md)) |

## 12. The editor

Measured with `tools/MeterCapture` screenshots, films and drag, click and wheel scripts on the reference editor
(screenshots in the git-ignored `reference/hdelay/`, including `reference/hdelay/ppdual`). Every value is a
measurement the redrawn editor uses, not a copied pixel.

| Quantity | Value |
|---|---|
| Panel | a base colour (183.5, 100.5, 36) with a broad warm highlight (+42.5, +55.9, +15.8) centred at (107, 73) over about 100 x 106 px, darkened from y 229; the edges plateaus, not a fall-off: 0.89 inside the left edge, 0.82 inside the right, 0.795 along the bottom; a 1 px black outline; under the toolbar a brown-grey row (115, 96, 81) and a pale lip; residual 2.4 levels rms (the grain); the mean difference over the editor below the toolbar 8.0 levels, the panel's flat parts within 1.6, no colour bias |
| Knobs | measured as Flatwater Comp's (`plugins/FwComp/scripts/fit_knobs.py`, captures of Feedback, Mix, Output and the four small knobs typed to nine values): Delay and Feedback are fluted black knobs in a chrome skirt of radius 63.4, the flute ring (mean radius 49, ten flutes +-2.3 px) 1.1 px right of the skirt's centre and the cap 1.1 px right and 2.3 px down (a raised knob seen a little from above), the flutes up to 3 degrees off the pointer's angle (each capture's own flute phase is used), the pointer fin from radius 45.5 to 64.5; the small knobs (radius 32.4, centres at (181.4, 223.2), (286.2, 222.6), (407.2, 222.6), (542.3, 222.6)) have a knurled rim; the right column's (radius 23.65) a light line on the cap from the centre to 13.5 px; scales: 21 ticks of 14.1 degrees from 39 to 321 degrees on every knob (Analog 5), radii 64.3 to 72.2 / 33.5 to 38.7 / 24.7 to 30.6; the remaining 20 levels on the flute ring come from the edge's sub-pixel position, the chrome's fine streaks and the cap's scratches |
| Meter | two red triangles, one per channel, pointing inward along the cream scale, 10 x 8 px, a pink lip on top, a shadow under them, their point 1 px below the scale's minus infinity at rest; the scale y = 83.70 + 157.82 (1 - 10^(-dB/40)) on the ticks within 0.3 px (the printed marks: 0 at 83.3, 6 at 129.3, 24 at 201, 48 at 231.7; rest at 239.3); ticks 7 px from both sides; numbers in a bold serif, red (136, 12, 10) to 24, olive below; the frame by depth per side (7.8 rms, the corners), the window as a rank-3 product of column and row profiles with the print masked (2.5 rms), `tools/fit_hdelay_meter.py`; mean difference 5.7 |
| Display | 292, 66, 139 x 72: dark glass with a smoky streak (its lip (171, 147); a sunk window: 1 px black rim, a dark inner ring, the panel lit along its right and bottom edges, darkened on the left); amber segment characters (255, 180, 37 at their brightest): the note as a stacked fraction and T / D, the tempo in 13.5 x 23 px digits with BPM under it (Host, BPM), four digits of ms (MS), the time and the start side in an amber box (Ping Pong), L and R halves (Dual) |
| Display font | a 7-segment renderer measured on the reference's amber coverage (R - B of its pixels, `tools/fit_hdelay_display.py`): the tempo and ms digits 12.85 x 22.65 px, strokes 2.45, slant 0.175, pitch 17.05, their 1 one stroke; a note's figures 9.85 x 16, strokes 2.0, slant 0.19; T and D, BAR, BPM, Ping Pong's and Dual's sizes each measured on their own; the fraction's bar one dash per denominator digit (pitch 13.6); the figures of a note are not evenly spaced (a bitmap font): positions measured on every note (1 at 302 or 308, T / D at 321.7 or 331.7, the denominators each their own); coverage rms 0.04 to 0.11 of full per item; every display state 4 to 5 levels off the reference |
| Start box (Ping Pong) | an amber (255, 180, 0) box 391.2 to 412.8 x 95 to 122, leaning 0.04 px right per px up, corners of 2.1; the letter straight strokes that stop short of each other, as on a segment display: a 1.27 px black core in a 1.65 px stroke at 0.53; unselected, its rim a black line down the left and rows 0.27 darker above and below; a click flips the start side and selects the box, drawn as a yellow line round it at half strength (`fit_hdelay_display.py ppbox`, rms 0.038 of full coverage) |
| Ping Pong and Dual | two independent switches that can both be on; Ping Pong's layout then shows and the Delay knob sets the left side; Ping Pong's layout is Dual's left half, the same pixels (the small note and the tempo, or the left time in MS), with no L / R labels and no divider, and the start box on the right |
| Dual's sides | clicking the left or right time or note chooses the side the Delay knob sets, and dragging it sets that side's value; on the right side a small box follows DELAY under the knob (black, 11 x 10 px at (235, 169), a half-dark shadow along its left and bottom, the R cut out of it in the panel's colour, drawn over the knob's shadow); the chosen side is kept while Ping Pong is on and shown again when it goes off; switching Dual on starts the right side from the left's time |
| Display drags | a note steps 1 value per 40 px of drag, as on the knob, in every layout; in MS a drag on the right time moves it 1 ms per px; in BPM the tempo drags in Dual's and Ping Pong's layouts too (20 px gives 5 to 6 BPM); dragged or wheeled with the knobs' law it sets the time in MS and the tempo over its digits in BPM (+27 BPM for 50 px fast, +12 for three notches) |
| Hit areas | follow the reference's control areas; where two overlap, one takes the point: in Ping Pong the start box, the left time or note and the tempo each have their own area; in Dual the left and right times or notes and the tempo; in the plain layout the whole display sets the time in MS; with notes, the note has its own area and the rest of the display sets the tempo |
| Display match | within 3.8 to 4.7 levels in every layout; the start box 2.2 to 4.2; the R mark 2.5 |
| Tap pad | two zones (the body by its depth in from the outline, the face by its depth in from its frame) with per-side profiles, released and pressed (pressed: the top lip dark, the face sunk, its lower edge lit), `tools/fit_hdelay_tap.py`: 6.0 / 6.7 levels rms, render 3.8 / 4.1 off outside the mark; its cast shadow 0.44 deep, (0.24, 1.81) px; selected after a click, a 1 px yellow line just outside the pad |
| Buttons | a 2 px black frame and 2 px dividers; faces (215, 209, 186), lit (173, 166, 139); three lit rows on top, two lighter rows and a 4-row bevel at the bottom, dark columns next to the dividers; labels Arial Bold 10.2 px unstretched (the closest of ten system fonts to the reference's ink), within 0.25 px of the reference's; a sharp shadow 0.63 of the panel for 6 rows under each button, 0.87 under a lit one |
| Selection | one control at a time: a knob (its ring), a group (Sync: the yellow line over its frame's outer edge), a button of the row (the line round its face), LoFi, the tap pad |
| Print | black (the Comp reference's print is dark brown); the brackets' strokes end 2 px from their words; the reference's product name in Arial at 81 x 18 px from x 42 (ours carries the Flatwater name plate there) |
| Lamp and Link | the lamp's bezel bright only on top, its red arc 4 to 7 o'clock; the Link wheel's 8 square teeth about (473.2, 218.3), the ring of 19.2 about (473.8, 219.2), the arc's top at y 195 |
| Toolbar | Flatwater Comp's, its preset box 106 px wider (the editor is 749 px wide, Flatwater Comp's 643) |
| Knob feel | Flatwater Comp's law on every knob: 0.001 of the range per px on mouse steps up to 2 px, 0.002 from 3 px, Ctrl a quarter of either; the wheel 1/72 of the range per notch; Analog one position per 50 px; the Delay note one value per 50 px slow, two per 50 px fast; ours give the same values within the display's last digit (Feedback 60 -> 70 / 70 -> 50, Output -1.8 -> -1.6 with Ctrl, Delay in MS 375 -> 425 ms slow, -100 ms fast, Rate 0.10 -> 0.17 Hz) |
| Link wheel | moves both filters by the same share of their range, with the knobs' law, as far as the one nearer its end allows (HiPass 154 -> 77 Hz and LoPass Off -> 10.0 kHz for 50 px fast down; three notches 77 -> 103 Hz, 10.0 -> 13.3 kHz; ours 13.4) |
| Script artefact | the drag script moves the mouse out of the editor after each drag; the next drag then loses part of its travel on both sides (Rate 0.10 -> 0.12 instead of 0.17 Hz); `away=0` keeps the mouse over the editor; some scripted drags on the reference do nothing at random, so laws are read from fresh instances |
| Not matched | the text's hinting (the reference's glyphs sit on whole pixels; JUCE draws outlines), the toolbar's own content (ours), the knobs' fine texture |

## CPU

| Configuration | Cost |
|---|---|
| Stereo at 48 kHz, Feedback 60, Analog 2 (`DeRender`) | about 1.6 % of one core |

## Provenance

The reference is the original H-Delay, run from a licensed install.

- Measured: renders of the reference's output (impulses, sweeps, noise, music) at its settings and sample rates,
  including its glides, LoFi and Analog modes; screenshots, films and drag scripts of its editor.
- Fitted or learned from the reference's output: the delay path, the interpolator's kernel and the Analog modes'
  measured tables (`plugins/FwDelay/Source/dsp/AnalogTables.h`), identified from those renders.
- Redrawn: the editor, drawn in code in Flatwater's own dark look (`shared/ui/Theme.h`) on the original's
  measured layout (the positions and sizes of the controls), with our own colours and shading; no table of the
  original's pixels ships.
- Presets: the original's factory preset values under our own names and groups, the suite's standing rule; the
  preset files themselves are not copied.
- Shipped on recorded approval: none beyond the above.
- None of the original's code, artwork, preset files, fonts or other files ships with Flatwater Delay.
