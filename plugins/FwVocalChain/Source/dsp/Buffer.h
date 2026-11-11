#pragma once
#include <vector>

// Two channels of samples that the chain's stages hand each other, as long as the longest block asked for (their
// contents are always written before they are read).
namespace ch::dsp
{
struct StereoBuffer
{
    void setSize (int n) { for (auto& c : ch) c.resize ((size_t) n, 0.0f); }
    int size() const { return (int) ch[0].size(); }
    float* write (int c) { return ch[c].data(); }
    const float* read (int c) const { return ch[c].data(); }
    float sample (int c, int i) const { return ch[c][(size_t) i]; }
    void set (int c, int i, float v) { ch[c][(size_t) i] = v; }
    std::vector<float> ch[2];
};
}
