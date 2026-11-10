# Contributing to Flatwater

Bug reports and null-test findings go to the [issues](https://github.com/MaymiPR/Flatwater/issues/new/choose). This
page covers changing the code: how it is built, tested and checked, how a change reaches `main`, and how a release is
made. [STANDARDS.md](STANDARDS.md) holds the rules in full; the last section here lists where Flatwater differs.

## The one rule

Each plug-in follows its original, and the original is the reference for every behaviour. A change that affects the
sound is measured against the original's own render of the same settings ([Null tests](#null-tests)), and what was
measured goes into the plug-in's MEASUREMENTS.md and NULLTESTS.md. Nothing of the originals (renders, files, art,
presets) enters the repository; a decision is recorded as the rule it produced, with the date of its approval, and
nobody's words are quoted in a file, a commit message, a tag or release notes.

## Build

Requirements and the commands per platform are in the [build guide](docs/BUILDING.md). Every build makes each
plug-in's VST3, Standalone (and CLAP, AU), its unit tests and its host test (`<Id>HostTest`). With the tools option
(`FLATWATER_TOOLS`, on) it also makes each plug-in's offline renderer (`<Ab>Render`, for example `CoRender`) and
editor snapshot tool (`<Id>Snapshot`), ParamDump and RefRender, and on Windows RefCapture, MeterCapture, CoVuRender,
OggDecode and the keyboard check (FocusCheck). The plug-ins are included from `plugins/<Id>/plugin.cmake`, so every
target lands in `build/<Target>_artefacts/Release/`; with `FLATWATER_DEV_COPY` (on) a copy of every VST3 also lands
in `build/plugins/<Product name>.vst3`, the folder a DAW's plug-in junction points at. `FLATWATER_CLAP` (on) builds
the CLAP plug-ins. The `ci_targets` target builds what CI builds: every plug-in's VST3, unit tests, host test and
renderer, and on Windows its snapshot tool.

| Plug-in | Renderer |
|---|---|
| FwBassLeveler | `BlRender` |
| FwBassLift | `BfRender` |
| FwBassXLift | `BxRender` |
| FwComp | `CoRender` (and `CoVuRender`, the VU dial alone) |
| FwDelay | `DeRender` |
| FwPitchBend | `PbRender` (and `PbShifterRender`, the shifter alone) |
| FwVocalChain | `ChRender` |
| FwVocalComp | `VcRender` |
| FwVocalLeveler | `VlRender` |

## Tests

| Test | What it does | Command |
|---|---|---|
| `<Id>Tests` (one per plug-in) | the plug-in's parameters (ranges, defaults, texts), its DSP laws against the reference's measured values, latency, presets, state and editor; the Validation section of each plug-in's MEASUREMENTS.md lists what each covers | `ctest --test-dir build -C Release --output-on-failure` |
| `<Id>HostTest` (one per plug-in) | the built VST3 hosted as a DAW hosts it: the parameter list, save and reload, the editor, a sample-rate change, offline rendering, and every saved state of a release in `plugins/<Id>/tests/data/` (`state-1.0.0.bin` with the texts each parameter showed) loading with the same texts | `ctest --test-dir build -C Release --output-on-failure` |
| Render regression | the cases in `tests/regress/<Id>/jobs.txt` (every plug-in has cases) through that plug-in's offline renderer against the committed goldens; our output does not change unless intended (`--update` after an intended DSP change, committed with it) | `python scripts/render_regress.py --build build` |
| Null tests | the same signals through the original and through ours; the residual in dB | [Null tests](#null-tests) |
| Repository checks | absolute paths, old names, attribution forms, document style, whitespace | `python scripts/check_repo.py` |

Each command returns non-zero on a failure.

## The local check

`scripts/check.ps1` (Windows) or `scripts/check.sh` (Linux, macOS) is the one command before any build is handed
over and the proof on a private repository, where no workflow runs. It closes a running host, runs `check_repo.py`
and the lint tools it finds on the PATH (a missing tool is reported, never installed), configures when
`build/CMakeCache.txt` is missing, builds `ci_targets` into a log, counts warnings by the same rule as the workflows
and fails above zero, asserts every VST3 is newer than the build start, runs `ctest` and the render regression, and
prints one `CHECK` line with the counts. Switches: `-SkipBuild`, `-SkipTests`, `-KeepFl`.

## Changing the code

1. Branch `fix/<slug>`, `feature/<slug>` or `chore/<slug>` from `main`.
2. Make the change. A DSP change is measured with the plug-in's null test; a case worse by more than 0.5 dB blocks
   it.
3. `scripts/check.ps1` (or `check.sh`) passes.
4. Open a pull request; Lint and CI run on a public repository, the local check stands in on a private one.
5. Squash merge; the branch is deleted on both sides at merge.
6. Add the line to `CHANGELOG.md` under `## Unreleased` (New, Improved, Fixed or Known issues, with before and
   after numbers in dB for a sound change).

## Making a release

[.github/RELEASE_CHECKLIST.md](.github/RELEASE_CHECKLIST.md) is the one list; the release workflow opens it as an
issue for a `release/x.y.z` branch or by hand. In short:

1. `## Unreleased` in [CHANGELOG.md](CHANGELOG.md) holds everything user-facing.
2. Optional: branch `release/x.y.z`; the checklist issue opens.
3. The local check passes with zero warnings; on a public repository Lint and CI are green on `main`.
4. `python scripts/bump_version.py x.y.z`: the version in CMakeLists.txt, the dated CHANGELOG section, the commit
   `Release x.y.z`. Push `main`.
5. When packaging changed, run Build and release by hand once and check the artifacts.
6. On approval: `python scripts/bump_version.py x.y.z --tag` and `git push origin vx.y.z`. The workflow builds
   Windows, macOS (universal) and Linux, validates every plug-in with pluginval at strictness 10 (and `auval` on
   macOS), checks on Windows that every editor leaves the space bar to the host (FocusCheck), signs and notarizes
   when the repository has the secrets, and publishes the three installers with `SHA256SUMS.txt`, attestations and
   the notes from CHANGELOG.md. A tag with a hyphen publishes a pre-release.
7. Check the release page, download one file on another machine, close the checklist issue.
8. A release that fails after the tag is fixed on `main` and shipped as the next patch version.

The signing secrets are named in `.github/workflows/release.yml`; without them the workflow publishes the same files
unsigned and the notes say so.

The Windows installer also builds locally with Inno Setup 7 (7.1.1 or newer), from PowerShell:

```powershell
pwsh packaging/make_iss_include.ps1 build
iscc /DAppVersion=1.0.0 packaging/Flatwater.iss
```

It lands in `dist/`. `/DTestRoot=<folder>` makes a test installer that installs everything under that folder without
administrator rights.

## Repository layout

| Path | What it holds |
|---|---|
| `plugins/<Id>/` | one plug-in: `plugin.cmake`; `Source/` (`Params.h`, `PluginProcessor`, `PluginEditor`, `Presets.h/.cpp` with the generated `PresetTable.h`, `dsp/` with `Engine` and the other DSP and generated headers, `ui/` with the plug-in's own `Controls`); `tests/<Id>Tests.cpp` and `tests/data/` (the saved-state fixtures); `tools/<Ab>Render.cpp`; `scripts/` (its Python: exports, fits, preset tables, UI measurements); `nulltest/` (its null tests, measurements and fits, with a README); `resources/` where it has binary resources (FwVocalChain's measured responses); and its documents: README (the page users read), RESEARCH (what the original documents), MEASUREMENTS (what was measured and identified), NULLTESTS.md (the null report, generated) |
| `shared/` | code shared by the plug-ins (`shared.cmake` builds it as the static library FlatwaterShared): `ui/` (`Widgets` with the toolbar, meters, wheel, menu look, mark and settings; the editor styles FlatUI, HybridUI and RenUI; `Theme.h`; `HostFocus`, the host's keys after a click), `dsp/Math.h` (the unfused math), `Presets.h` |
| `tests/regress/` | the render regression's cases and goldens, per plug-in |
| `tools/` | the C++ hosts for the reference plug-ins (RefRender, ParamDump, RefCapture, MeterCapture, OggDecode), FocusCheck, `HostTest.cpp` (every plug-in's host test) and `Snapshot.cpp`, the editor snapshot every plug-in but FwVocalComp builds |
| `scripts/` | `bump_version.py`, `release_notes.py`, `render_regress.py`, `make_site.py`, `check_repo.py`, `check.ps1`, `check.sh`, `build_native.py` (builds every `plugins/<Id>/nulltest/native/` helper into `build/native/`), `make_presets.py` (the preset tables of eight plug-ins), `nulltest/` (the modules every plug-in's null tests import: `local.yaml`, the reference hosts, the corpora, the meter readers; [scripts/nulltest/README.md](scripts/nulltest/README.md)) and `feel/` (the control-feel scripts) |
| `measurements/<Id>/` | the measurement data behind each plug-in's MEASUREMENTS.md (published) |
| `packaging/` | the installers; `plugins.txt` lists the plug-ins they hold |
| `docs/` | the build guide (`BUILDING.md`), `branding/` (banners, icon, social preview, `colours.json`) and `screenshots/`; everything about one plug-in is in its own documents |
| `.github/` | `workflows/` (Lint, CI, Build and release, Release checklist, Landing page), the issue forms, the pull request template, `RELEASE_CHECKLIST.md`, `dependabot.yml`, CODEOWNERS |
| `local.example.yaml` | the keys of `local.yaml` (git-ignored): the reference's VST3 and names, the interpreters, the tool folders, and in its `# Suite keys:` block the keys this suite's scripts add ([scripts/nulltest/README.md](scripts/nulltest/README.md)) |

Reference renders and anything taken from the original plug-ins stay out of the repository (`.gitignore`).

## Null tests

Each plug-in's `nulltest/` renders the same signals through the original and through ours and reports the residual.
It needs Windows, Python 3.12 or newer with the packages in `requirements.txt` (`requirements-train.txt` adds torch for
the learned blocks), the original plug-ins installed and named in `local.yaml`, and the tools built
(`python scripts/build_native.py` for the C helpers). The folder is flat: the harness `<ab>_ref.py` (the reference
and our renderer behind one interface), `nulltest_lib.py`, `first_null.py` (one bypass null, the smoke test),
`nulltest.py` (every case; `--report` writes `plugins/<Id>/NULLTESTS.md` from `measurements/<Id>/nulltests.json`,
`--vst3` checks the built VST3 against the offline renderer), the purpose scripts (`measure_`, `probe_`, `fit_`,
`train_`, `null_`, `render_`, `compare_`, `check_`, `read_`) and the `_lib.py` helpers; its README lists every
script by role. None of this runs in CI, which has no originals.

## Versions

One version for the suite (`project(... VERSION ...)` in `CMakeLists.txt`). The first release is 1.0.0; a new
plug-in raises the middle number, a fix the last. Tags are `vx.y.z`; `vx.y.z-rc.N` publishes a pre-release.
`scripts/bump_version.py` writes every version.

## Frozen identity

What released users' projects, presets and installs depend on (STANDARDS.md); none of it changes, whatever else is
renamed. The values that differ from what the naming rules would give today:

| Item | Value | Set in |
|---|---|---|
| Manufacturer code | `Fltw` | `CMakeLists.txt` (`flatwater_add_plugin`), `packaging/plugins.txt` |
| Plug-in codes | `Fbe1` FwBassLeveler, `Fbs1` FwBassLift, `Fbx1` FwBassXLift, `Fcp1` FwComp, `Fdl1` FwDelay, `Fpb1` FwPitchBend, `Fch1` FwVocalChain, `Fvc1` FwVocalComp, `Fve1` FwVocalLeveler (a digit, not the lower-case letters of the rule) | each `plugin.cmake`, `packaging/plugins.txt` |
| Bundle IDs | `com.flatwater.<Id>` with the plug-in id's case (`com.flatwater.FwComp`) | `CMakeLists.txt` (`flatwater_add_plugin`) |
| CLAP IDs | `com.flatwater.<name>` without the `Fw` prefix (`com.flatwater.comp`, `com.flatwater.bassleveler`) | each `plugin.cmake` |
| Parameter IDs | as released in 1.0.0 | each `Source/Params.h` |
| Company name | `Flatwater` | `CMakeLists.txt` |
| Installer AppId | `{FD12655C-898E-4BCF-9B39-16B5540D1933}` (the test installer `{398E4AB3-D38D-4221-8D28-A241FC7F460D}`) | `packaging/Flatwater.iss` |
| Install folders | `VST3\Flatwater`, `CLAP\Flatwater` and `Flatwater` for the apps on Windows; the macOS subfolder `Flatwater` | `packaging/Flatwater.iss`, `packaging/make_iss_include.ps1`, `release.yml` (`SUBFOLDER`) |

## Deviations from the standard

| Deviation | Reason |
|---|---|
| `dsp/Tuning.h` exists in FwBassLift and FwVocalComp only, the plug-ins whose renderers override constants by name; the other plug-ins keep their constants beside the DSP that uses them, and FwPitchBend has no `dsp/Engine` (its processor joins the modulators, `dsp/Mod.h`, and the shifter, `dsp/Shifter`) | moving every constant or the processor's own DSP is a DSP change, which a layout change does not make |
| The renderers of FwBassLift, FwBassXLift, FwComp, FwDelay and FwVocalComp, and `PbShifterRender`, are plain executables of the DSP alone, without JUCE and without a `RenderDev` copy; they land in `build/<Target>_artefacts/Release/` like the others | they need no plug-in wrapper |
