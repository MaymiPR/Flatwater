#pragma once
// The host's bypass of a plug-in that reports a latency: the input, delayed by that latency, so a bypassed track
// stays in time with the others. write() takes the input of every processed block, so the line already holds the
// last samples when the bypass comes on; process() takes the input of a bypassed block and gives the delayed input
// in its place. Plain C++; prepare() allocates, the other two do not.
#include <algorithm>
#include <cstddef>
#include <vector>

namespace flatwater
{
class BypassDelay
{
public:
    // channels lines of delaySamples each, silent
    void prepare (int channels, int delaySamples)
    {
        delay = std::max (0, delaySamples);
        lines.assign ((size_t) std::max (0, channels), std::vector<float> ((size_t) std::max (1, delay), 0.0f));
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
