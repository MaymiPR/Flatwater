#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "dsp/Model.h"
#include "dsp/Mod.h"
#include "dsp/Shifter.h"
#include "Presets.h"
#include "Params.h"

class FwPitchBendProcessor : public juce::AudioProcessor, private juce::ValueTree::Listener
{
public:
    FwPitchBendProcessor();
    ~FwPitchBendProcessor() override { apvts.state.removeListener (this); }

    void prepareToPlay (double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Flatwater Pitch Bend"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1 + (int) pb::factoryPresets().size(); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override
    {
        if (index <= 0 || index >= getNumPrograms()) return "Default";
        const auto& p = pb::factoryPresets()[(size_t) index - 1];
        return juce::String (p.group) + ": " + p.name;
    }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::UndoManager undo { 32, 32 };
    juce::AudioProcessorValueTreeState apvts;

    float get (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    bool fine() const { return (bool) apvts.state.getProperty (pb::fineId, false); }
    // Pitch / Formant in semitones as they act: whole semitones while Fine is off, cents (0.01) while it is on
    float shiftSemitones (const char* id) const
    {
        const float v = get (id);
        return fine() ? std::round (v * 100.0f) / 100.0f : std::round (v);
    }

    // editor taps: each modulatable control's value with its modulation (real units; the white dot), the two
    // modulators' place in their cycle (0 .. 1, the graph's play-head), AM's envelope and PT's pitch value (-1 .. 1)
    std::array<std::atomic<float>, pb::numTargets> modulated;
    std::atomic<float> cycle[2] { 0.0f, 0.0f }, amValue { 0.0f }, ptValue { 0.0f };

    // what Default Preset resets beyond the parameters: no slots, Fine off, the default shapes, the steps at 0
    void resetExtras() { applyPresetExtras (juce::ValueTree { "Defaults" }); }

    // the last block's Pitch with its modulation, in semitones as the shifter takes it (to +-24, past the knob's range)
    float pitchShifted() const { return 24.0f * modNorm[0] - 12.0f; }

private:
    void ensureState();                                          // the state tree's parts, with their defaults
    juce::AudioProcessorValueTreeState::ParameterLayout layout();
    void refreshModel();                                         // the tree's settings -> the audio thread's copy
    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override { refreshModel(); }
    void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree& child) override;
    void applyPresetExtras (const juce::ValueTree& exact);      // a factory preset's slots, Fine, shapes and steps
    int currentProgram = 0;
    void valueTreeRedirected (juce::ValueTree&) override { refreshModel(); }

    // the copy the audio thread works on: written on the message thread under the lock, read with a try-lock
    juce::SpinLock modelLock;
    struct Model
    {
        std::array<pb::Slot, pb::numTargets * pb::numSlots> slots {};
        pb::Table tables[2] {};
        bool fine = false;
    } shared, audio;

    double sr = 48000.0;
    long long processed = 0;                                     // samples since the start of processing
    std::array<juce::RangedAudioParameter*, pb::numTargets> targetParams {};
    std::array<float, pb::numTargets> modNorm {};       // last block's modulated values (normalised)
    std::array<double, pb::numTargets> modSum {};       // last block's sums of the slots (depth / 100 x source)
    std::array<double, pb::numSources> lastValue {};    // the sources' values at the last sample
    double modulatedValue (int t) const;                         // a control with last block's modulation, real units
    pb::Lfo lfo[2];
    pb::Follower am;
    pb::PitchFollower pt;
    pb::Shifter shifter;
    juce::AudioBuffer<float> dry;
    std::vector<float> mono;                                     // the input's mix, then the Mix per sample
    std::vector<float> pitchPer, formantPer;                     // the Pitch and Formant totals per sample (semitones)
};
