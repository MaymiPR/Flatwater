#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "shared/ui/Widgets.h"
#include "shared/Presets.h"
#include "Presets.h"
#include "dsp/Engine.h"
#include "Params.h"

class FwBassXLiftProcessor : public juce::AudioProcessor, public flatwater::HasPresets
{
public:
    FwBassXLiftProcessor();

    void prepareToPlay (double sampleRate, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Flatwater BassX Lift"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    // programs for the host's preset list: 0 = Default, then the factory presets
    int getNumPrograms() override { return 1 + (int) bx::factoryPresets().size(); }
    int getCurrentProgram() override { return presets.program(); }
    void setCurrentProgram (int index) override { presets.setProgram (index); }
    const juce::String getProgramName (int index) override
    {
        if (index <= 0 || index >= getNumPrograms()) return "Default";
        const auto& p = bx::factoryPresets()[(size_t) index - 1];
        return juce::String (p.group) + ": " + p.name;
    }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::UndoManager undo { 32, 32 };
    juce::AudioProcessorValueTreeState apvts;
    // the preset in use, the two setups and the host's program (shared/Presets.h); the toolbar drives it
    flatwater::PresetState presets;
    flatwater::PresetState& presetState() override { return presets; }

    // output meters (dB, highest peak since the editor last read them) and the clip latch
    flatwater::PeakTap outLDb, outRDb;
    std::atomic<bool> clipped { false };
    std::atomic<int> blocks { 0 };          // counts processed blocks (the meters show their floor dot only while audio runs)

private:
    bx::dsp::Processor dsp;
    float get (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
};
