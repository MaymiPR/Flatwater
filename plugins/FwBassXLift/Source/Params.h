#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "dsp/Engine.h"

// Parameters follow the reference's parameter list (its order, ranges, tapers and defaults; see RESEARCH.md,
// Flatwater BassX Lift). The DSP is in dsp/Engine.h (MEASUREMENTS.md, "Flatwater BassX Lift model").
namespace bx
{
inline constexpr const char* input = "input";          // -inf..0 dB (gain fader taper), default 0
inline constexpr const char* freq = "freq";            // 32..256 Hz, logarithmic, default 80
inline constexpr const char* decay = "decay";          // -24..-9 dB per harmonic, default -15
inline constexpr const char* response = "response";    // 10..30 ms, default 20
inline constexpr const char* ratio = "ratio";          // 1..4 (upward compression of the harmonics), logarithmic, default 1
inline constexpr const char* monitor = "monitor";      // Audio / Harmonics / Original Bass
inline constexpr const char* highpass = "highpass";    // 6 / 12 / 24 dB per octave on the harmonics, default 12
inline constexpr const char* origBass = "origBass";    // -inf..+12 dB, default 0
inline constexpr const char* harmonics = "harmonics";  // -inf..+12 dB, default -inf
inline constexpr const char* mode = "mode";            // 0 Stereo, 1 Mono
}
