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
namespace id
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
}

inline const juce::StringArray& modeNames() { static const juce::StringArray m { "Stereo", "Mono", "Live Stereo", "Live Mono" }; return m; }
inline bool isLive (int modeIndex) { return modeIndex >= 2; }
inline bool musicOff (float value) { return value < -12.05f; }
// the Idle arrow's value in dB: its place in the range
inline float idleDb (float fraction, float lo, float hi) { return lo + fraction * (hi - lo); }
enum Automation { Off, Write, Read };

inline constexpr int numParams = 12;
// every parameter in the layout's order (createLayout below, the order hosts list them in)
inline constexpr const char* const allIds[numParams] {
    id::target, id::vocalSensitivity, id::musicSensitivity, id::rangeMax, id::idle, id::rangeMin, id::rider, id::output,
    id::attack, id::spill, id::automation, id::mode };
// what a factory preset sets: every parameter but Mode, which a preset leaves as it is (flatwater::applyPreset)
inline constexpr const char* const presetIds[numParams - 1] {
    id::target, id::vocalSensitivity, id::musicSensitivity, id::rangeMax, id::idle, id::rangeMin, id::rider, id::output,
    id::attack, id::spill, id::automation };

// the host parameters: the reference's names, ranges, tapers, defaults and display texts, in its order
inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto addF = [&] (const char* pid, const char* name, NormalisableRange<float> r, float def, std::function<String (float)> text,
                     bool automatable = true)
    {
        auto a = AudioParameterFloatAttributes().withStringFromValueFunction ([text] (float v, int) { return text (v); })
                                                .withAutomatable (automatable);
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { pid, 1 }, name, r, def, a));
    };
    auto db1 = [] (float v) { return String (v, 1) + " dB"; };
    // in the reference's parameter order
    addF (vl::id::target, "Target", { -42.0f, 0.0f, 0.1f }, -21.0f, db1);
    addF (vl::id::vocalSensitivity, "Vocal Sensitivity", { -12.0f, 12.0f, 0.1f }, 0.0f, db1);
    addF (vl::id::musicSensitivity, "Music Sensitivity", { -12.1f, 12.0f, 0.1f }, 0.0f,
          [] (float v) { return vl::musicOff (v) ? String ("Off") : String (v, 1) + " dB"; });
    addF (vl::id::rangeMax, "Range Max", { -11.0f, 12.0f, 0.1f }, 6.0f, db1);
    // the Idle arrow is stored as its place in the range (as the reference's preset arrays); the host sees it in dB
    // over a fixed +-12 dB (as the reference's: its text stays 0.0 at 0.5 whatever the range)
    addF (vl::id::idle, "Idle", { 0.02f, 0.98f, 0.01f }, 0.5f, [] (float v) { return String (vl::idleDb (v, -12.0f, 12.0f), 1); });
    addF (vl::id::rangeMin, "Range Min", { -12.0f, 11.0f, 0.1f }, -6.0f, db1);
    addF (vl::id::rider, "Rider Fader", { -12.0f, 12.0f, 0.1f }, 0.0f, db1);
    addF (vl::id::output, "Output", { -12.0f, 12.0f, 0.1f }, 0.0f, db1);
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { vl::id::attack, 1 }, "Attack", StringArray { "Slow", "Fast" }, 0));
    addF (vl::id::spill, "Spill", { -48.0f, -12.0f, 0.1f }, -48.0f, db1);
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { vl::id::automation, 1 }, "Automation", StringArray { "Off", "Write", "Read" }, 0,
                                                   AudioParameterChoiceAttributes().withAutomatable (false)));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { vl::id::mode, 1 }, "Mode", vl::modeNames(), 0));
    return l;
}
}
