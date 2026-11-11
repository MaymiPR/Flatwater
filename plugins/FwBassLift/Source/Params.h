#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "dsp/Engine.h"

// Parameters follow the reference's parameter list (its order, ranges and defaults; see RESEARCH.md, Flatwater Bass Lift).
namespace bf
{
namespace id
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

inline constexpr int numParams = 13;
// every parameter in the layout's order (createLayout below, the order hosts list them in)
inline constexpr const char* const allIds[numParams] {
    id::inGain, id::freq, id::response, id::ratio, id::intensity, id::origBass, id::gain, id::decay, id::smoothTc,
    id::tc, id::xfadeTc, id::smooth, id::mode };
// what a factory preset sets: every parameter but Mode, which a preset leaves as it is (flatwater::applyPreset)
inline constexpr const char* const presetIds[numParams - 1] {
    id::inGain, id::freq, id::response, id::ratio, id::intensity, id::origBass, id::gain, id::decay, id::smoothTc,
    id::tc, id::xfadeTc, id::smooth };

// the host parameters: the reference's names, ranges, tapers, defaults and display texts, in its order
inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto addF = [&] (const char* pid, const char* name, NormalisableRange<float> r, float def, bool automatable, const char* unit, int decimals)
    {
        auto a = AudioParameterFloatAttributes()
                     .withStringFromValueFunction ([unit, decimals] (float v, int) { return String (v, decimals) + (unit[0] ? String (" ") + unit : String()); })
                     .withAutomatable (automatable);
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { pid, 1 }, name, r, def, a));
    };
    // on / off switches as two-choice parameters: unlike AudioParameterBool their value always snaps to 0 or 1, so a
    // restored state can never leave them half-way (pluginval's state test)
    auto addB = [&] (const char* pid, const char* name, bool def, bool automatable, StringArray labels)
    {
        l.add (std::make_unique<AudioParameterChoice> (ParameterID { pid, 1 }, name, labels, def ? 1 : 0,
                                                       AudioParameterChoiceAttributes().withAutomatable (automatable)));
    };
    // Freq is logarithmic in the reference (32..256 Hz, 1 Hz steps)
    NormalisableRange<float> freqRange (32.0f, 256.0f,
        [] (float s, float e, float n) { return s * std::pow (e / s, n); },
        [] (float s, float e, float v) { return std::log (v / s) / std::log (e / s); },
        [] (float, float, float v) { return std::round (v); });

    addF (bf::id::inGain, "Input", { -30.0f, 0.0f, 0.1f }, 0.0f, false, "dB", 1);
    addF (bf::id::freq, "Freq", freqRange, 80.0f, true, "Hz", 0);
    addF (bf::id::response, "Response", { 0.0f, 30.0f, 0.1f }, 30.0f, false, "ms", 1);
    addF (bf::id::ratio, "Ratio", { 0.25f, 1.0f, 0.01f }, 0.63f, false, "", 2);
    addF (bf::id::intensity, "Intensity", { -24.0f, 24.0f, 0.1f }, 0.0f, true, "dB", 1);
    addB (bf::id::origBass, "Original Bass", true, true, { "Out", "In" });
    addF (bf::id::gain, "Gain", { -30.0f, 0.0f, 0.1f }, 0.0f, true, "dB", 1);
    addF (bf::id::decay, "Decay", { 0.0f, 1.0f, 0.1f }, 1.0f, false, "", 1);
    addF (bf::id::smoothTc, "SmoothTc", { 0.0f, 1000.0f, 0.1f }, 20.0f, false, "", 1);
    addF (bf::id::tc, "Tc", { 0.0f, 2000.0f, 0.1f }, 20.0f, false, "ms", 1);
    addF (bf::id::xfadeTc, "XFadeTc", { 0.0f, 5000.0f, 0.1f }, 10.0f, false, "", 1);
    addB (bf::id::smooth, "Smooth", true, false, { "Off", "On" });
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { bf::id::mode, 1 }, "Mode", StringArray { "Stereo", "Mono" }, 0));
    return l;
}
}
