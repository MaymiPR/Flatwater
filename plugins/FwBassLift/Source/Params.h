#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "dsp/Engine.h"

// Parameters follow the reference's parameter list (its order, ranges and defaults; see RESEARCH.md, Flatwater Bass Lift).
namespace bf
{
inline constexpr const char* inGain = "inGain";        // -30..0 dB, hidden, default 0
inline constexpr const char* freq = "freq";            // 32..256 Hz, logarithmic, default 80
inline constexpr const char* response = "response";    // 0..30 ms, hidden, default 30
inline constexpr const char* ratio = "ratio";          // 0.25..1, hidden, default 0.63
inline constexpr const char* intensity = "intensity";  // -24..+24 dB, default 0
inline constexpr const char* origBass = "origBass";    // Original Bass In (true) / Out (false)
inline constexpr const char* gain = "gain";            // -30..0 dB output, default 0
inline constexpr const char* decay = "decay";          // 0..1, hidden, default 1
inline constexpr const char* smoothTc = "smoothTc";    // 0..1000, hidden, default 20
inline constexpr const char* tc = "tc";                // 0..2000 ms, hidden, default 20
inline constexpr const char* xfadeTc = "xfadeTc";      // 0..5000, hidden, default 10
inline constexpr const char* smooth = "smooth";        // on / off, hidden, default on
inline constexpr const char* mode = "mode";            // 0 Stereo, 1 Mono
}
