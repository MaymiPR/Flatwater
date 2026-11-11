#include "ReverbConvolution.h"
#include "FwVocalChainIR.h"
#include <thread>

using namespace juce;

namespace ch
{
namespace
{
// the reverb's responses for one rate (plugins/FwVocalChain/scripts/pack_reverb.py): per colour a length, then LL, LR, RL, RR
// interleaved as 16-bit floats
struct Packed { double fs; const char* data; int size; };

float half (uint16 h)
{
    const uint32 s = (uint32) (h & 0x8000u) << 16, e = (h >> 10) & 0x1fu, m = h & 0x3ffu;
    uint32 f;
    if (e == 0) { if (m == 0) f = s; else { int ee = -1; uint32 mm = m; do { ++ee; mm <<= 1; } while ((mm & 0x400u) == 0); f = s | ((uint32) (127 - 15 - ee) << 23) | ((mm & 0x3ffu) << 13); } }
    else if (e == 31) f = s | 0x7f800000u | (m << 13);
    else f = s | ((e + 112u) << 23) | (m << 13);
    float v;
    std::memcpy (&v, &f, 4);
    return v;
}

Packed nearestPacked (double fs)
{
    const Packed all[] = { { 44100.0, ch::ir::reverb_44100_bin, ch::ir::reverb_44100_binSize },
                           { 48000.0, ch::ir::reverb_48000_bin, ch::ir::reverb_48000_binSize },
                           { 88200.0, ch::ir::reverb_88200_bin, ch::ir::reverb_88200_binSize },
                           { 96000.0, ch::ir::reverb_96000_bin, ch::ir::reverb_96000_binSize } };
    const Packed* best = &all[0];
    for (const auto& p : all) if (std::abs (std::log (p.fs / fs)) < std::abs (std::log (best->fs / fs))) best = &p;
    return *best;
}
}

//==============================================================================
void ReverbConvolution::prepare (double fs, int block, int skip)
{
    const auto pk = nearestPacked (fs);
    const auto* p = reinterpret_cast<const uint8*> (pk.data);
    const juce::dsp::ProcessSpec spec { fs, (uint32) jmax (1, block), 2 };
    int want[3] {};
    for (int c = 0; c < 3; ++c)
    {
        uint32 n;
        std::memcpy (&n, p, 4); p += 4;
        const int m = (int) n - skip;
        AudioBuffer<float> direct (2, m), cross (2, m);                  // (LL, RR) and (RL, LR): the outputs from each input
        for (int i = 0; i < m; ++i)
        {
            uint16 v[4];
            std::memcpy (v, p + (size_t) (i + skip) * 8, 8);
            direct.setSample (0, i, half (v[0])); direct.setSample (1, i, half (v[3]));
            cross.setSample (0, i, half (v[2])); cross.setSample (1, i, half (v[1]));
        }
        p += (size_t) n * 8;
        want[c] = (int) (0.99 * m * fs / pk.fs);                       // (its length at this rate, less a margin)
        for (int k = 0; k < 2; ++k)
        {
            auto& e = conv[(size_t) c][(size_t) k];
            e = std::make_unique<juce::dsp::Convolution> (juce::dsp::Convolution::NonUniform { 256 });
            e->prepare (spec);
            e->loadImpulseResponse (k == 0 ? std::move (direct) : std::move (cross), pk.fs, juce::dsp::Convolution::Stereo::yes,
                                    juce::dsp::Convolution::Trim::no, juce::dsp::Convolution::Normalise::no);
        }
    }
    // the responses load on a background thread and are swapped in during processing (with a short crossfade): run
    // silence through every engine until each holds its whole response (a busy machine can take seconds), so the first
    // real block already has it
    a.setSize (2, jmax (1, block)); b.setSize (2, jmax (1, block));
    const auto t0 = Time::getMillisecondCounter();
    for (;;)
    {
        bool ready = true;
        for (int c = 0; c < 3; ++c)
            for (auto& e : conv[(size_t) c])
            {
                a.clear();
                juce::dsp::AudioBlock<float> blk (a);
                e->process (juce::dsp::ProcessContextReplacing<float> (blk));
                ready = ready && e->getCurrentIRSize() >= want[c];
            }
        if (ready || Time::getMillisecondCounter() - t0 > 20000) break;
        std::this_thread::sleep_for (std::chrono::milliseconds (2));
    }
    for (int i = 0; i < (int) (0.2 * fs) / jmax (1, block) + 2; ++i)    // (past the crossfade)
        for (auto& c : conv)
            for (auto& e : c) { a.clear(); juce::dsp::AudioBlock<float> blk (a); e->process (juce::dsp::ProcessContextReplacing<float> (blk)); }
    reset();
    running = 0;
}

void ReverbConvolution::process (const float* inL, const float* inR, float* outL, float* outR, int n, int colour, float gain)
{
    if (colour > 0 && colour != running) { running = colour; for (auto& e : conv[(size_t) running - 1]) e->reset(); }
    if (running == 0) { FloatVectorOperations::clear (outL, n); FloatVectorOperations::clear (outR, n); return; }
    a.setSize (2, n, false, false, true); b.setSize (2, n, false, false, true);
    a.copyFrom (0, 0, inL, n); a.copyFrom (1, 0, inR, n);
    b.copyFrom (0, 0, inR, n); b.copyFrom (1, 0, inL, n);
    {
        juce::dsp::AudioBlock<float> ba (a), bb (b);
        conv[(size_t) running - 1][0]->process (juce::dsp::ProcessContextReplacing<float> (ba));
        conv[(size_t) running - 1][1]->process (juce::dsp::ProcessContextReplacing<float> (bb));
    }
    const float g = colour > 0 ? gain : 0.0f;                          // (clear: muted, the engine still running)
    for (int i = 0; i < n; ++i)
    {
        outL[i] = g * (a.getSample (0, i) + b.getSample (0, i));
        outR[i] = g * (a.getSample (1, i) + b.getSample (1, i));
    }
}
}
