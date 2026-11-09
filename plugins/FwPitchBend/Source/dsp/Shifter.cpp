#include "Shifter.h"
#include "Math.h"
#include <algorithm>
#include <cmath>

// Flatwater Pitch Bend's shifter (the header has the method). Numbers marked "measured" come from
// measurements/fwpitchbend/shifter/SUMMARY.md; the rest are our own choices, tuned on its scorecard
// (nulltest/score_pitchbend_shifter.py).
namespace fwpitchbend
{
namespace
{
constexpr double pi = 3.14159265358979323846;

double besselI0 (double x)
{
    double s = 1.0, t = 1.0;
    for (int k = 1; k < 50; ++k) { t *= (x / (2.0 * k)) * (x / (2.0 * k)); s += t; if (t < 1e-17 * s) break; }
    return s;
}

int nextPow2 (int v) { int p = 1; while (p < v) p <<= 1; return p; }

// analysis
constexpr double lowpassHz = 3000.0;          // the detector's band
constexpr double coarseHz = 12000.0;          // its decimated rate (about)
constexpr double fminHz = 60.0, fmaxHz = 1250.0;     // (voice 70 .. 1000 Hz, with room at both ends)
constexpr double winShortS = 0.012, winLongS = 0.040, evalS = 0.00067;
constexpr double gateOpenMs = 7.943e-6;       // -51 dBFS rms (measured: shifted at -50, not at -52)
constexpr double gateCloseMs = 7.079e-6;      // -51.5
constexpr double voicedOn = 0.30, voicedKeep = 0.45;
constexpr double smoothS = 0.002;             // the detector period's smoothing time constant
constexpr double maxTrend = 0.0087;           // the marks' period trend reaches at most 15 cents
constexpr double readStepSt = 1.5;             // the Formant total (semitones) past which the grains read a period further back
constexpr double snapScale = 0.01;            // a time scale further from 1 reads the voiced grains at the marks (as any shift)
constexpr double periodReach = 0.03;          // a grain's period read from the marks: at most 50 cents from the newest
constexpr double noiseTiltHz = 1500.0, noiseAperiodic = 0.78;

// grains
constexpr double passHopS = 0.005, passFadeS = 0.0025;   // unvoiced / pass-through grains: 5 ms apart, 2.5 ms fades
constexpr double wetFadeS = 0.010;                         // the delay path <-> grains cross-fade
constexpr float noiseGain = 1.047f;           // +0.4 dB: re-granulated noise at the measured -0.6 dB
constexpr double levelS = 0.080;              // the voiced level's averaging time

// measured (levelgrid.json, printed by nulltest/measure_pitchbend_level_grid.py fit): the reference's voiced level
// (output minus input, dB) per Pitch and Formant total, with every control set, over vowels at 80 .. 300 Hz and sung
// and spoken phrases; a straight line in the ln kurtosis of the input (20 ms windows, energy-weighted), gridI its
// value at kurtRef (a typical voice) and gridS its slope. The grid is read bilinearly.
constexpr int gridNP = 13, gridNF = 11;
const double gridP[gridNP] = { -12, -9, -7, -5, -3, -1, 0, 1, 3, 5, 7, 9, 12 };
const double gridF[gridNF] = { -12, -9, -7, -5, -3, 0, 3, 5, 7, 9, 12 };
// the level (dB) per Pitch (rows) and Formant (columns) total at kurtRef, and its slope against ln kurtosis
const double gridI[gridNP][gridNF] = {
    { -3.38, -1.76, -0.79, -1.57, -2.47, -2.89, -3.53, -4.72, -5.91, -7.02, -8.63 },
    { 0.12, -0.16, -1.85, -0.99, -1.12, -2.15, -2.84, -3.17, -4.11, -5.27, -7.05 },
    { 0.77, 0.71, -0.01, -1.89, -1.16, -1.81, -2.49, -2.80, -3.24, -4.17, -5.99 },
    { 1.27, 0.94, 0.78, 0.04, -3.09, -1.27, -2.19, -2.71, -2.82, -3.30, -5.04 },
    { 1.03, 0.98, 0.77, 0.75, -0.42, -1.22, -1.79, -2.50, -2.72, -3.05, -4.30 },
    { 0.17, 0.29, 0.22, 0.11, -0.29, -0.53, -1.30, -2.22, -2.52, -2.83, -3.56 },
    { -0.53, 0.25, 0.22, -0.13, -0.27, 0.00, -0.86, -1.74, -2.47, -2.80, -3.20 },
    { 0.04, 0.17, 0.21, 0.04, 0.07, -0.64, -0.67, -1.34, -2.23, -2.74, -3.25 },
    { 0.39, 0.45, 0.62, 0.55, 0.22, -0.44, -1.11, -0.78, -1.53, -2.42, -2.89 },
    { 1.59, -0.47, 0.87, 1.07, 0.47, -0.01, -3.12, -1.00, -1.18, -2.03, -2.40 },
    { 2.18, 1.31, 0.11, 0.82, 1.04, 0.37, -0.59, -1.96, -0.95, -1.57, -1.68 },
    { 1.39, 2.34, 1.42, 0.14, 0.89, 0.80, 0.36, -0.57, -1.96, -1.04, -1.13 },
    { 2.87, 1.75, 2.30, 1.97, 0.65, 1.00, 0.72, 0.71, 0.04, -1.83, -3.39 }
};
const double gridS[gridNP][gridNF] = {
    { -2.86, -0.39, -2.06, -1.84, -1.35, -1.79, -1.61, -1.31, -1.02, -0.78, -0.39 },
    { -2.59, -2.60, -1.07, -1.49, -1.11, -0.79, -2.05, -1.89, -1.66, -1.43, -1.19 },
    { -1.26, -1.43, -2.18, -1.08, -2.22, -1.82, -1.36, -2.00, -2.05, -1.90, -1.57 },
    { 0.29, -0.20, -0.81, -1.28, -1.92, -2.38, -1.93, -1.62, -2.07, -2.32, -1.99 },
    { 0.35, 1.38, 0.49, -0.54, -0.51, -2.27, -2.01, -1.47, -1.74, -1.89, -1.80 },
    { 0.68, 0.24, -0.38, -0.76, -0.91, -0.17, -2.36, -1.53, -1.57, -1.56, -1.68 },
    { -0.38, -0.44, -0.51, -1.47, -2.36, 0.00, -1.99, -1.31, -1.21, -1.54, -1.67 },
    { 0.16, -0.91, -1.24, -2.23, -2.15, 0.06, -1.40, -1.46, -1.02, -1.30, -1.89 },
    { -0.58, -0.51, -1.15, -1.62, -2.14, -2.39, -0.75, -0.88, -0.79, -0.93, -1.55 },
    { -0.68, -0.30, 0.12, -1.15, -1.78, -2.65, -1.84, -1.28, -0.87, -0.81, -0.96 },
    { 0.38, -0.18, 0.63, -0.01, -0.70, -1.75, -1.76, -0.93, -1.91, -1.17, -0.82 },
    { 1.42, 0.35, 0.18, 0.79, -0.16, -0.51, -1.89, -1.78, -1.03, -2.23, -2.06 },
    { 0.92, 1.33, 0.05, 0.39, 1.03, -0.19, -0.20, -1.33, -2.15, -0.98, -2.48 }
};
constexpr double kurtRef = 0.83;
double grid (const double (&t)[gridNP][gridNF], double p, double f)
{
    int i = 0, j = 0;
    while (i < gridNP - 2 && p > gridP[i + 1]) ++i;
    while (j < gridNF - 2 && f > gridF[j + 1]) ++j;
    const double u = std::max (0.0, std::min (1.0, (p - gridP[i]) / (gridP[i + 1] - gridP[i])));
    const double v = std::max (0.0, std::min (1.0, (f - gridF[j]) / (gridF[j + 1] - gridF[j])));
    return (1 - u) * ((1 - v) * t[i][j] + v * t[i][j + 1]) + u * ((1 - v) * t[i + 1][j] + v * t[i + 1][j + 1]);
}
}

void Shifter::prepare (double sampleRate, int, int ch)
{
    sr = sampleRate > 1000.0 ? sampleRate : 48000.0;
    channels = std::max (1, ch);
    // measured: 105.531 samples at 48 kHz, 211.063 at 96 kHz, 120.438 at 44.1 kHz
    delay = std::abs (sr - 44100.0) < 1.0 ? 120.4375 : 105.53125 * sr / 48000.0;

    const int ringLen = nextPow2 (std::max (8192, (int) (0.17 * sr)));
    ringMask = ringLen - 1;
    history.assign ((size_t) channels, std::vector<float> ((size_t) ringLen, 0.0f));
    monoRing.assign ((size_t) ringLen, 0.0f);
    lpRing.assign ((size_t) ringLen, 0.0f);
    acc.assign ((size_t) channels, 0.0f);
    dly.assign ((size_t) channels, 0.0f);

    // the no-shift path: 32-tap Kaiser-windowed sinc at the measured delay (flat to about 20 kHz at 48 kHz)
    const int T = 32;
    const int di = (int) std::floor (delay);
    const double frac = delay - di;
    fir.assign ((size_t) T, 0.0f);
    firBase = di - (T / 2 - 1);
    if (frac < 1e-9)
        fir[(size_t) (T / 2 - 1)] = 1.0f;
    else
    {
        const double beta = 8.0, i0b = besselI0 (beta);
        double sum = 0.0;
        std::vector<double> h ((size_t) T);
        for (int k = 0; k < T; ++k)
        {
            const double x = (firBase + k) - delay;
            const double xr = x / (T / 2.0);
            const double w = besselI0 (beta * std::sqrt (std::max (0.0, 1.0 - xr * xr))) / i0b;
            h[(size_t) k] = (std::abs (x) < 1e-12 ? 1.0 : pbmath::sin (pi * x) / (pi * x)) * w;
            sum += h[(size_t) k];
        }
        for (int k = 0; k < T; ++k) fir[(size_t) k] = (float) (h[(size_t) k] / sum);
    }

    // the detector's low-pass: 4th-order Butterworth (two biquads)
    const double qs[2] = { 0.54119610, 1.30656296 };
    const double w0 = 2.0 * pi * lowpassHz / sr, cw = pbmath::cos (w0), sw = pbmath::sin (w0);
    for (int b = 0; b < 2; ++b)
    {
        const double al = sw / (2.0 * qs[b]), a0 = 1.0 + al;
        bq[b][0] = (1.0 - cw) / 2.0 / a0; bq[b][1] = (1.0 - cw) / a0; bq[b][2] = (1.0 - cw) / 2.0 / a0;
        bq[b][3] = -2.0 * cw / a0; bq[b][4] = (1.0 - al) / a0;
    }
    lpDelay = 2.6131 / (2.0 * pi * lowpassHz) * sr;     // its group delay at low frequencies

    decim = std::max (1, (int) std::lround (sr / coarseHz));
    fd = sr / decim;
    tmin = std::max (2, (int) std::floor (fd / fmaxHz));
    tmax = (int) std::ceil (fd / fminHz) + 1;
    winS = (int) std::lround (winShortS * fd);
    winL = (int) std::lround (winLongS * fd);
    evalEvery = std::max (1, (int) std::lround (evalS * fd));
    smoothA = 1.0 - pbmath::exp (-evalEvery / (smoothS * fd));
    const int cl = nextPow2 (winL + tmax + 8);
    coarse.assign ((size_t) cl, 0.0f);
    coarseMask = cl - 1;
    dS.assign ((size_t) tmax + 2, 0.0);
    dL.assign ((size_t) tmax + 2, 0.0);
    cmS.assign ((size_t) tmax + 2, 1.0);
    gateLen = (int) std::lround (0.020 * sr);
    slopeLen = (int) std::lround (0.010 * sr);

    levelA = 1.0 - pbmath::exp (-1.0 / (levelS * sr));
    // (measured: the run 1029 .. 1030 samples at 48 kHz, 976 .. 980 at 44.1, 2101 .. 2400 at 96; the delay 39 samples
    // at 44.1 and 48 kHz, 78 at 96)
    flatRun = (int) std::lround (1024.0 * sr / 48000.0);
    flatDelay = 39 * std::max (1, (int) std::lround (sr / 48000.0));
    fadeTab.assign (1026, 0.0f);
    for (int i = 0; i <= 1025; ++i) fadeTab[(size_t) i] = (float) (0.5 - 0.5 * pbmath::cos (pi * std::min (1024, i) / 1024.0));
    reset();
}

void Shifter::reset()
{
    for (auto& h : history) std::fill (h.begin(), h.end(), 0.0f);
    std::fill (monoRing.begin(), monoRing.end(), 0.0f);
    std::fill (lpRing.begin(), lpRing.end(), 0.0f);
    std::fill (coarse.begin(), coarse.end(), 0.0f);
    std::fill (dS.begin(), dS.end(), 0.0);
    std::fill (dL.begin(), dL.end(), 0.0);
    now = -1;
    coarseN = -1;
    decimPhase = 0; evalPhase = 0;
    for (auto& s : bqs) s[0] = s[1] = 0.0;
    gateSum = slopeSum = slopeE = eS = quartSum = 0.0;
    lastMono = 0.0f;
    apShort = apLong = 1.0;
    gateOpen = voiced = noiseLike = false;
    gateOpenSince = 0;
    onCount = offCount = jumpCount = 0;
    period = jumpRef = 0.0;
    detected = 0.0f;
    markCount = 0; chain = false; markPred = 0.0;
    for (auto& g : grains) g.on = false;
    sched = false; lastWasVoiced = false;
    nextCreate = nextStart = nextFade = lastCentreV = 0.0;
    lastKind = kNone;
    rng = 0x2545F491u;
    wetMix = 0.0;
    engaged = false;
    flatCount = 0; played = flushed = false; lastIn[0] = lastIn[1] = 0.0f;
    lvIn = lvOut = 0.0;
    lvCount = 0;
    lastPeff = 1e9;
    lnKurt = kurtRef;
}

float Shifter::fade (double v) const
{
    const double x = std::min (1.0, std::max (0.0, v)) * 1024.0;
    const int i = (int) x;
    const float f = (float) (x - i);
    return fadeTab[(size_t) i] + f * (fadeTab[(size_t) i + 1] - fadeTab[(size_t) i]);
}

float Shifter::readCubic (const std::vector<float>& r, double p) const
{
    const double fl = std::floor (p);
    const int64_t i = (int64_t) fl;
    const float f = (float) (p - fl);
    const float x0 = r[(size_t) ((i - 1) & ringMask)], x1 = r[(size_t) (i & ringMask)];
    const float x2 = r[(size_t) ((i + 1) & ringMask)], x3 = r[(size_t) ((i + 2) & ringMask)];
    return x1 + 0.5f * f * (x2 - x0 + f * (2.0f * x0 - 5.0f * x1 + 4.0f * x2 - x3 + f * (3.0f * (x1 - x2) + x3 - x0)));
}

// ---- analysis ---------------------------------------------------------------------------------------------------
void Shifter::analyse (float mono)
{
    const int i = (int) (now & ringMask);
    // the gate (20 ms rms) and the spectral tilt (10 ms: slope energy against energy)
    {
        const float old = monoRing[(size_t) ((now - gateLen) & ringMask)];
        gateSum += (double) mono * mono - (double) old * old;
        const double m2 = (double) mono * mono, o2 = (double) old * old;
        quartSum += m2 * m2 - o2 * o2;
        const float o1 = monoRing[(size_t) ((now - slopeLen) & ringMask)], o0 = monoRing[(size_t) ((now - slopeLen - 1) & ringMask)];
        const float dn = mono - lastMono, dold = o1 - o0;
        slopeSum += (double) dn * dn - (double) dold * dold;
        slopeE += (double) mono * mono - (double) o1 * o1;
        monoRing[(size_t) i] = mono;
        lastMono = mono;
        const double ms = std::max (0.0, gateSum) / gateLen;
        if (! gateOpen && ms >= gateOpenMs) { gateOpen = true; gateOpenSince = now; lnKurt = kurtRef; lvCount = 0; }
        else if (gateOpen && ms < gateCloseMs) { gateOpen = false; voiced = false; chain = false; onCount = 0; }
    }
    // low-pass for the detector and the marks
    double v = mono;
    for (int b = 0; b < 2; ++b)
    {
        const double y = bq[b][0] * v + bqs[b][0];
        bqs[b][0] = bq[b][1] * v - bq[b][3] * y + bqs[b][1];
        bqs[b][1] = bq[b][2] * v - bq[b][4] * y;
        v = y;
    }
    if (std::abs (v) < 1e-25) v = 0.0;
    lpRing[(size_t) i] = (float) v;

    if (++decimPhase >= decim)
    {
        decimPhase = 0;
        ++coarseN;
        const float c = (float) v;
        coarse[(size_t) (coarseN & coarseMask)] = c;
        auto at = [this] (int64_t k) { return coarse[(size_t) (k & coarseMask)]; };
        const float cs = at (coarseN - winS), cL = at (coarseN - winL);
        eS += (double) c * c - (double) cs * cs;
        for (int t = 1; t <= tmax; ++t)
        {
            const float a = c - at (coarseN - t);
            const float bS = cs - at (coarseN - winS - t);
            const float bL = cL - at (coarseN - winL - t);
            dS[(size_t) t] += (double) a * a - (double) bS * bS;
            dL[(size_t) t] += (double) a * a - (double) bL * bL;
        }
        if (++evalPhase >= evalEvery) { evalPhase = 0; evaluate(); }
    }
    updateMarks();
}

void Shifter::evaluate()
{
    // short window: the period (YIN: cumulative-mean-normalised difference, the first dip near the deepest)
    double pr = 0.0;
    if (eS > 1e-11 * winS)
    {
        double cum = 0.0;
        for (int t = 1; t <= tmax; ++t)
        {
            const double d = std::max (0.0, dS[(size_t) t]);
            cum += d;
            cmS[(size_t) t] = cum > 0.0 ? d * t / cum : 1.0;
        }
        double gmin = 1e9;
        int arg = tmin;
        for (int t = tmin; t < tmax; ++t) if (cmS[(size_t) t] < gmin) { gmin = cmS[(size_t) t]; arg = t; }
        const double thr = std::min (0.2, gmin + 0.12);
        int t = -1;
        for (int k = tmin; k < tmax; ++k) if (cmS[(size_t) k] < thr) { t = k; break; }
        if (t < 0) t = arg;
        else while (t + 1 < tmax && cmS[(size_t) t + 1] < cmS[(size_t) t]) ++t;
        apShort = cmS[(size_t) t];
        if (t <= tmin || t >= tmax - 1) apShort = std::max (apShort, 0.99);
        else if (apShort < voicedKeep)
        {
            const double a = cmS[(size_t) t - 1], b = cmS[(size_t) t], cc = cmS[(size_t) t + 1];
            const double den = a - 2.0 * b + cc;
            const double dt = std::abs (den) > 1e-12 ? std::max (-0.5, std::min (0.5, 0.5 * (a - cc) / den)) : 0.0;
            // refine at the full rate: the plain difference function around the coarse lag
            const double tc = (t + dt) * decim;
            const int K = decim / 2 + 2, t0 = (int) std::lround (tc);
            const int wf = winS * decim;
            double best = 1e300, dm = 0.0, dp = 0.0;
            int bt = t0;
            double vals[64];
            const int n = std::min (63, 2 * K + 1);
            for (int k = 0; k < n; ++k)
            {
                const int tau = t0 - K + k;
                double s = 0.0;
                for (int j = 0; j < wf; ++j)
                {
                    const float e = lpRing[(size_t) ((now - j) & ringMask)] - lpRing[(size_t) ((now - j - tau) & ringMask)];
                    s += (double) e * e;
                }
                vals[k] = s;
                if (s < best) { best = s; bt = k; }
            }
            double frac = 0.0;
            if (bt > 0 && bt < n - 1)
            {
                dm = vals[bt - 1]; dp = vals[bt + 1];
                const double den2 = dm - 2.0 * best + dp;
                frac = den2 > 1e-30 ? std::max (-0.5, std::min (0.5, 0.5 * (dm - dp) / den2)) : 0.0;
            }
            pr = (t0 - K + bt) + frac;
        }
    }
    else apShort = 1.0;

    // long window: how aperiodic (noise against chords)
    if (eS > 1e-11 * winS)
    {
        double cum = 0.0, mn = 1e9;
        for (int t = 1; t <= tmax; ++t)
        {
            const double d = std::max (0.0, dL[(size_t) t]);
            cum += d;
            const double c = cum > 0.0 ? d * t / cum : 1.0;
            if (t >= tmin && t < tmax && c < mn) mn = c;
        }
        apLong = mn;
    }
    else apLong = 1.0;

    const double tilt = (2.0 * pi * noiseTiltHz / sr) * (2.0 * pi * noiseTiltHz / sr);
    noiseLike = gateOpen && (slopeSum > tilt * std::max (slopeE, 1e-30)
                             || (apLong > noiseAperiodic && now - gateOpenSince > (int64_t) winL * decim));

    // voicing and the smoothed period
    const bool cand = gateOpen && pr > 0.0 && apShort < voicedOn;
    if (! voiced)
    {
        onCount = cand ? onCount + 1 : 0;
        if (onCount >= 2) { voiced = true; period = pr; jumpCount = 0; offCount = 0; chain = false; }
    }
    else
    {
        const bool keep = gateOpen && pr > 0.0 && apShort < voicedKeep;
        if (! keep)
        {
            if (++offCount >= 3) { voiced = false; chain = false; onCount = 0; }
        }
        else
        {
            offCount = 0;
            const double c = 1200.0 * pbmath::log2 (pr / period);
            if (std::abs (c) < 60.0) { period += smoothA * (pr - period); jumpCount = 0; }
            else
            {
                if (jumpCount > 0 && std::abs (1200.0 * pbmath::log2 (pr / jumpRef)) < 40.0) ++jumpCount;
                else { jumpCount = 1; jumpRef = pr; }
                const bool octave = std::abs (std::abs (c) - 1200.0) < 100.0;
                if (jumpCount >= (octave ? 12 : 3))
                {
                    // a jump: the marks keep their chain unless the period moves by more than a fourth
                    if (std::abs (pr / period - 1.0) > 0.25) chain = false;
                    period = pr; jumpCount = 0;
                }
            }
        }
    }
    detected = voiced && period > 0.0 ? (float) (sr / period) : 0.0f;
}

void Shifter::updateMarks()
{
    if (! voiced || period <= 0.0) { chain = false; return; }
    const double P = period, R = std::max (2.0, 0.25 * P);
    auto pick = [this] (int64_t a, int64_t b)
    {
        int64_t best = a;
        float bv = lpRing[(size_t) (a & ringMask)];
        for (int64_t k = a + 1; k <= b; ++k)
        {
            const float v = lpRing[(size_t) (k & ringMask)];
            if (v > bv) { bv = v; best = k; }
        }
        double frac = 0.0;
        if (best > a && best < b)
        {
            const double ym = lpRing[(size_t) ((best - 1) & ringMask)], yp = lpRing[(size_t) ((best + 1) & ringMask)];
            const double den = ym - 2.0 * bv + yp;
            if (den < -1e-30) frac = std::max (-0.5, std::min (0.5, 0.5 * (ym - yp) / den));
        }
        return (double) best + frac;
    };
    if (! chain)
    {
        const double m = pick ((int64_t) std::ceil (now - R - P), (int64_t) std::floor (now - R));
        markCount = 0;
        chainMarks = 1;
        markPer[0] = P;
        marks[markCount++ % 8] = m - lpDelay;
        markPred = m + P;
        lastLpMark = m;
        chain = true;
        return;
    }
    // the next mark: the lag (near one period) at which the period before it best matches the period before the last
    // mark (normalised cross-correlation, parabolic peak), so the marks keep to the same place in the waveform
    const double R2 = 0.1 * P + 2.0;
    if ((double) now < markPred + R2) return;
    const int64_t ref = (int64_t) std::lround (lastLpMark);
    const int wc = std::max (8, (int) (0.6 * P));
    const int64_t lo = (int64_t) std::ceil (markPred - R2), hi = (int64_t) std::floor (markPred + R2);
    double best = -1e300, cprev = 0.0, cbest = 0.0, cnext = 0.0, cm1 = 0.0;
    int64_t bt = lo;
    bool haveNext = false;
    for (int64_t c = lo; c <= hi; ++c)
    {
        double xy = 0.0, yy = 1e-30;
        for (int j = 1; j <= wc; ++j)
        {
            const float a = lpRing[(size_t) ((ref - j) & ringMask)], b = lpRing[(size_t) ((c - j) & ringMask)];
            xy += (double) a * b; yy += (double) b * b;
        }
        const double v = xy / std::sqrt (yy);
        if (v > best) { best = v; bt = c; cprev = cm1; cbest = v; haveNext = false; }
        else if (c == bt + 1) { cnext = v; haveNext = true; }
        cm1 = v;
    }
    double frac = 0.0;
    if (bt > lo && haveNext)
    {
        const double den = cprev - 2.0 * cbest + cnext;
        if (den < -1e-30) frac = std::max (-0.5, std::min (0.5, 0.5 * (cprev - cnext) / den));
    }
    const double m = lastLpMark + ((double) (bt - ref) + frac);
    markPer[markCount % 8] = m - lastLpMark;
    ++chainMarks;
    marks[markCount++ % 8] = m - lpDelay;
    if (markCount >= 16) markCount = 8 + (markCount % 8);
    markPred = m + P;
    lastLpMark = m;
}

double Shifter::markPeriod (int last, double ahead) const
{
    // a straight line through the last spacings (up to five), read `ahead` periods after the last mark
    const int n = std::min (5, chainMarks - 1);
    double p;
    if (n < 2) p = markPer[last];
    else
    {
        double sx = 0.0, sy = 0.0, sxx = 0.0, sxy = 0.0;
        for (int k = 0; k < n; ++k)
        {
            const double x = -k, y = markPer[(last - k + 8) % 8];
            sx += x; sy += y; sxx += x * x; sxy += x * y;
        }
        const double b = (n * sxy - sx * sy) / (n * sxx - sx * sx), a = (sy - b * sx) / n;
        // (the trend's reach is limited: a step in the pitch must not overshoot)
        const double pk = markPer[last], lim = maxTrend * pk;
        p = std::max (pk - lim, std::min (pk + lim, a + b * (ahead - 0.5)));
    }
    return std::abs (p / period - 1.0) < 0.15 ? p : period;
}

double Shifter::periodAt (int last, double t) const
{
    // the input period at input time t: the newest spacings of the marks (each at the midpoint of its two marks)
    // fitted by a straight line in time and read at t; past the newest, along the line for at most two periods and
    // by at most periodReach
    const int n = std::min (4, chainMarks - 1);
    if (n < 1) return period;
    const double pk = markPer[last];
    double p = pk;
    if (n >= 2)
    {
        const double t0 = marks[last] - 0.5 * pk;
        double sx = 0.0, sy = 0.0, sxx = 0.0, sxy = 0.0;
        for (int k = 0; k < n; ++k)
        {
            const int i = (last - k + 8) % 8;
            const double x = marks[i] - 0.5 * markPer[i] - t0, y = markPer[i];
            sx += x; sy += y; sxx += x * x; sxy += x * y;
        }
        const double den = n * sxx - sx * sx;
        const double b = den > 1e-9 ? (n * sxy - sx * sy) / den : 0.0, a = (sy - b * sx) / n;
        p = a + b * std::min (t - t0, 2.0 * pk);
        p = std::max (pk * (1.0 - periodReach), std::min (pk * (1.0 + periodReach), p));
    }
    return std::abs (p / period - 1.0) < 0.15 ? p : period;
}

double Shifter::readDelay (double H, double pin, double scale) const
{
    // how far the voiced grains read behind their centres (samples). Measured with clicks on steady vowels and pulse
    // trains at 150 and 200 Hz, the reference's grains are centred on the pulses and read its no-shift delay less
    // F / 12 of an input period behind (one period per octave of Formant up; within 1 .. 7 samples over Formant -6 ..
    // +1 at both, to +2.5 at 150 Hz); at no time scale, the no-shift delay itself. Past Formant +1.5 it reads further
    // back by an amount that depends on the voice (from the vibrato lags: one input period more, and 1.2 periods per
    // unit of time scale over 1); never under what the half grain drifts (a grain reads only what has arrived)
    if (lastF > readStepSt) return pin * (1.0 + 1.2 * (scale - 1.0));
    if (std::abs (scale - 1.0) > snapScale) return std::max (delay - lastF / 12.0 * pin, H * std::abs (scale - 1.0) + 4.0);
    return delay;
}

// ---- grains -----------------------------------------------------------------------------------------------------
double Shifter::desiredVoicedHalf (double pout, double pin, double scale, double periods) const
{
    // a Hann grain 2 x periods output periods long (overlap-add sums to periods), but never covering more than one
    // input period on each side of its mark once read at the formant time scale (no extra pulses when pitched down)
    return std::max (8.0, std::min (periods * pout, pin / scale));
}

Shifter::Grain* Shifter::startGrain (int kind, double startT, double fi, double fl, double fo, double inCentre, double scale, float gain)
{
    Grain* g = nullptr;
    for (auto& q : grains) if (! q.on) { g = &q; break; }
    if (g == nullptr) return nullptr;
    g->on = true;
    g->exact = false;
    g->fi = fi; g->fl = fl; g->fo = fo; g->len = fi + fl + fo;
    g->invFi = 1.0 / fi; g->invFo = 1.0 / fo;
    g->u = (double) now - startT;
    const double centre = startT + 0.5 * g->len;
    g->scale = scale;
    g->pos = inCentre + ((double) now - centre) * scale;
    g->gain = gain;
    g->kind = kind;
    lastKind = kind;
    return g;
}

void Shifter::schedule (double ratio, double scale, double flattenHz)
{
    int guard = 0;
    while ((double) now >= nextCreate && guard++ < 8)
    {
        const int last = markCount > 0 ? (markCount - 1) % 8 : 0;
        const bool canVoice = gateOpen && voiced && chain && markCount > 0;
        const int kind = canVoice ? kVoiced : (gateOpen && noiseLike && regranulate ? kNoise : kPass);
        if (kind == kVoiced)
        {
            // the read point: a lattice one input period apart, readDelay behind the grain's centre, and the grains
            // spaced by the input period there (the marks' spacing, read at that time): at no shift the output is the
            // input delayed, and a moving pitch comes out readDelay late (measured on vibrato, as the reference's).
            // Shifted (the formant time scale away from 1, or the spacing not the input's period) the read point is a
            // mark (the pulse at the grain's centre: off it, neighbouring grains put one pulse in twice, the envelope
            // ripples) and the spacing follows the marks'
            const double p0 = markPeriod (last, 0.5);
            if (! lastWasVoiced || chainMarks < lastChainMarks) phaseSet = false;
            lastChainMarks = chainMarks;
            const bool snap = std::abs (scale - 1.0) > snapScale || std::abs (ratio - 1.0) > 1e-6 || flattenHz > 0.0;
            double pout = 0.0, H = 0.0, Hd = 0.0, o = 0.0, m = 0.0, fi = 0.0;
            double pin = phaseSet ? periodAt (last, lastM + 0.5 * p0) : p0;
            for (int pass = 0; pass < 2; ++pass)
            {
                pout = flattenHz > 0.0 ? sr / (flattenHz * ratio) : pin / ratio;
                pout = std::max (16.0, std::min (pout, 4.0 * sr / fminHz));
                // (under Flatten the grains are twice as long: measured on speech flattened up an octave, the
                // reference's output periods are as alike as ours are with grains of four periods, 0.95 median
                // correlation of one period with the next, 0.92 with two; the octave readings of ours go from 11 % of
                // the voiced frames to 1.5 %. Elsewhere the longer grains cost: sung gross errors and the onsets)
                H = Hd = desiredVoicedHalf (pout, pin, scale, flattenHz > 0.0 ? 2.0 : 1.0);
                // (dmin: what the half grain drifts, less a little: a read past the newest input is held there, a few
                // samples of a fade at most)
                const double dreq = readDelay (H, pin, scale), dmin = std::max (4.0, H * std::abs (scale - 1.0) + 4.0 - 0.1 * pin);
                fi = 0.0;
                if (lastWasVoiced) o = std::max (lastCentreV + pout, (double) now + 8.0);
                else o = std::max (nextStart, (double) now) + H;
                if (phaseSet)
                {
                    // (the delay keeps between readDelay and a period more; at no pitch shift, where it does not move,
                    // about readDelay: from a quarter period under it (half a period, read at the marks), so rounding
                    // does not repeat a period)
                    const bool still = std::abs (ratio - 1.0) < 1e-6 && flattenHz <= 0.0;
                    const double lo = still ? std::max (dmin, dreq - (snap ? 0.5 : 0.25) * pin) : dreq;
                    m = lastM + pin * std::floor ((o - lo - lastM) / pin);
                    if (o - m < dmin) m -= pin;
                    if (snap)
                    {
                        const double mk = marks[last] + std::round ((m - marks[last]) / p0) * p0;
                        m = o - mk >= dmin ? mk : mk - p0;
                        const double k = std::round ((m - lastM) / pin);
                        if (flattenHz <= 0.0 && k >= 1.0 && std::abs ((m - lastM) / (k * pin) - 1.0) < 0.1)
                        {
                            const double o2 = std::max (lastCentreV + (m - lastM) / (k * ratio), (double) now + 8.0);
                            if (o2 - m >= dmin) o = o2;
                        }
                    }
                }
                else if (snap)
                {
                    m = marks[last] + std::round ((o - dreq - marks[last]) / p0) * p0;
                    if (! lastWasVoiced)
                    {
                        // (a voice's first grain moves to the mark readDelay before it, its fade-in shorter; under a
                        // tenth of a grain, to the next mark)
                        const double st = std::max (nextStart, (double) now);
                        m = marks[last] + std::floor ((o - dreq - marks[last]) / p0) * p0;
                        if (m + dreq < st + 0.1 * H) m += p0;
                        o = m + dreq; fi = o - st;
                    }
                    else if (o - m < dmin) m -= p0;
                }
                else m = o - dreq;
                if (lastWasVoiced) H = std::max (8.0, std::min (H, o - (double) now));
                pin = periodAt (last, m - 0.5 * pin);
            }
            phaseSet = true; lastM = m;
            // level: the measured level per setting (levelTarget), reached by comparing the voiced grains' own
            // level with the input's over the last 80 ms (the level does not depend on the input's)
            // (the target follows the shift this grain makes: under Flatten it is not the Pitch setting)
            const double peff = 12.0 * pbmath::log2 (pin / pout);
            if (std::abs (peff - lastPeff) > 0.02)
            {
                lastPeff = peff;
                levelTarget = pbmath::pow10 (levelTableDb (peff, lastPc, lastF, lastBeta, levelMode, &levelSlope) / 20.0);
            }
            // (and the voice's peakiness: the kurtosis of the last 20 ms, a peakier waveform coming out quieter; not
            // while those 20 ms reach back to before the gate opened, where the onset itself reads as peaky; within
            // the range measured; eased in over a few grains)
            if (gateSum > 1e-30 && now - gateOpenSince >= gateLen)
            {
                const double ms = gateSum / gateLen, k = std::max (0.0, quartSum) / gateLen / (ms * ms);
                lnKurt += 0.25 * (pbmath::log (std::max (1.5, std::min (9.5, k))) - lnKurt);
            }
            const double kc = pbmath::pow10 (levelSlope * (lnKurt - kurtRef) / 20.0);
            float gv = (float) (levelTarget * kc);
            // (not until 20 ms are measured since the gate opened: a voice's first grain, its fade-in short, reads
            // as quiet and the correction overshot)
            if (lvCount >= gateLen && lvIn > 1e-20 && lvOut > 1e-20) gv = (float) std::max (0.1, std::min (4.0, levelTarget * kc * std::sqrt (lvIn / lvOut)));
            if (fi > 0.0) startGrain (kVoiced, o - fi, fi, 0.0, H, m, scale, gv);
            else startGrain (kVoiced, o - H, H, 0.0, H, m, scale, gv);
            lastCentreV = o;
            nextCreate = std::max (o + pout - 1.1 * Hd - 1.0, (double) now + 1.0);
            nextStart = o; nextFade = H;
            lastWasVoiced = true;
        }
        else
        {
            const double fu = passFadeS * sr;
            double start = nextStart, fi = nextFade;
            if (start < (double) now - fi + 1.0 || fi < 8.0) { start = (double) now; fi = fu; }
            rng = rng * 1664525u + 1013904223u;
            const double h = passHopS * sr * (0.75 + 0.5 * (rng >> 8) / 16777216.0);
            const double fl = std::max (0.0, h - fi), fo = fu;
            const double half = 0.5 * (fi + fl + fo);
            const double sc = kind == kNoise ? scale : 1.0;
            const double du = kind == kPass ? delay : std::max (delay, half * std::abs (sc - 1.0) + 4.0);
            Grain* g = startGrain (kind, start, fi, fl, fo, start + half - du, sc, kind == kNoise ? noiseGain : 1.0f);
            // (unvoiced input not re-granulated is the delayed input itself: these grains read the no-shift path)
            if (g != nullptr && kind == kPass && gateOpen && noiseLike) g->exact = true;
            nextStart = start + fi + fl; nextFade = fo;
            nextCreate = nextStart;
            lastWasVoiced = false;
        }
    }
}

namespace
{
double interp (const double* xs, const double* ys, int n, double x)
{
    if (x <= xs[0]) return ys[0];
    for (int i = 1; i < n; ++i)
        if (x <= xs[i]) return ys[i - 1] + (ys[i] - ys[i - 1]) * (x - xs[i - 1]) / (xs[i] - xs[i - 1]);
    return ys[n - 1];
}
}

double Shifter::levelTableDb (double Peff, double Pc, double F, double beta, int mode, double* slope)
{
    // measured (level.json, level_flatten.json, control.json, levelgrid.json): the reference's output minus input level
    // on voice. Peff: the shift the grains really make (input period over output period, in semitones: the Pitch
    // total, or under Flatten the note against the voice); Pc: the Pitch control. *slope: how the level moves with the
    // voice's peakiness (dB per unit of ln kurtosis, about kurtRef).
    // - kLevelSet (a control or the Formant total away from 0, no Flatten): the grid per Pitch and Formant total
    //   (+-12), the fit's value at kurtRef, and its slope; past +-12 the Pitch curve (py) from the control at +-12
    //   with the rest modulated. With the Pitch control set and the total near 0 the grains still run: the level there
    //   lies between those at -1 and +1.
    // - kLevelFlatten: two parts, one following Peff (-36 .. +30 measured), one the Pitch control (which also sets
    //   the grain time scale). Inside +-12 the two add up to the Pitch table. The Formant total: the mean of adding
    //   its curve and of reading it at F + beta. No peakiness term (measured on Flatten it did not help).
    // - kLevelFree (every control at 0, only the Pitch total moved by modulation): the reference corrects nothing;
    //   its level per Pitch total (uy) is matched here.
    static const double px[] = { -24, -19, -16, -12, -7, -1, 0, 1, 5, 12, 13, 16, 19, 24 };
    static const double py[] = { -8.80, -5.85, -4.05, -2.65, -1.44, -0.77, 0.0, -0.69, 0.35, 1.26, 0.95, 1.55, 1.78, 2.50 };
    static const double bx[] = { -12, -6, 0, 6, 12 }, by[] = { 0.65, 0.65, 0.0, -0.60, -0.27 };
    static const double ex[] = { -36, -24, -18, -12, 12, 18, 24, 30 }, ey[] = { -15.28, -9.67, -7.20, -3.30, 1.53, 2.23, 3.45, 3.87 };
    static const double ux[] = { -24, -19, -16, -13, -12, -7, -5, -3, -1, 1, 3, 5, 7, 12, 13, 16, 19, 24 };
    static const double uy[] = { -1.43, -0.03, 0.93, 1.64, 1.90, 1.35, 1.98, 2.50, 2.51, 2.47, 3.66, 4.29, 3.91, 2.85, 4.46, 4.84, 4.04, 4.50 };
    static const double fx[] = { -12, -7, 0, 7, 12 }, fy[] = { -0.49, 0.27, 0.0, -2.33, -3.22 };
    *slope = 0.0;
    // (towards no shift the level falls to the input's: measured on a vowel at +-0.02 .. +-1 semitone, 0.27 of the
    // level at +-1 by 0.1 semitone, 0.86 by 0.5)
    if (mode == kLevelFree) return interp (ux, uy, 18, Peff) * pbmath::tanh (std::abs (Peff) / 0.36);
    const double pc = std::max (-12.0, std::min (12.0, Peff)), fc = std::max (-12.0, std::min (12.0, F));
    if (mode == kLevelFlatten)
    {
        const double tp = (std::abs (Peff) <= 12.0 ? interp (px, py, 14, Peff) - interp (bx, by, 5, Peff) : interp (ex, ey, 8, Peff))
                          + interp (bx, by, 5, Pc);
        const double tf = interp (fx, fy, 5, F), tfb = interp (fx, fy, 5, F + beta) - interp (fx, fy, 5, beta);
        return tp + 0.5 * (tf + tfb);
    }
    if (std::abs (Peff) < 1.0 && std::abs (Pc) > 1e-6 && std::abs (F) < 1e-6)
    {
        const double u = 0.5 * (Peff + 1.0);
        *slope = grid (gridS, -1.0, 0.0) + u * (grid (gridS, 1.0, 0.0) - grid (gridS, -1.0, 0.0));
        return grid (gridI, -1.0, 0.0) + u * (grid (gridI, 1.0, 0.0) - grid (gridI, -1.0, 0.0));
    }
    *slope = grid (gridS, pc, fc);
    return grid (gridI, pc, fc) + interp (px, py, 14, Peff) - interp (px, py, 14, pc);
}

void Shifter::process (float* const* data, int ch, int n, float pitchSt, float formantSt, float flattenHz,
                       float pitchControlSt, float formantControlSt)
{
    const int cn = std::min (ch, channels);
    const double P = pitchSt, F = formantSt;
    // the controls' own values (NaN: the totals). Measured (control.json): the grain time scale's share of Pitch, the
    // level correction and whether unvoiced input is re-granulated follow the controls; modulation moves the output
    // pitch, and the Formant total moves the envelope
    const double Pc = std::isnan (pitchControlSt) ? P : (double) pitchControlSt;
    const double Fc = std::isnan (formantControlSt) ? F : (double) formantControlSt;
    const bool pSet = std::abs (Pc) > 1e-6, fSet = std::abs (Fc) > 1e-6 || std::abs (F) > 1e-6, flat = flattenHz > 0.0f;
    const double ratio = pbmath::exp2 (P / 12.0);
    const double beta = std::max (-2.51, std::min (2.51, 0.418 * Pc));    // measured
    // measured: the grain time scale stops at +-12 semitones (Pitch +12 with Formant +12 moves the envelope by 12)
    const double scale = pbmath::exp2 (std::max (-12.0, std::min (12.0, F + beta)) / 12.0);
    lastBeta = beta;
    regranulate = pSet || fSet;
    const int mode = flat ? kLevelFlatten : (pSet || fSet ? kLevelSet : kLevelFree);
    if (! fw::math::exactlyEqual (pitchSt, lastP) || ! fw::math::exactlyEqual (formantSt, lastF)
        || ! fw::math::exactlyEqual (flattenHz, lastFlat) || ! fw::math::exactlyEqual (Pc, lastPc) || mode != levelMode)
    {
        lastP = pitchSt; lastF = formantSt; lastFlat = flattenHz; lastPc = Pc; levelMode = mode; lastPeff = 1e9;
    }
    // (the grains run while a control or a total is away from 0: a control set with the total back at 0 still shifts;
    // once they have run they keep running, the totals back at 0 included (measured: after Pitch +3 and back, by the
    // knob or by modulation, the reference's output does not return to its pass-through for as long as it was run))
    engaged = engaged || std::abs (P) > 1e-6 || std::abs (F) > 1e-6 || flat || pSet || std::abs (Fc) > 1e-6;
    const bool shifting = engaged;
    const double target = shifting ? 1.0 : 0.0, step = 1.0 / (wetFadeS * sr);
    const int T = (int) fir.size();

    for (int i = 0; i < n; ++i)
    {
        ++now;
        const int w = (int) (now & ringMask);
        float mono = 0.0f;
        bool still = true;
        for (int c = 0; c < cn; ++c)
        {
            history[(size_t) c][(size_t) w] = data[c][i]; mono += data[c][i];
            still = still && fw::math::exactlyEqual (data[c][i], lastIn[c]); lastIn[c] = data[c][i];
        }
        // (measured: a run of zeros or of a constant: 1030 samples from a place 0 mod 8 of the processing, not 1029;
        // 1029 from 3 mod 8; a sine of 1e-9 never)
        if (! still) { flatCount = 0; played = true; }
        else if ((flatCount = std::min (flatCount + 1, flatRun)) >= flatRun && played && (now & 7) == 5)
        {
            played = false; flushed = true; engaged = false;
            sched = false; lastWasVoiced = false; wetMix = 0.0;
            for (auto& g : grains) g.on = false;
        }
        analyse (mono / (float) std::max (1, cn));

        if (wetMix < target) wetMix = std::min (target, wetMix + step);
        else if (wetMix > target) wetMix = std::max (target, wetMix - step);
        if (! sched && wetMix > 0.0)
        {
            sched = true; lastWasVoiced = false;
            nextStart = (double) now; nextFade = passFadeS * sr; nextCreate = (double) now;
        }
        if (sched && wetMix <= 0.0 && target <= 0.0)
        {
            sched = false;
            for (auto& g : grains) g.on = false;
        }
        if (sched) schedule (ratio, scale, flattenHz);

        for (int c = 0; c < cn; ++c)
        {
            acc[(size_t) c] = 0.0f;
            float dp = 0.0f;
            const auto& h = history[(size_t) c];
            if (flushed) dp = h[(size_t) ((now - flatDelay) & ringMask)];
            else for (int k = 0; k < T; ++k) dp += fir[(size_t) k] * h[(size_t) ((now - firBase - k) & ringMask)];
            dly[(size_t) c] = dp;
        }
        if (sched)
        {
            float raw = 0.0f;
            bool allVoiced = true, any = false;
            for (auto& g : grains)
            {
                if (! g.on) continue;
                const double u = g.u;
                float wv = 0.0f;
                if (u >= 0.0)
                {
                    if (u < g.fi) wv = fade (u * g.invFi);
                    else if (u < g.fi + g.fl) wv = 1.0f;
                    else if (u < g.len) wv = fade ((g.len - u) * g.invFo);
                }
                if (wv > 0.0f)
                {
                    const float k = wv * g.gain;
                    const double p = std::min (g.pos, (double) now - 2.0);
                    if (g.exact) for (int c = 0; c < cn; ++c) acc[(size_t) c] += k * dly[(size_t) c];
                    else for (int c = 0; c < cn; ++c) acc[(size_t) c] += k * readCubic (history[(size_t) c], p);
                    any = true;
                    if (g.kind == kVoiced) raw += wv * readCubic (monoRing, p);
                    else allVoiced = false;
                }
                g.u += 1.0;
                g.pos += g.scale;
                if (g.u >= g.len) g.on = false;
            }
            // (the gaps between voiced grains count too: pitched far down the grains do not overlap)
            if (allVoiced && gateOpen && (any || lastWasVoiced))
            {
                const double xin = monoRing[(size_t) (now & ringMask)];
                lvIn += levelA * (xin * xin - lvIn);
                lvOut += levelA * ((double) raw * raw - lvOut);
                ++lvCount;
            }
        }
        for (int c = 0; c < cn; ++c)
        {
            const float dp = dly[(size_t) c];
            data[c][i] = (float) (dp + wetMix * (acc[(size_t) c] - dp));
        }
    }
}
}
