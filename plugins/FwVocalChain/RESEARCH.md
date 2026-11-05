# Flatwater Vocal Chain: research

What the vendor documents for CLA Vocals, the original Flatwater Vocal Chain recreates, and how the original behaves in
a host: its behaviour, parameters, editor and presets. No file of the original is copied into this repository. The
measured behaviour is in [MEASUREMENTS.md](MEASUREMENTS.md).

## Sources

| Source | What it is |
|---|---|
| C1 | The official product page, [waves.com/plugins/cla-vocals](https://www.waves.com/plugins/cla-vocals), and the manual linked from it, [cla-vocals.pdf](https://assets.wavescdn.com/pdf/plugins/cla-vocals.pdf) |
| C2 | The original plug-in observed in a host: its parameters and state, its 43 factory presets, the chain's own stored settings, and its editor on both components |
| C3 | The original plug-in as a host loads it (`nulltest/probe_vocalchain.py`): the Stereo component and the Mono/Stereo component (1 in, 2 out: pedalboard cannot host it; `tools/RefRender` can, loaded by `name@uid`, since by name alone it loads another plug-in; `RefRender --list` prints the uids) |
| C4 | Frames of the real editor read back from its OpenGL drawing (`tools/MeterCapture` with `gl=1`, 738 x 474 including the toolbar, git-ignored `reference/fwvocalchain/shots/`) |

## Documented behaviour (C1)

- An artist-signature vocal chain: six faders, each driving one stage of a fixed chain designed by the engineer, each
  with a Color selector (three flavours and a clear position). With every colour clear, some fixed processing is
  still active. The EQ faders act only once moved (at 0 the EQ colours do nothing); the other faders are active at 0
  and give the default sound there.
- Sensitivity (input) +-10, 0.1 steps, default 0, with the Sensitivity LED: off (too low), green (good), yellow
  (optimal), red (very hot). Input meter -26 ... 0 dBFS peak with a clip LED (click to reset).
- Bass +-10 (0.1), colour Clear (bypass) / Sub (default) / Lower / Upper. Treble +-10, Clear / Bite (default) / Top /
  Roof. Compress +-10, Clear / Push (default) / Spank / Wall. Reverb +-10 (wet mix), Clear (mute) / Tight (default) /
  Large / Chamber. Delay +-10 (wet mix), Clear / Slap (1/16 note, default) / Eight (1/8) / Quarter (1/4), synced to
  the host tempo. Pitch +-10 (wet mix of a pitch modulation: chorusing and doubling), Clear / Stereo (default) /
  Wide / Spreader. Output +-10 dB with a peak meter and clip LED. Balance +-6 dB (Stereo component only).

## Parameters (C2) and the host's view (C3)

Flatwater Vocal Chain's parameters, in the original's host order, with the original's ranges, steps and defaults:

| ID | Label | Range | Default | Notes |
|---|---|---|---|---|
| `sensitivity` | Sensitivity | +-10 dB, 0.1 | 0 | input gain; the original keeps it when a preset loads |
| `bass`, `bassColor` | Bass, Bass Color | +-10, 0.1; Bypass / Sub / Lower / Upper | 0, Sub | |
| `treble`, `trebleColor` | Treble, Treble Color | +-10, 0.1; Bypass / Bite / Top / Roof | 0, Bite | |
| `compress`, `compressColor` | Compress, Compress Color | +-10, 0.1; Bypass / Push / Spank / Wall | 0, Push | |
| `reverb`, `reverbColor` | Reverb, Reverb Color | +-10, 0.1; Mute / Tight / Large / Chamber | 0, Tight | wet level |
| `delay`, `delayColor` | Delay, Delay Color | +-10, 0.1; Mute / Slap / Eight / Quarter | 0, Slap | wet level, synced to the host's tempo |
| `pitch`, `pitchColor` | Pitch, Pitch Color | +-10, 0.1; Mute / Stereo / Wide / Spreader | 0, Stereo | wet level |
| `output` | Output | +-10 dB, 0.1 | 0 | the original keeps it when a preset loads |
| `balance` | Balance | +-6 dB, 0.1 | 0 | Stereo component only |
| `mode` | Mode | Stereo / Mono/Stereo | Stereo | which of the original's two components Flatwater Vocal Chain behaves as |
| `ioLink` | Link In/Out | off / on | off | Flatwater's own, not in the original |

The original shows a host 15 parameters: bypass, Sensitivity, the six faders and their colours, Output and Balance (its
colour choices have a trailing space: `'Sub '`, `'Push '`). Its state holds more that a host does not see: the
Sensitivity LED and its level curve, the meters, a bypass for each stage of its chain, and Noise (a toggle, on by
default).

| Item | Value (C3, RefRender) |
|---|---|
| Latency | 193 samples at 44.1 and 48 kHz, 385 at 96 kHz (a fixed number of samples, doubled at 96 kHz), the same on both components; an impulse's peak lands 1 sample after the reported latency (2 at 96 kHz) |
| Impulse | an impulse of 0.5 comes out at 0.375 (48 kHz) with the defaults |

## The chain

The original is a fixed chain of processing stages: a channel EQ and a second EQ (Bass and Treble), a limiter and a
compressor in series (Compress), two de-essers (the main one, and one on the reverb's send), a reverb, a delay and a
tap delay (Delay), a doubler (Pitch), and gain stages after the wet effects. Each colour loads a stored setting of a
stage and each fader moves one of that stage's settings; which settings, by which law, is measured on the audio
(MEASUREMENTS.md). The delay stage is the processor Flatwater Delay recreates. No stage is available on its own, so
each is measured inside the chain, the others cleared through the colours.

## Presets

43 factory presets, each storing the faders, the colours and Balance (Sensitivity, Output and the meters kept). Most
names are the engineer's own, or songs' and artists' names: Flatwater Vocal Chain renames all of them, carrying only
the settings.

## The editor (C4)

738 x 474 px with the toolbar; its layout was measured on the frames. A dark metal panel: a header plate with the
engineer's signature (Flatwater Vocal Chain uses its own name plate and draws no signature, name, initials or
likeness), a grille, a Vocals badge; a colour display over the six channel faders (each stage's three flavour names
stacked, the selected one lit) with a lit push button per fader that steps the colour; eight vertical faders (Input
Sens, Bass, Treble, Compress, Reverb, Delay, Pitch, Output) on +-10 scales; stereo input and output LED meters
(0 ... 24 dB down) with clip LEDs; the Sensitivity LED over the input meter; Balance as a small knob under the input
meter (L / R).

- The two components: the Mono/Stereo editor differs from the Stereo one only round the Balance tab. A marked control
  is drawn in yellow (the fader cap's frame, the push button's ring, the knob's ring). The flavour window's dots: every
  other pixel of every other row lit.
- Text: the flavour names are bold italic capitals 6 px tall in measured ink boxes (rows at y 119, 131, 143); lit
  green (100, 192, 72), blue (68, 160, 202), red (255, 60, 60), unlit (74, 74, 74). The value box: 37 x 13 at y 454
  under the control, black at 0.8 in a 1 px white frame, the digits 7 px tall in grey 200, centred.

## The editor's behaviour (C4)

Filmed with `tools/MeterCapture gl=1` click, hover and drag scripts on the Stereo and Mono/Stereo components
(`build/meters/vc_a` ... `vc_d`, `vc_m`):

| Element | Behaviour |
|---|---|
| Hover | no hover marks: the mouse over any control shows nothing |
| Selection | a click on a fader's cap marks it (the yellow frame) and it stays marked when the mouse leaves; a click on another control moves the mark; a click on the background (the red plate, the badge, the grille, the panel's foot), on a meter, on a flavour's name, or on a fader's track off its cap clears it; Ctrl-click marks; Shift-click on the marked control unmarks it; dragging on the background draws no lasso and marks nothing |
| Value box | shown while the marked control is under the mouse and while it is dragged (live value); hidden when the mouse leaves, though the mark stays |
| Faders | grabbed on the cap only (a click above or below it moves nothing); the cap follows the mouse at 7.55 px per step (40 px: 5.3; 30 px: 4.0); the cursor is hidden while dragging and put back on the cap where it went; Alt-click on the cap resets to 0 (and marks); the wheel moves 1/72 of the range per notch over the whole control (3 notches: 0.9); a double-click on the cap opens the text entry (3.5 typed: 3.5) |
| Colour push button | a click steps to the next flavour (Sub, Lower, Upper, off, Sub) and marks the button (yellow ring); unlit (grey) while the colour is off; a click on a flavour's name selects it, a click on the lit name turns the colour off; neither marks anything (the mark is cleared) |
| Balance | a click marks it (yellow ring round the cap); a vertical drag turns it at the knobs' rate (30 px in 0.6 px steps: 0.4 of +-6); the cursor is hidden and put back where the drag began; Alt-click resets to 0; a double-click types a value; hidden on the Mono/Stereo component, its tab showing the meter's screw |
| Meter | a click marks nothing (and clears the mark); the clip lights reset on a click (C1) |

Flatwater Vocal Chain filmed with the same scripts (`build/meters/wc_a`, `wc_c`) matches the marks, the value box and
every value frame for frame. MeterCapture's typed keys do not reach a JUCE plug-in's text box, so the text entry is
checked in a DAW.
