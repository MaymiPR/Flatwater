#pragma once
// What every Flatwater plug-in's unit tests check of AudioProcessor::reset(), which a host calls to clear the tails:
// after a loud signal and reset(), the processor gives what a freshly prepared one gives, sample for sample, first
// on silence (no tail, no old audio out of a delay line) and then on a second signal (no envelope, gain or filter
// state left over), and reset() itself allocates nothing. A test program calls resetChecks<Processor> with a setup
// function (the settings, the layout and prepareToPlay, the same for both processors) and its own check function.
// This header includes AllocCounter.h, which replaces the program's operator new: see there.
#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>
#include <vector>
#include "shared/test/AllocCounter.h"

namespace flatwater::test
{
// `seconds` of a signal through p in blocks: two tones at the given peak level (the right side 2 dB lower), or
// silence at level 0. Returns the main output, the channels one after the other.
inline std::vector<float> runSignal (juce::AudioProcessor& p, double fs, int block, double seconds, double f1, double f2, double level)
{
    const int channels = juce::jmax (p.getTotalNumInputChannels(), p.getTotalNumOutputChannels());
    const int outs = p.getMainBusNumOutputChannels(), total = (int) (seconds * fs);
    std::vector<float> out ((size_t) outs * (size_t) total, 0.0f);
    juce::AudioBuffer<float> buf (channels, block);
    juce::MidiBuffer midi;
    for (int at = 0; at < total; at += block)
    {
        const int n = juce::jmin (block, total - at);
        buf.clear();
        for (int c = 0; c < juce::jmin (2, p.getMainBusNumInputChannels()); ++c)
            for (int i = 0; i < n; ++i)
            {
                const double t = (double) (at + i) / fs, w = 2.0 * juce::MathConstants<double>::pi;
                buf.setSample (c, i, (float) (level * (c == 0 ? 1.0 : 0.8) * (0.6 * std::sin (w * f1 * t) + 0.4 * std::sin (w * f2 * t))));
            }
        juce::AudioBuffer<float> part (buf.getArrayOfWritePointers(), channels, n);
        p.processBlock (part, midi);
        for (int c = 0; c < outs; ++c)
            for (int i = 0; i < n; ++i) out[(size_t) c * (size_t) total + (size_t) (at + i)] = part.getSample (c, i);
    }
    return out;
}

template <typename Processor, typename Setup, typename Check>
void resetChecks (Setup&& setup, Check&& check, double fs = 48000.0, int block = 512)
{
    Processor used, fresh;
    setup (used);
    setup (fresh);
    runSignal (used, fs, block, 1.0, 110.0, 1000.0, 0.9);              // loud, long enough to fill every line
    int allocations = 0;
    {
        AllocScope scope;
        used.reset();
        allocations = scope.count();
    }
    check (allocations == 0, "reset() allocates nothing");
    const auto same = [] (const std::vector<float>& a, const std::vector<float>& b)
    {
        if (a.size() != b.size()) return false;
        for (size_t i = 0; i < a.size(); ++i)
            if (! juce::exactlyEqual (a[i], b[i])) return false;
        return true;
    };
    check (same (runSignal (used, fs, block, 0.5, 0.0, 0.0, 0.0), runSignal (fresh, fs, block, 0.5, 0.0, 0.0, 0.0)),
           "after reset(), silence gives what a fresh processor gives (no tail)");
    check (same (runSignal (used, fs, block, 0.5, 220.0, 3000.0, 0.25), runSignal (fresh, fs, block, 0.5, 220.0, 3000.0, 0.25)),
           "after reset(), a signal gives what a fresh processor gives (no state left)");
}
}
