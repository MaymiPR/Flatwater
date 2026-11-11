#pragma once
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

// Every constant of Flatwater Delay's delay path (dsp/Engine.h), as identified on the reference (MEASUREMENTS.md). Kept in
// one struct so the offline renderer (tools/DeRender.cpp) can override any of them by name (tune:key=value, or its
// own glide= and modscale=); the plug-in uses the defaults. The Analog stage's modes stay in dsp/AnalogTables.h
// (generated).
namespace de::dsp
{
struct Tuning
{
    // fitted on the reference
    double glideOffset = 16.0;   // the glide's pace: (T - d) / (T + 16) per sample
    double modScale = 0.5;   // the target's swing at Depth 100: +-50 %
    // measured
    double releaseMs = 10.0;   // the gain stage's recovery (per-sample 0.997919 at 48 kHz)
    double minDelay = 16.0;   // the shortest delay (samples)
    double maxSeconds = 5.6;   // the line's length
    double kaiserBeta = 4.968;   // the interpolator's window
    double lofiCutoffHz = 6277.0;   // LoFi: the interpolator cut off here
    double hipassQ = 0.8;   // HiPass and LoPass: RBJ biquads, their frequency warped
    double lopassQ = 0.95;

    // sets a constant by name (an array element as the name and its index: onThr0); false for an unknown key
    bool set (const std::string& key, double v)
    {
        if (key == "glideOffset") { glideOffset = v; return true; }
        if (key == "modScale") { modScale = v; return true; }
        if (key == "releaseMs") { releaseMs = v; return true; }
        if (key == "minDelay") { minDelay = v; return true; }
        if (key == "maxSeconds") { maxSeconds = v; return true; }
        if (key == "kaiserBeta") { kaiserBeta = v; return true; }
        if (key == "lofiCutoffHz") { lofiCutoffHz = v; return true; }
        if (key == "hipassQ") { hipassQ = v; return true; }
        if (key == "lopassQ") { lopassQ = v; return true; }
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
