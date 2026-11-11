#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <vector>
#include "shared/ui/Widgets.h"
#include "Presets.h"
#include "dsp/Engine.h"
#include "Params.h"

class FwBassLevelerProcessor : public juce::AudioProcessor
{
public:
    FwBassLevelerProcessor();

    void prepareToPlay (double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Flatwater Bass Leveler"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    // programs for the host's preset list: 0 = Default, then the factory presets (bl::factoryPresets())
    int getNumPrograms() override { return 1 + (int) bl::factoryPresets().size(); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override
    {
        if (index == currentProgram || index < 0 || index >= getNumPrograms()) return;
        currentProgram = index;
        static const flatwater::FactoryPreset defaults { "", "Default", {} };
        flatwater::applyPreset (apvts, index == 0 ? defaults : bl::factoryPresets()[(size_t) index - 1], apvts.getParameter (bl::id::mode));
    }
    const juce::String getProgramName (int index) override
    {
        if (index <= 0 || index >= getNumPrograms()) return "Default";
        const auto& p = bl::factoryPresets()[(size_t) index - 1];
        return juce::String (p.group) + ": " + p.name;
    }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::UndoManager undo { 32, 32 };
    juce::AudioProcessorValueTreeState apvts;

    // editor taps: the input's energy (dB, the highest of the block, for the Target meter), the output's peak (clip LED),
    // the Rider's gain (dB)
    flatwater::PeakTap energyDb, outDb;
    std::atomic<float> rideDb { 0.0f };
    // the Rider fader grabbed in the editor: its gain while held (riding resumes on release)
    std::atomic<bool> userRide { false };
    std::atomic<float> userRideDb { 0.0f };

    float get (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    int modeIndex() const { return juce::roundToInt (get (bl::id::mode)); }
    float idleDb() const { return bl::idleDb (get (bl::id::idle), get (bl::id::rangeMin), get (bl::id::rangeMax)); }

private:
    int currentProgram = 0;
    void configure();
    double sr = 48000.0;
    int delaySamples = 0, writePos = 0, configuredMode = -1;
    std::vector<std::vector<float>> line;
    bl::dsp::Ride ride;
    double outGain = 1.0, heldDb = 0.0, smooth5 = 0.0;      // the Output gain and a held Rider, each smoothed over 5 ms
    bool wasHeld = false, returning = false;
};
