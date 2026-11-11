# Flatwater Delay: research

What the vendor documents for H-Delay, the original Flatwater Delay recreates, and how the original behaves in a host:
its behaviour, parameters, editor and presets. No file of the original is copied into this repository. The measured
behaviour is in [MEASUREMENTS.md](MEASUREMENTS.md).

## Sources

| Source | What it is |
|---|---|
| D1 | The official product page, [waves.com/plugins/h-delay-hybrid-delay](https://www.waves.com/plugins/h-delay-hybrid-delay), and the manual linked from it, [h-delay-hybrid-delay.pdf](https://assets.wavescdn.com/pdf/plugins/h-delay-hybrid-delay.pdf) (H-Delay Hybrid Delay User Guide) |
| D2 | The original plug-in observed in a host: its parameters, its editor and its 88 factory presets (read back through the reference plug-in, `plugins/FwDelay/nulltest/read_ref_presets.py`) |
| D3 | The original plug-in as a host loads it: components H-Delay Mono, H-Delay Stereo and H-Delay Mono/Stereo (one input and two outputs, which pedalboard cannot load); automatable parameters below; no side-chain bus |
| D4 | Screenshots of the real editor taken with `tools/MeterCapture` (git-ignored `reference/hdelay/`): stereo and mono at the defaults, MS sync, a Delay drag, Ping Pong, LoFi, Dual |

## Documented behaviour (D1)

- A hybrid delay: slap-back, ping-pong, tempo sync, modulation, filtering, flanging and phasing.
- Delay 1 ... 3500 ms, or a note value when synced (default 1/8D, Host sync). Changing the delay time works like a tape
  machine changing its motor speed: the echoes already recorded change pitch until the new speed reaches the playback
  head.
- Sync: Host (note values of the host tempo), BPM (note values of a BPM set in the plug-in, set by tapping), MS
  (milliseconds). The time display shows the nearest value when the mode changes.
- Tap Pad: the delay is the interval between the last two clicks (Host: quantised to the nearest note value; BPM: the
  tap sets the BPM).
- Feedback 0 ... 200 % (default 60): repeats decay below 100 and build up above it.
- Ping Pong (stereo components): two delay lines feeding each other, with a start direction (left or right). Dual:
  separate left and right times (the display's halves are dragged; the Delay knob follows the selected side);
  Feedback, Modulation and the filters stay common.
- Phase L / R invert each delay line (linked on the mono-to-stereo component).
- Modulation: Depth 0 ... 100 (default 0), Rate 0.1 ... 6000 Hz (default 0.1), a triangle LFO on the delay time
  (flanging and chorus up to frequency modulation).
- LoPass 20 Hz ... 20 kHz or Off, HiPass Off or 20 Hz ... 20 kHz (both Off by default); Link moves both together like
  a band-pass.
- LoFi: the reduced sample rate of early digital delays, keeping the delay range.
- Mix 0 ... 100 (default 100), Output +-18 dB, Analog Off / 1 ... 4 (default 2), an output meter 0 ... -48 dBFS with a
  clip light.

## Specifications (D3, measured)

| Item | Value |
|---|---|
| Latency | 0 samples, reported and real: the first echo of an impulse lands on the set time at 44.1, 48 and 96 kHz (16 537, 18 000, 35 999 samples for 375 ms) |
| Echo kernel | not a single sample: at 48 kHz an impulse comes back as a symmetric kernel over +-14 samples (centre 0.666, side taps -0.020 / +0.021, -0.010, +0.007 ...) whose sum is 2/3 (-3.5 dB) with Feedback 0 and Mix 100; at 96 kHz it is twice as wide (centre 0.333): an interpolated read with a fixed-frequency low-pass, and a wet gain that MEASUREMENTS.md section 3 explains |
| Mix | at Mix 50 the dry signal is at 0.707: an equal-power law (MEASUREMENTS.md section 7) |
| Glide | in a running instance, changing Delay from 375 to 100 ms put the next echo at 148 ms, and 100 to 50 ms put it at 52 ms: the read point moves at a limited speed (the documented tape behaviour); loading a state with the new time jumps at once |
| Harness | parameters set through pedalboard before the instance's first block are shown, but the processing keeps its previous values (the echo stayed at 375 ms while Delay read 100 ms), so settings go through the state's preset array (`scripts/nulltest/ref_state_lib.py`) or after a first rendered block; values set from outside do not redraw the reference's editor, so screenshots of other states are taken by clicking its buttons (MeterCapture `click`) |

## Parameters (D2, D3), in preset-array order

Flatwater Delay's parameters, in the original's order, with the original's ranges, steps and defaults as a host sees
them. A preset of the original holds 45 values; the right side's times and Dual are not stored in presets. The
original's state also holds values a host does not see (meters, the Analog stage's settings, display switches).

| ID | Label | Range | Default | Notes |
|---|---|---|---|---|
| `sync` | Sync | BPM / Host / ms | Host | |
| `bpm` | BPM | 30 ... 300, 1 | 120 | used in BPM sync |
| `delay` | Delay | 1 ... 3500 ms, 1 ms (1001 host steps, 10 ms steps at the top) | 375 | the time in MS sync |
| `delayNote` | Delay Note | 20 note values: 1/64T 1/64 1/64D 1/32T 1/32 1/32D 1/16T 1/16 1/16D 1/8T 1/8 1/8D 1/4T 1/4 1/4D 1/2T 1/2 1/2D 1B 2B | 1/8D | the time in Host and BPM sync; shares the Delay knob |
| `mix` | Mix | 0 ... 100, 1 | 100 | the original keeps it when a preset loads |
| `feedback` | Feedback | 0 ... 200, 1 | 60 | |
| `phaseL`, `phaseR` | Phase L / R | + / - | + | |
| `pingPong` | Ping Pong | Stereo / PingPong | Stereo | |
| `output` | Output | +-18 dB, 0.1 | 0 | |
| `lopass` | LoPass | 19 ... 20 138.8 Hz, logarithmic; Off at the top | Off | factory presets store Off as 20 100 or 20 139 |
| `hipass` | HiPass | 19 ... 20 138.8 Hz, logarithmic; Off at the bottom (19) | Off | |
| `depth` | Depth | 0 ... 100 %, stored with fractions (0.69 shows as 1 %) | 0 | |
| `rate` | Rate | 0.1 ... 6000 Hz, logarithmic (881 host steps) | 0.1 | |
| `lofi` | LoFi | HiFi / LoFi | HiFi | |
| `analog` | Analog | Off / 1 ... 4 | 2 | a fresh instance and the default preset are at 2 |
| `pingPongStart` | Ping Pong Start | Left / Right | Left | |
| `delayR`, `delayNoteR` | Delay R, Delay Note R | as Delay and Delay Note | as the left ones | the right side's time (Dual) |
| `dual` | Dual | Off / On | Off | not a host parameter in the original |
| `mode` | Mode | Stereo / Mono / M>S | Stereo | which of the original's three components Flatwater Delay behaves as |

The original's Analog settings carry the same numbers as the original of Flatwater Comp; MEASUREMENTS.md section 8
shows that mode 4 is the same stage and modes 1 to 3 are not. The filter Qs (0.95 low-pass, 0.8 high-pass) are fixed
design values of the original, confirmed in MEASUREMENTS.md section 4; a fixed 10 ms release and a fixed 6280 Hz
interpolation low-pass are among its fixed settings too (sections 3 and 1). A host sees no right-side Feedback, filter
or modulation, so Dual only splits the times.

## GUI geometry (D2, D4)

The editor is 749 x 292 px below the product line's toolbar (the same toolbar as the original of Flatwater Comp:
logo, undo / redo, preset box, previous / next, A to B, Setup A, Save, menu), with the same orange panel, knob, button
and display family. Its layout was measured on the screenshots (D4).

- On the left the Tap pad (a black rubber pad with the vendor's mark, TAP below) and the LoFi button.
- The big Delay knob (scale 1/64T ... 2Bar when synced, 1 ... 3500 in MS); while it is dragged a value box appears in
  its centre. Delay BPM and Delay ms share the knob (and the right side's copies for Dual).
- A button row over the display (stereo): phase L, PING PONG, DUAL, phase R; mono shows a single phase button.
- The Display (amber segment digits): the note value and the BPM (1/8 D 120 BPM) in Host / BPM, four digits (0375) in
  MS; with Ping Pong the time and an L / R start-direction box; with Dual L and R halves with a value each. The Sync
  radio BPM / HOST / MS under it.
- The big Feedback knob (0 ... 200); a clip light and the output meter (two bars on stereo) on a cream scale 0, 3, 6,
  12, 24, 36, 48.
- Right column: Dry/Wet, Output, and the stepped Analog knob.
- Bottom row: Depth and Rate under a MODULATION bracket; HiPass, Link (a small knurled wheel moving both filters) and
  LoPass under FILTERS.
- The plug-in's name in large type at the lower left. Value texts are shown while a knob is dragged; Alt resets; text
  entry on the knobs and the displays. A 40 px drag up on Delay in MS moved it 375 to 415 ms.
- New against Flatwater Comp's widget kit (`shared/ui/HybridUI`): the tap pad, the vertical bar meter with its scale, the Link
  wheel, the delay display's layouts (note / BPM, ms, Ping Pong, Dual) and the stepped note scale on the Delay knob.

## Presets (D2)

88 factory presets of 45 values each (all read back through the reference: `plugins/FwDelay/nulltest/read_ref_presets.py`, to the
local `build/ref_presets_FwDelay.json`): a reset, a live-sound start, 22 Classics, 11 Modulation, 11 Ambience, 22 Special FX
(several with Feedback over 100), and 20 signed by engineers whose names are in the preset names. 34 are for the
stereo components only. They use Host or ms sync (none BPM), Ping Pong on 39, LoFi on 30, every Analog mode, Mix mostly 100.
Flatwater's presets carry them over by their values under Flatwater names (no person's or product's name).
