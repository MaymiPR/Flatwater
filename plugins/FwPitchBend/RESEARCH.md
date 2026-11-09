# Flatwater Pitch Bend: research

What the vendor documents for Vocal Bender, the original Flatwater Pitch Bend recreates, and what the original shows in
a host: its controls, the settings only its editor sets, its presets and its editor. Nothing of the original is copied
into this repository, and its shape library is not shipped. The measured behaviour is in
[MEASUREMENTS.md](MEASUREMENTS.md).

## Sources

| Source | What it is |
|---|---|
| P1 | The public manual, Vocal Bender User Guide (12 pages, also on waves.com) |
| P2 | The original's VST3 as a host sees it (ParamDump, 41 points per parameter): Vocal Bender Mono and Vocal Bender Stereo (version 17.1), category Fx, Pitch Shift; 1 in / 1 out and 2 in / 2 out, latency 0, tail 0, no programs, 29 parameters and the host's Bypass (130 more are MIDI controllers, not automatable) |
| P3 | The original's saved state and its 41 factory presets read in our terms (`plugins/FwPitchBend/nulltest/editor_state.py`, `plugins/FwPitchBend/nulltest/read_ref_presets.py`, `measurements/fwpitchbend/presets.json`): the settings only its editor sets (Fine, the modulation slots, the modulators' shapes), checked by loading changed states back into the original and reading them again |
| P4 | The original's audio (`plugins/FwPitchBend/nulltest/vb.py`, `plugins/FwPitchBend/nulltest/measure_mods.py`, `plugins/FwPitchBend/nulltest/measure_shifter.py`; [MEASUREMENTS.md](MEASUREMENTS.md)) |
| P5 | The original's editor captured in a host (git-ignored `reference/fwpitchbend/ui/`) |

## Documented behaviour (P1)

- Alters the pitch and the formants of a monophonic voice in real time, with zero latency (studio and live use);
  polyphonic input (a choir, a mix) may give artefacts.
- Pitch shifts the fundamental and its harmonics without changing the duration (for doubling in real time). Formant
  changes the voice's character (the size of the source) without changing the notes: negative deeper and looser,
  positive tighter, far positive childlike, the extreme a chipmunk.
- Both run -12 ... +12 semitones, or -1200 ... +1200 cents with Fine on (a button between the two views).
- Link: a small knob between Pitch and Formant moves both together, keeping their offset, limited by either's range
  end.
- Flatten forces the whole vocal onto one note (a robot-like monotone), C2 ... B3; used with pitch modulation to make
  melodies.
- Mix 0 % (dry) ... 100 % (wet), at the plug-in's output.
- Modulation: four modulators: M1 and M2 (each an LFO or a step sequencer) and two organic ones, AM (the voice's
  amplitude envelope) and PT (its detected pitch). A modulator's label is dragged onto a control (the controls that
  take one are framed in blue while dragging): up to four per control, each in a slot under the control with its own
  depth (dragged vertically) shown as a coloured arc in the knob; a moving white dot outside the knob shows the range
  the modulation covers (depth 50 % on -12 ... +12 moves -6 ... +6). A right-click on a slot opens the assignment
  menu (None removes it). Every control can be modulated except Smooth and Offset; modulators can modulate each
  other's controls.
- M1 and M2: LFO or Sequencer. Rate 0.06 ... 30 Hz, or 1/64 ... 8 bars with Sync on (the host's tempo): the time of a
  whole LFO cycle, or of one step. Phase (the starting position), Warp 0.01 ... 100 (1 linear; below 1 slow at the
  start of the cycle and faster to its end, above 1 the opposite, keeping the cycle's time: a pulse-width modulation
  of the cycle), Smooth 1 ... 1000 (low: distinct onsets, clicks possible; high: smooth, can lose energy), Level
  0 ... 1. Sequencer: Steps 2 ... 16, values whole numbers -24 ... +24, dragged per step. Shapes: Draw (pencil), Erase
  (resets the shape to None), Browse (a library of factory and user shapes; Save to an empty cell, Delete a user
  shape; user shapes saved in a shared settings folder).
- AM: Attack and Release 0.1 ... 1000 ms, Level 0 ... 2. PT: Smooth 1 ... 1000 (a glide on the detected pitch), Offset
  -1 ... 1 (a reference note the modulation swings about), Level 0 ... 2.
- The toolbar: presets, A/B compare, undo and redo, window size.

## Parameters (P2), as hosts show them

Normalised value v (0 ... 1) to the value shown. Every taper below gives the displayed text at all 41 points (ties
such as 0.075 are shown rounded up: 0.08); the choices are evenly spaced.

| # | Name | Range, taper | Default | Shown as |
|---|---|---|---|---|
| 1, 10 | Mod1 / Mod2 Warp | 0.01 · 10000^v | 1.00 | `"1.00 "` |
| 2, 11 | Mod1 / Mod2 Level | 0 ... 1, linear | 1.00 | `"1.00 "` |
| 3, 12 | Mod1 / Mod2 Rate Synced | 24 choices, 32B ... 1/64T (slowest first) | 1B | `"1B "`, `"1/8D "`, `"1/16T "` |
| 4, 13 | Mod1 / Mod2 Rate | 0.06 · 500^v Hz | 0.500 Hz | `0.500 Hz` |
| 5, 14 | Mod1 / Mod2 Sync On/Off | Note / ms | Note | |
| 6, 15 | Mod1 / Mod2 Phase | 0 ... 1, linear | 0.000 | |
| 7, 16 | Mod1 / Mod2 Type | LFO / Seq | LFO | |
| 8, 17 | Mod1 / Mod2 Smooth | 1000^v | 1.5 | `"1.5 "` |
| 9, 18 | Mod1 / Mod2 Steps | 2 ... 16 (rounded) | 16 | |
| 19, 20 | AM Attack, AM Release | 0.1 · 10000^v ms | 10.0 ms, 100.0 ms | `10.0 ms` |
| 21 | AM Level | 0 ... 2, linear | 1.00 | |
| 22 | PT Smooth | 1000^v | 10.0 | |
| 23 | PT Offset | -1 ... 1, linear | 0.00 | |
| 24 | PT Level | 0 ... 2, linear | 1.00 | |
| 25, 26 | Pitch, Formant | -12 ... +12 semitones, linear | 0 | `"-5 semitones "` (whole semitones) |
| 27 | Mix | 0 ... 100, linear | 100.0 | `"100.0 "` |
| 28 | Flatten Note | 24 choices, C2 ... B3 | C3 | `"C3 "` |
| 29 | Flatten On/Off | Off / On | Off | |

Not host parameters (P3): Fine, the modulation slots and their depths, the shapes, and whether a rate in Hz runs free.
Link keeps no state (P5). Host quirks: the original reads a typed Pitch or Formant (`-5`) as 0 semitones (set them by
normalised value); Pitch and Formant from the host are whole semitones while Fine is off.

## Settings only the editor sets (P3)

| Setting | Behaviour |
|---|---|
| Pitch, Formant | held in cents (-1200 ... 1200), whole hundreds while Fine is off |
| Fine, Flatten | on and off; Flatten's note C2 ... B3 |
| M1, M2 rate | a rate in Hz and a synced length (1/64T ... 32 bars); two settings decide which plays: with Hz shown only (all the host's Sync On/Off changes), a rate in Hz is still snapped to the nearest synced length at the host's tempo (3.7 Hz at 100 bpm runs at 3.333 Hz); with free running on too, it runs free (3.7014 Hz) |
| Shapes | per modulator; an LFO plays one cycle of 256 values (-1 ... 1); the defaults are a sine (M1) and a triangle (M2) |
| Modulation slots | four per modulatable control, each a source (none, M1, M2, AM, PT) and a depth (-100 ... 100); targets: Pitch, Formant, Mix, M1 and M2 Rate, Level, Warp and Phase, AM Attack, Release and Level, PT Level (the original's state also has places for PT Smooth, which its editor does not offer) |

## Presets (P3)

40 factory presets and a reset (`measurements/fwpitchbend/presets.json`). Their modulators' shapes, by their math:
sine, triangle, square or flat in most; a drawn LFO shape in 11 (in 10 of them the drawn modulator is assigned: these
become shape substituted with the nearest of our own shapes); step sequences in 15 (2, 8 or 16 steps). Flatten in 7,
Fine in 5. Every preset keeps the original's values under a Flatwater name.

## The editor (P5)

564 x 289 px with the modulator panel closed, 564 x 465 open (a 21 px toolbar included), 75 ... 200 %. Pitch and
Formant are big knobs (pointer +-125.8°), the others 68 px (+-131.6°); the white range dot orbits at 62.7 px (Pitch,
Formant) and 25 px (Mix). The panel has four tabs (M1, M2, AM, PT); a modulator's label is dragged onto a target (the
targets framed while dragging), a slot's depth is dragged vertically (0.2 per px, Alt-click 0).

| Gesture | Measured |
|---|---|
| Knob drags (40 px at 1, 2, 4 and 8 px mouse steps on every knob) | the 68 px knobs and Mix move 0.001 of their range per px at slow steps (1 ... 2 px) and 0.002 at fast ones (4 px and more); Pitch and Formant 0.00053 and 0.0016, sticking on whole semitones with Fine off, a cent per px and 0.002 with it on; Ctrl a quarter of either; Alt-click resets |
| Wheel | Pitch, Formant and Link a semitone per notch (32 cents with Fine); the other knobs 0.9 ... 1.6 % of their range per notch (Warp 0.88, Phase 1.18, Mix 1.3, Rate, Smooth, AM Attack and Release, PT Smooth about 1.35, AM and PT Level and PT Offset about 1.5, Level 1.6) |
| Double-click | a typed entry (Pitch takes whole semitones) |
| Link | no state of its own: it moves Pitch and Formant together and stops as soon as either reaches its end |
| Steps | a knob (no drop-down) |
| Hover | no control has a hover look |

States saved after real editor actions confirm the slot layout (source and depth per slot) and the Fine setting
above. The captures and films stay in the git-ignored `reference/fwpitchbend/ui/` (the original's artwork is never
published); our editor keeps the measured layout and draws everything in Flatwater's own look.
