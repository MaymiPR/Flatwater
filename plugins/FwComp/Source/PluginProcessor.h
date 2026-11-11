#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "shared/ui/Widgets.h"
#include "Presets.h"
#include "dsp/Engine.h"
#include "dsp/Analog.h"
#include "Params.h"

class FwCompProcessor : public juce::AudioProcessor
{
public:
    FwCompProcessor();

    void prepareToPlay (double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Flatwater Comp"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1 + (int) co::factoryPresets().size(); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override
    {
        if (index == currentProgram || index < 0 || index >= getNumPrograms()) return;
        currentProgram = index;
        static const flatwater::FactoryPreset defaults { "", "Default", {} };
        flatwater::applyPreset (apvts, index == 0 ? defaults : co::factoryPresets()[(size_t) index - 1], apvts.getParameter (co::id::mode),
                           { apvts.getParameter (co::id::mix) });           // presets leave Mix alone, as the reference
    }
    const juce::String getProgramName (int index) override
    {
        if (index <= 0 || index >= getNumPrograms()) return "Default";
        const auto& p = co::factoryPresets()[(size_t) index - 1];
        return juce::String (p.group) + ": " + p.name;
    }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::UndoManager undo { 32, 32 };
    juce::AudioProcessorValueTreeState apvts;

    // meters (dB, highest peak since the editor last read them): input, output; the compressor's and the limiter's
    // gain reduction (positive dB)
    flatwater::PeakTap inLDb, inRDb, outLDb, outRDb, grDb, limDb;
    std::atomic<int> blocks { 0 };
    std::atomic<double> hostBpm { 0.0 };                  // 0: the host gives no tempo

private:
    float get (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    void configure();
    co::dsp::Compressor::Settings settings() const;
    int currentProgram = 0;
    co::dsp::Compressor comps[2];
    co::dsp::Analog analogStage;
    double sr = 44100.0;
    bool prepared = false, layoutDual = false;
};
