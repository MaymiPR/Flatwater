#pragma once
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include "Tables.h"

// Every constant of Flatwater Comp's compressor (dsp/Engine.h), from the measurements (MEASUREMENTS.md "Flatwater Comp";
// the fitted ones written by scripts/export_tables.py into dsp/Tables.h, whose values are the defaults here). Kept in
// one struct so the offline renderer (tools/CoRender.cpp) can override any of them by name (tune:key=value); the
// plug-in uses the defaults. The curves and their knots stay in Tables.h (generated), the Analog stage's too.
// Sizes in samples are for 44.1 / 48 kHz and scale by 2 / 4 at 88.2-96 / 176.4-192 kHz.
namespace co
{
struct Tuning
{
    // the fitted release (Tables.h)
    float floorLevel = tables::floorLevel;   // the detector's floor: 20 log10 (peak + this)
    float crestMs = tables::crestMs;   // the slow state's charge (the Crest time constant)
    float startupMs = tables::startupMs;   // the start-up's slow release (0: none)
    float relSeedMs = tables::relSeedMs;   // the release after short events: relSeedMs / (R - S)^relSeedExp
    float relSeedExp = tables::relSeedExp;
    float chargeBlend = tables::chargeBlend;   // the slow state charges toward R by this share of R - output
    float memoryFloorMs = tables::memoryFloorMs;   // the memory's release time is at least this
    float crest4Ms = tables::crest4Ms;   // the fourth form (declined, off): its averages and their start
    float fast4Ms = tables::fast4Ms;
    float start4Db = tables::start4Db;
    float start4FastDb = tables::start4FastDb;
    // the structure (measured)
    int lookAhead = 64;   // the detector's hold and the latency (samples)
    int box1 = 38;   // the two box filters of the gain (samples); the limiter's the same
    int box2 = 27;
    int limiterHold = 7;   // Limiter On holds the output limiter this much longer (samples)
    double punchMaxMs = 30.0;   // Punch's longest delay of the detector
    double attackScale = 0.99;   // the attack one-pole's time: this x Attack
    double attackMinMs = 0.05;
    double releaseMinS = 1e-3;
    // the learned correction (GainNet)
    bool learned = true;   // on; off: the analytical model alone
    int netStep = 16;   // a step every this many samples
    double guardFlatDb = 0.05;   // no correction while the target stayed within this
    double guardRampDb = 0.15;   // full past it by this
    int limEstimate = 3;   // how the limiter estimates the output (3: the current gain times the pending peak; 0-2 tried, worse)

    // sets a constant by name (an array element as the name and its index: onThr0); false for an unknown key
    bool set (const std::string& key, double v)
    {
        if (key == "floorLevel") { floorLevel = (float) v; return true; }
        if (key == "crestMs") { crestMs = (float) v; return true; }
        if (key == "startupMs") { startupMs = (float) v; return true; }
        if (key == "relSeedMs") { relSeedMs = (float) v; return true; }
        if (key == "relSeedExp") { relSeedExp = (float) v; return true; }
        if (key == "chargeBlend") { chargeBlend = (float) v; return true; }
        if (key == "memoryFloorMs") { memoryFloorMs = (float) v; return true; }
        if (key == "crest4Ms") { crest4Ms = (float) v; return true; }
        if (key == "fast4Ms") { fast4Ms = (float) v; return true; }
        if (key == "start4Db") { start4Db = (float) v; return true; }
        if (key == "start4FastDb") { start4FastDb = (float) v; return true; }
        if (key == "lookAhead") { lookAhead = (int) v; return true; }
        if (key == "box1") { box1 = (int) v; return true; }
        if (key == "box2") { box2 = (int) v; return true; }
        if (key == "limiterHold") { limiterHold = (int) v; return true; }
        if (key == "punchMaxMs") { punchMaxMs = v; return true; }
        if (key == "attackScale") { attackScale = v; return true; }
        if (key == "attackMinMs") { attackMinMs = v; return true; }
        if (key == "releaseMinS") { releaseMinS = v; return true; }
        if (key == "learned") { learned = v != 0.0; return true; }
        if (key == "netStep") { netStep = (int) v; return true; }
        if (key == "guardFlatDb") { guardFlatDb = v; return true; }
        if (key == "guardRampDb") { guardRampDb = v; return true; }
        if (key == "limEstimate") { limEstimate = (int) v; return true; }
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
