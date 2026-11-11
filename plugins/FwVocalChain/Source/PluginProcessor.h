#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "shared/Presets.h"
#include "shared/ui/Widgets.h"
#include "dsp/Engine.h"
#include "Params.h"
#include "Presets.h"

class FwVocalChainProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    FwVocalChainProcessor();
    ~FwVocalChainProcessor() override { stopTimer(); }

    // the In/Out link (Flatwater's own): while it is on, Output is the mirror of Sensitivity (Output = -Sensitivity) in
    // the audio at every block, whatever the Output parameter holds; syncLink() sets the Output parameter to that
    // mirror (a timer does it too, so automation, presets, setups and undo cannot leave the two apart)
    bool linked() const { return get (ch::id::link) > 0.5f; }
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
        static const flatwater::FactoryPreset defaults { "", "Default", {}, { ch::id::sensitivity, ch::id::output, ch::id::link } };
        flatwater::applyPreset (apvts, index == 0 ? defaults : ch::factoryPresets()[(size_t) index - 1], apvts.getParameter (ch::id::mode));
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
    int modeIndex() const { return juce::roundToInt (get (ch::id::mode)); }

private:
    void timerCallback() override { syncLink(); }
    juce::AudioProcessorValueTreeState::ParameterLayout layout();
    double sr = 48000.0;
    ch::dsp::Chain chain;
    juce::AudioBuffer<float> work;
    int currentProgram = 0;
};
