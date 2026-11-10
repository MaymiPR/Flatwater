#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "dsp/Engine.h"

// Parameters follow the reference's (its VST3 parameters and the ranges, defaults and resolutions; see
// RESEARCH.md, Flatwater Vocal Leveler). No latency (as the reference's); a side-chain input for the music. The riding is the model
// identified on the reference's audio (dsp/Ride.h; MEASUREMENTS.md, Flatwater Vocal Leveler Phase 3 / 4). The music does not
// steer it yet (the reference's use of it could not be measured outside a host that activates its side-chain).
namespace vl
{
inline constexpr const char* target = "target";                    // -42 .. 0 dB, default -21 (the Target slider)
inline constexpr const char* vocalSensitivity = "vocalSensitivity"; // -12 .. +12 dB, default 0
inline constexpr const char* musicSensitivity = "musicSensitivity"; // -12.1 (Off) .. +12 dB, default 0
inline constexpr const char* rangeMax = "rangeMax";                // -11 .. +12 dB, default +6
inline constexpr const char* idle = "idle";                        // 0.02 .. 0.98 of the range (the Idle arrow), default 0.5
inline constexpr const char* rangeMin = "rangeMin";                // -12 .. +11 dB, default -6
inline constexpr const char* rider = "rider";                      // -12 .. +12 dB: the Rider fader (written / read by Automation)
inline constexpr const char* output = "output";                    // -12 .. +12 dB, default 0
inline constexpr const char* attack = "attack";                    // Slow / Fast
inline constexpr const char* spill = "spill";                      // -48 .. -12 dB, default -48 (Live components)
inline constexpr const char* automation = "automation";            // Off / Write / Read
inline constexpr const char* mode = "mode";                        // Stereo, Mono, Live Stereo, Live Mono (the four components)

inline const juce::StringArray& modeNames() { static const juce::StringArray m { "Stereo", "Mono", "Live Stereo", "Live Mono" }; return m; }
inline bool isLive (int modeIndex) { return modeIndex >= 2; }
inline bool musicOff (float value) { return value < -12.05f; }
// the Idle arrow's value in dB: its place in the range
inline float idleDb (float fraction, float lo, float hi) { return lo + fraction * (hi - lo); }
enum Automation { Off, Write, Read };
}
