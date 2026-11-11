#pragma once
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

// Every constant of Flatwater Bass Leveler's riding (dsp/Engine.h), from the measurements (MEASUREMENTS.md "Flatwater
// Bass Leveler: Phase 3 / 4") and the note detector's fit (nulltest/fit_detector.py, measurements/FwBassLeveler/
// detector_fit.json). Kept in one struct so the offline renderer (tools/BlRender.cpp) can override any of them by
// name (tune:key=value); the plug-in uses the defaults. The quadrature pairs per rate stay in Engine.h (fitted tables).
namespace bl
{
struct Tuning
{
    // the level read: the weighting (two first-order low shelves, the hidden HPF), its follower, the law's offset
    double shelfHz = 30.0;   // the weighting shelf's frequency
    double shelfDb = -30.0;   // and its depth (two sections of half)
    double levelAttMs = 2.0;   // the level follower: up
    double levelRelMs = 100.0;   // and down
    double lawOffsetDb = 0.68;   // gain = Target - level + this
    // the detector's bands and followers
    double noteLpHz = 1020.0;   // the note level's low-pass
    double noteLpQ = 0.60;
    double illegalHpHz = 2000.0;   // the illegal-note band's high-pass
    double illegalHpQ = 0.7071;
    double illegalAttMs = 1.0;   // its follower: up
    double illegalRelMs = 1.0;   // and down
    double refAttMs = 1.0;   // the long reference R: its rise
    double outSmoothMs = 5.0;   // the ride's output smoothing
    double idleGlideDbS = 32.0;   // the glide to the Idle value (dB per second)
    double cycleShare = 0.0631;   // a cycle counts when both halves reach this share of the note's power (-12 dB)
    double minPeriodS = 0.0004;   // the shortest period counted (s)
    double noteSpanDb = 38.0;   // a note ends this far under its peak; a new one starts within it
    double floorHystDb = 3.0;   // the floor's margin for the end of a note
    double artBaseDb = 20.0;   // the Artifacts gate: (base - slope x Artifacts) dB under R and the Spill level
    double artSlopeDb = 0.17;
    double studioSpillDb = -40.0;   // Spill in the Studio components (its default)
    double silenceDb = -77.7;   // before any note: the level read under which the gain glides to Idle
    double reRideSensitivity = 37.5;   // Response Fast re-rides from this Sensitivity up
    // the note detector (fitted)
    double slowAtt = 9.8499;   // the slow envelope S: rise (ms)
    double slowRel = 1351.1864;   // and fall (ms, amplitude)
    double refRel = 5677.9174;   // the long reference R: fall (ms, amplitude)
    double hyst = 0.0043;   // the onset threshold's hysteresis (dB)
    double relMs = 1.3193;   // a release (or a re-ride) holds this long
    double holdMs = 53.7331;   // the attack is kept this long under F
    double followAtt = 10.4011;   // the follower F: rise (ms)
    double followRel = 345.0301;   // and fall (ms)
    double bandHi = 172.0;   // the window by pitch band: 1024 samples over this (Hz)
    double bandLo = 78.0;   // 2048 over this, 4096 under
    double reRideLo = 36.7818;   // the re-ride band (Hz)
    double reRideHi = 49.2817;
    double reRideDepth = 4.4;   // a held note this far under S is ridden afresh (dB)
    double offMs = 26.5947;   // the end of a note: this long under its span
    double offRelMs = 6.6073;   // the same for a released note
    double winMs = 1.1550;   // after a note: this long any legal note within the span starts one
    double floorDb = -48.75;   // the detector's floor
    // at Sensitivity 0 .. 100 by 25: the rise over S that is a new note, the margin over F that keeps the attack, a
    // released note's rise that is a new note, and the release depth under S (Slow, Fast)
    double onThr[5] { 1.5826, 0.3252, 1.3870, 3.2738, -0.8331 };
    double margin[5] { 1.2801, 1.7792, 0.3778, 0.0042, -0.9979 };
    double onRise[5] { 2.0745, 2.4965, 1.8808, 0.9627, 1.1710 };
    double depthSlow[5] { 12.0000, 12.0000, 12.0000, 12.0000, 12.0000 };
    double depthFast[5] { 4.0000, 4.0000, 4.0000, 4.0000, 4.0000 };

    // sets a constant by name (an array element as the name and its index: onThr0); false for an unknown key
    bool set (const std::string& key, double v)
    {
        if (key == "shelfHz") { shelfHz = v; return true; }
        if (key == "shelfDb") { shelfDb = v; return true; }
        if (key == "levelAttMs") { levelAttMs = v; return true; }
        if (key == "levelRelMs") { levelRelMs = v; return true; }
        if (key == "lawOffsetDb") { lawOffsetDb = v; return true; }
        if (key == "noteLpHz") { noteLpHz = v; return true; }
        if (key == "noteLpQ") { noteLpQ = v; return true; }
        if (key == "illegalHpHz") { illegalHpHz = v; return true; }
        if (key == "illegalHpQ") { illegalHpQ = v; return true; }
        if (key == "illegalAttMs") { illegalAttMs = v; return true; }
        if (key == "illegalRelMs") { illegalRelMs = v; return true; }
        if (key == "refAttMs") { refAttMs = v; return true; }
        if (key == "outSmoothMs") { outSmoothMs = v; return true; }
        if (key == "idleGlideDbS") { idleGlideDbS = v; return true; }
        if (key == "cycleShare") { cycleShare = v; return true; }
        if (key == "minPeriodS") { minPeriodS = v; return true; }
        if (key == "noteSpanDb") { noteSpanDb = v; return true; }
        if (key == "floorHystDb") { floorHystDb = v; return true; }
        if (key == "artBaseDb") { artBaseDb = v; return true; }
        if (key == "artSlopeDb") { artSlopeDb = v; return true; }
        if (key == "studioSpillDb") { studioSpillDb = v; return true; }
        if (key == "silenceDb") { silenceDb = v; return true; }
        if (key == "reRideSensitivity") { reRideSensitivity = v; return true; }
        if (key == "slowAtt") { slowAtt = v; return true; }
        if (key == "slowRel") { slowRel = v; return true; }
        if (key == "refRel") { refRel = v; return true; }
        if (key == "hyst") { hyst = v; return true; }
        if (key == "relMs") { relMs = v; return true; }
        if (key == "holdMs") { holdMs = v; return true; }
        if (key == "followAtt") { followAtt = v; return true; }
        if (key == "followRel") { followRel = v; return true; }
        if (key == "bandHi") { bandHi = v; return true; }
        if (key == "bandLo") { bandLo = v; return true; }
        if (key == "reRideLo") { reRideLo = v; return true; }
        if (key == "reRideHi") { reRideHi = v; return true; }
        if (key == "reRideDepth") { reRideDepth = v; return true; }
        if (key == "offMs") { offMs = v; return true; }
        if (key == "offRelMs") { offRelMs = v; return true; }
        if (key == "winMs") { winMs = v; return true; }
        if (key == "floorDb") { floorDb = v; return true; }
        if (key == "onThr0") { onThr[0] = v; return true; }
        if (key == "onThr1") { onThr[1] = v; return true; }
        if (key == "onThr2") { onThr[2] = v; return true; }
        if (key == "onThr3") { onThr[3] = v; return true; }
        if (key == "onThr4") { onThr[4] = v; return true; }
        if (key == "margin0") { margin[0] = v; return true; }
        if (key == "margin1") { margin[1] = v; return true; }
        if (key == "margin2") { margin[2] = v; return true; }
        if (key == "margin3") { margin[3] = v; return true; }
        if (key == "margin4") { margin[4] = v; return true; }
        if (key == "onRise0") { onRise[0] = v; return true; }
        if (key == "onRise1") { onRise[1] = v; return true; }
        if (key == "onRise2") { onRise[2] = v; return true; }
        if (key == "onRise3") { onRise[3] = v; return true; }
        if (key == "onRise4") { onRise[4] = v; return true; }
        if (key == "depthSlow0") { depthSlow[0] = v; return true; }
        if (key == "depthSlow1") { depthSlow[1] = v; return true; }
        if (key == "depthSlow2") { depthSlow[2] = v; return true; }
        if (key == "depthSlow3") { depthSlow[3] = v; return true; }
        if (key == "depthSlow4") { depthSlow[4] = v; return true; }
        if (key == "depthFast0") { depthFast[0] = v; return true; }
        if (key == "depthFast1") { depthFast[1] = v; return true; }
        if (key == "depthFast2") { depthFast[2] = v; return true; }
        if (key == "depthFast3") { depthFast[3] = v; return true; }
        if (key == "depthFast4") { depthFast[4] = v; return true; }
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
