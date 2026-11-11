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

inline constexpr int numParams = 5;
// every parameter in the layout's order (createLayout below, the order hosts list them in)
inline constexpr const char* const allIds[numParams] {
    id::comp, id::gate, id::gain, id::gateRel, id::mode };
// what a factory preset sets: every parameter but Mode, which a preset leaves as it is (flatwater::applyPreset)
inline constexpr const char* const presetIds[numParams - 1] {
    id::comp, id::gate, id::gain, id::gateRel };

// the host parameters: the reference's names, ranges, tapers, defaults and display texts, in its order
inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto db = [] (float v) { return String (v, 1) + " dB"; };
    auto dbParse = [] (const String& s) { return -std::abs (s.getFloatValue()); };
    auto add = [&] (const char* pid, const char* name, NormalisableRange<float> r, float def, bool visible = true)
    {
        AudioParameterFloatAttributes a;
        a = a.withStringFromValueFunction ([db] (float v, int) { return db (v); })
             .withValueFromStringFunction (dbParse)
             .withAutomatable (visible);
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { pid, 1 }, name, r, def, a));
    };
    add (vc::id::comp, "Comp", { -36.0f, 0.0f, 0.1f }, 0.0f);
    add (vc::id::gate, "Gate", { -80.0f, 0.0f, 0.1f }, -80.0f);
    add (vc::id::gain, "Gain", { -36.0f, 0.0f, 0.1f }, 0.0f);
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { vc::id::gateRel, 1 }, "Gate Release",
                                                  NormalisableRange<float> { 10.0f, 250.0f, 0.1f }, 130.0f,
                                                  AudioParameterFloatAttributes().withAutomatable (false)));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { vc::id::mode, 1 }, "Mode", StringArray { "Stereo", "Mono" }, 0));
    return l;
}
}
