#pragma once
// Every constant of the DSP that a fit or a render may override (tools/ renderers set the fields by name).

namespace bf
{
struct Tuning
{
    // fitted jointly to time-domain renders at Freq 40 / 80 / 160 / 256, transients (kicks, bass notes, a riff) and the
    // steady harmonic and detector-pinning sets (plugins/FwBassLift/nulltest/fit_combo.py, MEASUREMENTS.md "Flatwater Bass Lift model")
    double C0 = 315.9608630559933, E0 = 0.030499720821978243, p = 0.55503236720374;                // gain law
    double cA = 1.0386281298252804, QA = 1.7618851483536635, cB = 10.374984;                // level band (x Freq)
    double kr = 1.0;                                               // rectifier exponent
    double att1 = 0.002489532680531953, rel1 = 0.0024505484900786867;                       // follower 1 (s) at Freq 80
    double att2 = 0.015066489274832208, rel2 = 0.028267150668494348;                       // follower 2 (s) at Freq 80
    double s1 = 0.24816982832999762, s2 = 0.3189141441137469;                           // follower times scale with (80 / Freq)^s
    double sA = 0.0;                                               // extra (80 / Freq)^sA on the attack times
    double El0 = 0.007175230751448638, pL = 0.0660965110542429, tLa = 0.006268107590220969, tLr = 0.015175753598975574, sL = 0.0;  // l-driven stage
    double alpha = 0.24535396843164406;                                             // share of the gain ahead of the first high-pass
    double k2 = -0.007985047672314461;                                                // harmonic product gain at Decay 1
    double limT = 0.70287, limRel = 0.02844;                       // generated-signal limiter: ceiling, gain release (s)
    double gainGlideMs = 25.0;                                     // Out Gain smoothing
    bool learned = true;                                           // learned gain block (else the fitted analytical one)
    double ceiling = 32767.0 / 32768.0;                            // output clip: [-1, 32767/32768], hard (measured)
};
}
