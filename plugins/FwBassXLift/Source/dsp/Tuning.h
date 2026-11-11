#pragma once
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

// Every constant of the DSP (dsp/Engine.h), from the Phase 4 identification of the reference (MEASUREMENTS.md, "Flatwater
// BassX Lift model"). Kept in one struct so the offline renderer (tools/BxRender.cpp) can override any of them by name
// (tune:key=value); the plug-in uses the defaults. K(F) per Freq setting and the learned level control stay in
// dsp/Net.h (generated).
namespace bx::dsp
{
struct Tuning
{
    double harmonicsCapDb = 10.958;   // the Harmonics fader's effect stops here (measured)
    double decayFeedback = 8.0;   // the Decay feedback: y = w (1 + this x D x HP_B(y))
    double decayHpRatio = 3.0;   // HP_B: Butterworth at Freq / this
    double decayHpMinHz = 20.0;   // held within these
    double decayHpMaxHz = 40.0;
    double generatedHpHz = 16.0;   // HP_G at 6 and 12 dB/oct: the fixed sections' frequency
    double generatedHp24Ratio = 0.8;   // HP_G at 24 dB/oct: two sections at this x Freq
    double featureScale = 1.0e-3;   // the level control's inputs: asinh (v / scale) / div
    double featureDiv = 8.0;
    double clipLevel = 1.0;   // the generated signal and the output are held within +-this

    // sets a constant by name (an array element as the name and its index: onThr0); false for an unknown key
    bool set (const std::string& key, double v)
    {
        if (key == "harmonicsCapDb") { harmonicsCapDb = v; return true; }
        if (key == "decayFeedback") { decayFeedback = v; return true; }
        if (key == "decayHpRatio") { decayHpRatio = v; return true; }
        if (key == "decayHpMinHz") { decayHpMinHz = v; return true; }
        if (key == "decayHpMaxHz") { decayHpMaxHz = v; return true; }
        if (key == "generatedHpHz") { generatedHpHz = v; return true; }
        if (key == "generatedHp24Ratio") { generatedHp24Ratio = v; return true; }
        if (key == "featureScale") { featureScale = v; return true; }
        if (key == "featureDiv") { featureDiv = v; return true; }
        if (key == "clipLevel") { clipLevel = v; return true; }
        return false;
    }

    // key=value lines, one per line (# starts a comment); returns 0 or the number of the first bad line
    int load (const char* path)
    {
        std::ifstream f (path);
        if (! f) { std::printf ("cannot read tuning file %s\n", path); return 1; }
        std::string s;
        int n = 0;
        while (std::getline (f, s))
        {
            ++n;
            while (! s.empty() && (s.back() == '\r' || s.back() == ' ')) s.pop_back();
            if (s.empty() || s[0] == '#') continue;
            const auto eq = s.find ('=');
            if (eq == std::string::npos || ! set (s.substr (0, eq), std::strtod (s.c_str() + eq + 1, nullptr)))
            {
                std::printf ("bad tuning line %d: %s\n", n, s.c_str());
                return n;
            }
        }
        return 0;
    }
};

// the tuning in use: the defaults in the plug-in, overridden by the offline renderer (tune:key=value, tuning=file)
inline Tuning& tuning()
{
    static Tuning t;
    return t;
}
}
