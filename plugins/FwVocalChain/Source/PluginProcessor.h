#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "shared/Presets.h"
#include "shared/dsp/BypassDelay.h"
#include "shared/ui/Widgets.h"
#include "dsp/Engine.h"
#include "ReverbConvolution.h"
#include "Params.h"
#include "Presets.h"

class FwVocalChainProcessor : public juce::AudioProcessor, public flatwater::HasPresets, private juce::Timer
{
public:
    FwVocalChainProcessor();
    ~FwVocalChainProcessor() override { stopTimer(); }

    // the In/Out link (Flatwater's own): while it is on, Output is the mirror of Sensitivity (Output = -Sensitivity) in
    // the audio at every block, whatever the Output parameter holds; syncLink() sets the Output parameter to that
    // mirror (a timer does it too, so automation, presets, setups and undo cannot leave the two apart)
    bool linked() const { return get (ch::id::link) > 0.5f; }
    void syncLink();

    void prepareToPlay (double, int) override;
    void releaseResources() override {}
    // the host clears the tails: the chain's lines and filters, Compress, the echoes, the doubler, the reverb's
    // convolutions and the bypass line as prepareToPlay leaves them; the Noise goes on from where it is. No allocation.
    void reset() override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;
    // the host's bypass: the input delayed by the reported latency (shared/dsp/BypassDelay.h); the chain goes on
    // running on the input, unheard, so the audio after the bypass is the audio of now
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlockBypassed;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Flatwater Vocal Chain"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 5.0; }
    int getNumPrograms() override { return 1 + (int) ch::factoryPresets().size(); }
    int getCurrentProgram() override { return presets.program(); }
    void setCurrentProgram (int index) override { presets.setProgram (index); }
    const juce::String getProgramName (int index) override
    {
        if (index <= 0 || index >= getNumPrograms()) return "Default";
        const auto& p = ch::factoryPresets()[(size_t) index - 1];
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

    // editor taps: the input and output peaks per channel (dB, the highest of the block) for the meters, and the
    // input's rms level after Sensitivity for the Sensitivity LED
    flatwater::PeakTap inL, inR, outL, outR;
    std::atomic<float> sensLevelDb { -100.0f };

    float get (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    void setNoise (bool on) { chain.noiseOn = on; }                   // (the reference's state-only Noise; off for null tests)
    int modeIndex() const { return juce::roundToInt (get (ch::id::mode)); }

    // The reverb's responses load on a background thread and prepareToPlay waits for them, so the first block has
    // the reverb. reverbLoaded: they are there. reverbLoadTimedOut: the last prepareToPlay gave up waiting, and the
    // reverb was silent from then until they were there. setReverbLoadTimeout: the longest wait (ms; 0: none),
    // before prepareToPlay.
    bool reverbLoaded() const noexcept { return reverb.loaded(); }
    bool reverbLoadTimedOut() const noexcept { return reverb.loadTimedOut(); }
    void setReverbLoadTimeout (int ms) { reverb.loadTimeoutMs = ms; }

private:
    flatwater::BypassDelay bypassLine;
    ch::dsp::Params chainParams();                                    // the chain's settings of this block
    void timerCallback() override { syncLink(); }
    double sr = 48000.0;
    ch::ReverbConvolution reverb;                                     // (before the chain, which uses it)
    ch::dsp::Chain chain;
    juce::AudioBuffer<float> work;
    int maxBlock = 0;                                                 // the chain's block size (prepareToPlay)
};
