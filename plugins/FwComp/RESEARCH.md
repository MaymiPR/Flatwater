# Flatwater Comp: research

What the vendor documents for H-Comp, the original Flatwater Comp recreates, and what the original shows in a host:
its behaviour, parameters, editor and presets. Nothing of the original is copied into this repository. The measured
behaviour is in [MEASUREMENTS.md](MEASUREMENTS.md).

## Sources

| Source | What it is |
|---|---|
| C1 | The H-Comp Hybrid Compressor User Guide (10 pages), [h-comp-hybrid-compressor.pdf](https://assets.wavescdn.com/pdf/plugins/h-comp-hybrid-compressor.pdf); the product page [waves.com/plugins/h-comp-hybrid-compressor](https://www.waves.com/plugins/h-comp-hybrid-compressor) |
| C2 | The original plug-in observed in a host: its editor, its controls with the ranges, steps and defaults it shows, and its 26 factory presets |
| C3 | The original's VST3 as a host sees it: components H-Comp Mono and H-Comp Stereo; automatable parameters threshold, ratio, mix, attack, punch, release, sync, analog, output, limiter, meter_select, releasebpm; a side-chain input bus (Stereo SideChain, disabled by default) |
| C4 | A screenshot of the real editor taken with `tools/MeterCapture` (git-ignored `reference/fwcomp/`) |

## Documented behaviour (C1)

- A hybrid compressor: modelled transformer, tube and transistor behaviour plus plug-in precision.
- Threshold 0 ... -48 dB (default -18), Ratio 1 ... 50 (default 3), Attack 0.5 ... 100 ms (default 7), Release
  3 ms ... 3 s (default 100 ms) or note values when synced.
- Automatic makeup gain keeps the compressed and uncompressed levels about the same.
- Punch 0 ... 30 (default 1): lets transients through the compressor even with a very fast attack (measured as a
  delay of the detector against the audio, MEASUREMENTS.md section 2).
- Mix 0 ... 100 (default 100): parallel compression with the dry input.
- Sync: ms (manual release), Host (release in note values of the host tempo), BPM (note values of a BPM set in the
  plug-in, 30 ... 300 or Off).
- Analog Off / 1 ... 4 (default 2): 1 transformer or tube, 2 transistor, 3 driven tube / transistor tone 1, 4 driven
  tone 2 without noise; modes 1 to 3 add harmonics and a small amount of noise.
- Limiter On: an output peak limiter; Off (default): a clipper instead. The default preset's Limiter setting
  disagrees with this documented default.
- Output +-18 dB (default 0). Meter: In / GR / Out (default GR), a VU needle 0 ... -48 dB; GR shows the total
  attenuation including mix and limiter.

## Specifications (C3, measured)

| Item | Value |
|---|---|
| Latency | 64 samples at 44.1 / 48 kHz, 128 at 88.2 / 96 kHz, 256 at 192 kHz (1.33 ms), reported and compensated exactly (an impulse comes out at 0 after compensation) |
| Components | mono and stereo (Flatwater Comp: one plug-in with a Mode switch), plus the side-chain bus, which pedalboard cannot feed and `tools/RefRender` can |

## Parameters (C2, C3), in preset-array order

Flatwater Comp's parameters (IDs from `plugins/FwComp/Source/PluginProcessor.h`), in the original's parameter order:

| ID | Label | Range | Default | Control | Notes |
|---|---|---|---|---|---|
| threshold | Threshold | 0 ... -48 dB, 0.1 dB | -18 dB | large knob | turns clockwise toward -48 |
| ratio | Ratio | 1 ... 50, the original's ratio taper, 0.01 | 3 | big knob | |
| punch | Punch | 0 ... 30, 0.1 | 1 | knob | delays the detector against the audio |
| mix | Mix | 0 (dry) ... 100 (wet), 0.1 | 100 | big knob | not changed by presets |
| output | Output | +-18 dB, 0.1 dB | 0 dB | knob | |
| attack | Attack | 0.5 ... 100 ms, logarithmic, 0.01 ms | 7 ms | knob | |
| release | Release | 3 ... 3000 ms, logarithmic, 0.01 ms | 100.25 ms | knob | shares its knob with Release Note |
| releaseNote | Release Note | 20 note values (1/64T ... 2 Bars) | 1/32D | Release knob, when synced | |
| sync | Sync | BPM / Host / ms | ms | three buttons | |
| bpm | BPM | 30 ... 300, 1 | 120 | LED display | the tempo for BPM sync |
| meter | Meter | In / GR / Out | GR | three buttons | what the VU needle shows |
| limiter | Limiter | Off / On | Off | toggle, with a lamp | Off: a clipper (manual) |
| analog | Analog | Off / 1 ... 4 | 2 | stepped knob (5 positions) | |
| mode | Mode | Stereo / Mono | Stereo | none | one plug-in in place of the original's Mono and Stereo components |

The meters and the limiter lamp are display values in the editor, not parameters. Besides the controls, the
original carries hidden settings that a host does not see and presets do not store: a knee, a program-dependent
release with a crest time of 149.62 ms, a side-chain filter, detector and release types, a limiter release, and the
noise and harmonic shaping of the Analog modes. MEASUREMENTS.md measures what each does rather than taking these
numbers on trust.

## GUI geometry (C2, C4)

The view is 643 x 454 px including the toolbar (the product line's logo, undo / redo, preset box, previous / next,
A to B, Setup A, Save, menu). A skeuomorphic orange panel with photographic knobs (bitmap sequences in the original),
all drawn by Flatwater Comp itself:

- Top row: the Analog stepped knob (5 positions) on the left, the Meter select In / GR / Out buttons and the Limiter
  toggle with its lamp in the middle, the Output knob on the right.
- VU meter in the centre: a backlit cream dial (scale 48 ... 0 in GR), a red needle, a screw-head pivot, chrome screws
  in the black housing; separate faces for GR and In / Out, 71 needle positions each.
- The Ratio big knob (scale 1 ... 50) on the left and the Mix big knob (Dry 0 ... 100 Wet) on the right.
- The Threshold large knob (0 ... -48) at the bottom centre.
- Punch and Attack bottom left, Release bottom right (Release and Release Note share the knob); the Sync buttons
  BPM / HOST / MS and the BPM LED display (BPM OFF, digit bitmaps) on the right.
- Value texts sit below the small knobs; Alt-click resets; every knob takes a typed value.

The layout was measured on the screenshot (C4); the next section gives the measured geometry.

## Drawn appearance (measured on the reference's screenshots, git-ignored `reference/fwcomp`)

- Knobs: a knob turning under a fixed light (upper left, 320 degrees): the flutes turn with the value (each capture's
  flute phase follows its pointer angle; on Ratio / Mix a step of 35 degrees is almost one flute period, 36 degrees, so
  it barely shows), the shading and the cast shadow stay. The pointer is a dark fin with a cream line on its ridge,
  its facets lit by their angle to the light, casting a soft shadow about (1.7, 5) px away, 0.45 deep. Bodies: small
  radius 32.4, cap ring 20; Ratio / Mix skirt 53.9, ten flutes at 44.3 +- 1.7, cap 32.5; Threshold skirt 73, flutes at
  61.3 +- 2.3, cap 44. The knob centres were measured on the captures (Threshold: (320.65, 333.35)). Cast shadows: the
  panel darkened by 0.35 to 0.42 under a disc offset (5 to 8, 16 to 25) px. `plugins/FwComp/scripts/fit_knobs.py` measures the
  shading on the captures and reports it; Flatwater Comp's knobs are drawn in Flatwater's own dark look on these
  measured sizes (`shared/ui/HybridUI.cpp`), with no fitted data.
- Scales: ticks every 14 degrees, small knobs r 33.8 ... 40 (1.7 px), Ratio / Mix r 56.2 ... 64.3, Threshold 19 ticks
  r 72.2 ... 80.4. Labels Arial Black 10.75 px stretched 1.2x; numbers Arial Bold 8.6 px; each placed at its measured
  ink centre. The Threshold legend (0, THRESHOLD, -48): Arial Black 12.6 px, one glyph per measured angle at r 85
  around the knob.
- VU: window frame 197.5, 73.5, 250 x 147 (top corners radius 46, bottom 13); cream dial inset 7 px. Scale arc centre
  (320.7, 220.9), r 99.8, -57.7 ... 60.1 degrees, ticks standing outward on it (minor 2.3 px; at 12 and 24, and the stop
  at the 48 end, 6 px); red band r 99.8 ... 107 from 0.2 to 60 degrees, dividers at 7.7, 15.6, 24.6, 35.6, 46.5
  degrees; dots at r 117.2; numbers Times bold italic 14.5 px turned along the arc (48 at -58.6 degrees r 116.3, 24 at
  -49.6 degrees 116.4, 12 at -29.9 degrees 116, 6 at 0.2 degrees 115.5, 3 at 24.9 degrees 116.8, 0 at 59 degrees
  117.1). On the screenshot the needle turns about (321.3, 222.1) (58.2 degrees at rest): (124, 0, 0), 2 px, 114 px
  long, a cream rivet at 109 px, a soft shadow 5 px below it (the films refine these: MEASUREMENTS.md section 12).
  Housing 174.5, 52.5 to 469.5, 280.5, a raised box (light top and left faces, 5 to 6 px dark right and bottom faces),
  arched around Threshold (r 83.2 about (320.15, 333.35), a 5.6 px dark lip, 6 px fillets into the bottom edge, the
  panel inside shaded); screws (head 9, recess 11.2) at (198.4, 75.95), (196.6, 261.05), (449.2, 260.95); the limiter
  lamp at (444.5, 75.5), a black dome (r 8.6) in a bezel (r 11.2) bright on its upper left.
- Buttons: 1 px black frames: In / GR / Out 245, 27, 151 x 21; Limiter 417, 27, 52 x 21; Sync 547, 315, 50 x 61;
  19 px faces. Display 506, 404, 90.5 x 34, a segment font (6 x 9 px cells; tempo digits 12 x 20).

## Presets (C2)

26 presets: Full Reset, PA Default, 21 genre and instrument presets (drums, kick, bass, piano, guitars, strings,
vocal, mastering, parallel, a broken-radio effect), and presets carrying artist names or other products' names.
Flatwater's presets carry them over by their values under Flatwater names.
