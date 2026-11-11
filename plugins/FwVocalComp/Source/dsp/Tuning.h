#pragma once
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

// Every constant of the DSP (dsp/Engine.h), from the measurements and the fits. Kept in one struct so the offline
// renderer (tools/VcRender.cpp) can override any of them by name (tune:key=value, or its own short keys); the plug-in
// uses the defaults.
namespace vc
{
struct Tuning
{
    // fast path, in samples at 44.1 / 48 kHz (scaled by latency / 64 above)
    int hold = 65;
    int box1 = 39;
    int box2 = 26;
    // slow path (ms, dB): stage 1 rise / fall, dead band, stage 2 rise / fall
    int slowMode = 2;   // 0: two dB stages with dead band; 1: amplitude charge stage + dB/s peak decay;
    //                                 2: amplitude stage (ta1 / tr1), then the gain ratio through a one-pole (ta2 / tr2)
    float decayDbS = 20.0f;   // slowMode 1: fall rate of the held level (dB per second)
    float slowWinMs = 2.1754f;   // peak window in front of the slow path
    float ta1 = 1.5704f;
    float tr1 = 0.67043f;
    float band = 0.048f;
    float ta2 = 249.5f;
    float tr2 = 498.49f;
    // gate level release (ms): the reference's hidden GateRel parameter, 10 ... 250 ms, default 130
    float gateRelMs = 130.0f;
    float fixedGain = 0.99f;   // the reference's constant -0.0873 dB
    bool learned = true;   // the learned correction of the slow path (SlowNet); off = the analytical path only

    // sets a constant by name (an array element as the name and its index: onThr0); false for an unknown key
    bool set (const std::string& key, double v)
    {
        if (key == "hold") { hold = (int) v; return true; }
        if (key == "box1") { box1 = (int) v; return true; }
        if (key == "box2") { box2 = (int) v; return true; }
        if (key == "slowMode") { slowMode = (int) v; return true; }
        if (key == "decayDbS") { decayDbS = (float) v; return true; }
        if (key == "slowWinMs") { slowWinMs = (float) v; return true; }
        if (key == "ta1") { ta1 = (float) v; return true; }
        if (key == "tr1") { tr1 = (float) v; return true; }
        if (key == "band") { band = (float) v; return true; }
        if (key == "ta2") { ta2 = (float) v; return true; }
        if (key == "tr2") { tr2 = (float) v; return true; }
        if (key == "gateRelMs") { gateRelMs = (float) v; return true; }
        if (key == "fixedGain") { fixedGain = (float) v; return true; }
        if (key == "learned") { learned = v < 0.0 || v > 0.0; return true; }
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
