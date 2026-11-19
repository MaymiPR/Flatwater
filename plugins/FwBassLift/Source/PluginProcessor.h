#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "shared/ui/Widgets.h"
#include "shared/Presets.h"
#include "Presets.h"
#include "dsp/Engine.h"
#include "Params.h"

class FwBassLiftProcessor : public juce::AudioProcessor, public flatwater::HasPresets
{
public:
    FwBassLiftProcessor();

    void prepareToPlay (double sampleRate, int) override;
    void releaseResources() override {}
    // the host clears the tails: the filters, the detectors and the learned gain block as prepareToPlay leaves them. No allocation.
    void reset() override { dsp.reset(); }
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
    int getCurrentProgram() override { return presets.program(); }
    void setCurrentProgram (int index) override { presets.setProgram (index); }
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
    // the preset in use, the two setups and the host's program (shared/Presets.h); the toolbar drives it
    flatwater::PresetState presets;
    flatwater::PresetState& presetState() override { return presets; }

    // meters (dB, highest peak since the editor last read them): original bass below Freq, created harmonics, output
    // peaks; clip latch
    flatwater::PeakTap origDb, harmDb, outLDb, outRDb;
    std::atomic<bool> clipped { false };

private:
    bf::dsp::Processor dsp;
};
