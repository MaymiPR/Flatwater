#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <vector>
#include "dsp/Engine.h"

// Parameters follow the reference's (its VST3 parameters and the ranges, defaults and resolutions; see
// RESEARCH.md, Flatwater Bass Leveler). The audio is delayed by the component's latency (the look-ahead) and ridden by the model
// identified on the reference's audio (dsp/Ride.h; MEASUREMENTS.md, Flatwater Bass Leveler Phase 3 / 4).
namespace bl
{
inline constexpr const char* target = "target";            // -42 .. 0 dB, default -14 (the Target slider)
inline constexpr const char* sensitivity = "sensitivity";  // 0 .. 100, default 50 (note detection)
inline constexpr const char* response = "response";        // Slow / Fast
inline constexpr const char* spill = "spill";              // -50 .. -3 dB, default -40 (Ignore Spill)
inline constexpr const char* artifacts = "artifacts";      // 0 .. 100, default 50 (Ignore Artifacts; -20 .. -3 dB inside)
inline constexpr const char* rangeMax = "rangeMax";        // -11 .. +12 dB, default +12
inline constexpr const char* rangeMin = "rangeMin";        // -12 .. +11 dB, default -12
inline constexpr const char* idle = "idle";                // 0.02 .. 0.98 of the range (the Idle arrow), default 0.5
inline constexpr const char* output = "output";            // -12 .. +12 dB, default 0
inline constexpr const char* mode = "mode";                // Stereo, Mono, Live Stereo, Live Mono (the four components)

inline const juce::StringArray& modeNames() { static const juce::StringArray m { "Stereo", "Mono", "Live Stereo", "Live Mono" }; return m; }
inline bool isLive (int modeIndex) { return modeIndex >= 2; }
inline double latencyMs (int modeIndex) { return isLive (modeIndex) ? 5.0 : 42.0; }   // measured: 42.00 / 5.00 ms
// the Idle arrow's value in dB: its place in the range
inline float idleDb (float fraction, float lo, float hi) { return lo + fraction * (hi - lo); }
}
