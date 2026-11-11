#pragma once
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

// Every constant of Flatwater Vocal Chain's chain (dsp/Engine.h, dsp/Engine.cpp), as measured on the reference
// (MEASUREMENTS.md, Flatwater Vocal Chain). Kept in one struct so the offline renderer (tools/ChRender.cpp) can
// override any of them by name (tune:key=value); the plug-in uses the defaults. The per-colour and per-rate tables
// (the fixed filter, Bass and Treble, Compress, the de-esser, the doubler's voices) stay in their generated headers
// and in Engine.cpp; the reverb's responses in resources/.
namespace ch::dsp
{
struct Tuning
{
    // the chain
    int latency48 = 193;   // the latency at 44.1 / 48 kHz (samples); times the rate's multiple, less the multiple - 1
    double clearShaperSq = 0.00144547;   // the clear chain's own shaper: x - this x^2 + the next x^3
    double clearShaperCube = 0.00145842;
    double noiseDb = -108.7;   // the Noise: white, uncorrelated L / R (-108.3 dBFS rms at the output, colours clear)
    double levelS = 0.010;   // the Sensitivity LED's level: rms over this (s)
    double faderDbPerStep = 1.2;   // Reverb, Delay and Pitch: dB per fader step from the measured response at 0
    double reverbDeessOffsetDb = -8.0;   // the reverb send's de-esser threshold against the main one's
    double eqGlideS = 0.054;   // the Bass and Treble faders' glide: a one-pole on the fader (s)
    // the echoes against the reference's: three sections (frequency, Q, gain in dB)
    double delayFixLowHz = 132.45;
    double delayFixLowQ = 0.7071;
    double delayFixLowDb = 0.1561;
    double delayFixPeakHz = 429.81;
    double delayFixPeakQ = 1.0076;
    double delayFixPeakDb = -0.1509;
    double delayFixHighHz = 1929.09;
    double delayFixHighQ = 0.7071;
    double delayFixHighDb = 0.3470;
    // the delays: the two taps' gains and the second's cross-feed (calibrated on the Slap colour), the loop's level
    float tap1 = 0.1566f;
    float tap2 = 0.1566f;
    float tap2Cross = 0.995f;
    float loopIn = 0.6f;   // the level the delay's loop works at, against the send
    int tapDelay = 1;   // the first tap: samples past the line's
    double tap2Bars = 0.004;   // the second tap: bars later
    float delayHipassHz = 150.0f;   // the line's HiPass and LoPass
    float delayLopassHz = 5000.0f;
    // the doubler: its shelves (frequency, Q, gain in dB), the step search's memory, the window and the crossfade
    double shelfLowHz = 200.4691;
    double shelfLowQ = 0.4996;
    double shelfLowDb = -8.6057;
    double shelfHighHz = 5539.4842;
    double shelfHighQ = 0.5003;
    double shelfHighDb = 4.4971;
    double corrMemory = 960.0;   // samples at 48 kHz
    double halfWindow = 256.0;   // samples, at every rate
    double fadeLength = 256.0;

    // sets a constant by name (an array element as the name and its index: onThr0); false for an unknown key
    bool set (const std::string& key, double v)
    {
        if (key == "latency48") { latency48 = (int) v; return true; }
        if (key == "clearShaperSq") { clearShaperSq = v; return true; }
        if (key == "clearShaperCube") { clearShaperCube = v; return true; }
        if (key == "noiseDb") { noiseDb = v; return true; }
        if (key == "levelS") { levelS = v; return true; }
        if (key == "faderDbPerStep") { faderDbPerStep = v; return true; }
        if (key == "reverbDeessOffsetDb") { reverbDeessOffsetDb = v; return true; }
        if (key == "eqGlideS") { eqGlideS = v; return true; }
        if (key == "delayFixLowHz") { delayFixLowHz = v; return true; }
        if (key == "delayFixLowQ") { delayFixLowQ = v; return true; }
        if (key == "delayFixLowDb") { delayFixLowDb = v; return true; }
        if (key == "delayFixPeakHz") { delayFixPeakHz = v; return true; }
        if (key == "delayFixPeakQ") { delayFixPeakQ = v; return true; }
        if (key == "delayFixPeakDb") { delayFixPeakDb = v; return true; }
        if (key == "delayFixHighHz") { delayFixHighHz = v; return true; }
        if (key == "delayFixHighQ") { delayFixHighQ = v; return true; }
        if (key == "delayFixHighDb") { delayFixHighDb = v; return true; }
        if (key == "tap1") { tap1 = (float) v; return true; }
        if (key == "tap2") { tap2 = (float) v; return true; }
        if (key == "tap2Cross") { tap2Cross = (float) v; return true; }
        if (key == "loopIn") { loopIn = (float) v; return true; }
        if (key == "tapDelay") { tapDelay = (int) v; return true; }
        if (key == "tap2Bars") { tap2Bars = v; return true; }
        if (key == "delayHipassHz") { delayHipassHz = (float) v; return true; }
        if (key == "delayLopassHz") { delayLopassHz = (float) v; return true; }
        if (key == "shelfLowHz") { shelfLowHz = v; return true; }
        if (key == "shelfLowQ") { shelfLowQ = v; return true; }
        if (key == "shelfLowDb") { shelfLowDb = v; return true; }
        if (key == "shelfHighHz") { shelfHighHz = v; return true; }
        if (key == "shelfHighQ") { shelfHighQ = v; return true; }
        if (key == "shelfHighDb") { shelfHighDb = v; return true; }
        if (key == "corrMemory") { corrMemory = v; return true; }
        if (key == "halfWindow") { halfWindow = v; return true; }
        if (key == "fadeLength") { fadeLength = v; return true; }
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
