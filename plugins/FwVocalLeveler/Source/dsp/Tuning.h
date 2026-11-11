#pragma once
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include "Fit.h"

// Every constant of Flatwater Vocal Leveler's riding (dsp/Engine.h): the reference's hidden values (dsp/Fit.h, written by
// scripts/export_fit.py) and the measured weighting (MEASUREMENTS.md "Flatwater Vocal Leveler: Phase 3 / 4"). Kept in one
// struct so the offline renderer (tools/VlRender.cpp) can override any of them by name (tune:key=value); the plug-in
// uses the defaults. Times in ms, levels in dB.
namespace vl
{
struct Tuning
{
    // the weighting: two shelves, each two identical first-order sections with half the gain
    double lowShelfHz = 100.0;
    double lowShelfDb = -18.0;
    double highShelfHz = 6500.0;
    double highShelfDb = -6.0;
    double spillOffDb = -47.95;   // Spill at or under this is off
    // the hidden values (Fit.h)
    double c = fit::c;
    double bias = fit::bias;   // the follower's floor
    double followerAttackSlow = fit::followerAttackSlow;
    double followerAttackFast = fit::followerAttackFast;
    double followerRelease = fit::followerRelease;
    double followerHold = fit::followerHold;
    double followerPower = fit::followerPower;
    double vadAttack = fit::vadAttack;
    double vadRelease = fit::vadRelease;
    double vk = fit::vk;   // the detector's threshold under Q, less Vocal Sensitivity
    double rideAttackSlow = fit::rideAttackSlow;
    double rideAttackFast = fit::rideAttackFast;
    double rideRelease = fit::rideRelease;
    double ridePower = fit::ridePower;
    double parkTime = fit::parkTime;
    double parkHoldSlow = fit::parkHoldSlow;
    double parkHoldFast = fit::parkHoldFast;
    double spillOffset = fit::spillOffset;
    double initialV = fit::initialV;   // the ride's start

    // sets a constant by name (an array element as the name and its index: onThr0); false for an unknown key
    bool set (const std::string& key, double v)
    {
        if (key == "lowShelfHz") { lowShelfHz = v; return true; }
        if (key == "lowShelfDb") { lowShelfDb = v; return true; }
        if (key == "highShelfHz") { highShelfHz = v; return true; }
        if (key == "highShelfDb") { highShelfDb = v; return true; }
        if (key == "spillOffDb") { spillOffDb = v; return true; }
        if (key == "c") { c = v; return true; }
        if (key == "bias") { bias = v; return true; }
        if (key == "followerAttackSlow") { followerAttackSlow = v; return true; }
        if (key == "followerAttackFast") { followerAttackFast = v; return true; }
        if (key == "followerRelease") { followerRelease = v; return true; }
        if (key == "followerHold") { followerHold = v; return true; }
        if (key == "followerPower") { followerPower = v; return true; }
        if (key == "vadAttack") { vadAttack = v; return true; }
        if (key == "vadRelease") { vadRelease = v; return true; }
        if (key == "vk") { vk = v; return true; }
        if (key == "rideAttackSlow") { rideAttackSlow = v; return true; }
        if (key == "rideAttackFast") { rideAttackFast = v; return true; }
        if (key == "rideRelease") { rideRelease = v; return true; }
        if (key == "ridePower") { ridePower = v; return true; }
        if (key == "parkTime") { parkTime = v; return true; }
        if (key == "parkHoldSlow") { parkHoldSlow = v; return true; }
        if (key == "parkHoldFast") { parkHoldFast = v; return true; }
        if (key == "spillOffset") { spillOffset = v; return true; }
        if (key == "initialV") { initialV = v; return true; }
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
