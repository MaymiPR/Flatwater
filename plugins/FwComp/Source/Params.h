#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "dsp/Engine.h"
#include "dsp/Analog.h"

// Parameters follow the reference's parameter list (its order, ranges, tapers and defaults; see RESEARCH.md,
// Flatwater Comp); the DSP is in dsp/Engine.h.
namespace co
{
namespace id
{
inline constexpr const char* threshold = "threshold";  // 0 .. -48 dB, default -18
inline constexpr const char* ratio = "ratio";          // 1 .. 50 (the reference's ratio taper), default 3
inline constexpr const char* punch = "punch";          // 0 .. 30, default 1
inline constexpr const char* mix = "mix";              // 0 (dry) .. 100 (wet), default 100
inline constexpr const char* output = "output";        // -18 .. +18 dB, default 0
inline constexpr const char* attack = "attack";        // 0.5 .. 100 ms, logarithmic, default 7
inline constexpr const char* release = "release";      // 3 .. 3000 ms, logarithmic, default 100
inline constexpr const char* releaseNote = "releaseNote"; // 20 note values (1/64T .. 2 bars), default 1/32D
inline constexpr const char* sync = "sync";            // BPM / Host / ms, default ms
inline constexpr const char* bpm = "bpm";              // 30 .. 300, default 120 (used in BPM sync)
inline constexpr const char* analog = "analog";        // Off / 1 .. 4, default 2
inline constexpr const char* limiter = "limiter";      // Off (clipper) / On (limiter), default Off
inline constexpr const char* meter = "meter";          // In / GR / Out, default GR
inline constexpr const char* mode = "mode";            // 0 Stereo, 1 Mono
}

// the reference's ratio taper (measured, RESEARCH.md): ln(ratio) = 4.40703 v - 0.494827 v^2
float ratioFromNorm (float v);
float ratioToNorm (float r);
// "0.50 ms", "12.0 ms", "100 ms": three significant digits, as the reference shows times
juce::String timeText (float ms);
// the Release (ms) a Release Note gives at a tempo (0: none, 120 BPM), with the reference's fallback for long notes
float syncedReleaseMs (int note, double tempo);

inline constexpr int numParams = 14;
// every parameter in the layout's order (createLayout below, the order hosts list them in)
inline constexpr const char* const allIds[numParams] {
    id::threshold, id::ratio, id::punch, id::mix, id::output, id::attack, id::release, id::releaseNote, id::sync,
    id::bpm, id::analog, id::limiter, id::meter, id::mode };
// what a factory preset sets: every parameter but Mode, which a preset leaves as it is (flatwater::applyPreset)
inline constexpr const char* const presetIds[numParams - 1] {
    id::threshold, id::ratio, id::punch, id::mix, id::output, id::attack, id::release, id::releaseNote, id::sync,
    id::bpm, id::analog, id::limiter, id::meter };

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
    const NormalisableRange<float> ratioRange (1.0f, 50.0f,
        [] (float, float, float n) { return co::ratioFromNorm (n); },
        [] (float, float, float v) { return co::ratioToNorm (v); },
        [] (float, float, float v) { return std::round (v * 100.0f) / 100.0f; });

    // as the reference: normalised 0 is 0 dB and 1 is -48 dB (the knob turns clockwise toward -48, dragging up)
    NormalisableRange<float> thr (-48.0f, 0.0f,
        [] (float, float, float n) { return -48.0f * n; },
        [] (float, float, float v) { return jlimit (0.0f, 1.0f, -v / 48.0f); },
        [] (float, float, float v) { return std::round (v * 10.0f) / 10.0f; });
    addF (co::id::threshold, "Threshold", thr, -18.0f, [] (float v) { return String (v, 1) + " dB"; });
    addF (co::id::ratio, "Ratio", ratioRange, 3.0f, [] (float v) { return String (v, 2); });
    addF (co::id::punch, "Punch", { 0.0f, 30.0f, 0.1f }, 1.0f, [] (float v) { return String (v, 1); });
    addF (co::id::mix, "Mix", { 0.0f, 100.0f, 0.1f }, 100.0f, [] (float v) { return String (v, 1); });
    addF (co::id::output, "Output", { -18.0f, 18.0f, 0.1f }, 0.0f, [] (float v) { return String (v, 1) + " dB"; });
    addF (co::id::attack, "Attack", logRange (0.5f, 100.0f, 0.01f), 7.0f, [] (float v) { return co::timeText (v); });
    addF (co::id::release, "Release", logRange (3.0f, 3000.0f, 0.01f), 100.25f, [] (float v) { return co::timeText (v); });
    addC (co::id::releaseNote, "Release Note", { "1/64T", "1/64", "1/64D", "1/32T", "1/32", "1/32D", "1/16T", "1/16", "1/16D",
                                                "1/8T", "1/8", "1/8D", "1/4T", "1/4", "1/4D", "1/2T", "1/2", "1/2D", "1 Bar", "2 Bars" }, 5);
    addC (co::id::sync, "Sync", { "BPM", "Host", "ms" }, 2);
    addF (co::id::bpm, "BPM", { 30.0f, 300.0f, 1.0f }, 120.0f, [] (float v) { return String (roundToInt (v)); });
    addC (co::id::analog, "Analog", { "Off", "1", "2", "3", "4" }, 2);
    addC (co::id::limiter, "Limiter", { "Off", "On" }, 0);
    addC (co::id::meter, "Meter", { "In", "GR", "Out" }, 1);
    addC (co::id::mode, "Mode", { "Stereo", "Mono" }, 0);
    return l;
}
}
