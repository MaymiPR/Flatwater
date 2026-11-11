#pragma once
// Flatwater Comp's compressor, as identified on the reference (MEASUREMENTS.md "Flatwater Comp", plugins/FwComp/nulltest/fit_*.py).
//
//   detector  the larger channel's |x|, held over the next 64 samples (the look-ahead; Punch delays it by 1 ms per unit)
//   static    GR = h((1 - 1/ratio) * max(0, 20 log10(peak + 0.001) - threshold)), h a measured table
//   release   in the gain-reduction domain: a slow state S follows the output (charging at the Crest time constant)
//             and releases at fs(S) / release; the output R falls at the same law with an effective release
//             min(release, release * w0(S), K / (R - S)^p), much shorter after short events
//   attack    a one-pole on the linear gain (falling only), time constant = Attack
//   smoothing two box filters (38 and 27 samples), then makeup, the dry / wet mix and the output gain
//   limiter   a 64-sample look-ahead peak limiter on the output (Off: as the reference's "clip", On: held longer)
//   learned   a small GRU (GainNet, trained on renders of the reference by plugins/FwComp/nulltest/train_net.py) reads these states
//             and the settings every 16 samples and corrects the gain (dB, held over the step; none without
//             reduction): where the reference's level detection departs from this structure (low notes with long
//             attacks, long releases, deep compression)
// All sizes in samples are for 44.1 / 48 kHz and scale by 2 / 4 at 88.2-96 / 176.4-192 kHz (the latency does).
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>
#include "Tuning.h"
#include "shared/dsp/Math.h"
#include "Net.h"

namespace co
{
// the learned gain correction: one GRU step (PyTorch layout) per call; returns the correction in dB
class GainNet
{
public:
    void reset() { std::fill (h, h + gainnet::hidden, 0.0f); }
    float step (const float* in)
    {
        constexpr int H = gainnet::hidden, I = gainnet::inputs;
        float nh[H];
        for (int j = 0; j < H; ++j)
        {
            float g[3];
            for (int q = 0; q < 3; ++q)
            {
                const int row = q * H + j;
                float a = gainnet::bih[row];
                for (int k = 0; k < I; ++k) a += gainnet::Wih[row * I + k] * in[k];
                g[q] = a;
            }
            float hr = gainnet::bhh[j], hz = gainnet::bhh[H + j], hn = gainnet::bhh[2 * H + j];
            for (int m = 0; m < H; ++m)
            {
                hr += gainnet::Whh[j * H + m] * h[m];
                hz += gainnet::Whh[(H + j) * H + m] * h[m];
                hn += gainnet::Whh[(2 * H + j) * H + m] * h[m];
            }
            const float r = 1.0f / (1.0f + flatwater::math::expf (-(g[0] + hr))), z = 1.0f / (1.0f + flatwater::math::expf (-(g[1] + hz)));
            const float n = flatwater::math::tanhf (g[2] + r * hn);
            nh[j] = (1.0f - z) * n + z * h[j];
        }
        float o = gainnet::bout[0];
        for (int m = 0; m < H; ++m) { h[m] = nh[m]; o += gainnet::Wout[m] * h[m]; }
        return 9.0f * flatwater::math::tanhf (o / 9.0f);
    }
private:
    float h[gainnet::hidden] {};
};

class Compressor
{
public:
    struct Settings
    {
        float thresholdDb = -18.0f, ratio = 3.0f, attackMs = 7.0f, releaseMs = 100.0f, punch = 1.0f, mix = 1.0f, outputDb = 0.0f;
        bool limiter = false;
    };

    void prepare (double sampleRate, int numChannels)
    {
        fs = sampleRate;
        scale = fs > 100000.0 ? 4 : fs > 50000.0 ? 2 : 1;
        la = t.lookAhead * scale;
        channels = std::max (1, std::min (2, numChannels));
        maxPunch = (int) std::ceil (t.punchMaxMs * 1e-3 * fs) + 1;
        chargeBlend = t.chargeBlend;
        memFloorS = t.memoryFloorMs * 1e-3;
        delay.assign ((size_t) channels, std::vector<float> ((size_t) la, 0.0f));
        det.assign ((size_t) (la + maxPunch + 2), 0.0f);
        b1.assign ((size_t) (t.box1 * scale), 1.0f); b2.assign ((size_t) (t.box2 * scale), 1.0f);
        lb1.assign ((size_t) (t.box1 * scale), 1.0f); lb2.assign ((size_t) (t.box2 * scale), 1.0f);
        lim.assign ((size_t) (la + 8 * scale + 2), 0.0f);
        reset();
    }

    void reset()
    {
        for (auto& d : delay) std::fill (d.begin(), d.end(), 0.0f);
        std::fill (det.begin(), det.end(), 0.0f);
        std::fill (lim.begin(), lim.end(), 0.0f);
        std::fill (b1.begin(), b1.end(), 1.0f); std::fill (b2.begin(), b2.end(), 1.0f);
        std::fill (lb1.begin(), lb1.end(), 1.0f); std::fill (lb2.begin(), lb2.end(), 1.0f);
        sum1 = (double) b1.size(); sum2 = (double) b2.size(); lsum1 = (double) lb1.size(); lsum2 = (double) lb2.size();
        i1 = i2 = li1 = li2 = 0;
        pos = 0; detPos = 0; limPos = 0;
        startU = t.startupMs > 0.0f ? 1.0 : 0.0;
        R = 0.0; S = tables::hasRelease4 ? (double) t.start4Db : 0.0; S2 = tables::hasRelease4 ? (double) t.start4FastDb : 0.0; g1 = 1.0;
        net.reset(); netCount = 0; corrGain = 1.0; flatPos = flatFill = 0; std::fill (std::begin (flatHist), std::end (flatHist), 0.0f);
    }

    int latency() const { return la; }

    void setSettings (const Settings& s)
    {
        set = s;
        slope = 1.0 - 1.0 / std::max (1.0, (double) s.ratio);
        attC = flatwater::math::exp (-1.0 / (t.attackScale * std::max (t.attackMinMs, (double) s.attackMs) * 1e-3 * fs));
        relS = std::max (t.releaseMinS, (double) s.releaseMs * 1e-3);
        crestC = flatwater::math::exp (-1.0 / ((double) t.crestMs * 1e-3 * fs));
        startC = t.startupMs > 0.0f ? flatwater::math::exp (-1.0 / ((double) t.startupMs * 1e-3 * fs)) : 0.0;
        if constexpr (tables::hasRelease4)
        {
            crest4C = flatwater::math::exp (-1.0 / ((double) t.crest4Ms * 1e-3 * fs));
            fast4C = flatwater::math::exp (-1.0 / ((double) t.fast4Ms * 1e-3 * fs));
        }
        punchSamples = std::clamp ((int) std::lround (s.punch * 1e-3 * fs), 0, maxPunch - 1);
        makeup = flatwater::math::pow10 (makeupDb (s.thresholdDb, s.ratio) * makeupScale (s.releaseMs) / 20.0);
        outGain = flatwater::math::pow10 (s.outputDb / 20.0);
    }

    static double makeupDb (double thr, double ratio)
    {
        using namespace tables;
        double k;
        if (ratio <= makeupRatio[0]) k = makeupSlope[0];
        else if (ratio >= makeupRatio[makeupCount - 1]) k = makeupSlope[makeupCount - 1];
        else
        {
            int i = 0;
            while (ratio > makeupRatio[i + 1]) ++i;
            // interpolate in 1 / ratio (the slope follows 1 - 1/ratio closely)
            const double a = 1.0 / makeupRatio[i], b = 1.0 / makeupRatio[i + 1], f = (1.0 / ratio - a) / (b - a);
            k = makeupSlope[i] + f * (makeupSlope[i + 1] - makeupSlope[i]);
        }
        return k * std::max (0.0, -9.0 - thr);
    }

    // the makeup measured at Release 100 ms scales with the release time (the same factor for every threshold and ratio)
    static double makeupScale (double releaseMs)
    {
        using namespace tables;
        const double r = flatwater::math::log (std::clamp (releaseMs, (double) makeupRelMs[0], (double) makeupRelMs[makeupRelCount - 1]));
        int i = 0;
        while (i < makeupRelCount - 2 && r > flatwater::math::log ((double) makeupRelMs[i + 1])) ++i;
        const double a = flatwater::math::log ((double) makeupRelMs[i]), b = flatwater::math::log ((double) makeupRelMs[i + 1]), f = (r - a) / (b - a);
        return makeupRelScale[i] + f * (makeupRelScale[i + 1] - makeupRelScale[i]);
    }

    // in-place; ch[0..channels-1]
    void process (float* const* ch, int n)
    {
        minGain = minLimGain = 1.0;
        for (int i = 0; i < n; ++i)
        {
            // input side: the detector level and the delayed audio
            float a = 0.0f;
            for (int c = 0; c < channels; ++c) a = std::max (a, std::abs (ch[c][i]));
            det[(size_t) detPos] = a;
            const double h = heldPeak();
            const double Rt = targetGr (h);
            stepRelease (Rt);
            const double gt = flatwater::math::pow10 (-R / 20.0);
            g1 = gt < g1 ? gt + attC * (g1 - gt) : gt;
            sum1 += g1 - b1[(size_t) i1]; b1[(size_t) i1] = (float) g1; i1 = (i1 + 1) % (int) b1.size();
            const double v1 = sum1 / (double) b1.size();
            sum2 += v1 - b2[(size_t) i2]; b2[(size_t) i2] = (float) v1; i2 = (i2 + 1) % (int) b2.size();
            double g = sum2 / (double) b2.size();
            if (t.learned)
            {
                // the steps fall on samples la, la + 16, ... after a reset, as in the training (plugins/FwComp/nulltest/render_trainset.py)
                if (netCount >= la && (netCount - la) % (t.netStep * scale) == 0)
                {
                    const double out = -20.0 * flatwater::math::log10 (std::max (g, 1e-6));
                    const float in[gainnet::inputs] = {
                        (float) (std::clamp (20.0 * flatwater::math::log10 (h + 1e-5) - set.thresholdDb, -40.0, 40.0) / 24.0), (float) (Rt / 24.0),
                        (float) (R / 24.0), (float) (S / 24.0), (float) (-20.0 * flatwater::math::log10 (std::max (g1, 1e-6)) / 24.0), (float) (out / 24.0),
                        (float) (flatwater::math::log (set.attackMs) / flatwater::math::log (100.0)), (float) (flatwater::math::log (set.releaseMs) / flatwater::math::log (3000.0)),
                        (float) (1.0 / set.ratio), (float) (set.punch / 30.0), (float) (set.thresholdDb / 48.0) };
                    // the guards (as in plugins/FwComp/nulltest/train_net.py): no correction without reduction, nor while the target
                    // stayed within 0.05 dB over the last 60 steps (20 ms: a steady tone, which the analytical model nulls)
                    flatHist[(size_t) flatPos] = (float) Rt; flatPos = (flatPos + 1) % flatSteps;
                    if (flatFill < flatSteps) ++flatFill;
                    float lo = (float) Rt, hi = (float) Rt;
                    for (int q = 0; q < flatSteps; ++q)
                    {
                        const float v = q < flatFill ? flatHist[(size_t) q] : flatHist[(size_t) ((flatPos + flatSteps - flatFill) % flatSteps)];
                        lo = std::min (lo, v); hi = std::max (hi, v);
                    }
                    const double guard = std::clamp (R + out, 0.0, 1.0) * std::clamp ((hi - lo - t.guardFlatDb) / t.guardRampDb, 0.0, 1.0);
                    corrGain = flatwater::math::pow10 (net.step (in) * guard / 20.0);
                }
                if (netCount < std::numeric_limits<int>::max()) ++netCount;
                g *= corrGain;
            }
            minGain = std::min (minGain, g);
            if (trace != nullptr)                      // per sample: held peak, target and release states (dB), gains
            {
                const float v[6] = { (float) h, (float) Rt, (float) R, (float) S, (float) g1, (float) g };
                trace->insert (trace->end(), v, v + 6);
            }

            // the output limiter sees the incoming samples at the current gain (what they will become)
            const double gTarget = flatwater::math::pow10 (-Rt / 20.0);
            const double gEst = t.limEstimate == 1 ? gTarget : t.limEstimate == 2 ? std::max (g, gTarget) : g;
            const double wetGain = (1.0 - set.mix) + set.mix * gEst * makeup;
            const double wetNow = (1.0 - set.mix) + set.mix * g * makeup;
            lim[(size_t) limPos] = (float) (t.limEstimate == 3 ? a : a * wetGain * outGain);
            const double lh = t.limEstimate == 3 ? limiterHeld() * wetNow * outGain : limiterHeld();
            const double lt = lh > 1.0 ? 1.0 / lh : 1.0;
            lsum1 += lt - lb1[(size_t) li1]; lb1[(size_t) li1] = (float) lt; li1 = (li1 + 1) % (int) lb1.size();
            const double lv = lsum1 / (double) lb1.size();
            lsum2 += lv - lb2[(size_t) li2]; lb2[(size_t) li2] = (float) lv; li2 = (li2 + 1) % (int) lb2.size();
            const double lg = lsum2 / (double) lb2.size();
            minLimGain = std::min (minLimGain, lg);

            for (int c = 0; c < channels; ++c)
            {
                auto& d = delay[(size_t) c];
                const float x = d[(size_t) pos];
                d[(size_t) pos] = ch[c][i];
                const double y = ((1.0 - set.mix) * x + set.mix * x * g * makeup) * outGain * lg;
                ch[c][i] = (float) y;
            }
            pos = (pos + 1) % la;                      // the delay line returns the input of la samples ago
            detPos = (detPos + 1) % (int) det.size();
            limPos = (limPos + 1) % (int) lim.size();
        }
    }

    double gainReductionDb() const { return R; }
    Tuning t = tuning();                               // dsp/Tuning.h: the defaults, or the renderer's overrides
    std::vector<float>* trace = nullptr;               // plugins/FwComp/tools/CoRender trace=: the model's states for plugins/FwComp/nulltest/train_net.py
    // the deepest reduction of the smoothed compressor gain during the last process() call (dB, positive)
    double blockGainReductionDb() const { return minGain < 1.0 ? -20.0 * flatwater::math::log10 (std::max (minGain, 1e-6)) : 0.0; }
    // the same for the output limiter
    double blockLimiterDb() const { return minLimGain < 1.0 ? -20.0 * flatwater::math::log10 (std::max (minLimGain, 1e-6)) : 0.0; }

private:
    // max of the detector over the 64 newest samples, delayed by Punch (the output is la samples behind the input)
    double heldPeak() const
    {
        const int N = (int) det.size();
        double m = 0.0;
        for (int k = 0; k < la; ++k)
        {
            int j = detPos - punchSamples - k;
            j = ((j % N) + N) % N;
            m = std::max (m, (double) det[(size_t) j]);
        }
        return m;
    }

    double limiterHeld() const
    {
        const int N = (int) lim.size(), span = la + (set.limiter ? t.limiterHold * scale : 0);
        double m = 0.0;
        for (int k = 0; k < span; ++k)
        {
            int j = limPos - k;
            j = ((j % N) + N) % N;
            m = std::max (m, (double) lim[(size_t) j]);
        }
        return m;
    }

    double targetGr (double h) const
    {
        using namespace tables;
        const double x = slope * std::max (0.0, 20.0 * flatwater::math::log10 (h + t.floorLevel) - set.thresholdDb);
        if (x <= 0.0) return 0.0;
        if (x >= staticX[staticCount - 1]) return x - (staticX[staticCount - 1] - staticH[staticCount - 1]);
        // the table is uniform within three ranges; a short search is cheap enough
        int lo = 0, hi = staticCount - 1;
        while (hi - lo > 1) { const int m = (lo + hi) / 2; (x < staticX[m] ? hi : lo) = m; }
        const double f = (x - staticX[lo]) / (staticX[hi] - staticX[lo]);
        return staticH[lo] + f * (staticH[hi] - staticH[lo]);
    }

    static double table (const float* knot, const float* logv, int n, double x)
    {
        if (x <= knot[0]) return flatwater::math::exp ((double) logv[0]);
        if (x >= knot[n - 1]) return flatwater::math::exp ((double) logv[n - 1]);
        int i = 0;
        while (x > knot[i + 1]) ++i;
        const double f = (x - knot[i]) / (knot[i + 1] - knot[i]);
        return flatwater::math::exp (logv[i] + f * (logv[i + 1] - logv[i]));
    }

    // the fourth / fifth form (declined, off: MEASUREMENTS.md section 5): the release time shrinks as R rises above two averages of itself,
    // a slow one (the reference's Crest detector) and a faster one, so short events release almost at once and long
    // ones at the Release time; after a reset the averages start high (the reference releases slowly at first)
    void stepRelease4 (double target)
    {
        using namespace tables;
        if constexpr (hasRelease4)
        {
            const double dt = 1.0 / fs;
            const double w = (R > S ? table (w4Knot, w4Log, w4Count, R - S) : flatwater::math::exp ((double) w4Log[0]))
                           * (R > S2 ? table (w4Knot, w4fLog, w4fCount, R - S2) : flatwater::math::exp ((double) w4fLog[0]));
            R -= (table (fs4Knot, fs4Log, fs4Count, R) / (relS * w) + table (as4Knot, as4Log, as4Count, R)) * dt;
            R = std::max ({ R, target, 0.0 });
            const double gt = flatwater::math::pow10 (-R / 20.0);
            S2 = -20.0 * flatwater::math::log10 (gt + fast4C * (flatwater::math::pow10 (-S2 / 20.0) - gt));
            S = -20.0 * flatwater::math::log10 (gt + crest4C * (flatwater::math::pow10 (-S / 20.0) - gt));
        }
    }

    void stepRelease (double target)
    {
        using namespace tables;
        if constexpr (hasRelease4) { stepRelease4 (target); return; }
        const double dt = 1.0 / fs;
        // the slow state charges (a one-pole on the linear gain) toward the reduction the listener gets, after the
        // attack, blended with R (chargeBlend), or releases with the slow law
        double src = std::max (R, target);
        if (chargeBlend < 1.0)
        {
            const double out = -20.0 * flatwater::math::log10 (g1);
            src = out + chargeBlend * (src - out);
        }
        if (src > S + 1e-9)                             // rising by more than rounding: charges; level or falling: releases
        {
            const double gs = flatwater::math::pow10 (-S / 20.0), gt = flatwater::math::pow10 (-src / 20.0);
            S = std::min (src, -20.0 * flatwater::math::log10 (gt + crestC * (gs - gt)));
        }
        else { S -= (table (fsKnot, fsLog, fsCount, S) / std::max (relS, memFloorS) + table (asKnot, asLog, asCount, S)) * dt; S = std::max (S, target); }
        S = std::max (S, 0.0);
        double re = relS;
        if (R - S > 1e-9)
            re = std::min ({ re, relS * table (w0Knot, w0Log, w0Count, S), t.relSeedMs * 1e-3 / flatwater::math::pow (R - S, (double) t.relSeedExp) });
        if (startU > 1e-6) { re += startU * (relS - re); startU *= startC; }   // start-up: slow at first, as the reference
        R -= (table (fsKnot, fsLog, fsCount, R) / re + table (asKnot, asLog, asCount, R)) * dt;
        if constexpr (tables::rFloorAtS) R = std::max (R, S);   // otherwise the memory only slows the release
        R = std::max ({ R, target, 0.0 });
    }

    Settings set;
    double fs = 48000.0;
    int scale = 1, la = 64, channels = 2, maxPunch = 1441, punchSamples = 48;
    double slope = 2.0 / 3.0, attC = 0.0, relS = 0.1, crestC = 0.0, crest4C = 0.0, fast4C = 0.0, makeup = 1.0, outGain = 1.0;
    std::vector<std::vector<float>> delay;
    std::vector<float> det, lim, b1, b2, lb1, lb2;
    double sum1 = 0.0, sum2 = 0.0, lsum1 = 0.0, lsum2 = 0.0;
    int i1 = 0, i2 = 0, li1 = 0, li2 = 0, pos = 0, detPos = 0, limPos = 0;
    double startU = 0.0, startC = 0.0;
    double chargeBlend = 0.0, memFloorS = 0.0;                          // from the tuning (prepare)
    double R = 0.0, S = 0.0, S2 = 0.0, g1 = 1.0, minGain = 1.0, minLimGain = 1.0;
    GainNet net;
    static constexpr int flatSteps = 60;
    float flatHist[flatSteps] {};
    int netCount = 0, flatPos = 0, flatFill = 0;
    double corrGain = 1.0;
};
}
