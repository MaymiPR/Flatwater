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
- **Parameter lists:** every `Params.h` holds `numParams`, `allIds` (the layout's order, checked by each plug-in's
  tests), `presetIds` and `createLayout`; parameter ids, order, ranges, names and defaults are unchanged.
- **Screenshots:** the snapshot tool is the starter's (`--scale` resizes the editor as a host would); the editors
  take `--click=x,y` and `--bench` as snapshot options, and `scripts/make_screenshots.py` renders `docs/screenshots`.
- **VST3 against the renderer:** `nulltest.py --vst3` of every plug-in compares the built VST3 with the offline
  renderer case by case, at the rate `--fs` gives, and ends with exit code 1 when a case is above -120 dB; the check
  script's null step runs it at 48 and 96 kHz. The reference and CPU checks of FwBassLeveler and FwVocalLeveler are
  `nulltest.py --checks`.

### Added

- **Flatwater Comp, side-chain:** a signal on the side-chain input keys the compressor, as in the original. While a
  host enables the side-chain bus, the detector and the output limiter read the key and nothing of the main signal,
  so a silent key means no compression; the louder key channel counts, and in Mode Mono each side follows its own
  key channel. Without the bus the sound is unchanged. Before, the plug-in ignored its side-chain input
  (`plugins/FwComp/MEASUREMENTS.md`, Side-chain; the null with a key: `plugins/FwComp/NULLTESTS.md`).

- **Flatwater Vocal Leveler, music side-chain:** music on the side-chain input raises the voice, as in the original.
  While a host enables the side-chain bus and Music Sensitivity is not Off, the ride aims higher by the music's
  level: the music adds to the Target as a power, 10 dB down, moved by Music Sensitivity dB for dB. Without the bus,
  or at Off, the sound is unchanged. Against the original with music: median -70.7 dB, worst -51.7 dB over 21 cases
  (`plugins/FwVocalLeveler/MEASUREMENTS.md`, section 4; `plugins/FwVocalLeveler/NULLTESTS.md`). Not yet tried in
  a DAW.

### Fixed

- **Bypass with latency:** FwBassLeveler, FwComp, FwVocalChain and FwVocalComp, the four plug-ins that report a
  latency, delay the input by that latency while the host bypasses them, so a bypassed track stays in time with the
  others. Before, the bypassed signal came out early by the latency (64 samples to 42 ms at 48 kHz).
- **FwComp, Attack and Release:** the processor reads the two times on the 0.01 ms grid their controls show. Before,
  the Attack control at 7.00 ms held a value one float step above 7, and the output differed from the offline
  renderer's by about -90 dB at that setting, the default one.
- **Bypass released, Flatwater Bass Leveler:** the first 42 ms after the host's bypass is switched off (5 ms on Live)
  are the audio of now. Before, they were the audio from before the bypass was switched on.
- **Bypass released, Flatwater Comp, Vocal Comp and Vocal Chain:** the audio after the host's bypass is switched
  off is the audio of now. Before, the first samples (the plug-in's latency) were audio from before the bypass was
  switched on. Flatwater Vocal Chain goes on running on its input while it is bypassed, unheard, so it uses its
  processing time then too.
- **Flatwater Vocal Comp, long sessions:** two sample counters wrap without an undefined step. Before, they were
  signed and overflowed after 2^31 samples of continuous running. The sound is unchanged (the render regression is
  byte-identical).
- **Toolbar:** the shared toolbar is repainted when what it shows changes (undo and redo, the star of a changed
  preset, the Mode), not at every frame of the editor's timer. It looks and works as before.
- **Offline renderers:** the renderers of the eight plug-ins besides Flatwater Vocal Leveler (whose VlRender checks
  its keys and its output already) end with exit code 2 and one line on stderr for an unknown key, an argument that
  is no `key=value`, a channel count other than 1 or 2 or a block under 1, and with 1 for an output they could not
  write in full. Before, a mistyped key was ignored, a channel count of 0 was a division by zero in BfRender,
  BxRender, CoRender and VcRender, and CoRender ran without end at `block=0`.
- **Input that is not a number:** a NaN or infinite input sample is taken as silence in every plug-in. Before, one
  such sample stayed in the filters until playback was restarted, and it could crash Flatwater Vocal Chain with
  Compress on.
- **Closing the editor:** a Mode menu left open no longer acts on a closed editor; a Rider fader held when the
  window of Flatwater Bass Leveler or Vocal Leveler closes is let go; Flatwater Vocal Leveler ends its automation
  write when the plug-in is removed.
- **User presets:** a preset file that is not this plug-in's is refused. Before, it replaced the plug-in's state.
- **Range in the two levelers:** Range Max automated under Range Min is read as equal to it.
- **Flatwater Delay:** its settings are read without building text on the audio thread.
- **Mode automated, Flatwater Comp, Vocal Comp, Bass Leveler and Delay:** a Mode change during playback no longer
  allocates memory on the audio thread; what both layouts need is made when the host prepares the plug-in, and the
  change is a reset. The sound right after the change is the same as before (it starts from silence, as a fresh
  instance in the new Mode). Flatwater Bass Leveler's latency differs between its studio and Live modes: the host
  now learns the new latency from a timer on the message thread, not from the audio thread.
- **Analog automated, Flatwater Comp and Delay:** the noise filters of all four Analog modes are made when the host
  prepares the plug-in, so an Analog change during playback no longer builds one on the audio thread. The sound is
  unchanged (the render regression is byte-identical).
- **Large blocks, Flatwater Vocal Chain and Pitch Bend:** a block larger than the size the host announced goes
  through in pieces of that size, without resizing a buffer on the audio thread. Before, Flatwater Pitch Bend
  allocated memory there, and Flatwater Vocal Chain crashed on such a block in its test.
- **Flatwater Vocal Chain, reverb loading:** the reverb's responses load in the background and the plug-in waits
  for them when the host prepares it, for 20 s at most, so a render has the reverb from its first sample. When the
  wait runs out, the plug-in now records it, the reverb is silent until the responses are there, and then comes in.
  Before, nothing recorded it and the reverb's input came through unprocessed meanwhile.
- **Presets and Setup A/B kept:** the preset's name, its place in the factory list and both setups are part of
  the plug-in's saved state, in every plug-in. They are still there when the window is closed and opened again and
  when the project is reloaded, and each setup keeps its own preset name. Before, they lived in the window: closing
  it or reloading the project put the bar back on Default Preset, setup A, and the other setup was lost. A project
  saved by 1.0.0 loads with the same settings, on Default Preset; a project saved now still loads in 1.0.0, which
  skips the added entries.
- **Undo across presets:** loading a factory or user preset, switching the setup and a program change from the host
  are each one undo step, so undo after a preset gives back the settings before it. Before, a user preset and a setup
  switch cleared the undo history.
- **Programs after a reload:** the host's program is saved with the state and comes back without being loaded
  again, and a program the host sets in the first half second after it restored a state is not applied. Before, a
  host that set a program after restoring could load that preset over the restored settings.
- **Flatwater Delay, Sync:** the change between ms and note values converts the delay time only when Sync is
  clicked in the window. A preset, an undo, a setup switch, the host's automation of Sync and a reloaded project keep
  the times they bring. Before, with the window open, a preset in ms loaded after a synced one had its Delay
  replaced by the note's time (505 ms became 500 ms at 120 BPM), and with the window closed it had not.
- **Flatwater Vocal Leveler, threads:** a preset's own Idle value is read when the state changes, on the thread
  that changes it. Before, a timer read the state 30 times a second, also while a host replaced it from another
  thread.
- **Documents:** the How close sentences of Flatwater Delay, Vocal Comp, Comp and Bass Lift say what their tables
  show: Delay's two cases with a gliding time (-41.9 to -96.2 dB) and its 96 kHz table, Vocal Comp's 100 Hz tone
  (-47.8 to -51.7 dB), Comp's weakest preset (-20.8 dB) and Bass Lift's weakest case (-4.0 dB). Before, each claimed
  more than its table.
- **Null tests:** the harnesses of Flatwater Comp, Vocal Comp and Bass Lift run as scripts again (`nulltest.py`
  starts them that way); each called its `main()` above the functions it needs.
- **Resize corner:** the grip in the window's lower right corner is drawn in Flatwater BassX Lift, Comp, Delay and
  Vocal Chain, as in the other five plug-ins. Before, the corner resized those four windows but nothing showed it.
- **Window size, one range:** the corner resizes a window between 75 and 200 % of its design size, the range of the
  window-size menu. Before, the corner went up to 300 %. A project saved with a larger window opens at 200 %.
- **Window size in the saved state:** the size is saved as `uiScale`, the name the standard gives it, beside the
  older `uiWidth`. A project saved by 1.0.0 opens at its size, and a project saved now opens at its size in 1.0.0.
- **Closing the editor, Flatwater Pitch Bend:** the list of the Flatten note and a slot's list of sources, left
  open, no longer act on a closed editor.

### Known issues

- Flatwater Comp: a sustained chord at full level with Ratio 50 and Release 30 ms matches at only -6.7 dB. The
  original holds its gain reduction between the chord's envelope peaks; Flatwater Comp lets go between them. The same
  chord 6 dB lower is at -60.6 dB.

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
