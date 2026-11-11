#include "Engine.h"

using namespace juce;

namespace pb
{
void Engine::prepare (double sampleRate, int maxBlock, int channels)
{
    sr = sampleRate;
    const int block = jmax (1, maxBlock);
    shifter.tune = t;
    shifter.prepare (sr, block, channels);
    dry.assign ((size_t) channels, std::vector<float> ((size_t) block, 0.0f));
    mono.assign ((size_t) block, 0.0f);
    pitchPer.assign ((size_t) block, 0.0f);
    formantPer.assign ((size_t) block, 0.0f);
    for (auto& l : lfo) l.reset();
    am.prepare (sr, t);
    pt.refHz = t.ptRefHz;
    pt.reset();
    processed = 0;
    for (int k = 0; k < numTargets; ++k) modNorm[(size_t) k] = targets[(size_t) k]->getValue();
    modSum.fill (0.0);
    lastValue.fill (0.0);
}

// a control with its modulation (last block's sums): the scaling ones multiplied (oddsScale, as measured), the rest
// added on the normalised value; Pitch to +-24 semitones
double Engine::modulatedValue (int target, const Params& p) const
{
    const auto* q = targets[(size_t) target];
    const double s = modSum[(size_t) target];
    const double own = q->convertFrom0to1 (p.norm[(size_t) target]);
    auto scaled = [&] (double a, double e) { const auto r = q->getNormalisableRange();
                                              return jlimit ((double) r.start, (double) r.end, own * oddsScale (s, a, e)); };
    switch (target)
    {
        case 3: case 7: return scaled (t.rateOddsA, t.rateOddsP);                        // Rate (Hz)
        case 4: case 8: case 11: case 12: case 14: return scaled (1.0, 1.0);           // Warp, AM Attack / Release, PT Smooth
        case 13: case 15: return scaled (0.5, 1.0);                                    // AM Level, PT Level
        case 5: case 9: return own + s;                                                // Phase: wraps (measured: -0.25 is 0.75)
        default: return q->convertFrom0to1 (jlimit (0.0f, 1.0f, modNorm[(size_t) target]));
    }
}

void Engine::process (float* const* io, int ch, int n, const Params& prm, const Setup& audio)
{
    if ((int) dry.size() < ch) dry.resize ((size_t) ch);
    for (auto& d : dry) if ((int) d.size() < n) d.resize ((size_t) n);     // (a host's larger block)
    if ((int) mono.size() < n) { mono.resize ((size_t) n); pitchPer.resize ((size_t) n); formantPer.resize ((size_t) n); }
    for (int c = 0; c < ch; ++c) std::copy (io[c], io[c] + n, dry[(size_t) c].begin());
    for (int i = 0; i < n; ++i)
    {
        float s = 0.0f;
        for (int c = 0; c < ch; ++c) s += io[c][i];
        mono[(size_t) i] = s / (float) ch;
    }

    // the transport: the song position for synced rates (counted from the start of processing without one)
    const double bpm = prm.bpm;
    const double ppq = prm.havePpq ? prm.ppq : (double) processed / sr * bpm / 60.0;

    // the controls' own values (normalised): Pitch and Formant whole semitones without Fine, cents with it
    std::array<float, numTargets> base = prm.norm;
    for (int k : { 0, 1 })
    {
        const float st = targets[(size_t) k]->convertFrom0to1 (base[(size_t) k]);
        base[(size_t) k] = targets[(size_t) k]->convertTo0to1 (audio.fine ? std::round (st * 100.0f) / 100.0f : std::round (st));
    }
    auto real = [this] (int k, float norm) { return targets[(size_t) k]->convertFrom0to1 (jlimit (0.0f, 1.0f, norm)); };

    // the slots: depth / 100 x the source on the control's normalised value, clipped to its range; Pitch's to twice its
    // range, +-24 semitones (measured: depth 100 of a +1 source moves it +24, Formant stops at +12)
    auto applySlots = [&] (const double* v)
    {
        for (int k = 0; k < numTargets; ++k)
        {
            double sum = 0.0;
            for (int s = 0; s < numSlots; ++s)
            {
                const auto& sl = audio.slots[(size_t) (k * numSlots + s)];
                if (sl.source > None && sl.source < numSources) sum += sl.depth / 100.0 * v[sl.source];
            }
            modSum[(size_t) k] = sum;
            modNorm[(size_t) k] = k == 0 ? jlimit (-0.5f, 1.5f, base[0] + (float) sum) : jlimit (0.0f, 1.0f, base[(size_t) k] + (float) sum);
            modulated[(size_t) k] = (float) modulatedValue (k, prm);   // (the knobs show Pitch at their end past +-12)
        }
    };

    // the sources, per sample (a modulator's own controls take last block's modulation)
    struct Src { bool seq, synced, ctlMod; int steps; double beats, cps, phase, warp, gain, pole, baseBeats, baseHz, baseWarp, basePhase, baseLevel; } src[2];
    const double beatsPerSample = bpm / 60.0 / sr;
    // the cycle's place: synced, the song position (a fixed delay late); free, the count of cycles, started freeDelay
    // late (so a modulated rate moves it smoothly: measured on a stepped modulation of a free rate)
    auto cyclesAt = [&] (int m, int i, Lfo& l)
    {
        const auto& c = src[m];
        if (c.synced) return (ppq + (i + 1 - t.syncedDelay * sr) * beatsPerSample) / c.beats;
        if (! l.counting) { l.freeCycles = -c.cps * t.freeDelay; l.counting = true; }
        l.freeCycles += c.cps / sr;
        return l.freeCycles;
    };
    auto setupSources = [&]
    {
    for (int m = 0; m < 2; ++m)
    {
        const auto& d = prm.mod[m];
        const int t0 = 3 + 4 * m;                                  // its Rate, Warp, Phase, Level in targets[]
        auto& c = src[m];
        c.seq = d.type > 0.5f;
        c.synced = d.sync < 0.5f;
        c.steps = jlimit (2, maxSteps, roundToInt (d.steps));
        // a modulated Rate scales the synced rate too (measured: 1 beat at 120 bpm with x1.68 runs at 3.37 Hz), up to 30 Hz
        const double rateScale = oddsScale (modSum[(size_t) t0], t.rateOddsA, t.rateOddsP);
        const int index = roundToInt (d.rateSynced * 23.0f);
        c.beats = std::max (syncedBeats (index) / rateScale, bpm / 60.0 / t.maxRateHz) * (c.seq ? c.steps : 1);
        c.baseBeats = syncedBeats (index);
        c.baseHz = freeRate (d.rate, sr);
        c.baseWarp = d.warp;
        c.basePhase = d.phase;
        c.baseLevel = d.level;
        c.ctlMod = false;
        for (int s = 0; s < 4 * numSlots; ++s)
            c.ctlMod = c.ctlMod || audio.slots[(size_t) (t0 * numSlots + s)].source > None;
        // a free rate: its period in whole ticks first, then the modulation's scale (measured: 1 Hz x1.6845 runs at
        // 1.6843 Hz, not at the 1.6848 of a whole-tick period)
        c.cps = jlimit (0.06, t.maxRateHz, freeRate (d.rate, sr) * rateScale) / (c.seq ? c.steps : 1);
        c.warp = modulatedValue (t0 + 1, prm);
        c.phase = modulatedValue (t0 + 2, prm);
        c.gain = levelGain (modulatedValue (t0 + 3, prm));
        c.pole = poleFor (d.smooth, sr);
    }
    };
    setupSources();
    if (processed == 0)
    {
        // the first block: the modulators' first values on their own controls already (the reference's free rates
        // start modulated), from copies so nothing is advanced, before their smoothing (which starts from 0)
        double first[numSources] {};
        for (int m = 0; m < 2; ++m)
        {
            Lfo probe = lfo[m];
            first[M1 + m] = probe.next (audio.tables[m], src[m].seq, src[m].steps, cyclesAt (m, 0, probe), src[m].phase, src[m].warp, src[m].gain, 0.0);
        }
        applySlots (first);
        setupSources();
        for (int k = 0; k < numSources; ++k) lastValue[(size_t) k] = first[k];
    }
    const double attackMs = modulatedValue (11, prm), amA = Follower::attackPole (attackMs, sr);
    const double amR = am.releasePole (attackMs, modulatedValue (12, prm), sr), amGain = modulatedValue (13, prm);
    // PT holds its pitch where the voice's pitch is unclear (the detector's aperiodicity over 0.15): on speech the
    // reference's holds through such stretches, where an octave error would otherwise come through
    const float ptHz = shifter.aperiodicity() <= t.ptHoldAperiodicity ? shifter.detectedHz() : 0.0f;
    const double ptV = (pt.next (ptHz, modulatedValue (14, prm), n, sr) + prm.ptOffset) * modulatedValue (15, prm);
    // Mix's slots per sample; every other control's at the block's end
    double mixSum = 0.0;
    const float mixBase = base[2];
    double value[numSources] {};
    for (int k = 0; k < numSources; ++k) value[k] = lastValue[(size_t) k];
    for (int i = 0; i < n; ++i)
    {
        for (int m = 0; m < 2; ++m)
        {
            auto& c = src[m];
            if (c.ctlMod)
            {
                // a modulator's own controls follow their sources sample by sample (measured: stepped modulation on
                // Rate, Warp and Phase lands without a block's lag), from the sources' values at the sample before
                const int t0 = 3 + 4 * m;
                double sum[4] {};
                for (int k = 0; k < 4; ++k)
                    for (int s = 0; s < numSlots; ++s)
                    {
                        const auto& sl = audio.slots[(size_t) ((t0 + k) * numSlots + s)];
                        if (sl.source > None && sl.source < numSources) sum[k] += sl.depth / 100.0 * value[sl.source];
                    }
                const double scale = oddsScale (sum[0], t.rateOddsA, t.rateOddsP);
                c.cps = jlimit (0.06, t.maxRateHz, c.baseHz * scale) / (c.seq ? c.steps : 1);
                c.beats = std::max (c.baseBeats / scale, bpm / 60.0 / t.maxRateHz) * (c.seq ? c.steps : 1);
                c.warp = jlimit (0.01, 100.0, c.baseWarp * oddsScale (sum[1], 1.0, 1.0));
                c.phase = c.basePhase + sum[2];
                c.gain = levelGain (jlimit (0.0, 1.0, c.baseLevel + sum[3]));
            }
            const double cycles = cyclesAt (m, i, lfo[m]);
            value[M1 + m] = lfo[m].next (audio.tables[m], c.seq, c.steps, cycles, c.phase, c.warp, c.gain, c.pole);
        }
        value[AM] = am.next (mono[(size_t) i], (processed + i) % Follower::tick == 0, amA, amR) * amGain;
        value[PT] = ptV;
        mixSum = 0.0;
        for (int s = 0; s < numSlots; ++s)
        {
            const auto& sl = audio.slots[(size_t) (2 * numSlots + s)];
            if (sl.source > None && sl.source < numSources) mixSum += sl.depth / 100.0 * value[sl.source];
        }
        mono[(size_t) i] = (float) (real (2, mixBase + (float) mixSum) / 100.0);       // (mono now holds the Mix per sample)
        double pf[2] {};
        for (int k = 0; k < 2; ++k)
            for (int s = 0; s < numSlots; ++s)
            {
                const auto& sl = audio.slots[(size_t) (k * numSlots + s)];
                if (sl.source > None && sl.source < numSources) pf[k] += sl.depth / 100.0 * value[sl.source];
            }
        pitchPer[(size_t) i] = 24.0f * jlimit (-0.5f, 1.5f, base[0] + (float) pf[0]) - 12.0f;
        formantPer[(size_t) i] = real (1, base[1] + (float) pf[1]);
    }
    for (int k = 0; k < numSources; ++k) lastValue[(size_t) k] = value[k];
    for (int m = 0; m < 2; ++m) cycle[m] = lfo[m].place;
    amValue = (float) value[AM];
    ptValue = (float) value[PT];

    applySlots (value);
    // the wet path, then Mix (per sample, with its modulation)
    const float flattenHz = prm.flatten > 0.5f ? t.flattenRefHz * (float) pbmath::exp2 (std::round (prm.flattenNote) / 12.0) : 0.0f;
    // the modulated totals set the pitch and the formant, followed in steps of 16 samples (measured: a stepped Pitch
    // lands about 2 ms after the step wherever it falls in the host's block, at blocks of 64 and 512); the knobs' own
    // values (rounded as they act) set the envelope's share of Pitch, which level table applies and whether noise is
    // re-granulated, as on the reference
    float* wet[2] {};
    for (int i0 = 0; i0 < n; i0 += t.shiftStep)
    {
        for (int c = 0; c < ch; ++c) wet[c] = io[c] + i0;
        shifter.process (wet, ch, jmin (t.shiftStep, n - i0), pitchPer[(size_t) i0], formantPer[(size_t) i0], flattenHz,
                         real (0, base[0]), real (1, base[1]));
    }
    for (int i = 0; i < n; ++i)
    {
        const double m = mono[(size_t) i];
        for (int c = 0; c < ch; ++c)
        {
            const double dr = dry[(size_t) c][(size_t) i], w = io[c][i];
            io[c][i] = (float) (dr + m * (w - dr));
        }
    }
    processed += n;
}
}
