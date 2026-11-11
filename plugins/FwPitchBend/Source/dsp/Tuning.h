#pragma once
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

// Every constant of Flatwater Pitch Bend's DSP: the modulators (dsp/Mod.h, dsp/Engine.cpp; MEASUREMENTS.md, "Phase 1, the Mix
// and the modulators" and Phase 3) and the shifter (dsp/Shifter.cpp; "the pitch and formant shifter"). Numbers marked
// measured come from measurements/FwPitchBend; the rest are our own choices, tuned on the shifter's scorecard
// (nulltest/check_shifter.py). Kept in one struct so the offline renderers (tools/PbRender.cpp, tools/PbShifterRender.cpp)
// can override any of them by name (tune:key=value); the plug-in uses the defaults. The level grid stays in Shifter.cpp.
namespace pb::dsp
{
struct Tuning
{
    // the modulators (measured)
    double syncedDelay = 0.0001;   // the cycle's place runs this late, synced (s)
    double freeDelay = 0.0081;   // and at a free rate (s)
    double rateOddsA = 1.5;   // a Rate's slots scale it by ((a + s) / (a - s))^p
    double rateOddsP = 3.905;
    double maxRateHz = 30.0;   // a modulated rate stops here
    double amGateLevel = 5.5e-4;   // AM's gate: the input under this
    double amGateHoldS = 0.110;   // for this long closes it
    double amGateOpenS = 0.00192;   // and this long over it opens it
    double amReleaseAttackDiv = 85.0;   // AM's release time: Release + Attack / this
    double ptRefHz = 261.6256;   // PT: log2 (f / this) / 2 (C4)
    float ptHoldAperiodicity = 0.15f;   // PT holds its pitch where the detector's aperiodicity is over this
    float flattenRefHz = 130.8128f;   // Flatten Note 0 (C3)
    int shiftStep = 16;   // the shifter follows the modulated totals in steps of this many samples
    // the shifter's analysis
    double delay48 = 105.53125;   // the no-shift delay at 48 kHz, scaled with the rate (measured)
    double delay44 = 120.4375;   // at 44.1 kHz (measured)
    double firBeta = 8.0;   // the no-shift path's Kaiser window
    double lowpassHz = 3000.0;   // the detector's band
    double coarseHz = 12000.0;   // its decimated rate (about)
    double fminHz = 60.0;   // (voice 70 .. 1000 Hz, with room at both ends)
    double fmaxHz = 1250.0;
    double winShortS = 0.012;
    double winLongS = 0.040;
    double evalS = 0.00067;
    double gateOpenMs = 7.943e-6;   // -51 dBFS rms (measured: shifted at -50, not at -52)
    double gateCloseMs = 7.079e-6;   // -51.5
    double voicedOn = 0.30;
    double voicedKeep = 0.45;
    double smoothS = 0.002;   // the detector period's smoothing time constant
    double maxTrend = 0.0087;   // the marks' period trend reaches at most 15 cents
    double readStepSt = 1.5;   // the Formant total (semitones) past which the grains read a period further back
    double snapScale = 0.01;   // a time scale further from 1 reads the voiced grains at the marks (as any shift)
    double periodReach = 0.03;   // a grain's period read from the marks: at most 50 cents from the newest
    double noiseTiltHz = 1500.0;
    double noiseAperiodic = 0.78;
    // the shifter's grains
    double passHopS = 0.005;   // unvoiced / pass-through grains: 5 ms apart
    double passFadeS = 0.0025;   // with 2.5 ms fades
    double wetFadeS = 0.010;   // the delay path <-> grains cross-fade
    float noiseGain = 1.047f;   // +0.4 dB: re-granulated noise at the measured -0.6 dB
    double levelS = 0.080;   // the voiced level's averaging time
    double kurtRef = 0.83;   // the level grid's typical voice (ln kurtosis)

    // sets a constant by name (an array element as the name and its index: onThr0); false for an unknown key
    bool set (const std::string& key, double v)
    {
        if (key == "syncedDelay") { syncedDelay = v; return true; }
        if (key == "freeDelay") { freeDelay = v; return true; }
        if (key == "rateOddsA") { rateOddsA = v; return true; }
        if (key == "rateOddsP") { rateOddsP = v; return true; }
        if (key == "maxRateHz") { maxRateHz = v; return true; }
        if (key == "amGateLevel") { amGateLevel = v; return true; }
        if (key == "amGateHoldS") { amGateHoldS = v; return true; }
        if (key == "amGateOpenS") { amGateOpenS = v; return true; }
        if (key == "amReleaseAttackDiv") { amReleaseAttackDiv = v; return true; }
        if (key == "ptRefHz") { ptRefHz = v; return true; }
        if (key == "ptHoldAperiodicity") { ptHoldAperiodicity = (float) v; return true; }
        if (key == "flattenRefHz") { flattenRefHz = (float) v; return true; }
        if (key == "shiftStep") { shiftStep = (int) v; return true; }
        if (key == "delay48") { delay48 = v; return true; }
        if (key == "delay44") { delay44 = v; return true; }
        if (key == "firBeta") { firBeta = v; return true; }
        if (key == "lowpassHz") { lowpassHz = v; return true; }
        if (key == "coarseHz") { coarseHz = v; return true; }
        if (key == "fminHz") { fminHz = v; return true; }
        if (key == "fmaxHz") { fmaxHz = v; return true; }
        if (key == "winShortS") { winShortS = v; return true; }
        if (key == "winLongS") { winLongS = v; return true; }
        if (key == "evalS") { evalS = v; return true; }
        if (key == "gateOpenMs") { gateOpenMs = v; return true; }
        if (key == "gateCloseMs") { gateCloseMs = v; return true; }
        if (key == "voicedOn") { voicedOn = v; return true; }
        if (key == "voicedKeep") { voicedKeep = v; return true; }
        if (key == "smoothS") { smoothS = v; return true; }
        if (key == "maxTrend") { maxTrend = v; return true; }
        if (key == "readStepSt") { readStepSt = v; return true; }
        if (key == "snapScale") { snapScale = v; return true; }
        if (key == "periodReach") { periodReach = v; return true; }
        if (key == "noiseTiltHz") { noiseTiltHz = v; return true; }
        if (key == "noiseAperiodic") { noiseAperiodic = v; return true; }
        if (key == "passHopS") { passHopS = v; return true; }
        if (key == "passFadeS") { passFadeS = v; return true; }
        if (key == "wetFadeS") { wetFadeS = v; return true; }
        if (key == "noiseGain") { noiseGain = (float) v; return true; }
        if (key == "levelS") { levelS = v; return true; }
        if (key == "kurtRef") { kurtRef = v; return true; }
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
