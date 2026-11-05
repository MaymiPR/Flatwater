#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "FwMeter.h"
#include "FwCompPresets.h"
#include "FwCompDsp.h"
#include "FwCompAnalog.h"

// Parameters follow the reference's parameter list (its order, ranges, tapers and defaults; see RESEARCH.md,
// Flatwater Comp); the DSP is in FwCompDsp.h.
namespace fwcomp
{
inline constexpr const char* threshold = "threshold";  // 0 .. -48 dB, default -18
inline constexpr const char* ratio = "ratio";          // 1 .. 50 (the reference's ratio taper), default 3
inline constexpr const char* punch = "punch";          // 0 .. 30, default 1
inline constexpr const char* mix = "mix";              // 0 (dry) .. 100 (wet), default 100
inline constexpr const char* output = "output";        // -18 .. +18 dB, default 0
inline constexpr const char* attack = "attack";        // 0.5 .. 100 ms, logarithmic, default 7
inline constexpr const char* release = "release";      // 3 .. 3000 ms, logarithmic, default 100
inline constexpr const char* releaseNote = "releaseNote"; // 20 note values (1/64T .. 2 bars), default 1/32D
inline constexpr const char* sync = "sync";            // BPM / Host / ms, default ms
inline constexpr const char* bpm = "bpm";              // 30 .. 300, default 120 (used in BPM sync)
inline constexpr const char* analog = "analog";        // Off / 1 .. 4, default 2
inline constexpr const char* limiter = "limiter";      // Off (clipper) / On (limiter), default Off
inline constexpr const char* meter = "meter";          // In / GR / Out, default GR
inline constexpr const char* mode = "mode";            // 0 Stereo, 1 Mono

// the reference's ratio taper (measured, RESEARCH.md): ln(ratio) = 4.40703 v - 0.494827 v^2
float ratioFromNorm (float v);
float ratioToNorm (float r);
// "0.50 ms", "12.0 ms", "100 ms": three significant digits, as the reference shows times
juce::String timeText (float ms);
// the Release (ms) a Release Note gives at a tempo (0: none, 120 BPM), with the reference's fallback for long notes
float syncedReleaseMs (int note, double tempo);
}

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
    int getNumPrograms() override { return 1 + (int) fwcomp::factoryPresets().size(); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override
    {
        if (index == currentProgram || index < 0 || index >= getNumPrograms()) return;
        currentProgram = index;
        static const fw::FactoryPreset defaults { "", "Default", {} };
        fw::applyPreset (apvts, index == 0 ? defaults : fwcomp::factoryPresets()[(size_t) index - 1], apvts.getParameter (fwcomp::mode),
                           { apvts.getParameter (fwcomp::mix) });           // presets leave Mix alone, as the reference
    }
    const juce::String getProgramName (int index) override
    {
        if (index <= 0 || index >= getNumPrograms()) return "Default";
        const auto& p = fwcomp::factoryPresets()[(size_t) index - 1];
        return juce::String (p.group) + ": " + p.name;
    }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::UndoManager undo { 32, 32 };
    juce::AudioProcessorValueTreeState apvts;

    // meters (dB, highest peak since the editor last read them): input, output; the compressor's and the limiter's
    // gain reduction (positive dB)
    fw::PeakTap inLDb, inRDb, outLDb, outRDb, grDb, limDb;
    std::atomic<int> blocks { 0 };
    std::atomic<double> hostBpm { 0.0 };                  // 0: the host gives no tempo

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    float get (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    void configure();
    fwcomp::Compressor::Settings settings() const;
    int currentProgram = 0;
    fwcomp::Compressor comps[2];
    fwcomp::Analog analogStage;
    double sr = 44100.0;
    bool prepared = false, layoutDual = false;
};
