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
namespace id
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
}

inline const juce::StringArray& modeNames() { static const juce::StringArray m { "Stereo", "Mono", "Live Stereo", "Live Mono" }; return m; }
inline bool isLive (int modeIndex) { return modeIndex >= 2; }
inline double latencyMs (int modeIndex) { return isLive (modeIndex) ? 5.0 : 42.0; }   // measured: 42.00 / 5.00 ms
// the Idle arrow's value in dB: its place in the range
inline float idleDb (float fraction, float lo, float hi) { return lo + fraction * (hi - lo); }

inline constexpr int numParams = 10;
// every parameter in the layout's order (createLayout below, the order hosts list them in)
inline constexpr const char* const allIds[numParams] {
    id::target, id::sensitivity, id::response, id::spill, id::artifacts, id::rangeMax, id::rangeMin, id::idle,
    id::output, id::mode };
// what a factory preset sets: every parameter but Mode, which a preset leaves as it is (flatwater::applyPreset)
inline constexpr const char* const presetIds[numParams - 1] {
    id::target, id::sensitivity, id::response, id::spill, id::artifacts, id::rangeMax, id::rangeMin, id::idle,
    id::output };

// the host parameters: the reference's names, ranges, tapers, defaults and display texts, in its order
inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto addF = [&] (const char* pid, const char* name, NormalisableRange<float> r, float def, std::function<String (float)> text)
    {
        auto a = AudioParameterFloatAttributes().withStringFromValueFunction ([text] (float v, int) { return text (v); });
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { pid, 1 }, name, r, def, a));
    };
    auto db1 = [] (float v) { return String (v, 1) + " dB"; };
    auto plain1 = [] (float v) { return String (v, 1); };
    addF (bl::id::target, "Target", { -42.0f, 0.0f, 0.1f }, -14.0f, db1);
    addF (bl::id::sensitivity, "Sensitivity", { 0.0f, 100.0f, 0.1f }, 50.0f, plain1);
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { bl::id::response, 1 }, "Response", StringArray { "Slow", "Fast" }, 0));
    addF (bl::id::spill, "Spill", { -50.0f, -3.0f, 0.1f }, -40.0f, db1);
    addF (bl::id::artifacts, "Artifacts", { 0.0f, 100.0f, 0.1f }, 50.0f, plain1);
    addF (bl::id::rangeMax, "Range Max", { -11.0f, 12.0f, 0.1f }, 12.0f, db1);
    addF (bl::id::rangeMin, "Range Min", { -12.0f, 11.0f, 0.1f }, -12.0f, db1);
    // the Idle arrow is stored as its place in the range (as the reference's preset arrays); the host sees it in dB
    // over a fixed +-12 dB (as the reference's: its text stays 0.0 at 0.5 whatever the range)
    addF (bl::id::idle, "Idle", { 0.02f, 0.98f, 0.01f }, 0.5f, [] (float v) { return String (bl::idleDb (v, -12.0f, 12.0f), 1); });
    addF (bl::id::output, "Output", { -12.0f, 12.0f, 0.1f }, 0.0f, db1);
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { bl::id::mode, 1 }, "Mode", bl::modeNames(), 0));
    return l;
}
}
