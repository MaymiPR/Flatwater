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
}
