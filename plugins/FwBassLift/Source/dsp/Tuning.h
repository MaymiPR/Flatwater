#pragma once
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

// Every constant of the DSP (dsp/Engine.h), from the measurements and the fits. Kept in one struct so the offline
// renderer (tools/BfRender.cpp) can override any of them by name (tune:key=value, or its own short keys); the plug-in
// uses the defaults.
namespace bf
{
struct Tuning
{
    // fitted jointly to time-domain renders at Freq 40 / 80 / 160 / 256, transients (kicks, bass notes, a riff) and the
    // steady harmonic and detector-pinning sets (plugins/FwBassLift/nulltest/fit_combo.py, MEASUREMENTS.md "Flatwater Bass Lift model")
    double C0 = 315.9608630559933;   // gain law
    double E0 = 0.030499720821978243;
    double p = 0.55503236720374;
    double cA = 1.0386281298252804;   // level band (x Freq)
    double QA = 1.7618851483536635;
    double cB = 10.374984;
    double kr = 1.0;   // rectifier exponent
    double att1 = 0.002489532680531953;   // follower 1 (s) at Freq 80
    double rel1 = 0.0024505484900786867;
    double att2 = 0.015066489274832208;   // follower 2 (s) at Freq 80
    double rel2 = 0.028267150668494348;
    double s1 = 0.24816982832999762;   // follower times scale with (80 / Freq)^s
    double s2 = 0.3189141441137469;
    double sA = 0.0;   // extra (80 / Freq)^sA on the attack times
    double El0 = 0.007175230751448638;   // l-driven stage
    double pL = 0.0660965110542429;
    double tLa = 0.006268107590220969;
    double tLr = 0.015175753598975574;
    double sL = 0.0;
    double alpha = 0.24535396843164406;   // share of the gain ahead of the first high-pass
    double k2 = -0.007985047672314461;   // harmonic product gain at Decay 1
    double limT = 0.70287;   // generated-signal limiter: ceiling
    double limRel = 0.02844;   // and gain release (s)
    double gainGlideMs = 25.0;   // Out Gain smoothing
    bool learned = true;   // learned gain block (else the fitted analytical one)
    double ceiling = 32767.0 / 32768.0;   // output clip: [-1, 32767/32768], hard (measured)

    // sets a constant by name (an array element as the name and its index: onThr0); false for an unknown key
    bool set (const std::string& key, double v)
    {
        if (key == "C0") { C0 = v; return true; }
        if (key == "E0") { E0 = v; return true; }
        if (key == "p") { p = v; return true; }
        if (key == "cA") { cA = v; return true; }
        if (key == "QA") { QA = v; return true; }
        if (key == "cB") { cB = v; return true; }
        if (key == "kr") { kr = v; return true; }
        if (key == "att1") { att1 = v; return true; }
        if (key == "rel1") { rel1 = v; return true; }
        if (key == "att2") { att2 = v; return true; }
        if (key == "rel2") { rel2 = v; return true; }
        if (key == "s1") { s1 = v; return true; }
        if (key == "s2") { s2 = v; return true; }
        if (key == "sA") { sA = v; return true; }
        if (key == "El0") { El0 = v; return true; }
        if (key == "pL") { pL = v; return true; }
        if (key == "tLa") { tLa = v; return true; }
        if (key == "tLr") { tLr = v; return true; }
        if (key == "sL") { sL = v; return true; }
        if (key == "alpha") { alpha = v; return true; }
        if (key == "k2") { k2 = v; return true; }
        if (key == "limT") { limT = v; return true; }
        if (key == "limRel") { limRel = v; return true; }
        if (key == "gainGlideMs") { gainGlideMs = v; return true; }
        if (key == "learned") { learned = v < 0.0 || v > 0.0; return true; }
        if (key == "ceiling") { ceiling = v; return true; }
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
