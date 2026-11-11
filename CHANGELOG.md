# Changelog

## Unreleased

### Improved

- Factory presets: 98 presets in the nine plug-ins renamed, so that no preset name repeats or closely follows the
  original's; each preset keeps its setting values, and a project that uses one loads as before. The factory default
  preset is named Default in every plug-in.
- Keys in VST3 hosts: the VST3 editors take the keys the host offers them, so a focused editor receives its keys
  in FL Studio; every key the editor does not use still goes to the host (the space bar, Ctrl+Z).
- **Null-test layout:** each plug-in's null tests have a harness `<ab>_ref.py`, `nulltest_lib.py` and `first_null.py`;
  scripts are named by their purpose, helpers end in `_lib.py`, the iteration scripts whose output does not ship left
  the tree, the C helpers are built by `scripts/build_native.py`, and one `NULLTESTS.md` per plug-in (Settings,
  Presets, Grid) is written from `measurements/<Id>/nulltests.json` by `nulltest.py --report` with the same numbers.
- **Source names:** the C++ namespaces are the plug-ins' two-letter abbreviations and `flatwater` for the shared
  code; the shared controls are `shared/ui/Widgets`, a plug-in's own `Source/ui/Controls`; FwComp's VU dial renderer
  is `CoVuRender`. No sound or saved state changes (the render regression is byte-identical).
- **Sources and build:** each plug-in has `Params.h`, `Presets.h/.cpp` over the generated `PresetTable.h`, and its
  top DSP class in `dsp/Engine.h`; the shared code is the static library `FlatwaterShared`; the build options are
  `FLATWATER_CLAP`, `FLATWATER_TOOLS` and `FLATWATER_DEV_COPY`; ParamDump and RefCapture join the reference hosts.
- **Host test:** every plug-in's `<Id>HostTest` (under ctest) hosts the built VST3 as a DAW does and loads a project
  state saved by 1.0.0 with the same parameter texts.
- **Host keys:** every editor hands the host's keys back after a click (`shared/ui/HostFocus`), so the space bar
  and the host's shortcuts keep working in FL Studio after a knob is touched.
- **Documents:** each plug-in's MEASUREMENTS.md holds its null-test results in brief and its validation (the suite
  pages and the root indexes are gone); the root README links every plug-in's README, RESEARCH, MEASUREMENTS and
  NULLTESTS; code, scripts and the names of local data files carry no vendor word.
- **Constants:** every plug-in's fitted and design constants are in one `Tuning` struct in `dsp/Tuning.h`, with
  `set()` and `load()`; every renderer takes `tune:key=value` and `tuning=<file>` overrides. FwPitchBend's
  modulators and shifter are joined in `dsp/Engine`, which the processor calls. No sound or saved state changes (the
  render regression is byte-identical).
- **Reference names:** the null-test and measurement scripts read the reference's component names from `local.yaml`
  (`<Id>.ref_name` and the other component keys, `<Id>.ref_presets` for the preset files and folders) and name no
  reference product; the test helpers' namespaces nest in their plug-in's.
- **Plain C++ DSP:** every plug-in's `Source/dsp` builds without JUCE. FwPitchBend's modulatable controls take
  their ranges from the DSP (the host parameters are built from the same ranges) and its state-tree helpers are
  `Source/ModulationState.h`; FwVocalChain's reverb convolution is `Source/ReverbConvolution`, which the processor
  hands to the chain. No sound or saved state changes (renders and saved states are byte-identical).

## 1.0.0 (2026-11-04)

The first public release: nine plug-ins, each a recreation of a Waves original, identified from measurements of the
original's sound and behaviour and null-tested against it. VST3, CLAP and a standalone app on Windows, macOS and
Linux, plus AU on macOS; one installer per system holds them all. The numbers below are the residual against the
original (lower is closer); each plug-in's NULLTESTS.md has every case.

### New

- **Flatwater Vocal Comp** (Renaissance Vox): a three-control vocal compressor, gate and limiter. Steady tones null at
  -95 to -120 dB at any setting; a grid of 672 settings on dynamic material: median -70 dB. 21 factory presets.
- **Flatwater Bass Lift** (Renaissance Bass): a psychoacoustic bass enhancer, the gain of its created bass learned from
  the original's output. 12 signals × 8 settings: median -36.8 dB. 45 factory presets.
- **Flatwater BassX Lift** (MaxxBass): a bass enhancer with a harmonics compressor, filters and monitor modes. Median
  -48.5 dB on 96 cases, -35.2 dB on 120 random settings. 22 factory presets.
- **Flatwater Comp** (H-Comp): a hybrid compressor with Punch, a tempo-synced release, four Analog modes and a
  limiter, with a small learned correction of its gain. 168 cases: median -48.7 dB; 26 factory presets: median
  -44.0 dB. The VU needle and the limiter lamp move as the original's.
- **Flatwater Delay** (H-Delay): a hybrid delay with tape-style time changes, Ping Pong, Dual, modulation, filters,
  LoFi and four Analog modes. The delay path nulls at -84 to -104 dB; 88 factory presets: median -53.2 dB.
- **Flatwater Bass Leveler** (Bass Rider): a note-by-note bass level rider with look-ahead (42 ms, or 5 ms on Live).
  Steady tones and steps -48 to -49 dB; real bass parts held out from the fit: median -22 to -26 dB. 17 factory
  presets.
- **Flatwater Vocal Leveler** (Vocal Rider): a real-time vocal level rider that writes its ride to the host's
  automation and reads it back. Level steps -74 dB; held-out vocal parts -47 to -66 dB; 13 factory presets: median
  -60.9 dB.
- **Flatwater Vocal Chain** (CLA Vocals): a vocal chain on six faders (Bass, Treble, Compress, Reverb, Delay, Pitch),
  each with three colours or clear. The whole chain on held-out vocals: median -33.2 dB with the doubler clear,
  -36.0 dB on Stereo; 43 factory presets: median -30.1 dB.
- **Flatwater Pitch Bend** (Vocal Bender): a real-time pitch and formant shifter for a voice, with Link, Flatten to a
  note and four modulators (two LFO or step sequencers, the voice's loudness and its pitch), no latency. With no shift
  it nulls at -67 dB; with a shift it is matched by what is heard: the modulators 38 of 38 cases, 62 of 80 factory
  preset and take pairs within the level, pitch and tone targets. 40 factory presets.
- **The same toolbar on every plug-in:** undo and redo, factory and user presets, A / B setups with copy, a Mode
  switch for the mono, stereo and live variants, and a window-size menu.
- **Flatwater's own look:** each editor keeps the original's layout, hit areas and control feel, drawn in code in a
  neutral dark theme (cyan values, amber pointers). The factory presets carry the originals' values under Flatwater's
  own names.
- **Installers:** a Windows setup (VST3 and CLAP, the standalone apps optional, an uninstaller), a universal macOS
  package (AU, VST3, CLAP, standalone apps) and a Linux tarball with an install script; checksums in
  `SHA256SUMS.txt` and build provenance attestations.
- **Built on JUCE 9.0.5**, its security release: safer handling of malformed files and data (ZIP, XML, JSON,
  images, WAV and AIFF) and HarfBuzz 15.0.0 for text.
- **A rewording update.**

### Known issues

- The downloads are not signed: Windows SmartScreen and macOS Gatekeeper warn the first time (the README's Quick
  start says how to open them).
- Flatwater Vocal Leveler: the music side-chain (Music Sensitivity) is not modelled yet.
- Flatwater Pitch Bend: shifted settings do not null sample for sample on a real voice (the original resynthesises
  the voice); presets that use the original's drawn LFO shapes use Flatwater's own shapes instead. Undo after
  Default Preset does not bring back the modulator slots.
- Flatwater Vocal Chain: the Wide and Spreader doublers match in level, spectrum and delay, not sample for sample.
- Weakest cases elsewhere: Flatwater Comp on low notes under long attacks (-15 to -20 dB on a bass line), Flatwater
  Bass Lift on very low input at Freq 160 with Original Bass Out, Flatwater BassX Lift on kick-like transients
  (-19 dB), Flatwater Delay's Analog mode 3 (about -41 dB), Flatwater Bass Leveler at Response Fast with
  Sensitivity 100.

## 0.1.0 (2026-10-18)

Not published as a GitHub release; everything in it ships in the first release.

First release of Flatwater, with one plugin:

- **Flatwater Vocal Comp**, a three-control vocal compressor / gate / limiter (VST3, CLAP, AU; Windows, macOS, Linux).
  - Dark skin on the reference's pixel grid, resizable 75 to 200 %, Waves-style toolbar (Setup A/B with copy, undo /
    redo, presets with Save, Mode Stereo / Mono, window size), click a value to type it.
  - DSP from measurements: four-phase true-peak detector, exact static law, identified look-ahead (64 / 128 / 256 samples) and gate; fitted slow path.
  - Null test against the reference: steady material nulls (-95 to -120 dB at any setting); dynamic material is close
    (median -71 dB over a 672-case settings grid; dense noise at heavy compression -10 to -25 dB).
- One installer per operating system for the whole suite.
