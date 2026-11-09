# Flatwater Vocal Leveler: research

What the vendor documents for Vocal Rider, the original Flatwater Vocal Leveler recreates, and what the original shows
in a host: its behaviour, parameters, editor and presets. Nothing of the original is copied into this repository.
The measured behaviour is in [MEASUREMENTS.md](MEASUREMENTS.md).

## Sources

| Source | What it is |
|---|---|
| V1 | The public manual, [vocal-rider.pdf](https://assets.wavescdn.com/pdf/plugins/vocal-rider.pdf); the product page [waves.com/plugins/vocal-rider](https://www.waves.com/plugins/vocal-rider) |
| V2 | The original plug-in observed in a host: its parameters (ranges, steps, defaults) and its 13 factory presets, each loaded into the original and read back |
| V3 | The original's VST3 as a host sees it (`plugins/FwVocalLeveler/nulltest/probe_parameters.py`, and `tools/RefRender`, which prints the input buses and enables a mono side-chain on mono components): Vocal Rider Mono, Vocal Rider Stereo, Vocal Rider Live Mono, Vocal Rider Live Stereo |
| V4 | Frames of the real editor read back from its OpenGL drawing (`tools/MeterCapture gl=1 in=...`, 320 x 598, git-ignored `reference/fwvocalleveler/shots/`), with voice-like syllables (`reference/fwvocalleveler/syllables.f32`) |

## Documented behaviour (V1)

- Rides vocal levels automatically in real time, like a hand on the fader, with zero latency and no pre-scan; it
  does not colour the sound (gain only). Meant as the last plug-in on a vocal track or group.
- Splits the mix into vocals (the track) and music (everything else, fed post-fader to the side-chain).
- Target 0 ... -42 dBFS (default -21): the vocal level the riding aims at; it also calibrates the Rider fader's 0. A
  transparent screen over the vocal level meter.
- Vocal Sensitivity -12 ... +12 dB (default 0): the vocal detector's threshold, which tells vocal content from noise
  floor, spill and ambience and finds the starts and ends of words. Vocal activity display: Ride / Idle LEDs.
- Attack Fast / Slow (default Slow): the detection timing and the fader's riding speed.
- Music Sensitivity -12 ... +12 dB (default 0; the host shows an Off below -12): how much the riding follows changes
  of the instrumental level (needs the side-chain). Its LED goes from soft to bright green as the music affects the
  riding more.
- Spill -12 ... -48 dBFS (default -48), Live components only: signal below it is ambient stage noise, not vocal.
- Range Min -12 ... +11 dB (default -6), Range Max -11 ... +12 dB (default +6), the Effective Range between them
  (1 ... 24 dB); the Idle arrow (-12 ... +12, default 0) is the Rider fader's value when no vocal is detected.
- Rider fader -12 ... +12 dB: the automatic gain; grabbed with the mouse it is a manual gain, and riding resumes on
  release (a control surface's Manual button does the same).
- Automation Off / Write / Read: Off rides from the internal engine; Write sends the fader's movements to the host's
  automation lane as if written by hand; Read plays the lane back and ignores the engine.
- Output -12 ... +12 dB (default 0) after the riding, with a clip LED.

## Specifications (V3, V4, measured)

| Item | Value |
|---|---|
| Latency | 0 on all four components (reported and real: cross-correlation lag 0); Studio and Live differ in the Spill control (and a LIVE badge), not in latency |
| Buses | every component has a side-chain bus (Mono SideChain 1 channel, Stereo SideChain 2) besides its main input; an offline check (a steady -21 dBFS buzz, music a buzz stepping from -40 to -6 dBFS on the side-chain, Music Sensitivity -12 / 0 / +12) shows no effect of the side-chain on the gain (MEASUREMENTS.md section 4) |
| Static level at the defaults | a 180 Hz buzz (harmonics falling 6 dB per octave), after 4 s: gain +6.00 dB (the Max range) at -48 and -36 dBFS rms, +0.69 at -27, -3.48 at -21, -6.00 (the Min range) at -15 and louder; the law behind it is MEASUREMENTS.md section 3 (the default Target -21 does not mean 0 dB gain at -21 dBFS rms, and the slope between -27 and -21 is not -1 dB per dB) |
| VST3 parameters | 10 plus bypass (Spill only on Live): Target (421 steps of 0.1 dB), Vocal Sensitivity (+-12, 0.1), Music Sensitivity (Off, -12 ... +12), Range Max, Range Min (0.1 dB), Idle (shown in dB, +-11.5), Rider Fader (+-12 dB, 0.1: the fader is a host parameter, which is how Write sends automation), Output, Attack (Slow / Fast), Spill (-48 ... -12) |

## Parameters (V2)

Flatwater Vocal Leveler's parameters (`plugins/FwVocalLeveler/Source/PluginProcessor.cpp`). The meters, the Ride / Idle LEDs, the clip LED, the Effective Range and the Idle value in dB are display
only, not parameters. The original's presets do not store Spill.

| ID | Label | Range, step | Default | Notes |
|---|---|---|---|---|
| target | Target | -42 ... 0 dB, 0.1 | -21 | the vocal level the riding aims at |
| musicSensitivity | Music Sensitivity | -12.1 (Off) ... +12 dB, 0.1 | 0 | how much the riding follows the music on the side-chain |
| rangeMin | Range Min | -12 ... +11 dB, 0.1 | -6 | the lowest gain the riding may set |
| rangeMax | Range Max | -11 ... +12 dB, 0.1 | +6 | the highest gain the riding may set |
| vocalSensitivity | Vocal Sensitivity | -12 ... +12 dB, 0.1 | 0 | the vocal detector's threshold |
| output | Output | -12 ... +12 dB, 0.1 | 0 | gain after the riding |
| rider | Rider Fader | -12 ... +12 dB, 0.1 | 0 | the automatic gain; written and read by Automation |
| automation | Automation | Off / Write / Read | Off | not automatable |
| idle | Idle | 0.02 ... 0.98, 0.01 | 0.5 | the Idle arrow as its place in the range; the host shows it in dB over a fixed +-12 dB |
| attack | Attack | Slow / Fast | Slow | detection timing and riding speed |
| spill | Spill | -48 ... -12 dB, 0.1 | -48 | Live components only |
| mode | Mode | Stereo, Mono, Live Stereo, Live Mono | Stereo | the original's four components |

Besides its controls, the original keeps fixed internal settings that sketch the design: a voice activity detector
(a threshold, attack and release) on a weighted level (a 100 Hz high-pass at -18 dB and a 6.5 kHz low-pass at -6 dB,
measured as shelves: MEASUREMENTS.md section 1); rms followers for the vocal and for the music (attack, release, hold,
a -42 dB bias); a fader smoother (release 1000 ms); a park glide to the Idle value (1205 ms, hold 50 or 500 ms); an
output smoother. The presets tie four of them to Attack: Slow presets have a fader smoother attack of 0.55 ms, a vocal
follower attack of 50 ms, an output smoother attack of 50 ms and a park hold of 500 ms; Fast presets 0.35, 10, 10 and
50 ms (the original's own initial values are the Fast ones while Attack starts Slow). Switching Attack sets these
values (MEASUREMENTS.md section 3).

## GUI geometry (V2, V4)

The view is 320 x 598 px including the two-row toolbar (the Flatwater Bass Leveler original's is 316 wide). The layout of every control was
measured on the editor's frames (V4, `reference/fwvocalleveler/shots/default_400.png`, `live_400.png`):

- Dark top panel: the Target slider over the vocal level meter (a yellow bar); the Ride / Idle LEDs, the Fast / Slow
  switch below them; a Sensitivity group with the Vocal and Music knobs (0 at the top, minus and plus at the ends) and
  the Music LED (a round lens, right). Live adds a small Spill knob and moves Vocal and Music right.
- Cream lower panel as on the Flatwater Bass Leveler original: the Range column (Max and Min thumbs, the Idle marker with a hover text, the
  Range box), the Rider fader (a wide thumb with red centre lines, a +-12 scale on both sides; an area beside it
  shades the band between Min and Max and draws the Idle line), a small lamp above it that lights while the fader is
  grabbed, the Output fader with a clip LED, and value boxes below each fader.
- Automation OFF / WRITE / READ radio buttons (yellow, dark red, dark green), a lamp above.
- A music level bar is part of the original's layout.
- Bottom: dark leather with the product name in large type (and a LIVE badge on Live).

## Presets (V2)

13 presets (Full Reset and 12 riding styles: smooth, moderate, fast, low, high, rocky, loud, a VCA, an attenuator, a
hard-limit ride, a de-compression with the full +-12 range). Each stores Target, Music and Vocal Sensitivity, Range
Min and Max, Output, Idle, Attack and the Attack-tied internal settings above; the rest are shared. Flatwater's
presets carry them over by their values under Flatwater names; their null test loads the reference's own preset states
([NULLTESTS-presets.md](NULLTESTS-presets.md)).
