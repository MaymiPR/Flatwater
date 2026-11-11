#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "dsp/Engine.h"

// Flatwater Vocal Chain: a one-fader-per-stage vocal chain modelled on the reference (RESEARCH.md, Flatwater Vocal Chain): Sensitivity, Bass,
// Treble, Compress, Reverb, Delay and Pitch faders, each stage with a colour, Output and Balance. The audio is
// dsp/Engine.h's chain (Phase 4 under way).
namespace ch
{
namespace id
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
}
                                                                   // Output linked, the editor moving one against the other

// the six stages: fader and colour IDs, the label under the fader, the colours' names (the host's texts, the clear
// position first)
struct Stage { const char* fader; const char* colour; const char* label; const char* names[4]; };
inline const std::array<Stage, 6>& stages()
{
    static const std::array<Stage, 6> s { {
        { id::bass, id::bassColor, "BASS", { "Bypass", "Sub", "Lower", "Upper" } },
        { id::treble, id::trebleColor, "TREBLE", { "Bypass", "Bite", "Top", "Roof" } },
        { id::compress, id::compressColor, "COMPRESS", { "Bypass", "Push", "Spank", "Wall" } },
        { id::reverb, id::reverbColor, "REVERB", { "Mute", "Tight", "Large", "Chamber" } },
        { id::delay, id::delayColor, "DELAY", { "Mute", "Slap", "Eight", "Quarter" } },
        { id::pitch, id::pitchColor, "PITCH", { "Mute", "Stereo", "Wide", "Spreader" } } } };
    return s;
}
inline const juce::StringArray& modeNames() { static const juce::StringArray m { "Stereo", "Mono/Stereo" }; return m; }
inline bool isMonoIn (int modeIndex) { return modeIndex == 1; }
// the reference's latency (RefRender): 193 samples at 44.1 and 48 kHz, 385 at 96 kHz (fixed samples, doubled with
// the rate; assumed to go on doubling above)
inline int latencySamples (double fs) { const int k = fs > 144000.0 ? 4 : fs > 72000.0 ? 2 : 1; return 193 * k - (k - 1); }

inline constexpr int numParams = 17;
// every parameter in the layout's order (createLayout below, the order hosts list them in)
inline constexpr const char* const allIds[numParams] {
    id::sensitivity, id::bass, id::bassColor, id::treble, id::trebleColor, id::compress, id::compressColor, id::reverb,
    id::reverbColor, id::delay, id::delayColor, id::pitch, id::pitchColor, id::output, id::balance, id::mode, id::link };
// what a factory preset sets: every parameter but Mode, which a preset leaves as it is (flatwater::applyPreset)
inline constexpr const char* const presetIds[numParams - 1] {
    id::sensitivity, id::bass, id::bassColor, id::treble, id::trebleColor, id::compress, id::compressColor, id::reverb,
    id::reverbColor, id::delay, id::delayColor, id::pitch, id::pitchColor, id::output, id::balance, id::link };

// the host parameters: the reference's names, ranges, tapers, defaults and display texts, in its order
inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto text1 = [] (float v) { return String (v, 1); };
    auto addF = [&] (const char* pid, const char* name, float lo, float hi, std::function<String (float)> text, const char* unit = "")
    {
        auto a = AudioParameterFloatAttributes().withStringFromValueFunction ([text] (float v, int) { return text (v); }).withLabel (unit);
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { pid, 1 }, name, NormalisableRange<float> (lo, hi, 0.1f), 0.0f, a));
    };
    // in the reference's host order: Sensitivity, then each fader and its colour, Output, Balance
    addF (ch::id::sensitivity, "Sensitivity", -10.0f, 10.0f, text1, "dB");
    const char* names[6][2] = { { "Bass", "Bass Color" }, { "Treble", "Treble Color" }, { "Compress", "Compress Color" },
                                { "Reverb", "Reverb Color" }, { "Delay", "Delay Color" }, { "Pitch", "Pitch Color" } };
    for (size_t i = 0; i < 6; ++i)
    {
        const auto& s = ch::stages()[i];
        addF (s.fader, names[i][0], -10.0f, 10.0f, text1);
        l.add (std::make_unique<AudioParameterChoice> (ParameterID { s.colour, 1 }, names[i][1],
                                                       StringArray { s.names[0], s.names[1], s.names[2], s.names[3] }, 1));
    }
    addF (ch::id::output, "Output", -10.0f, 10.0f, text1, "dB");
    addF (ch::id::balance, "Balance", -6.0f, 6.0f, text1);
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ch::id::mode, 1 }, "Mode", ch::modeNames(), 0));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { ch::id::link, 1 }, "Link In/Out", false));
    return l;
}
}
