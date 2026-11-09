# Building Flatwater

JUCE and [clap-juce-extensions](https://github.com/free-audio/clap-juce-extensions) are downloaded when you
configure. JUCE is pinned to the commit its tag pointed to when it was chosen: configuring stops if the tag moves.
A local checkout at the pinned commits is used instead of a download with
`-DFETCHCONTENT_SOURCE_DIR_JUCE=<folder>` and `-DFETCHCONTENT_SOURCE_DIR_CLAP_JUCE_EXTENSIONS=<folder>`.

## Requirements

- CMake 3.22 or later, and Git
- Windows: Visual Studio 17 or 18 with "Desktop development with C++"
- macOS: Xcode
- Linux: GCC or Clang, plus these packages (Ubuntu / Debian names):

  ```bash
  sudo apt install ninja-build libasound2-dev libjack-jackd2-dev libfreetype-dev libfontconfig1-dev libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev libxi-dev libglu1-mesa-dev mesa-common-dev
  ```

`-DFW_CLAP=OFF` leaves the CLAP versions out and skips their download. There is no tools option: every build also
makes the offline renderers and snapshot tools, and on Windows the hosts for the reference plug-ins
([CONTRIBUTING.md](../CONTRIBUTING.md)).

## Windows (VST3, CLAP, Standalone)

```bash
cmake -B build -A x64
cmake --build build --config Release
```

CMake picks the newest Visual Studio installed. To choose one, add its generator, for example
`-G "Visual Studio 18 2026"`.

Keep the build folder's path short (for example `C:\src\Flatwater\build`), or run
`git config --global core.longpaths true` first. JUCE's checkout holds deeply nested files, and under a long path Git
stops with "Filename too long" while configuring.

## macOS (universal AU, VST3, CLAP, Standalone)

```bash
cmake -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=10.13
cmake --build build --config Release
```

## Linux (VST3, CLAP, Standalone)

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## What CI builds

`--target ci_targets` builds every plug-in's VST3, unit tests and offline renderer, and on Windows its snapshot tool:
the set the CI workflow and the local check (`scripts/check.ps1`, `scripts/check.sh`) build.

## Output

Each plug-in lands in `build/plugins/<Name>/<Name>_artefacts/Release/` (`VST3/`, `CLAP/`, `AU/`, `Standalone/`), for
example `build/FwVocalComp_artefacts/Release/VST3/Flatwater Vocal Comp.vst3`. A copy of every VST3
also lands in `build/plugins/`. Copy the plug-ins to your plug-in folders, or run a standalone app from there.

The same build makes the unit tests; run them with:

```bash
ctest --test-dir build -C Release --output-on-failure
```

[CONTRIBUTING.md](../CONTRIBUTING.md) describes the tests, the null tests against the originals, the installers and
how releases are made.
