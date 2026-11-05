#pragma once
#include <array>
#include <cmath>
#include "FwPitchBendModel.h"
#include "FwPitchBendMath.h"

// Flatwater Pitch Bend's modulators, as measured on the reference (MEASUREMENTS.md, Flatwater Pitch Bend, "Phase 1, the
// Mix and the modulators"; nulltest/measure_pitchbend_mods.py):
// - M1 / M2: an LFO reads the first 256 values of its table as one cycle, each value held for its 256th of the cycle;
//   a sequencer holds step k's value (value / 24) for one Rate period. The cycle's place is the host's song position
//   over the synced length, or counted from the start of processing at a free rate in Hz, its period a whole number
//   of 8-sample ticks (rounded down); Phase adds to it; Warp maps it as w p / (1 + (w - 1) p); Level scales by
//   L / (2 - L); Smooth is a one-pole low-pass whose time constant in ms is its value. They run per sample, their
//   cycle late by a fixed time (synced 0.1 ms, free 8.1 ms; fitted on the reference's curves to 0.0006 rms).
// - AM: a peak follower of the input, read once every 8 samples, with its own attack and release, x2; PT: the voice's pitch as
//   log2 (f / C4) / 2 (+-1 = +-2 octaves), plus Offset, smoothed by its Smooth (ms).
// - A slot adds depth / 100 x its source: on Pitch, Formant, Mix and the modulators' Phase and Level to the control's
//   normalised value, clipped to the control's range (Pitch's to twice its range, +-24 semitones); the controls that
//   scale (the Rates, Warps, AM's and PT's) are multiplied by ((a + s) / (a - s))^p of the slots' sum s (oddsScale).
// The modulators run per sample; a control other than Mix takes their value once per host block.
namespace fwpitchbend
{
// the synced lengths in beats, in the host parameter's order (32 bars ... 1/64 triplet)
inline double syncedBeats (int index)
{
    static const double b[24] { 128.0, 64.0, 32.0, 16.0, 8.0, 4.0, 3.0, 2.0, 1.5, 4.0 / 3.0, 1.0, 0.75,
                                2.0 / 3.0, 0.5, 0.375, 1.0 / 3.0, 0.25, 0.1875, 1.0 / 6.0, 0.125, 0.09375, 1.0 / 12.0, 0.0625, 1.0 / 24.0 };
    return b[index < 0 ? 0 : index > 23 ? 23 : index];
}

inline double warpPhase (double p, double w) { return w * p / (1.0 + (w - 1.0) * p); }
inline double levelGain (double level) { return level / (2.0 - level); }

// the delays of the cycle's place (s): synced, free
inline constexpr double syncedDelay = 0.0001, freeDelay = 0.0081;

// a free rate as it runs: its period rounded down to whole 8-sample ticks (measured at 48 kHz: 3.7 Hz runs at 12968
// samples, 3.7014 Hz). (A rate on a whole tick stays on it: the knob's taper brings a typed 4 Hz back a hair off, and
// rounded down that is a tick short; a millionth of a tick is allowed.)
inline double freeRate (double hz, double sr)
{
    return sr / (8.0 * std::max (1.0, std::floor (sr / (8.0 * std::max (hz, 1e-6)) + 1e-6)));
}

// one LFO / sequencer
struct Lfo
{
    double freeCycles = 0.0;                                      // cycles run at the free rate since processing began
    bool counting = false;                                        // (the count starts freeDelay late: see cyclesAt)
    double smoothed = 0.0;
    float place = 0.0f;                                           // where in its cycle (the editor's play-head)

    void reset() { freeCycles = 0.0; counting = false; smoothed = 0.0; place = 0.0f; }

    // one sample: cycles = the cycle's place before Phase (song position or free count, already delayed); gain the
    // Level's; a the Smooth filter's coefficient per sample
    double next (const Table& t, bool seq, int steps, double cycles, double phase, double warp, double gain, double a)
    {
        double p = cycles + phase;
        p -= std::floor (p);
        p = warpPhase (p, warp);
        place = (float) p;
        double v;
        if (seq)
        {
            const int k = juce::jlimit (0, steps - 1, (int) (p * steps));
            v = t[(size_t) (seqFirst + k)];
        }
        else
        {
            v = t[(size_t) juce::jlimit (0, lfoPoints - 1, (int) (p * lfoPoints))];
        }
        v *= gain;
        smoothed = v + a * (smoothed - v);          // (from 0: measured at Smooth 1000, where the start still shows)
        return smoothed;
    }
};

// how a scaling control takes its slots' sum s (MEASUREMENTS.md, Phase 3: M2 a constant on M1's controls, read through
// M1 on Mix): x ((a + s) / (a - s))^p. Warp, AM Attack and Release: a 1, p 1 (s = +-1 reaches the ends); AM and PT
// Level: a 0.5, p 1 (s = -0.5 gives 0); the Rates: a 1.5, p 3.905 (s 0.1 x1.68, 0.25 x3.72, 0.5 x15.6)
inline double oddsScale (double s, double a, double p)
{
    const double c = std::max (-0.999999 * a, std::min (0.999999 * a, s));
    return pbmath::pow ((a + c) / (a - c), p);
}

// a one-pole's coefficient per sample for a time constant in ms
inline double poleFor (double ms, double sr) { return pbmath::exp (-1.0 / (std::max (ms, 0.01) * 0.001 * sr)); }

// AM, as measured on stepped sines (MEASUREMENTS.md, Phase 3): the input's magnitude read once every 8 samples
// (counted from the start of processing; so a 3 kHz sine at 48 kHz, read at its zero crossings, gives nothing),
// through one-poles per read, the attack's time constant Attack ms and the release's Release + Attack / 85 ms;
// twice the result, held between reads. A gate: after 110 ms with the input under 5.5e-4 (about -65 dBFS) the
// follower reads nothing (it keeps releasing) until 1.92 ms after the input comes back
struct Follower
{
    static constexpr int tick = 8;
    static constexpr double gateLevel = 5.5e-4, gateHold = 0.110, gateOpen = 0.00192;
    double env = 0.0;
    int quiet = 0, opening = 0, holdSamples = 5280, openSamples = 92;
    bool closed = true;
    void prepare (double sr) { holdSamples = (int) std::lround (gateHold * sr); openSamples = (int) std::lround (gateOpen * sr); reset(); }
    void reset() { env = 0.0; quiet = 0; opening = 0; closed = true; }
    static double attackPole (double attackMs, double sr) { return pbmath::exp (-tick / (std::max (attackMs, 0.01) * 0.001 * sr)); }
    static double releasePole (double attackMs, double releaseMs, double sr)
    {
        return pbmath::exp (-tick / (std::max (releaseMs + attackMs / 85.0, 0.01) * 0.001 * sr));
    }
    // one sample: `read` on every 8th sample; a, r: the poles per read
    double next (float in, bool read, double a, double r)
    {
        const double x = std::abs ((double) in);
        if (x >= gateLevel)
        {
            quiet = 0;
            if (closed && ++opening >= openSamples) { closed = false; opening = 0; }
        }
        else if (! closed && ++quiet >= holdSamples) closed = true;
        if (read)
        {
            const double v = closed ? 0.0 : x;
            env = v + (v > env ? a : r) * (env - v);
        }
        return 2.0 * env;
    }
};

// PT: the detected pitch on the log scale about C4, smoothed; held through unvoiced parts
struct PitchFollower
{
    double value = 0.0, smoothed = 0.0;
    bool started = false;
    void reset() { value = 0.0; smoothed = 0.0; started = false; }
    double next (float hz, double smoothMs, int n, double sr)
    {
        if (hz > 0.0f) value = pbmath::log2 (hz / 261.6256) / 2.0;
        if (! started) { smoothed = value; started = true; }
        const double a = pbmath::exp (-(double) n / (std::max (smoothMs, 0.01) * 0.001 * sr));
        smoothed = value + a * (smoothed - value);
        return smoothed;
    }
};
}
