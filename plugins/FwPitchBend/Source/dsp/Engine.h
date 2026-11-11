#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <vector>
#include "Model.h"
#include "Mod.h"
#include "Shifter.h"
#include "Tuning.h"

// Flatwater Pitch Bend's audio path: the modulators (dsp/Mod.h) put on the controls through the slots, then the pitch
// and formant shifter (dsp/Shifter) on the wet path and Mix, per sample, with its modulation. The processor hands it
// the controls' values once per block (Params), the slots and shapes (Setup) and the host's transport; the controls'
// ranges come from their host parameters (setTargets), as the modulation acts on their normalised values. No latency.
namespace pb
{
class Engine
{
public:
    // the settings that are not host parameters: the slots, the two shape tables, Fine
    struct Setup
    {
        std::array<Slot, numTargets * numSlots> slots {};
        Table tables[2] {};
        bool fine = false;
    };
    // one modulator's own controls (real units; rateSynced as the parameter's normalised value)
    struct ModControls { float type = 0.0f, sync = 0.0f, steps = 16.0f, rate = 0.5f, warp = 1.0f, phase = 0.0f, level = 1.0f, smooth = 1.5f, rateSynced = 0.0f; };
    // a block's controls: the modulatable ones' normalised values (targets[] order), the modulators', PT Offset,
    // Flatten and its note, and the host's transport (havePpq false: counted from the start of processing)
    struct Params
    {
        std::array<float, numTargets> norm {};
        ModControls mod[2];
        float ptOffset = 0.0f, flatten = 0.0f, flattenNote = 12.0f;
        double bpm = 120.0, ppq = 0.0;
        bool havePpq = false;
    };

    void setTargets (const std::array<juce::RangedAudioParameter*, numTargets>& p) { targets = p; }
    void prepare (double sampleRate, int maxBlock, int channels);
    // in place, `channels` channels of n samples
    void process (float* const* io, int channels, int n, const Params&, const Setup&);

    // what the editor shows: each modulatable control with its modulation (real units), the modulators' place in their
    // cycle, AM's and PT's values, and the last block's modulated values (normalised; Pitch to +-24 semitones)
    std::array<float, numTargets> modulated {};
    float cycle[2] {}, amValue = 0.0f, ptValue = 0.0f;
    std::array<float, numTargets> modNorm {};

    Tuning t = tuning();                                          // dsp/Tuning.h: the defaults, or the renderer's overrides

private:
    double modulatedValue (int target, const Params&) const;     // a control with last block's modulation, real units

    std::array<juce::RangedAudioParameter*, numTargets> targets {};
    double sr = 48000.0;
    long long processed = 0;                                     // samples since the start of processing
    std::array<double, numTargets> modSum {};                    // last block's sums of the slots (depth / 100 x source)
    std::array<double, numSources> lastValue {};                 // the sources' values at the last sample
    Lfo lfo[2];
    Follower am;
    PitchFollower pt;
    Shifter shifter;
    std::vector<std::vector<float>> dry;
    std::vector<float> mono;                                     // the input's mix, then the Mix per sample
    std::vector<float> pitchPer, formantPer;                     // the Pitch and Formant totals per sample (semitones)
};
}
