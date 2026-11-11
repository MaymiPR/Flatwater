#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <memory>
#include <random>
#include "Fixed.h"
#include "Eq.h"
#include "EqTables.h"
#include "Compress.h"
#include "CompressNet.h"
#include "DeEss.h"
#include "Tuning.h"
#include "plugins/FwDelay/Source/dsp/Engine.h"
#include "plugins/FwComp/Source/dsp/Analog.h"
#include "plugins/FwDelay/Source/dsp/AnalogTables.h"

// Flatwater Vocal Chain's audio, as measured on the reference (MEASUREMENTS.md, Flatwater Vocal Chain: Phase 3 and the full campaign):
//   in -> Sensitivity -> Balance (the far side) -> the latency -> Bass, Treble -> Compress -> the de-esser -> [the sends'
//   tap] -> the fixed filter -> dry;  wet: the tap -> the reverb (true stereo convolution) / the delays / the doubler,
//   each muted by its colour's clear position; dry + wet -> Output.
// The fixed filter (every colour clear: two all-passes and gentle shelves) is linear and sits before any dynamics in
// the reference; here it goes after them, on the dry path only, so the wet stages' measured responses (which include
// it) apply to the tap as they are. Phase 4, first pass: the de-esser and Compress pass the signal (their linear part
// only); the delays and the doubler are silent.
namespace ch::dsp
{
struct Params
{
    float sensitivity = 0.0f, output = 0.0f, balance = 0.0f;
    float fader[6] {};                                                // Bass, Treble, Compress, Reverb, Delay, Pitch
    int colour[6] { 1, 1, 1, 1, 1, 1 };
    double bpm = 120.0;                                               // the host's tempo (the delays are synced to it)
};

// a second-order section, transposed direct form II, per channel
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    double z1[2] {}, z2[2] {};
    void set (const double c[5]) { b0 = c[0]; b1 = c[1]; b2 = c[2]; a1 = c[3]; a2 = c[4]; }
    double run (double x, int ch)
    {
        const double y = b0 * x + z1[ch];
        z1[ch] = b1 * x - a1 * y + z2[ch];
        z2[ch] = b2 * x - a2 * y;
        return y;
    }
    void reset() { z1[0] = z1[1] = z2[0] = z2[1] = 0.0; }
};

// RBJ cookbook sections, the frequency prewarped by the bilinear transform's own warping (as fitted)
inline void rbj (eq::Kind kind, double f0, double q, double gdb, double fs, double c[5])
{
    const double A = std::pow (10.0, gdb / 40.0), w = 2.0 * juce::MathConstants<double>::pi * juce::jlimit (1.0, fs * 0.49, f0) / fs;
    const double cw = std::cos (w), sw = std::sin (w), al = sw / (2.0 * q);
    double b[3], a[3];
    switch (kind)
    {
        case eq::peak: b[0] = 1 + al * A; b[1] = -2 * cw; b[2] = 1 - al * A; a[0] = 1 + al / A; a[1] = -2 * cw; a[2] = 1 - al / A; break;
        case eq::lowShelf:
        {
            const double s = 2 * std::sqrt (A) * al;
            b[0] = A * ((A + 1) - (A - 1) * cw + s); b[1] = 2 * A * ((A - 1) - (A + 1) * cw); b[2] = A * ((A + 1) - (A - 1) * cw - s);
            a[0] = (A + 1) + (A - 1) * cw + s; a[1] = -2 * ((A - 1) + (A + 1) * cw); a[2] = (A + 1) + (A - 1) * cw - s;
            break;
        }
        case eq::highShelf:
        {
            const double s = 2 * std::sqrt (A) * al;
            b[0] = A * ((A + 1) + (A - 1) * cw + s); b[1] = -2 * A * ((A - 1) + (A + 1) * cw); b[2] = A * ((A + 1) + (A - 1) * cw - s);
            a[0] = (A + 1) - (A - 1) * cw + s; a[1] = 2 * ((A - 1) - (A + 1) * cw); a[2] = (A + 1) - (A - 1) * cw - s;
            break;
        }
        case eq::highPass: default: b[0] = (1 + cw) / 2; b[1] = -(1 + cw); b[2] = (1 + cw) / 2; a[0] = 1 + al; a[1] = -2 * cw; a[2] = 1 - al; break;
    }
    c[0] = b[0] / a[0]; c[1] = b[1] / a[0]; c[2] = b[2] / a[0]; c[3] = a[1] / a[0]; c[4] = a[2] / a[0];
}

// Bass or Treble at the (gliding) fader. At the rates the reference was captured at (44.1 / 48 / 88.2 / 96 kHz) each of
// the fader's 201 positions is the reference's own filter (EqTables.h), the nearest 0.1 position of the gliding
// value; at other rates the fitted shapes (Eq.h: each section's gain interpolated between the steps, its Q from
// its gain, its frequency fixed)
class EqStage
{
public:
    void prepare (double sampleRate, bool isBass, double glideS)
    {
        fs = sampleRate; bassStage = isBass;
        glide = std::exp (-1.0 / (glideS * fs));
        rate = nullptr;
        for (const auto& r : eqtables::rates) if (std::abs (r.fs - fs) < 1.0) rate = &r;
        for (auto& s : sec) s.reset();
        colourIndex = 0; dirty = true;
    }
    // the Bass and Treble faders glide (90 % in about 124 ms, measured): a one-pole of 54 ms on the fader
    void set (int colourIdx, float target)
    {
        colourIdx = juce::jlimit (0, 3, colourIdx);
        if (colourIdx != colourIndex) { colourIndex = colourIdx; for (auto& s : sec) s.reset(); dirty = true; shown = target; }
        to = target;
    }
    void run (double& l, double& r)
    {
        if (colourIndex == 0) return;
        if (std::abs (shown - to) > 1.0e-4) { shown = to + glide * (shown - to); if (++count >= 16) { count = 0; dirty = true; } }
        else if (! juce::exactlyEqual (shown, to)) { shown = to; dirty = true; }
        if (dirty) update();
        for (int i = 0; i < n; ++i) { l = sec[(size_t) i].run (l, 0); r = sec[(size_t) i].run (r, 1); }
    }

private:
    void update()
    {
        dirty = false;
        if (rate != nullptr)
        {
            const int k = juce::jlimit (0, 200, (int) std::lround ((shown + 10.0) * 10.0));
            const auto& pos = (bassStage ? rate->bass : rate->treble)[colourIndex - 1][k];
            n = eqtables::sections;
            for (int i = 0; i < n; ++i) sec[(size_t) i].set (pos.sos[i]);
            return;
        }
        const auto* colour = (bassStage ? eq::bass : eq::treble)[colourIndex];
        const double x = juce::jlimit (0.0, 20.0, shown + 10.0);
        const int i = juce::jmin (19, (int) x);
        const double t = x - i;
        n = 0;
        for (int k = 0; k < colour->n; ++k)
        {
            const auto& s = colour->s[k];
            if (s.kind == eq::highPass && colour->hpFromZero && shown < -1.0e-6) continue;     // (Upper: from 0 up)
            const double g = s.g[i] + (s.g[i + 1] - s.g[i]) * t, q = juce::jmax (0.05, s.q0 + s.q1 * g + s.q2 * g * g), f = s.f;
            double c[5];
            rbj (s.kind, f, q, g, fs, c);
            sec[(size_t) n++].set (c);
        }
    }
    const eqtables::Rate* rate = nullptr;
    bool bassStage = true;
    int colourIndex = 0;
    std::array<Biquad, (size_t) std::max (8, eqtables::sections)> sec;
    int n = 0, count = 0;
    double fs = 48000.0, glide = 0.0, shown = 0.0, to = 0.0;
    bool dirty = true;
};

// the reverb: per colour, two stereo zero-latency convolutions (L->L with R->R, and R->L with L->R on the swapped input)
// with the reference's measured responses at the nearest rate; the last colour keeps running while Reverb is clear
class Reverb
{
public:
    // skip: samples dropped from the responses' (silent) start, the reverb's de-esser's delay
    void prepare (double fs, int block, int skip);
    void process (const float* inL, const float* inR, float* outL, float* outR, int n, int colour, float gain);
    void reset() { for (auto& c : conv) for (auto& e : c) if (e) e->reset(); }

private:
    std::array<std::array<std::unique_ptr<juce::dsp::Convolution>, 2>, 3> conv;
    juce::AudioBuffer<float> a, b;
    int running = 0;                                                  // the colour whose engine runs (0: none yet)
};

// The learned correction of Compress (plugins/FwVocalChain/nulltest/train_compress.py, MEASUREMENTS.md "Flatwater Vocal Chain: Compress, a
// learned correction"): per colour compressnet::members GRUs, one step per 1/12000 s (4 samples at 48 kHz; a phase
// accumulator at other rates). Per step each reads, for the samples leaving the compressor, the detector level (most),
// the compressor's reduction (mean) and the limiter's (most), the input's peak and power (dB), and the fader; the mean
// of their outputs c (ln, within +-2 dB) scales the following samples, moving linearly from the second-to-last to the
// last finished step.
class CompressNet
{
public:
    void prepare (double fs) { inc = 12000.0 / fs; reset(); }
    void reset()
    {
        for (auto& m : h) std::fill (m, m + compressnet::hidden, 0.0f);
        c1 = c2 = 0.0; phase = 0.0; resetStep();
    }
    // per sample leaving the compressor: detector level, reductions (dB), the input's peak and power (linear);
    // returns the sample's correction (ln)
    double process (int colour, double fader, double det, double red, double lred, double pk, double pw)
    {
        dmax = std::max (dmax, det); rsum += red; lmax = std::max (lmax, lred); pmax = std::max (pmax, pk); wsum += pw; ++count;
        phase += inc;
        const double corr = c2 + (c1 - c2) * std::min (phase, 1.0);
        if (phase >= 0.99999) { step (colour, fader); phase -= 1.0; if (phase < 1.0e-5) phase = 0.0; }
        return corr;
    }

private:
    void resetStep() { dmax = lmax = -1.0e300; pmax = rsum = wsum = 0.0; count = 0; }
    void step (int colour, double fader)
    {
        constexpr int I = compressnet::inputs;
        const double raw[I] = { std::max (dmax, -120.0) / 20.0, rsum / count / 20.0, lmax / 20.0,
                                std::max (20.0 * std::log10 (pmax + 1.0e-6), -120.0) / 20.0,
                                std::max (10.0 * std::log10 (wsum / count + 1.0e-12), -120.0) / 20.0, fader / 10.0 };
        double c = 0.0;
        for (int m = 0; m < compressnet::members; ++m)
            c += run (compressnet::nets[colour][m], raw, h[m]);
        c2 = c1; c1 = c / compressnet::members;
        resetStep();
    }
    // one GRU step of one net, its state st updated; returns its correction
    static double run (const compressnet::Net& net, const double* raw, float* st)
    {
        constexpr int H = compressnet::hidden, I = compressnet::inputs;
        float in[I];
        for (int k = 0; k < I; ++k) in[k] = (float) ((raw[k] - net.mean[k]) / net.scale[k]);
        float gi[3 * H], gh[3 * H];
        for (int j = 0; j < 3 * H; ++j)
        {
            float a = net.bih[j], c = net.bhh[j];
            for (int k = 0; k < I; ++k) a += net.Wih[j * I + k] * in[k];
            for (int m = 0; m < H; ++m) c += net.Whh[j * H + m] * st[m];
            gi[j] = a; gh[j] = c;
        }
        for (int j = 0; j < H; ++j)
        {
            const float r = 1.0f / (1.0f + std::exp (-(gi[j] + gh[j])));
            const float z = 1.0f / (1.0f + std::exp (-(gi[H + j] + gh[H + j])));
            const float n = std::tanh (gi[2 * H + j] + r * gh[2 * H + j]);
            st[j] = (1.0f - z) * n + z * st[j];
        }
        float o = net.bout[0];
        for (int m = 0; m < H; ++m) o += net.Wout[m] * st[m];
        return compressnet::cmax * std::tanh ((double) o);
    }

    float h[compressnet::members][compressnet::hidden] {};
    double c1 = 0.0, c2 = 0.0, phase = 0.0, inc = 0.25;
    double dmax = 0.0, rsum = 0.0, lmax = 0.0, pmax = 0.0, wsum = 0.0;
    int count = 0;
};

// Compress, as fitted (plugins/FwVocalChain/nulltest/fit_compress.py; plugins/FwVocalChain/nulltest/native/Compress.c is the same algorithm): an input gain into a
// compressor (stereo-linked; a power or peak detector, the threshold, ratio and knee, attack and a two-stage release),
// a static shaper (x^2, x |x|, x^3) before or after it, a look-ahead limiter holding the output under its ceiling, an
// output gain, the learned correction (CompressNet), then the colour's own linear filter (exact per rate). Its
// parameters per fader step, interpolated. The look-ahead delays the path by la samples (also while Compress is clear,
// a plain delay), taken out of the input delay.
class Compressor
{
public:
    void prepare (double fs);
    int lookAhead() const { return la; }
    void process (float* l, float* r, int n, int colour, float fader);

private:
    void reset();
    double fs = 48000.0;
    int la = 64, w = 0, active = -1;
    std::vector<float> bl, br;
    std::vector<long long> dq;
    std::vector<double> dqNeed;
    long long qh = 0, qt = 0;
    long long count = 0;
    double gf = 0.0, gs = 0.0, pw = 0.0, lg = 1.0, fbL = 0.0, fbR = 0.0, dhx[2] {}, dhy[2] {};
    std::array<Biquad, 4> linear;
    // the net's inputs per sample, held until the sample leaves the limiter (la later)
    struct Feature { double det, red, pk, pw; };
    std::vector<Feature> feat;
    int fw = 0;
    CompressNet net;
};

// The main de-esser, as fitted (plugins/FwVocalChain/nulltest/fit_deess.py; plugins/FwVocalChain/nulltest/native/DeEss.c is the same algorithm): a zero-phase high band H
// (its taps per rate) is turned down, x - (1 - g) H x; g from two stereo-linked detectors on the signal after the fixed
// filter: a high-pass one against a threshold that rises with a low-pass one (thr = T + k low, in amplitude), the
// reduction smoothed (attack, hold, release, a slow stage). The gain is computed lookAhead samples before the audio it
// turns down, so the audio is delayed by the band's half length and the look-ahead (taken out of the input delay).
// It turns down one or two signals with the same gain (the main one: the dry path after the fixed filter, and the tap
// before it that the reverb takes). The reverb's send has a second one, the same but for its threshold (the presets'
// -28 dB against the main one's -20; measured: 8 dB lower on every colour), after the main one.
class DeEsser
{
public:
    void prepare (double fs, double thresholdOffsetDb = 0.0);
    void reset();
    int delay() const { return nt + la; }
    // det: what the detectors see (read before a is written, so it may be a); b may be null
    void process (const float* detL, const float* detR, float* aL, float* aR, float* bL, float* bR, int n);

private:
    double fs = 48000.0;
    int nt = 24, la = 24, len = 1, w = 0, gw = 0;
    const double* h = nullptr;
    std::vector<float> al, ar, bl, br, gq;
    Biquad dh, dl;
    double eh = 0.0, el = 0.0, r = 0.0, rs = 0.0, gl = 1.0;
    int hold = 0, holdN = 0;
    double ha = 0, hr = 0, lla = 0, llr = 0, ga = 0, gr = 0, sa = 0, sr = 0, offset = 0;
};

// the delays, as measured (MEASUREMENTS.md, Flatwater Vocal Chain: Phase 4): the send through the delay reference's Analog 4 and its line
// (Flatwater Delay's model, set from each colour's preset: Slap a 1/16 note, no feedback, -12 dB; Eight 1/8, 50 %,
// -9 dB; Quarter 1/4, 30 %, -9 dB; high-pass 150 Hz, low-pass 5 kHz; synced to the host's tempo), then SuperTap's two
// taps, each turning the stereo image by 45 degrees: the first at once, L = g1 (L + R), R = g1 (R - L); the second 0.004
// bar later, L = g2 (L - 0.995 R), R = g2 (0.995 L + R). The line keeps running while Delay is clear.
class Delays
{
public:
    void prepare (double fs, int block, const Tuning&);
    void process (const float* inL, const float* inR, float* outL, float* outR, int n, int colour, float gain, double bpm);

private:
    double fs = 48000.0;
    Tuning tune;
    co::Analog analog;
    de::dsp::Delay line;
    juce::AudioBuffer<float> work;
    std::vector<float> histL, histR;                                  // the line's output, for the second tap
    int hpos = 0, running = 1;
};

// the doubler (Pitch), as measured: the reference doubler's two voices per colour, the left one on the left input, the right
// on the right. Stereo (MEASUREMENTS.md, the doubler's third pass): each voice a linearly interpolated read of the
// send whose speed swings as a triangle (+-7 cents, 0.2 / 0.3 Hz, from the plug-in's start) with a small steady drift,
// the delay one sample longer from the first whole-sample crossing after each turn down until the same after each turn
// up; then a 200 Hz low shelf (-8.6 dB) and a 5.54 kHz high shelf (+4.5 dB), -10 dB at fader 0. Wide (+-18 cents, the
// shelves) and Spreader (+-15 cents, 0.15 Hz, no shelves): the same read head kept within its start +-256 samples: when
// it leaves, a tap one of 20 steps nearer the middle (round (256 2^(k / 20)) samples, where the input best matches
// itself) fades in as the old one fades out. The LFOs run from the plug-in's start (prepare or reset). At other rates
// (the doubler at 44.1 and 96 kHz) the window, the steps and the crossfade stay in samples, so Stereo, which never leaves
// its window at 48 kHz, splices at 96 kHz as the reference does; Stereo's start delays are whole samples.
class Doubler
{
public:
    void prepare (double fs, int chainLatency, const Tuning&);
    void reset();
    void process (const float* inL, const float* inR, float* outL, float* outR, int n, int colour, float gain);

private:
    double clockOffset = 0.0;
    Tuning tune;
    // a measured voice's state: the last delay, its direction, the extra sample and the pending turn; the pitch
    // shifter's tap offset, the next one while they crossfade and how far the crossfade is (-1: none)
    struct TriState
    {
        double last = 0.0;
        bool down = false, armed = false;
        double armFloor = 0.0;
        int extra = 0;
        double offset = 0.0, next = 0.0;
        int fade = -1;
        std::array<double, 20> corr {};                                // the input's autocorrelation at the 20 steps
    };
    // a voice at this rate: its start delay, its LFO's rate and lead (samples) and its window
    struct Voice { double d0 = 0.0, f = 0.2, lead = 0.0, low = 0.0, high = 0.0; };
    std::array<std::array<Voice, 2>, 4> voices {};
    double triDelay (int colour, int voice, double n) const;           // the smooth delay (samples) at sample n
    void resetVoices (int colour);
    int voicesFor = -1;
    double fs = 48000.0, t = 0.0;
    std::vector<float> lineL, lineR;
    int pos = 0, running = 1;
    std::array<Biquad, 2> shelves;
    int shelvesFor = -1;
    std::array<TriState, 2> tri;
};

class Chain
{
public:
    void prepare (double fs, int maxBlock);
    void reset();
    // stereo in place (Mono/Stereo: the caller copies the input to both channels first)
    void process (float* l, float* r, int n, const Params&);
    // the Sensitivity LED's level: the input's rms (dB), after Sensitivity, over about 10 ms
    float levelDb() const { return level > 1.0e-14 ? (float) (10.0 * std::log10 (level)) : -144.0f; }
    // the Noise (on by default, as the reference; its own state setting, not a host parameter): off for null tests
    bool noiseOn = true;
    Tuning t = tuning();                                               // dsp/Tuning.h: the defaults, or the renderer's overrides

private:
    double fs = 48000.0;
    int latency = 193, pos = 0;
    std::vector<float> lineL, lineR;
    std::array<Biquad, 4> fixedFilter;
    EqStage bass, treble;
    Compressor compressor;
    DeEsser deesser, reverbDeesser;
    Reverb reverb;
    Delays delays;
    std::array<Biquad, 3> delayFix;                                    // (the echoes' response against the reference's)
    Doubler doubler;
    juce::AudioBuffer<float> tap, send, wet, wet2, wet3;
    std::minstd_rand noise { 0x5eed };
    double level = 0.0, levelCoef = 0.0, noiseGain = 0.0;
};
}
