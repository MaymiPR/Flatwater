# Flatwater Bass Lift: research

What the vendor documents for Renaissance Bass, the original Flatwater Bass Lift recreates, and how the original
behaves in a host: its behaviour, parameters, editor and presets. Nothing of the original is copied into this
repository. The measured behaviour is in [MEASUREMENTS.md](MEASUREMENTS.md).

## Sources

| Source | What it is |
|---|---|
| B1 | The official manual, [renaissance-bass.pdf](https://assets.wavescdn.com/pdf/plugins/renaissance-bass.pdf) (9 pages, Renaissance Bass User Guide) |
| B2 | The official product page, [waves.com/plugins/renaissance-bass](https://www.waves.com/plugins/renaissance-bass) (overview and Tech Specs tab) |
| B3 | The original plug-in observed in a host: components RBass Mono and RBass Stereo; automatable parameters Bypass, Frequency (32 to 256 Hz), Intensity (-24 ... +24 dB), Out Gain (-30 ... 0 dB); reported latency 0; the parameters it keeps in its state, the size of its editor and its factory and artist presets |

## Documented behaviour (B1, B2)

- Psychoacoustic bass enhancer: it generates harmonics of the low end so the ear infers a fundamental that small
  speakers cannot reproduce (the missing fundamental); it does not generate sub-harmonics. Based on the vendor's
  earlier bass-enhancement technology (the one Flatwater BassX Lift recreates), with a clip-free signal flow.
- Freq (32 to 256 Hz): the frequency from which harmonics are generated; the input below it is removed and replaced by
  generated harmonics.
- Original Bass In / Out: whether the original signal below Freq is mixed back with the generated harmonics.
- Intensity (-24 ... +24 dB): level of the generated harmonics; the manual suggests starting below 0 dB and warns that
  high settings can distort.
- Gain (minus infinity ... 0 dB in the manual, -30 ... 0 dB on the control): output level.
- Meters: Original Bass (input energy below Freq), Created Harmonics (generated harmonics, depends on Intensity and
  Freq), Output (full scale, infinite peak hold, click to reset; clip light, click to reset).
- Product page: clip protection simplifies gain staging; skins Light / Dark / Legacy, with the same controls.

## Specifications (B2)

| Item | Value |
|---|---|
| Latency | 0 samples at 44.1 to 192 kHz (3 on TDM at 48 kHz) |
| Components | mono and stereo only (no M/S, no surround) |
| Maximum sample rate | 192 kHz |

## Parameters (B3)

Flatwater Bass Lift's parameters (IDs from `plugins/FwBassLift/Source/PluginProcessor.h`), in the original's parameter order:

| ID | Label | Range | Default | Control | Automatable | Notes |
|---|---|---|---|---|---|---|
| `inGain` | Input | -30 ... 0 dB, 0.1 | 0 dB | none | no | input level |
| `freq` | Freq | 32 ... 256 Hz, logarithmic, 1 Hz | 80 Hz | horizontal slider | yes | |
| `response` | Response | 0 ... 30 ms, 0.1 | 30 ms | none | no | |
| `ratio` | Ratio | 0.25 ... 1, 0.01 | 0.63 | none | no | |
| `intensity` | Intensity | -24 ... +24 dB, 0.1 | 0 dB | big fader | yes | level of the generated harmonics |
| `origBass` | Original Bass | Out / In | In | switch | yes | the original does not expose it for automation, so the measurements set it through the saved state |
| `gain` | Gain | -30 ... 0 dB, 0.1 | 0 dB | fader | yes | output level |
| `decay` | Decay | 0 ... 1, 0.1 | 1 | none | no | |
| `smoothTc` | SmoothTc | 0 ... 1000, 0.1 | 20 | none | no | |
| `tc` | Tc | 0 ... 2000 ms, 0.1 | 20 | none | no | |
| `xfadeTc` | XFadeTc | 0 ... 5000, 0.1 | 10 | none | no | |
| `smooth` | Smooth | Off / On | On | none | no | |
| `mode` | Mode | Stereo / Mono | Stereo | | | the original's stereo and mono components |

The meters (Original Bass, Created Harmonics, Output with its clip light) are display only. The original also has a
skin selector.

The hidden parameters (Input, Response, Ratio, Decay, the three smoothing values, Smooth) are part of every preset of
the original and shape the sound, so Flatwater Bass Lift keeps them as non-automatable parameters with these defaults,
and presets round-trip.

## Editor

The original's editor is 371 x 392 px (dark skin); its layout was measured. Intensity is the big fader in the centre
over the Created Harmonics meter; the Original Bass switch and the Original Bass meter are on the left; Gain, the
output meters and the clip light are on the right; Freq is a horizontal slider along the bottom. The mono component
shows one output meter.

## Presets (B3, converted)

All 45 factory and artist presets of the reference (Full Reset excluded) are carried over as Flatwater Bass Lift
factory presets by their setting values only (`scripts/nulltest/read_ref_presets.py` converts the reference's presets through
the reference itself, `scripts/make_presets.py` writes `plugins/FwBassLift/Source/Presets.h`). Every name is new: no
artist names, no vendor product names, no other trademarks; the name says what the setting is for. The same is done
for Flatwater Vocal Comp (21 presets; the reference's presets store its linear scales, checked against the two it
converts itself).
