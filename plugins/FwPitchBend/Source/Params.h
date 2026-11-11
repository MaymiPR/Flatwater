#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "dsp/Model.h"
#include "dsp/Mod.h"
#include "dsp/Shifter.h"

// Parameters follow the reference's host parameters (their names, ranges, defaults and display texts as ParamDump
// reads them; RESEARCH.md, Flatwater Pitch Bend). The tapers are the exponential ones the texts show (Rate 0.06 ..
// 30 Hz, Smooth 1 .. 1000, Warp 0.01 .. 100, the AM times 0.1 .. 1000 ms). No latency (as the reference's).
// The settings that are not host parameters (the modulation slots, Fine, the shape tables) are in the state tree
// (ModulationState.h); the audio thread works on a copy of them, refreshed whenever the tree changes. Per host block:
// the modulators (dsp/Mod.h) move the controls, the shifter (dsp/Shifter.h) makes the wet signal, and
// Mix crossfades the dry input and the wet one (dry + m (wet - dry), as the reference's).
namespace pb
{
// the two LFO / step modulators share a layout: their IDs are "m1..." and "m2..."
struct ModIds { const char* warp; const char* level; const char* rateSynced; const char* rate; const char* sync;
                const char* phase; const char* type; const char* smooth; const char* steps; };
inline constexpr ModIds mod[2] {
    { "m1Warp", "m1Level", "m1RateSynced", "m1Rate", "m1Sync", "m1Phase", "m1Type", "m1Smooth", "m1Steps" },
    { "m2Warp", "m2Level", "m2RateSynced", "m2Rate", "m2Sync", "m2Phase", "m2Type", "m2Smooth", "m2Steps" } };
namespace id
{
inline constexpr const char* amAttack = "amAttack";        // 0.1 .. 1000 ms, default 10
inline constexpr const char* amRelease = "amRelease";      // 0.1 .. 1000 ms, default 100
inline constexpr const char* amLevel = "amLevel";          // 0 .. 2, default 1
inline constexpr const char* ptSmooth = "ptSmooth";        // 1 .. 1000, default 10
inline constexpr const char* ptOffset = "ptOffset";        // -1 .. 1, default 0
inline constexpr const char* ptLevel = "ptLevel";          // 0 .. 2, default 1
inline constexpr const char* pitch = "pitch";              // -12 .. +12 semitones, default 0
inline constexpr const char* formant = "formant";          // -12 .. +12 semitones, default 0
inline constexpr const char* mix = "mix";                  // 0 .. 100 %, default 100
inline constexpr const char* flattenNote = "flattenNote";  // C2 .. B3, default C3
inline constexpr const char* flatten = "flatten";          // Off / On
inline constexpr const char* mode = "mode";                // Stereo, Mono (the two components)
}

inline const juce::StringArray& modeNames() { static const juce::StringArray m { "Stereo", "Mono" }; return m; }
// the synced rates, slowest first (the reference's list)
inline const juce::StringArray& syncedRates()
{
    static const juce::StringArray r { "32B", "16B", "8B", "4B", "2B", "1B", "1/2D", "1/2", "1/4D", "1/2T", "1/4", "1/8D",
                                       "1/4T", "1/8", "1/16D", "1/8T", "1/16", "1/32D", "1/16T", "1/32", "1/64D", "1/32T", "1/64", "1/64T" };
    return r;
}
inline const juce::StringArray& noteNames()
{
    static const juce::StringArray n = []
    {
        juce::StringArray s;
        const char* names[] { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        for (int octave = 2; octave <= 3; ++octave)
            for (auto* nm : names) s.add (juce::String (nm) + juce::String (octave));
        return s;
    }();
    return n;
}

inline constexpr int numParams = 30;
// every parameter in the layout's order (createLayout below, the order hosts list them in)
inline constexpr const char* const allIds[numParams] {
    mod[0].warp, mod[0].level, mod[0].rateSynced, mod[0].rate, mod[0].sync, mod[0].phase, mod[0].type, mod[0].smooth,
    mod[0].steps, mod[1].warp, mod[1].level, mod[1].rateSynced, mod[1].rate, mod[1].sync, mod[1].phase, mod[1].type,
    mod[1].smooth, mod[1].steps, id::amAttack, id::amRelease, id::amLevel, id::ptSmooth, id::ptOffset, id::ptLevel,
    id::pitch, id::formant, id::mix, id::flattenNote, id::flatten, id::mode };
// what a factory preset sets: every parameter but Mode, which a preset leaves as it is (flatwater::applyPreset)
inline constexpr const char* const presetIds[numParams - 1] {
    mod[0].warp, mod[0].level, mod[0].rateSynced, mod[0].rate, mod[0].sync, mod[0].phase, mod[0].type, mod[0].smooth,
    mod[0].steps, mod[1].warp, mod[1].level, mod[1].rateSynced, mod[1].rate, mod[1].sync, mod[1].phase, mod[1].type,
    mod[1].smooth, mod[1].steps, id::amAttack, id::amRelease, id::amLevel, id::ptSmooth, id::ptOffset, id::ptLevel,
    id::pitch, id::formant, id::mix, id::flattenNote, id::flatten };

// a host parameter's range from the DSP's (dsp/Model.h Range): linear, or the reference's exponential taper
inline juce::NormalisableRange<float> hostRange (const dsp::Range& r)
{
    if (! r.exponential) return { r.start, r.end };
    return { r.start, r.end,
             [] (float a, float b, float v) { return dsp::Range::expFrom0to1 (a, b, v); },
             [] (float a, float b, float x) { return dsp::Range::expTo0to1 (a, b, x); } };
}

// the host parameters: the reference's names, ranges, tapers, defaults and display texts, in its order
inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto addF = [&] (const char* pid, const String& name, NormalisableRange<float> r, float def, std::function<String (float)> text)
    {
        auto a = AudioParameterFloatAttributes().withStringFromValueFunction ([text] (float v, int) { return text (v); });
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { pid, 1 }, name, r, def, a));
    };
    auto addC = [&] (const char* pid, const String& name, const StringArray& items, int def)
    {
        l.add (std::make_unique<AudioParameterChoice> (ParameterID { pid, 1 }, name, items, def));
    };
    auto fix = [] (int digits) { return [digits] (float v) { return String (v, digits); }; };
    auto ms = [] (float v) { return String (v, 1) + " ms"; };
    // in the reference's parameter order
    for (int m = 0; m < 2; ++m)
    {
        const auto& d = pb::mod[m];
        const String p = "Mod" + String (m + 1) + " ";
        addF (d.warp, p + "Warp", hostRange (dsp::ranges::warp), 1.0f, fix (2));
        addF (d.level, p + "Level", hostRange (dsp::ranges::unit), 1.0f, fix (2));
        addC (d.rateSynced, p + "Rate Synced", pb::syncedRates(), 5);
        addF (d.rate, p + "Rate", hostRange (dsp::ranges::rate), 0.5f, [] (float v) { return String (v, 3) + " Hz"; });
        addC (d.sync, p + "Sync On/Off", StringArray { "Note", "ms" }, 0);
        addF (d.phase, p + "Phase", hostRange (dsp::ranges::unit), 0.0f, fix (3));
        addC (d.type, p + "Type", StringArray { "LFO", "Seq" }, 0);
        addF (d.smooth, p + "Smooth", hostRange (dsp::ranges::smooth), 1.5f, fix (1));
        addF (d.steps, p + "Steps", { 2.0f, 16.0f, 1.0f }, 16.0f, fix (0));
    }
    addF (pb::id::amAttack, "AM Attack", hostRange (dsp::ranges::amTime), 10.0f, ms);
    addF (pb::id::amRelease, "AM Release", hostRange (dsp::ranges::amTime), 100.0f, ms);
    addF (pb::id::amLevel, "AM Level", hostRange (dsp::ranges::twice), 1.0f, fix (2));
    addF (pb::id::ptSmooth, "PT Smooth", hostRange (dsp::ranges::smooth), 10.0f, fix (1));
    addF (pb::id::ptOffset, "PT Offset", hostRange (dsp::ranges::offset), 0.0f, fix (2));
    addF (pb::id::ptLevel, "PT Level", hostRange (dsp::ranges::twice), 1.0f, fix (2));
    auto semis = [] (float v) { return String (roundToInt (v)) + " semitones"; };
    addF (pb::id::pitch, "Pitch", hostRange (dsp::ranges::semitones), 0.0f, semis);
    addF (pb::id::formant, "Formant", hostRange (dsp::ranges::semitones), 0.0f, semis);
    addF (pb::id::mix, "Mix", hostRange (dsp::ranges::mix), 100.0f, fix (1));
    addC (pb::id::flattenNote, "Flatten Note", pb::noteNames(), 12);
    addC (pb::id::flatten, "Flatten On/Off", StringArray { "Off", "On" }, 0);
    addC (pb::id::mode, "Mode", pb::modeNames(), 0);
    return l;
}
}
