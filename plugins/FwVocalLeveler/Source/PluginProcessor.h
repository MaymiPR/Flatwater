#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "shared/ui/Widgets.h"
#include "shared/Presets.h"
#include "dsp/Engine.h"
#include "Params.h"
#include "Presets.h"

class FwVocalLevelerProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    FwVocalLevelerProcessor();
    ~FwVocalLevelerProcessor() override { stopTimer(); }

    void prepareToPlay (double, int) override;
    void releaseResources() override {}
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
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override
    {
        if (index == currentProgram || index < 0 || index >= getNumPrograms()) return;
        currentProgram = index;
        static const flatwater::FactoryPreset defaults { "", "Default", {} };
        flatwater::applyPreset (apvts, index == 0 ? defaults : vl::factoryPresets()[(size_t) index - 1], apvts.getParameter (vl::id::mode));
        readExactIdle();
    }
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
    void readExactIdle();                                       // (after a preset: also called by the timer)

private:
    void timerCallback() override;                              // Automation Write: the ride to the host; the preset's Idle
    std::atomic<float> exactIdle { -1.0f };
    double sr = 48000.0;
    vl::dsp::Ride ride;
    double outGain = 1.0, heldDb = 0.0, smooth5 = 0.0;          // the Output gain and a held Rider, each smoothed over 5 ms
    double musicLevel = 1.0e-10;
    bool wasHeld = false, returning = false, writing = false;
    int currentProgram = 0;
};
