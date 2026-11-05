#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "FwPitchBendModel.h"
#include "FwPitchBendMod.h"
#include "FwPitchBendShifter.h"
#include "FwPitchBendPresets.h"

// Parameters follow the reference's host parameters (their names, ranges, defaults and display texts as ParamDump
// reads them; RESEARCH.md, Flatwater Pitch Bend). The tapers are the exponential ones the texts show (Rate 0.06 ..
// 30 Hz, Smooth 1 .. 1000, Warp 0.01 .. 100, the AM times 0.1 .. 1000 ms). No latency (as the reference's).
// The settings that are not host parameters (the modulation slots, Fine, the shape tables) are in the state tree
// (FwPitchBendModel.h); the audio thread works on a copy of them, refreshed whenever the tree changes. Per host block:
// the modulators (FwPitchBendMod.h) move the controls, the shifter (FwPitchBendShifter.h) makes the wet signal, and
// Mix crossfades the dry input and the wet one (dry + m (wet - dry), as the reference's).
namespace fwpitchbend
{
// the two LFO / step modulators share a layout: their IDs are "m1..." and "m2..."
struct ModIds { const char* warp; const char* level; const char* rateSynced; const char* rate; const char* sync;
                const char* phase; const char* type; const char* smooth; const char* steps; };
inline constexpr ModIds mod[2] {
    { "m1Warp", "m1Level", "m1RateSynced", "m1Rate", "m1Sync", "m1Phase", "m1Type", "m1Smooth", "m1Steps" },
    { "m2Warp", "m2Level", "m2RateSynced", "m2Rate", "m2Sync", "m2Phase", "m2Type", "m2Smooth", "m2Steps" } };
inline constexpr const char* amAttack = "amAttack";        // 0.1 .. 1000 ms, default 10
inline constexpr const char* amRelease = "amRelease";      // 0.1 .. 1000 ms, default 100
inline constexpr const char* amLevel = "amLevel";          // 0 .. 2, default 1
inline constexpr const char* ptSmooth = "ptSmooth";        // 1 .. 1000, default 10
inline constexpr const char* ptOffset = "ptOffset";        // -1 .. 1, default 0
inline constexpr const char* ptLevel = "ptLevel";          // 0 .. 2, default 1
inline constexpr const char* pitch = "pitch";              // -12 .. +12 semitones, default 0
inline constexpr const char* formant = "formant";          // -12 .. +12 semitones, default 0
inline constexpr const char* mix = "mix";                  // 0 .. 100 %, default 100
inline constexpr const char* flattenNote = "flattenNote";  // C2 .. B3, default C3
inline constexpr const char* flatten = "flatten";          // Off / On
inline constexpr const char* mode = "mode";                // Stereo, Mono (the two components)

inline const juce::StringArray& modeNames() { static const juce::StringArray m { "Stereo", "Mono" }; return m; }
// the synced rates, slowest first (the reference's list)
inline const juce::StringArray& syncedRates()
{
    static const juce::StringArray r { "32B", "16B", "8B", "4B", "2B", "1B", "1/2D", "1/2", "1/4D", "1/2T", "1/4", "1/8D",
                                       "1/4T", "1/8", "1/16D", "1/8T", "1/16", "1/32D", "1/16T", "1/32", "1/64D", "1/32T", "1/64", "1/64T" };
    return r;
}
inline const juce::StringArray& noteNames()
{
    static const juce::StringArray n = []
    {
        juce::StringArray s;
        const char* names[] { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        for (int octave = 2; octave <= 3; ++octave)
            for (auto* nm : names) s.add (juce::String (nm) + juce::String (octave));
        return s;
    }();
    return n;
}
}

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
    int getNumPrograms() override { return 1 + (int) fwpitchbend::factoryPresets().size(); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override
    {
        if (index <= 0 || index >= getNumPrograms()) return "Default";
        const auto& p = fwpitchbend::factoryPresets()[(size_t) index - 1];
        return juce::String (p.group) + ": " + p.name;
    }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::UndoManager undo { 32, 32 };
    juce::AudioProcessorValueTreeState apvts;

    float get (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    bool fine() const { return (bool) apvts.state.getProperty (fwpitchbend::fineId, false); }
    // Pitch / Formant in semitones as they act: whole semitones while Fine is off, cents (0.01) while it is on
    float shiftSemitones (const char* id) const
    {
        const float v = get (id);
        return fine() ? std::round (v * 100.0f) / 100.0f : std::round (v);
    }

    // editor taps: each modulatable control's value with its modulation (real units; the white dot), the two
    // modulators' place in their cycle (0 .. 1, the graph's play-head), AM's envelope and PT's pitch value (-1 .. 1)
    std::array<std::atomic<float>, fwpitchbend::numTargets> modulated;
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
        std::array<fwpitchbend::Slot, fwpitchbend::numTargets * fwpitchbend::numSlots> slots {};
        fwpitchbend::Table tables[2] {};
        bool fine = false;
    } shared, audio;

    double sr = 48000.0;
    long long processed = 0;                                     // samples since the start of processing
    std::array<juce::RangedAudioParameter*, fwpitchbend::numTargets> targetParams {};
    std::array<float, fwpitchbend::numTargets> modNorm {};       // last block's modulated values (normalised)
    std::array<double, fwpitchbend::numTargets> modSum {};       // last block's sums of the slots (depth / 100 x source)
    std::array<double, fwpitchbend::numSources> lastValue {};    // the sources' values at the last sample
    double modulatedValue (int t) const;                         // a control with last block's modulation, real units
    fwpitchbend::Lfo lfo[2];
    fwpitchbend::Follower am;
    fwpitchbend::PitchFollower pt;
    fwpitchbend::Shifter shifter;
    juce::AudioBuffer<float> dry;
    std::vector<float> mono;                                     // the input's mix, then the Mix per sample
    std::vector<float> pitchPer, formantPer;                     // the Pitch and Formant totals per sample (semitones)
};
