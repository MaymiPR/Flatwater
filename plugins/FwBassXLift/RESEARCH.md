# Flatwater BassX Lift: research

What the vendor documents for MaxxBass, the original Flatwater BassX Lift recreates, and what the original shows in a
host: its behaviour, parameters, editor and presets. Nothing of the original is copied into this repository. The
measured behaviour is in [MEASUREMENTS.md](MEASUREMENTS.md).

## Sources

| Source | What it is |
|---|---|
| M1 | The MaxxBass User Guide (13 pages), the manual the vendor publishes with the plug-in; the product page [waves.com/plugins/maxxbass](https://www.waves.com/plugins/maxxbass) |
| M2 | The original plug-in observed in a host: its editor, its controls with the ranges, steps and defaults it shows, and its 22 factory presets; the editor is the vendor's newer flat design, one look (no skins) |
| M3 | The original's VST3 as a host sees it: components MaxxBass Mono and MaxxBass Stereo; every control is an automatable parameter (input_gain, orig_bass_gain, harmonics_gain, ratio, response, harmonics_decay, highpass, frequency, monitor); reported latency 0 |
| M4 | A screenshot of the real editor taken with `tools/MeterCapture` (kept in the git-ignored `build/`) |

## Documented behaviour (M1)

- Psychoacoustic bass enhancer (the missing fundamental): below the crossover Frequency (32 to 256 Hz) the bass gets
  harmonics generated for it; above it the signal passes unaltered. The vendor describes the original of Flatwater
  Bass Lift as based on this technology.
- Mix section: Input (manual: 0 dB ... -inf; the VST3 goes to +12 dB), Original Bass, the level of the low band
  (+12 dB ... -inf), and Harmonics, the level of the generated harmonics (+12 dB ... -inf; default -inf, so the
  default preset adds nothing).
- Harmonics section: a high-pass on the generated harmonics: 6 dB/oct at 16 Hz (wideband, mostly DC removal),
  12 dB/oct at Frequency (default), 24 dB/oct at Frequency. Decay (-9 ... -24 dB): the level step from each harmonic to
  the next; nearer 0 is richer.
- Dynamics section: an upward compressor on the harmonics only: Ratio 1:1 (none; the harmonics follow the original
  bass's dynamics) to 4:1 (harmonics louder, less dynamic range); Response 10 to 30 ms sets attack and release
  together, reacting to the original bass.
- Monitor selector: Audio (the output), Harmonics (harmonics solo), Original Bass (the low band being removed, solo).
  It changes what is heard, not only the meter.
- Output meter -40 ... 0 dBFS for the monitored source, peak value under each bar (infinite hold), red clip light; a
  click resets peak and clip. The graph shows the original-bass band (blue) and the harmonics band (orange) as curves
  that follow Frequency, the levels, the high-pass and Decay.
- Usage notes: harmonics typically -18 ... -8 dB for mixing and mastering; Frequency 50 to 90 Hz; small speakers:
  original bass off, high-pass 24 dB/oct, Ratio 2:1.

## Observed in a host (M2, M3)

- At Original Bass 0 dB the dry path is not a plain pass-through (residual +4.7 dB against the input on a bass line):
  an all-pass crossover sum, as Flatwater Bass Lift's Original Bass In (MEASUREMENTS.md section 2).
- The Monitor modes change the audio output, and Flatwater BassX Lift implements them.

## Parameters (M2, M3), in preset-array order

Flatwater BassX Lift's parameters (IDs from `plugins/FwBassXLift/Source/PluginProcessor.h`), in the original's parameter order:

| ID | Label | Range | Default | Control | Notes |
|---|---|---|---|---|---|
| input | Input | -inf ... 0 dB, gain fader taper, 0.1 dB | 0 dB | vertical fader | the original's fader goes on to +12 dB, where +6 and +12 act as 0 dB (MEASUREMENTS.md section 1); presets do not change it |
| freq | Frequency | 32 ... 256 Hz, logarithmic, 1 Hz | 80 Hz | horizontal slider | the crossover |
| decay | Decay | -24 ... -9 dB, 0.1 dB | -15 dB | knob | level step from one harmonic to the next |
| response | Response | 10 ... 30 ms, 0.1 ms | 20 ms | knob | attack and release of the harmonics' level control |
| ratio | Ratio | 1 ... 4, logarithmic, 0.01 | 1.00:1 | knob | upward compression of the harmonics |
| monitor | Monitor | Audio / Harmonics / Original Bass | Audio | three buttons | changes the output, not only the meter; the original labels the second one with the product name |
| highpass | High Pass | 6 / 12 / 24 dB/oct | 12 dB/oct | popup | on the generated harmonics only |
| origBass | Original Bass | -inf ... +12 dB, gain fader taper, 0.1 dB | 0 dB | vertical fader | level of the low band |
| harmonics | Harmonics | -inf ... +12 dB, gain fader taper, 0.1 dB | -inf | vertical fader | level of the generated harmonics; the original labels it with the product name |
| mode | Mode | Stereo / Mono | Stereo | none | one plug-in in place of the original's Mono and Stereo components |

The output meters, the graph curves and the clip light are display values in the editor, not parameters.

Presets do not change the Input fader. Some older factory presets store High-pass values outside the three settings;
they are converted through the reference itself, as for Flatwater Bass Lift (`nulltest/read_ref_presets_mb.py`):
those High-pass states wrap modulo 3 (3 and 6 give 6 dB/oct, 4 gives 12, 5 and 17 give 24); every other value follows
the scales of the measured tapers, which reproduce all 16 presets the reference converted itself.

## GUI geometry (M2, M4)

The view is 800 x 395 px including the toolbar strip (logo, undo / redo, preset box, previous / next, A to B, Setup A,
save, menu; no Skins button). The band graph (dB grid +6 ... -24, frequency marks 16 / 32 / 64 / 128 / 256) fills the
upper left, with the Frequency slider under it; the Ratio, Response and Decay knobs (values under them) and the
High-pass popup sit below; the Input, Original Bass and Harmonics faders (values under them), the two output meters
(peak text above, clip light) and the Monitor buttons fill the right. The positions are measured on the screenshot
(M4). The editor uses widgets beyond the suite's first widget kit (`shared/ui/RenUI.h`): the band graph, vertical faders, rotary knobs,
a popup selector, radio buttons, and a toolbar without Skins.

## Presets (M2)

22 presets: 12 factory (Full Reset, PA Default, Ultralow Extender, Light, Medium, Aggressive, Multimedia, PC Laptop,
an effects preset, BoomBaxx, Hi-fi / Lo-fi system enhancer) and 10 artist presets in six artist groups. They are
carried over by their setting values only, with new Flatwater names (no artist names, no vendor product names).
