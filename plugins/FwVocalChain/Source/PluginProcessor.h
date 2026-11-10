#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "shared/Presets.h"
#include "shared/ui/Widgets.h"
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
#include "Presets.h"

class FwVocalChainProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    FwVocalChainProcessor();
    ~FwVocalChainProcessor() override { stopTimer(); }

    // the In/Out link (Flatwater's own): while it is on, Output is the mirror of Sensitivity (Output = -Sensitivity) in
    // the audio at every block, whatever the Output parameter holds; syncLink() sets the Output parameter to that
    // mirror (a timer does it too, so automation, presets, setups and undo cannot leave the two apart)
    bool linked() const { return get (ch::link) > 0.5f; }
    void syncLink();

    void prepareToPlay (double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Flatwater Vocal Chain"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 5.0; }
    int getNumPrograms() override { return 1 + (int) ch::factoryPresets().size(); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override
    {
        if (index == currentProgram || index < 0 || index >= getNumPrograms()) return;
        currentProgram = index;
        static const flatwater::FactoryPreset defaults { "", "Default", {}, { ch::sensitivity, ch::output, ch::link } };
        flatwater::applyPreset (apvts, index == 0 ? defaults : ch::factoryPresets()[(size_t) index - 1], apvts.getParameter (ch::mode));
    }
    const juce::String getProgramName (int index) override
    {
        if (index <= 0 || index >= getNumPrograms()) return "Default";
        const auto& p = ch::factoryPresets()[(size_t) index - 1];
        return juce::String (p.group) + ": " + p.name;
    }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::UndoManager undo { 32, 32 };
    juce::AudioProcessorValueTreeState apvts;

    // editor taps: the input and output peaks per channel (dB, the highest of the block) for the meters, and the
    // input's rms level after Sensitivity for the Sensitivity LED
    flatwater::PeakTap inL, inR, outL, outR;
    std::atomic<float> sensLevelDb { -100.0f };

    float get (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    void setNoise (bool on) { chain.noiseOn = on; }                   // (the reference's state-only Noise; off for null tests)
    int modeIndex() const { return juce::roundToInt (get (ch::mode)); }

private:
    void timerCallback() override { syncLink(); }
    juce::AudioProcessorValueTreeState::ParameterLayout layout();
    double sr = 48000.0;
    ch::dsp::Chain chain;
    juce::AudioBuffer<float> work;
    int currentProgram = 0;
};
