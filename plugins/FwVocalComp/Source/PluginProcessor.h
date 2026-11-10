#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "shared/ui/Widgets.h"
#include "Presets.h"
#include "dsp/Engine.h"
#include "Params.h"

class FwVocalCompProcessor : public juce::AudioProcessor
{
public:
    FwVocalCompProcessor();

    void prepareToPlay (double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Flatwater Vocal Comp"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    // programs for the host's preset list: 0 = Default, then the factory presets (vc::factoryPresets()).
    // A change applies only when the number changes, so a host that sets program 0 at load keeps the restored state.
    int getNumPrograms() override { return 1 + (int) vc::factoryPresets().size(); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override
    {
        if (index == currentProgram || index < 0 || index >= getNumPrograms()) return;
        currentProgram = index;
        static const flatwater::FactoryPreset defaults { "", "Default", {} };
        flatwater::applyPreset (apvts, index == 0 ? defaults : vc::factoryPresets()[(size_t) index - 1], apvts.getParameter (vc::mode));
    }
    const juce::String getProgramName (int index) override
    {
        if (index <= 0 || index >= getNumPrograms()) return "Default";
        const auto& p = vc::factoryPresets()[(size_t) index - 1];
        return juce::String (p.group) + ": " + p.name;
    }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::UndoManager undo { 32, 32 };   // the reference undoes the last 32 actions
    juce::AudioProcessorValueTreeState apvts;

    // Meter values for the editor, the highest since it last read them (dB): input energy, depth of the total gain
    // reduction (>= 0), output peaks.
    flatwater::PeakTap energyDb, attnDepth, outLDb, outRDb;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    int currentProgram = 0;
    void configure();

    vc::Processor procs[2];
    std::atomic<float>* pComp = nullptr; std::atomic<float>* pGate = nullptr; std::atomic<float>* pGain = nullptr; std::atomic<float>* pMode = nullptr;
    double sr = 48000.0;
    bool prepared = false, layoutDual = false;
};
