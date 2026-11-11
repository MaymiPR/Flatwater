#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "shared/ui/Widgets.h"
#include "Presets.h"
#include "dsp/Engine.h"
#include "Params.h"

class FwBassLiftProcessor : public juce::AudioProcessor
{
public:
    FwBassLiftProcessor();

    void prepareToPlay (double sampleRate, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Flatwater Bass Lift"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    // programs for the host's preset list: 0 = Default, then the factory presets (bf::factoryPresets()).
    // A change applies only when the number changes, so a host that sets program 0 at load keeps the restored state.
    int getNumPrograms() override { return 1 + (int) bf::factoryPresets().size(); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override
    {
        if (index == currentProgram || index < 0 || index >= getNumPrograms()) return;
        currentProgram = index;
        static const flatwater::FactoryPreset defaults { "", "Default", {} };
        flatwater::applyPreset (apvts, index == 0 ? defaults : bf::factoryPresets()[(size_t) index - 1], apvts.getParameter (bf::id::mode));
    }
    const juce::String getProgramName (int index) override
    {
        if (index <= 0 || index >= getNumPrograms()) return "Default";
        const auto& p = bf::factoryPresets()[(size_t) index - 1];
        return juce::String (p.group) + ": " + p.name;
    }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::UndoManager undo { 32, 32 };   // the reference undoes the last 32 actions
    juce::AudioProcessorValueTreeState apvts;

    // meters (dB, highest peak since the editor last read them): original bass below Freq, created harmonics, output
    // peaks; clip latch
    flatwater::PeakTap origDb, harmDb, outLDb, outRDb;
    std::atomic<bool> clipped { false };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    int currentProgram = 0;
    bf::dsp::Processor dsp;
};
