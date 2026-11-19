#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <vector>
#include "shared/dsp/BypassDelay.h"
#include "shared/ui/Widgets.h"
#include "shared/Presets.h"
#include "Presets.h"
#include "dsp/Engine.h"
#include "Params.h"

class FwBassLevelerProcessor : public juce::AudioProcessor, public flatwater::HasPresets, private juce::Timer
{
public:
    FwBassLevelerProcessor();
    ~FwBassLevelerProcessor() override { stopTimer(); }

    void prepareToPlay (double, int) override;
    void releaseResources() override {}
    // the host clears the tails: the look-ahead line, the ride, a held Rider and the bypass line as prepareToPlay leaves them. No allocation.
    void reset() override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;
    // the host's bypass: the input delayed by the reported latency (shared/dsp/BypassDelay.h)
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlockBypassed;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Flatwater Bass Leveler"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    // programs for the host's preset list: 0 = Default, then the factory presets (bl::factoryPresets())
    int getNumPrograms() override { return 1 + (int) bl::factoryPresets().size(); }
    int getCurrentProgram() override { return presets.program(); }
    void setCurrentProgram (int index) override { presets.setProgram (index); }
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
    // the preset in use, the two setups and the host's program (shared/Presets.h); the toolbar drives it
    flatwater::PresetState presets;
    flatwater::PresetState& presetState() override { return presets; }

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

    // The latency of the component (Mode) in use, told to the host when it is not the one the host has. A Mode change
    // that reaches the audio thread changes the latency there (42 ms, Live 5 ms) and leaves the telling to this, which
    // a timer calls on the message thread.
    void updateHostLatency();

private:
    flatwater::BypassDelay bypassLine;
    void timerCallback() override { updateHostLatency(); }
    void configure();                // allocates: prepareToPlay
    void setMode (int mode);         // a Mode change: a reset without allocation, also on the audio thread
    void followMode (int channels);  // at the start of a block: the Mode parameter's component
    std::atomic<int> latencyNow { 0 };
    double sr = 48000.0;
    int delaySamples = 0, writePos = 0, configuredMode = -1;
    std::vector<std::vector<float>> line;
    bl::dsp::Ride ride;
    double outGain = 1.0, heldDb = 0.0, smooth5 = 0.0;      // the Output gain and a held Rider, each smoothed over 5 ms
    bool wasHeld = false, returning = false;
};
