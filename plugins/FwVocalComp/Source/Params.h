#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "dsp/Engine.h"

// Parameters follow the reference plugin's parameter list (its ranges and defaults).
namespace vc
{
namespace id
{
inline constexpr const char* comp = "comp";          // -36..0 dB, default 0
inline constexpr const char* gate = "gate";          // -80..0 dB, default -80 (off)
inline constexpr const char* gateRel = "gateRel";    // 10..250 ms, hidden, default 130
inline constexpr const char* gain = "gain";          // -36..0 dB, default 0 (limiter ceiling)
inline constexpr const char* mode = "mode";          // 0 Stereo, 1 Mono
}
}
