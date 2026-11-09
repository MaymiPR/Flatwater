# Flatwater Vocal Comp: research

What the vendor documents for Renaissance Vox, the original Flatwater Vocal Comp recreates, and how the original
presents itself in a host: its behaviour, parameters and editor. The measured behaviour is in
[MEASUREMENTS.md](MEASUREMENTS.md).

## Sources

| Source | What it is |
|---|---|
| S1 | The official manual, [renaissance-vox.pdf](https://assets.wavescdn.com/pdf/plugins/renaissance-vox.pdf) (7 pages), linked from the product page (S4) |
| S2 | The original plug-in observed in a host: its controls, their ranges and defaults, its mono and stereo components and the size of its editor |
| S3 | Secondary web sources (reviews and shop pages, not the manufacturer): [Joseph Nilo on RVox](https://josephnilo.com/blog/waves-rvox-explained/), [Toolfarm](https://toolfarm.com/buy/waves_renaissance_vox), [Sweetwater](https://www.sweetwater.com/store/detail/RenVox--waves-renaissance-vox-plug-in/reviews) |
| S4 | The official product page, [waves.com/plugins/renaissance-vox](https://www.waves.com/plugins/renaissance-vox) (the shorter address `waves.com/plugins/rvox` is a 404) |

## Product-page facts (S4)

- Described as a classic compressor / limiter for vocals; three controls: Gate, Compression, Output Gain.
- The gate is described as gentle downward expansion, not a hard gate; the compressor as a soft-knee curve; limiting
  and level maximising run automatically (no limiter control); smart metering.
- Components: mono and stereo only. Skins: light, dark, legacy.
- Published latency: 64 samples at 44.1 and 48 kHz, 128 at 88.2 and 96 kHz, 256 at 176.4 and 192 kHz (67 on TDM at
  48 kHz). Maximum sample rate 192 kHz.

## Documented behaviour (S1)

- A dynamics processor for vocals that puts compression, gating and limiting behind three controls (p. 2).
- Components: mono and stereo, otherwise identical (p. 2). Skins: Light, Dark, Legacy; the same controls, values
  unchanged on a skin switch (p. 2).
- Compression fader: one control combining threshold and ratio. Lowering it increases gain reduction and output
  level; the output is held by a peak limiter. The manual prints the range as -36 dB to -36 dB (a typo; the control
  runs -36 ... 0, S2). A double-click enters a value (p. 5).
- Gain: described as the output ceiling of the limiter, not a plain fader. At high Gain settings make-up gain keeps
  the output level fairly even as Compression changes (p. 5). Compression is post-gate (p. 5).
- Gate: a downward expander, described as very gentle with a soft knee (the manual notes some reduction even with a
  very low threshold), range -inf ... 0 dB; superimposed on the input energy meter (p. 6).
- Total Compression display: the sum of the compressor's and the gate's gain reduction, the peak value below, a
  click resets the hold (p. 5).
- Output meters: full scale, the peak value below, a click resets; -inf ... 0 dB (p. 6).
- Input Energy meter on the left, the reference for setting the gate (p. 6).

Where the measurements contradict the manual (the Gain control, the gate knee): MEASUREMENTS.md section 9.

## Parameters (S2)

Flatwater Vocal Comp's parameters (IDs from `plugins/FwVocalComp/Source/PluginProcessor.h`):

| ID | Label | Range | Step | Default | Notes |
|---|---|---|---|---|---|
| `comp` | Comp (Compression) | -36 ... 0 dB | 0.1 | 0 dB (top) | threshold and ratio on one fader; a positive typed value is read as negative |
| `gate` | Gate | -80 ... 0 dB | 0.1 | -80 dB | -80 is off (the original shows `-Inf dB`) |
| `gateRel` | Gate Release | 10 ... 250 ms | 0.1 | 130 ms | hidden: not automatable, no control on the editor |
| `gain` | Gain | -36 ... 0 dB | 0.1 | 0 dB | the manual's limiter ceiling |
| `mode` | Mode | Stereo / Mono | choice | Stereo | the original's stereo and mono components |

The meters (input energy, total compression, output) are display only. The original also has a skin selector
(Light / Dark / Legacy).

## Editor

The original's editor is 371 x 357 px; its layout was measured. Gate sits on the left over the input energy meter,
Compression in the centre over the total compression display, Gain and the output meters on the right. The mono
component shows one output meter.
