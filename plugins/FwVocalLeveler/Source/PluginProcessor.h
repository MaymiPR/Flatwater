#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "shared/ui/Widgets.h"
#include "shared/Presets.h"
#include "dsp/Engine.h"
#include "Params.h"
#include "Presets.h"

class FwVocalLevelerProcessor : public juce::AudioProcessor, public flatwater::HasPresets, private juce::Timer,
                                private juce::ValueTree::Listener
{
public:
    FwVocalLevelerProcessor();
    ~FwVocalLevelerProcessor() override
    {
        stopTimer();
        apvts.state.removeListener (this);
        if (writing) apvts.getParameter (vl::id::rider)->endChangeGesture();
    }

    void prepareToPlay (double, int) override;
    void releaseResources() override {}
    // the host clears the tails: the ride, a held Rider and the music follower as prepareToPlay leaves them. No allocation.
    void reset() override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Flatwater Vocal Leveler"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1 + (int) vl::factoryPresets().size(); }
    int getCurrentProgram() override { return presets.program(); }
    void setCurrentProgram (int index) override { presets.setProgram (index); }
    const juce::String getProgramName (int index) override
    {
        if (index <= 0 || index >= getNumPrograms()) return "Default";
        const auto& p = vl::factoryPresets()[(size_t) index - 1];
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

    // editor taps: the vocal level (dB, the highest of the block, for the meter under Target), the output's peak (clip
    // LED), the music level (the Music LED), the Rider's gain (dB), vocal activity (the Ride / Idle LEDs)
    flatwater::PeakTap vocalDb, outDb, musicDb;
    std::atomic<float> rideDb { 0.0f };
    std::atomic<bool> riding { false }, sideChainOn { false };
    // the Rider fader grabbed in the editor: its gain while held (riding resumes on release)
    std::atomic<bool> userRide { false };
    std::atomic<float> userRideDb { 0.0f };

    float get (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    int modeIndex() const { return juce::roundToInt (get (vl::id::mode)); }
    int automationMode() const { return juce::roundToInt (get (vl::id::automation)); }
    // the Idle arrow's place: a factory preset's own value (presets use 0 and 1, outside the arrow's 0.02 .. 0.98) while
    // the arrow stays where the preset put it, else the parameter's
    float idleFraction() const
    {
        const float v = get (vl::id::idle), e = exactIdle.load();
        return (e >= 0.0f && std::abs (v - juce::jlimit (0.02f, 0.98f, e)) < 1.0e-4f) ? e : v;
    }
    float idleDb() const { return vl::idleDb (idleFraction(), get (vl::id::rangeMin), get (vl::id::rangeMax)); }

private:
    void timerCallback() override;                              // Automation Write: the ride to the host
    // the preset's Idle, read when the tree changes (a preset, an undo, a restore) on the thread that changes it
    void readExactIdle();
    void valueTreePropertyChanged (juce::ValueTree& t, const juce::Identifier&) override { if (t.hasType (flatwater::presetExactTag)) readExactIdle(); }
    void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&) override { readExactIdle(); }
    void valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int) override { readExactIdle(); }
    void valueTreeRedirected (juce::ValueTree&) override { readExactIdle(); }
    std::atomic<float> exactIdle { -1.0f };
    double sr = 48000.0;
    vl::dsp::Ride ride;
    double outGain = 1.0, heldDb = 0.0, smooth5 = 0.0;          // the Output gain and a held Rider, each smoothed over 5 ms
    double musicLevel = 1.0e-10;
    bool wasHeld = false, returning = false, writing = false;
};
