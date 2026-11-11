#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "dsp/Engine.h"
#include "shared/ui/FlatUI.h"

// Parameters follow the reference's parameter list (its order, ranges, tapers and defaults; see RESEARCH.md,
// Flatwater BassX Lift). The DSP is in dsp/Engine.h (MEASUREMENTS.md, "Flatwater BassX Lift model").
namespace bx
{
namespace id
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

inline constexpr int numParams = 10;
// every parameter in the layout's order (createLayout below, the order hosts list them in)
inline constexpr const char* const allIds[numParams] {
    id::input, id::freq, id::decay, id::response, id::ratio, id::monitor, id::highpass, id::origBass, id::harmonics,
    id::mode };
// what a factory preset sets: every parameter but Mode, which a preset leaves as it is (flatwater::applyPreset)
inline constexpr const char* const presetIds[numParams - 1] {
    id::input, id::freq, id::decay, id::response, id::ratio, id::monitor, id::highpass, id::origBass, id::harmonics };

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
    auto addC = [&] (const char* pid, const char* name, StringArray items, int def)
    {
        l.add (std::make_unique<AudioParameterChoice> (ParameterID { pid, 1 }, name, items, def));
    };
    auto logRange = [] (float lo, float hi, float step)
    {
        return NormalisableRange<float> (lo, hi,
            [] (float s, float e, float n) { return s * std::pow (e / s, n); },
            [] (float s, float e, float v) { return std::log (v / s) / std::log (e / s); },
            [step] (float, float, float v) { return std::round (v / step) * step; });
    };
    auto db = [] (float v) { return flatwater::flat::gainText (v) + " dB"; };

    addF (bx::id::input, "Input", flatwater::flat::gainRange (0.0f), 0.0f, db);
    addF (bx::id::freq, "Frequency", logRange (32.0f, 256.0f, 1.0f), 80.0f, [] (float v) { return String (roundToInt (v)) + " Hz"; });
    addF (bx::id::decay, "Decay", { -24.0f, -9.0f, 0.1f }, -15.0f, [] (float v) { return String (v, 1) + " dB"; });
    addF (bx::id::response, "Response", { 10.0f, 30.0f, 0.1f }, 20.0f, [] (float v) { return String (v, 1) + " ms"; });
    addF (bx::id::ratio, "Ratio", logRange (1.0f, 4.0f, 0.01f), 1.0f, [] (float v) { return String (v, 2) + ":1"; });
    addC (bx::id::monitor, "Monitor", { "Audio", "Harmonics", "Original Bass" }, 0);
    addC (bx::id::highpass, "High Pass", { "6 dB/oct", "12 dB/oct", "24 dB/oct" }, 1);
    addF (bx::id::origBass, "Original Bass", flatwater::flat::gainRange (12.0f), 0.0f, db);
    addF (bx::id::harmonics, "Harmonics", flatwater::flat::gainRange (12.0f), -100.0f, db);
    addC (bx::id::mode, "Mode", { "Stereo", "Mono" }, 0);
    return l;
}
}
