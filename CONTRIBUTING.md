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

Requirements and the commands per platform are in the [build guide](docs/BUILDING.md). Every build also makes each
plug-in's offline renderer (`<Ab>Render`, for example `CoRender`) and editor snapshot tool (`<Id>Snapshot`);
on Windows it adds the hosts for the reference plug-ins (RefRender, MeterCapture, VuRender, OggDecode) and the
keyboard check (FocusCheck). The plug-ins are included from `plugins/<Id>/plugin.cmake`, so every target lands in
`build/<Target>_artefacts/Release/`; a copy of every VST3 also lands in `build/plugins/`. The `ci_targets` target
builds what CI builds: every plug-in's VST3, unit tests and renderer, and on Windows its snapshot tool.

| Plug-in | Renderer |
|---|---|
| FwBassLeveler | `BlRender` |
| FwBassLift | `BfRender` |
| FwBassXLift | `BxRender` |
| FwComp | `CoRender` (and `VuRender`, the VU dial alone) |
| FwDelay | `DeRender` |
| FwPitchBend | `PbRender` (and `PbShifterRender`, the shifter alone) |
| FwVocalChain | `ChRender` |
| FwVocalComp | `VcRender` |
| FwVocalLeveler | `VlRender` |

## Tests

| Test | What it does | Command |
|---|---|---|
| `<Id>Tests` (one per plug-in) | the plug-in's parameters (ranges, defaults, texts), its DSP laws against the reference's measured values, latency, presets, state and editor; [docs/validation.md](docs/validation.md) lists what each covers | `ctest --test-dir build -C Release --output-on-failure` |
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
7. After a new section in a plug-in's MEASUREMENTS.md or RESEARCH.md, `python scripts/doc_index.py` rewrites the
   root indexes.

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
| `plugins/<Id>/` | one plug-in: `plugin.cmake`; `Source/` (`PluginProcessor`, `PluginEditor`, `Presets.h`, `dsp/` with `Engine` and the other DSP and generated headers, `ui/`); `tests/<Id>Tests.cpp`; `tools/<Ab>Render.cpp`; `scripts/` (its Python: exports, fits, preset tables, UI measurements); `nulltest/` (its null tests, measurements and fits, with a README); `resources/` where it has binary resources (FwVocalChain's measured responses); and its documents: README (the page users read), RESEARCH (what the original documents), MEASUREMENTS (what was measured and identified), NULLTESTS.md and `NULLTESTS-<topic>.md` (the null reports) |
| `shared/` | code shared by the plug-ins (`shared.cmake` builds it as FwCommon): `ui/` (the toolbar, the UI kits, the theme, meters, settings), `dsp/Math.h` (the unfused math), `Presets.h` |
| `tests/regress/` | the render regression's cases and goldens, per plug-in |
| `tools/` | the C++ hosts for the reference plug-ins (RefRender, MeterCapture, OggDecode), FocusCheck, and `Snapshot.cpp`, the editor snapshot every plug-in but FwVocalComp builds |
| `scripts/` | `bump_version.py`, `release_notes.py`, `render_regress.py`, `make_site.py`, `check_repo.py`, `check.ps1`, `check.sh`, `doc_index.py` (the root MEASUREMENTS.md and RESEARCH.md indexes), `make_presets.py` (the preset tables of eight plug-ins), `nulltest/` (the modules every plug-in's null tests import: `local.yaml`, the reference hosts, the corpora, the meter readers; [scripts/nulltest/README.md](scripts/nulltest/README.md)) and `feel/` (the control-feel scripts) |
| `measurements/<Id>/` | the measurement data behind each plug-in's MEASUREMENTS.md (published) |
| `packaging/` | the installers; `plugins.txt` lists the plug-ins they hold |
| `docs/` | [docs/README.md](docs/README.md) indexes it: the build guide, results.md (every plug-in's null tests in one page), validation.md, improvements.md, learned-blocks.md, `branding/` (banners, icon, social preview, `colours.json`) and `screenshots/` |
| `.github/` | `workflows/` (Lint, CI, Build and release, Release checklist, Landing page), the issue forms, the pull request template, `RELEASE_CHECKLIST.md`, `dependabot.yml`, CODEOWNERS |
| `local.example.yaml` | the keys of `local.yaml` (git-ignored): the reference's VST3 and names, the interpreters, the tool folders; [scripts/nulltest/README.md](scripts/nulltest/README.md) lists the keys this suite adds |

Reference renders and anything taken from the original plug-ins stay out of the repository (`.gitignore`).

## Null tests

Each plug-in's `nulltest/` renders the same signals through the original and through ours and reports the residual.
It needs Windows, Python 3.12 or newer with the packages in `requirements.txt` (`requirements-train.txt` adds torch for
the learned blocks), the original plug-ins installed and named in `local.yaml`, and the tools built. The entry point
is `plugins/<Id>/nulltest/nulltest.py` (every case for that plug-in; `--out` writes the report); the presets null is
`null_presets.py` beside it where the plug-in has presets of the original's, and Flatwater Pitch Bend also has
`first_null.py`. The folder's README lists every script by role. They write `plugins/<Id>/NULLTESTS.md` and
`NULLTESTS-<topic>.md`, summarised in [docs/results.md](docs/results.md). None of this runs in CI, which has no
originals.

## Versions

One version for the suite (`project(... VERSION ...)` in `CMakeLists.txt`). The first release is 1.0.0; a new
plug-in raises the middle number, a fix the last. Tags are `vx.y.z`; `vx.y.z-rc.N` publishes a pre-release.
`scripts/bump_version.py` writes every version.

## Deviations from the standard

| Deviation | Reason |
|---|---|
| The C++ namespaces are `fw<plugin>` (`fwcomp`, `fwvocalchain` and the others) and the shared code's `fw` and `th`, not the two-letter tool abbreviation | renaming them touches every source file and is no layout change; the renderers take the abbreviations |
| The shared code is a static library, FwCommon (`shared/shared.cmake`), which every plug-in, test, snapshot and renderer with JUCE links, not `SUITE_UI_SOURCES` compiled into each target | it is built once for nine plug-ins |
| One `tools/Snapshot.cpp` serves the snapshot tool of every plug-in but FwVocalComp, which has its own (`plugins/FwVocalComp/tools/Snapshot.cpp`) | the same program renders any editor |
| `Source/` has no `Params.h`, `Presets.cpp` or `dsp/Tuning.h`: the parameter ids are in `PluginProcessor.h`, the presets are generated headers, and FwBassLift and FwVocalComp keep their `Tuning` struct in `dsp/Engine.h` | the plug-ins predate the starter's files; moving the declarations is no layout change |
| The renderers of FwBassLift, FwBassXLift, FwComp, FwDelay and FwVocalComp, and `PbShifterRender`, are plain executables of the DSP alone, without JUCE and without a `RenderDev` copy; they land in `build/<Target>_artefacts/Release/` like the others | they need no plug-in wrapper |
| No `cmake/PatchJuceVst3Keys.cmake` | it changes how the VST3 editors take keys in FL Studio and needs an FL test before it is added |
| No tools option: the renderers and snapshot tools are always built, the Windows hosts on Windows | the render regression and the snapshots need them on every build |
| Every VST3 is also copied to `build/plugins/<Product name>.vst3` after it builds | a plug-in folder of the host on the development machine points there |
| FwVocalComp, FwComp and FwBassLift keep their main null script as `null_<id>.py` beside a `nulltest.py` that runs it | another plug-in's scripts import it, and two modules named `nulltest` cannot share one process |
| `measurements/` is published | the measurement data of this suite is public (approval recorded 2026-10-18) |
| `local.yaml` carries keys beyond `local.example.yaml`: the reference's preset files and FL Studio's sample packs | the preset null tests and the corpora read them ([scripts/nulltest/README.md](scripts/nulltest/README.md)) |
| Root `MEASUREMENTS.md` and `RESEARCH.md` are indexes written by `scripts/doc_index.py`; `docs/` adds results.md, validation.md, improvements.md and learned-blocks.md | nine plug-ins share one repository |
