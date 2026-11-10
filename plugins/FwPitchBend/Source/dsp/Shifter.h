#pragma once
#include <cstdint>
#include <limits>
#include <vector>

// Flatwater Pitch Bend's pitch and formant shifter (MEASUREMENTS.md, Flatwater Pitch Bend, "the pitch and formant
// shifter"): a monophonic voice's pitch and spectral envelope moved independently, no latency reported. Pitch moves
// the output's f0 by 2^(pitch / 12); the envelope moves by the formant setting plus a share of the pitch's
// (clip (0.418 pitch, ±2.51) semitones); Flatten replaces the voice's pitch by one note (Pitch adds on top). At no
// shift the output is the input delayed by a fixed fraction of a sample (2.2 ms at 48 kHz).
//
// How (our own method, from the measured behaviour): pitch-synchronous overlap-add.
// - Detector, on the channel mix: YIN (cumulative-mean-normalised difference) on a 3 kHz low-passed copy decimated to
//   about 12 kHz, kept up to date sample by sample, refined at the full rate; a gate at -51 dBFS rms.
// - Pitch marks one period apart, each placed where the period before it best matches the period before the last
//   one (so they keep to the same place in the waveform). A grain's mark may be predicted whole periods ahead: a
//   grain only reads what has arrived, which keeps the shifted path's delay near the no-shift one.
// - Each output grain is the input around a mark, Hann-windowed (two output periods, at most one input period each
//   side) and read at the formant time scale 2^((F + beta) / 12), F + beta stopping at +-12 (measured); grains are
//   placed at the output period, the newest input period (measured when the grain starts) over 2^(P / 12), or the
//   Flatten note's.
// - Unvoiced noise is re-granulated with grains at the formant time scale; tonal but aperiodic input (chords) and
//   anything under the gate is passed through grains that rebuild the delayed input.
// - Voiced level: the measured level per setting and per shift the grains make (under Flatten that is the note
//   against the voice's pitch), reached by comparing the grains' own level with the input's. Away from Flatten it
//   also follows the voice's peakiness (the kurtosis of the last 20 ms): the reference's shifted voice comes out
//   quieter the peakier the waveform (low voices, wide formants), by a measured slope per Pitch and Formant.
// - Modulation: the totals move the output pitch and the Formant envelope; the controls' own values set the
//   envelope's share of Pitch, the level correction and the unvoiced path (measured: with every control at 0 and only
//   Pitch modulated the reference neither corrects the level nor shifts the envelope, and passes noise through).
// - At Pitch 0 / Formant 0 / Flatten off the wet path is the fixed fractional delay; the two paths cross-fade over
//   10 ms when a setting leaves or reaches zero.
//
// Everything runs per sample (no block-size dependence), allocates only in prepare(), and is deterministic.
// The analysis (the voice's pitch) is shared by the channels; detectedHz() hands it to the PT modulator.
namespace pb
{
class Shifter
{
public:
    void prepare (double sampleRate, int maxBlock, int channels);
    void reset();
    // in place, `channels` channels of n samples. pitchSt / formantSt: this block's shifts in semitones (fractional
    // with Fine), the totals with modulation; flattenHz > 0: the output's pitch forced to it before Pitch is added.
    // pitchControlSt / formantControlSt: the controls' own values before modulation (NaN: the same as the totals).
    // The totals set the output pitch and the envelope's Formant shift; the controls set the envelope's share of
    // Pitch, the level correction and whether unvoiced input is re-granulated (as measured on the reference)
    void process (float* const* data, int channels, int n, float pitchSt, float formantSt, float flattenHz,
                  float pitchControlSt = std::numeric_limits<float>::quiet_NaN(),
                  float formantControlSt = std::numeric_limits<float>::quiet_NaN());
    float detectedHz() const { return detected; }                // the voice's pitch now (0: none found)
    double delaySamples() const { return delay; }                // the no-shift delay at this rate

    // read-only views of the analysis (scorecard and tests)
    float aperiodicity() const { return (float) apShort; }       // YIN dip depth of the last analysis frame
    int lastGrainKind() const { return lastKind; }               // 0 none, 1 voiced, 2 noise, 3 pass-through

private:
    enum Kind { kNone = 0, kVoiced = 1, kNoise = 2, kPass = 3 };
    struct Grain
    {
        bool on = false;
        double u = 0.0;              // samples since the grain's start (fractional start)
        double pos = 0.0;            // input read position now (absolute samples)
        double scale = 1.0;          // input samples per output sample
        double fi = 1.0, fl = 0.0, fo = 1.0, len = 2.0;
        double invFi = 1.0, invFo = 1.0;
        float gain = 1.0f;
        int kind = 0;
        bool exact = false;          // reads the no-shift path's delayed input (unvoiced, not re-granulated)
    };

    void analyse (float mono);
    void evaluate();
    void updateMarks();
    void schedule (double ratio, double scale, double flattenHz);
    Grain* startGrain (int kind, double startT, double fi, double fl, double fo, double inCentre, double scale, float gain);
    double markPeriod (int last, double ahead) const;
    double periodAt (int last, double t) const;
    double readDelay (double H, double pin, double scale) const;
    double desiredVoicedHalf (double pout, double pin, double scale, double periods) const;
    float fade (double v) const;
    float readCubic (const std::vector<float>& ring, double p) const;

    double sr = 48000.0, delay = 105.53125;
    int channels = 2;

    // input rings (per channel) and the absolute sample counter
    std::vector<std::vector<float>> history;
    int ringMask = 0;
    int64_t now = -1;

    // the no-shift path: a fixed windowed-sinc fractional delay
    std::vector<float> fir, acc, dly;
    int firBase = 0;

    // analysis: gate and spectral-tilt sums, low-pass, decimated YIN (short and long windows), refinement, marks
    std::vector<float> monoRing, lpRing;
    double gateSum = 0.0, slopeSum = 0.0, slopeE = 0.0, quartSum = 0.0;
    int gateLen = 960, slopeLen = 480;
    float lastMono = 0.0f;
    double bq[2][5] {}, bqs[2][2] {};
    int decim = 4, decimPhase = 0;
    double fd = 12000.0;
    std::vector<float> coarse;
    int coarseMask = 0;
    int64_t coarseN = -1;
    int tmin = 12, tmax = 171, winS = 192, winL = 480, evalEvery = 8, evalPhase = 0;
    std::vector<double> dS, dL, cmS;
    double lpDelay = 6.7, smoothA = 0.12;
    double eS = 0.0;
    double apShort = 1.0, apLong = 1.0;
    bool gateOpen = false, voiced = false, noiseLike = false;
    int64_t gateOpenSince = 0;
    int onCount = 0, offCount = 0, jumpCount = 0;
    double period = 0.0, jumpRef = 0.0;          // smoothed input period (full-rate samples)
    float detected = 0.0f;
    double marks[8] {}, markPer[8] {};
    int markCount = 0, chainMarks = 0;
    bool chain = false;
    double markPred = 0.0, lastLpMark = 0.0;

    // grains and their scheduler
    static constexpr int maxGrains = 24;
    Grain grains[maxGrains];
    std::vector<float> fadeTab;
    bool sched = false, lastWasVoiced = false;
    double nextCreate = 0.0, nextStart = 0.0, nextFade = 0.0, lastCentreV = 0.0;
    double lastM = 0.0;                         // where the last voiced grain read (its centre, input samples)
    bool phaseSet = false;                      // lastM holds for the next voiced grain
    int lastChainMarks = 0;
    int lastKind = 0;
    uint32_t rng = 1;
    double lvIn = 0.0, lvOut = 0.0, levelA = 0.001, levelTarget = 1.0;
    int lvCount = 0;                            // samples measured by lvIn / lvOut since the gate opened
    float lastP = 1e9f, lastF = 1e9f, lastFlat = -1.0f;
    double lastPc = 1e9;                        // the Pitch control the level target was last set for
    double lastPeff = 1e9, lastBeta = 0.0;      // the shift the level target was last set for; this block's beta
    enum { kLevelFree = 0, kLevelSet = 1, kLevelFlatten = 2 };
    int levelMode = -1;
    bool regranulate = true;                    // unvoiced input re-granulated (a control or the Formant total set)
    double lnKurt = 0.83;                       // the voice's ln kurtosis, eased per voiced grain
    double levelSlope = 0.0;                    // dB per unit of ln kurtosis at this setting (levelTableDb)
    static double levelTableDb (double Peff, double Pc, double F, double beta, int mode, double* slope);
    double wetMix = 0.0;          // 0: the delay path, 1: the grains
    bool engaged = false;         // the grains have run: they keep running at no shift (as the reference's)
    // an input that stops changing (digital silence, or a constant) for flatRun samples after it played, read on an
    // 8-sample grid from the start of processing: the grains stop (the latch with them) and the pass-through becomes a
    // whole-sample delay of flatDelay (as the reference's; set in prepare)
    int flatRun = 1024, flatDelay = 39;
    int flatCount = 0;
    bool played = false, flushed = false;
    float lastIn[2] {};
};
}
