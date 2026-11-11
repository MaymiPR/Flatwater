#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <memory>
#include "dsp/Engine.h"

// The reverb's convolution engine (dsp/Engine.h Reverb): per colour, two stereo zero-latency convolutions (L->L with
// R->R, and R->L with L->R on the swapped input) with the reference's measured responses at the nearest rate
// (resources/ir, packed by plugins/FwVocalChain/scripts/pack_reverb.py), run by JUCE's partitioned convolution; the
// last colour keeps running while Reverb is clear. The processor hands it to the chain.
namespace ch
{
class ReverbConvolution : public dsp::Reverb
{
public:
    // skip: samples dropped from the responses' (silent) start, the reverb's de-esser's delay
    void prepare (double fs, int block, int skip) override;
    void process (const float* inL, const float* inR, float* outL, float* outR, int n, int colour, float gain) override;
    void reset() override { for (auto& c : conv) for (auto& e : c) if (e) e->reset(); }

private:
    std::array<std::array<std::unique_ptr<juce::dsp::Convolution>, 2>, 3> conv;
    juce::AudioBuffer<float> a, b;
    int running = 0;                                                  // the colour whose engine runs (0: none yet)
};
}
