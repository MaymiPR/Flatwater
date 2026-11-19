#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include <cstring>
#include <limits>
#include "shared/ui/Widgets.h"
#include "shared/Presets.h"
#include "Presets.h"
#include "dsp/Engine.h"
#include "plugins/FwComp/Source/dsp/Analog.h"
#include "dsp/AnalogTables.h"
#include "Params.h"

class FwDelayProcessor : public juce::AudioProcessor, public flatwater::HasPresets, private juce::ValueTree::Listener
{
public:
    FwDelayProcessor();
    ~FwDelayProcessor() override { apvts.state.removeListener (this); }

    void prepareToPlay (double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Flatwater Delay"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }
    int getNumPrograms() override { return 1 + (int) de::factoryPresets().size(); }
    int getCurrentProgram() override { return presets.program(); }
    void setCurrentProgram (int index) override { presets.setProgram (index); }
    const juce::String getProgramName (int index) override
    {
        if (index <= 0 || index >= getNumPrograms()) return "Default";
        const auto& p = de::factoryPresets()[(size_t) index - 1];
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

    // meters (dB, highest peak since the editor last read them): the output, left and right
    flatwater::PeakTap outLDb, outRDb;
    std::atomic<int> blocks { 0 };
    std::atomic<double> hostBpm { 0.0 };                  // 0: the host gives no tempo

    // the DSP's settings from the parameters (the times in samples at the tempo the notes follow)
    de::dsp::Settings settings (double sampleRate) const;

    // The user's change of Sync from one choice to another (0 BPM, 1 Host, 2 MS), as the reference: between ms and
    // note values the time is kept as near as the new unit allows. Only the editor's click on Sync calls it; a preset,
    // an undo, a setup, the host's automation and a reload bring their own values, which stay.
    void convertDelayForSync (int from, int to);

private:
    void configure();                // allocates: prepareToPlay
    void setLayout (bool dual);      // a Mode change: a reset without allocation, also on the audio thread
    de::dsp::Delay delays[2];                // one linked delay, or two mono ones (Mode Mono on a stereo bus)
    co::dsp::Analog analogStage;
    double sr = 48000.0;
    bool prepared = false, layoutDual = false;
    float get (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

    // A factory preset's values as stored (flatwater::presetExactTag in the state): the reference takes a preset's values
    // exactly (Depth 0.69, Rate 281.7, LoPass 20 139 as a filter), but a value set from outside at its resolution.
    // A parameter uses its preset value while it stays where the preset put it.
    struct Exact { const char* id; std::atomic<float> value { std::numeric_limits<float>::quiet_NaN() }, norm { -1.0f }; };
    std::array<Exact, 6> exact { { { de::id::depth }, { de::id::rate }, { de::id::lopass }, { de::id::hipass }, { de::id::bpm },
                                   { de::id::feedback } } };
    // the value the DSP takes: the preset's (exactFromPreset true) or the parameter's at its resolution
    float value (const char* id, bool* exactFromPreset = nullptr) const;
    void readExact();
    void valueTreePropertyChanged (juce::ValueTree& t, const juce::Identifier&) override { if (t.hasType (flatwater::presetExactTag)) readExact(); }
    void valueTreeChildAdded (juce::ValueTree&, juce::ValueTree&) override { readExact(); }
    void valueTreeChildRemoved (juce::ValueTree&, juce::ValueTree&, int) override { readExact(); }
    void valueTreeRedirected (juce::ValueTree&) override { readExact(); }
};
