#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "dsp/Engine.h"

// Flatwater Vocal Chain: a one-fader-per-stage vocal chain modelled on the reference (RESEARCH.md, Flatwater Vocal Chain): Sensitivity, Bass,
// Treble, Compress, Reverb, Delay and Pitch faders, each stage with a colour, Output and Balance. The audio is
// dsp/Engine.h's chain (Phase 4 under way).
namespace ch
{
inline constexpr const char* sensitivity = "sensitivity";
inline constexpr const char* bass = "bass";
inline constexpr const char* bassColor = "bassColor";
inline constexpr const char* treble = "treble";
inline constexpr const char* trebleColor = "trebleColor";
inline constexpr const char* compress = "compress";
inline constexpr const char* compressColor = "compressColor";
inline constexpr const char* reverb = "reverb";
inline constexpr const char* reverbColor = "reverbColor";
inline constexpr const char* delay = "delay";
inline constexpr const char* delayColor = "delayColor";
inline constexpr const char* pitch = "pitch";
inline constexpr const char* pitchColor = "pitchColor";
inline constexpr const char* output = "output";
inline constexpr const char* balance = "balance";
inline constexpr const char* mode = "mode";                        // Stereo, Mono/Stereo (the two components)
inline constexpr const char* link = "ioLink";                      // Flatwater's own (not in the reference): Sensitivity and
                                                                   // Output linked, the editor moving one against the other

// the six stages: fader and colour IDs, the label under the fader, the colours' names (the host's texts, the clear
// position first)
struct Stage { const char* fader; const char* colour; const char* label; const char* names[4]; };
inline const std::array<Stage, 6>& stages()
{
    static const std::array<Stage, 6> s { {
        { bass, bassColor, "BASS", { "Bypass", "Sub", "Lower", "Upper" } },
        { treble, trebleColor, "TREBLE", { "Bypass", "Bite", "Top", "Roof" } },
        { compress, compressColor, "COMPRESS", { "Bypass", "Push", "Spank", "Wall" } },
        { reverb, reverbColor, "REVERB", { "Mute", "Tight", "Large", "Chamber" } },
        { delay, delayColor, "DELAY", { "Mute", "Slap", "Eight", "Quarter" } },
        { pitch, pitchColor, "PITCH", { "Mute", "Stereo", "Wide", "Spreader" } } } };
    return s;
}
inline const juce::StringArray& modeNames() { static const juce::StringArray m { "Stereo", "Mono/Stereo" }; return m; }
inline bool isMonoIn (int modeIndex) { return modeIndex == 1; }
// the reference's latency (RefRender): 193 samples at 44.1 and 48 kHz, 385 at 96 kHz (fixed samples, doubled with
// the rate; assumed to go on doubling above)
inline int latencySamples (double fs) { const int k = fs > 144000.0 ? 4 : fs > 72000.0 ? 2 : 1; return 193 * k - (k - 1); }
}
