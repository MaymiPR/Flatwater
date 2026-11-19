#pragma once
// The host's bypass of a plug-in that reports a latency: the input, delayed by that latency, so a bypassed track
// stays in time with the others. write() takes the input of every processed block, so the line already holds the
// last samples when the bypass comes on; process() takes the input of a bypassed block and gives the delayed input
// in its place. Plain C++; prepare() allocates, nothing else does.
#include <algorithm>
#include <cstddef>
#include <vector>

namespace flatwater
{
class BypassDelay
{
public:
    // channels lines of delaySamples each, silent; maxDelaySamples: the longest delay setDelay() may be given later
    // (a plug-in whose latency changes while it runs)
    void prepare (int channels, int delaySamples, int maxDelaySamples = 0)
    {
        delay = std::max (0, delaySamples);
        lines.assign ((size_t) std::max (0, channels), std::vector<float> ((size_t) std::max ({ 1, delay, maxDelaySamples }), 0.0f));
        pos = 0;
    }

    // another delay, up to the longest one prepare() was given, the lines silent again
    void setDelay (int delaySamples) noexcept
    {
        delay = std::clamp (delaySamples, 0, lines.empty() ? 0 : (int) lines[0].size());
        reset();
    }

    // the lines silent, as after prepare()
    void reset() noexcept
    {
        for (auto& l : lines) std::fill (l.begin(), l.end(), 0.0f);
        pos = 0;
    }

    // the input of a block that is processed (not bypassed)
    void write (const float* const* in, int channels, int n) noexcept
    {
        if (delay == 0) return;
        const int ch = std::min (channels, (int) lines.size());
        for (int c = 0; c < ch; ++c)
        {
            float* line = lines[(size_t) c].data();
            int p = pos;
            for (int i = 0; i < n; ++i) { line[p] = in[c][i]; if (++p == delay) p = 0; }
        }
        pos = (pos + n) % delay;
    }

    // a bypassed block, in place: each sample becomes the input of `delay` samples before
    void process (float* const* io, int channels, int n) noexcept
    {
        if (delay == 0) return;
        const int ch = std::min (channels, (int) lines.size());
        for (int c = 0; c < ch; ++c)
        {
            float* line = lines[(size_t) c].data();
            int p = pos;
            for (int i = 0; i < n; ++i) { const float in = io[c][i]; io[c][i] = line[p]; line[p] = in; if (++p == delay) p = 0; }
        }
        pos = (pos + n) % delay;
    }

private:
    std::vector<std::vector<float>> lines;
    int delay = 0, pos = 0;
};
}
