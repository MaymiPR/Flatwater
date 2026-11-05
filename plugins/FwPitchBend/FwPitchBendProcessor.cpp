#include "FwPitchBendProcessor.h"
#include "FwPitchBendEditor.h"

using namespace juce;

namespace
{
// a range lo .. hi, exponential in its normalised value (the reference's tapers: x = lo * (hi / lo)^v); computed the
// same on every platform, the value rounded to 1e-5 so that a value set comes back as itself (a free rate of 4 Hz a
// hair over 4 runs a tick short)
NormalisableRange<float> expRange (float lo, float hi)
{
    return { lo, hi,
             [] (float a, float b, float v) { return (float) (std::round (a * fwpitchbend::pbmath::pow (b / a, v) * 1e5) / 1e5); },
             [] (float a, float b, float x) { return (float) (fwpitchbend::pbmath::log (x / a) / fwpitchbend::pbmath::log (b / a)); } };
}
}

AudioProcessorValueTreeState::ParameterLayout FwPitchBendProcessor::layout()
{
    AudioProcessorValueTreeState::ParameterLayout l;
    auto addF = [&] (const char* id, const String& name, NormalisableRange<float> r, float def, std::function<String (float)> text)
    {
        auto a = AudioParameterFloatAttributes().withStringFromValueFunction ([text] (float v, int) { return text (v); });
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, r, def, a));
    };
    auto addC = [&] (const char* id, const String& name, const StringArray& items, int def)
    {
        l.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, items, def));
    };
    auto fix = [] (int digits) { return [digits] (float v) { return String (v, digits); }; };
    auto ms = [] (float v) { return String (v, 1) + " ms"; };
    // in the reference's parameter order
    for (int m = 0; m < 2; ++m)
    {
        const auto& d = fwpitchbend::mod[m];
        const String p = "Mod" + String (m + 1) + " ";
        addF (d.warp, p + "Warp", expRange (0.01f, 100.0f), 1.0f, fix (2));
        addF (d.level, p + "Level", { 0.0f, 1.0f }, 1.0f, fix (2));
        addC (d.rateSynced, p + "Rate Synced", fwpitchbend::syncedRates(), 5);
        addF (d.rate, p + "Rate", expRange (0.06f, 30.0f), 0.5f, [] (float v) { return String (v, 3) + " Hz"; });
        addC (d.sync, p + "Sync On/Off", StringArray { "Note", "ms" }, 0);
        addF (d.phase, p + "Phase", { 0.0f, 1.0f }, 0.0f, fix (3));
        addC (d.type, p + "Type", StringArray { "LFO", "Seq" }, 0);
        addF (d.smooth, p + "Smooth", expRange (1.0f, 1000.0f), 1.5f, fix (1));
        addF (d.steps, p + "Steps", { 2.0f, 16.0f, 1.0f }, 16.0f, fix (0));
    }
    addF (fwpitchbend::amAttack, "AM Attack", expRange (0.1f, 1000.0f), 10.0f, ms);
    addF (fwpitchbend::amRelease, "AM Release", expRange (0.1f, 1000.0f), 100.0f, ms);
    addF (fwpitchbend::amLevel, "AM Level", { 0.0f, 2.0f }, 1.0f, fix (2));
    addF (fwpitchbend::ptSmooth, "PT Smooth", expRange (1.0f, 1000.0f), 10.0f, fix (1));
    addF (fwpitchbend::ptOffset, "PT Offset", { -1.0f, 1.0f }, 0.0f, fix (2));
    addF (fwpitchbend::ptLevel, "PT Level", { 0.0f, 2.0f }, 1.0f, fix (2));
    auto semis = [] (float v) { return String (roundToInt (v)) + " semitones"; };
    addF (fwpitchbend::pitch, "Pitch", { -12.0f, 12.0f }, 0.0f, semis);
    addF (fwpitchbend::formant, "Formant", { -12.0f, 12.0f }, 0.0f, semis);
    addF (fwpitchbend::mix, "Mix", { 0.0f, 100.0f }, 100.0f, fix (1));
    addC (fwpitchbend::flattenNote, "Flatten Note", fwpitchbend::noteNames(), 12);
    addC (fwpitchbend::flatten, "Flatten On/Off", StringArray { "Off", "On" }, 0);
    addC (fwpitchbend::mode, "Mode", fwpitchbend::modeNames(), 0);
    return l;
}

FwPitchBendProcessor::FwPitchBendProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, &undo, "FwPitchBend", layout())
{
    ensureState();
    for (int t = 0; t < fwpitchbend::numTargets; ++t)
    {
        targetParams[(size_t) t] = apvts.getParameter (fwpitchbend::targets[t].param);
        modulated[(size_t) t] = get (fwpitchbend::targets[t].param);
        modNorm[(size_t) t] = targetParams[(size_t) t]->getValue();
    }
    apvts.state.addListener (this);
    refreshModel();
}

void FwPitchBendProcessor::ensureState()
{
    using namespace fwpitchbend;
    auto& st = apvts.state;
    child (st, modsTag);
    auto shapes = child (st, shapesTag);
    for (int m = 0; m < 2; ++m)
        if (! shapes.hasProperty (m == 0 ? "m1" : "m2")) setTable (st, m, defaultTable (m), nullptr);
    if (! st.hasProperty (fineId)) st.setProperty (fineId, false, nullptr);
    if (! st.hasProperty (expandedId)) st.setProperty (expandedId, false, nullptr);
    if (! st.hasProperty (tabId)) st.setProperty (tabId, (int) PT - 1, nullptr);      // the reference opens on PT
}

void FwPitchBendProcessor::refreshModel()
{
    using namespace fwpitchbend;
    Model m;
    auto& st = apvts.state;
    for (int t = 0; t < numTargets; ++t)
        for (int s = 0; s < numSlots; ++s) m.slots[(size_t) (t * numSlots + s)] = getSlot (st, t, s);
    m.tables[0] = getTable (st, 0);
    m.tables[1] = getTable (st, 1);
    m.fine = fine();
    const SpinLock::ScopedLockType sl (modelLock);
    shared = m;
}

// a factory preset: its host parameters (fw::applyPreset), then the rest (applyPresetExtras, when its values land in
// the state); index 0 the defaults
void FwPitchBendProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= getNumPrograms()) return;
    currentProgram = index;
    static const fw::FactoryPreset defaults { "", "Default", {} };
    fw::applyPreset (apvts, index == 0 ? defaults : fwpitchbend::factoryPresets()[(size_t) index - 1], apvts.getParameter (fwpitchbend::mode));
}

void FwPitchBendProcessor::valueTreeChildAdded (ValueTree&, ValueTree& child)
{
    if (child.hasType (fw::presetExactTag)) applyPresetExtras (child);
    refreshModel();
}

void FwPitchBendProcessor::applyPresetExtras (const ValueTree& exact)
{
    using namespace fwpitchbend;
    auto& st = apvts.state;
    st.setProperty (fineId, (bool) exact.getProperty ("fine", false), nullptr);
    for (int t = 0; t < numTargets; ++t)
        for (int s = 0; s < numSlots; ++s)
        {
            const String key = String ("slot.") + targets[t].key + "." + String (s);
            setSlot (st, t, s, { (int) exact.getProperty (key + ".src", 0), (float) exact.getProperty (key + ".depth", 0.0f) }, nullptr);
        }
    for (int m = 0; m < 2; ++m)
    {
        const String pre = m == 0 ? "m1" : "m2";
        Table t = defaultTable (m);
        if (exact.hasProperty (pre + "Shape")) setLfo (t, (Shape) jlimit (0, shapeNames().size() - 1, (int) exact.getProperty (pre + "Shape")));
        for (int k = 0; k < maxSteps; ++k) t[(size_t) (seqFirst + k)] = (float) (int) exact.getProperty (pre + "Step" + String (k), 0) / 24.0f;
        setTable (st, m, t, nullptr);
    }
}

// a control with its modulation (last block's sums): the scaling ones multiplied (oddsScale, as measured), the rest
// added on the normalised value; Pitch to +-24 semitones
double FwPitchBendProcessor::modulatedValue (int t) const
{
    using namespace fwpitchbend;
    const auto* p = targetParams[(size_t) t];
    const double s = modSum[(size_t) t];
    auto scaled = [&] (double a, double q) { const auto r = p->getNormalisableRange();
                                              return jlimit ((double) r.start, (double) r.end, (double) p->convertFrom0to1 (p->getValue()) * oddsScale (s, a, q)); };
    switch (t)
    {
        case 3: case 7: return scaled (1.5, 3.905);                                     // Rate (Hz)
        case 4: case 8: case 11: case 12: case 14: return scaled (1.0, 1.0);          // Warp, AM Attack / Release, PT Smooth
        case 13: case 15: return scaled (0.5, 1.0);                                   // AM Level, PT Level
        case 5: case 9: return p->convertFrom0to1 (p->getValue()) + s;               // Phase: wraps (measured: -0.25 is 0.75)
        default: return p->convertFrom0to1 (jlimit (0.0f, 1.0f, modNorm[(size_t) t]));
    }
}

bool FwPitchBendProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    return in == out && (in == AudioChannelSet::mono() || in == AudioChannelSet::stereo());
}

void FwPitchBendProcessor::prepareToPlay (double sampleRate, int block)
{
    sr = sampleRate;
    const int ch = jmax (1, getTotalNumInputChannels());
    shifter.prepare (sr, jmax (1, block), ch);
    dry.setSize (ch, jmax (1, block));
    mono.assign ((size_t) jmax (1, block), 0.0f);
    pitchPer.assign ((size_t) jmax (1, block), 0.0f);
    formantPer.assign ((size_t) jmax (1, block), 0.0f);
    for (auto& l : lfo) l.reset();
    am.prepare (sr);
    pt.reset();
    processed = 0;
    for (int t = 0; t < fwpitchbend::numTargets; ++t) modNorm[(size_t) t] = targetParams[(size_t) t]->getValue();
    modSum.fill (0.0);
    lastValue.fill (0.0);
}

void FwPitchBendProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    using namespace fwpitchbend;
    ScopedNoDenormals nd;
    const int n = buffer.getNumSamples(), ch = jmin (buffer.getNumChannels(), getTotalNumInputChannels());
    if (n == 0 || ch == 0) return;
    {
        const SpinLock::ScopedTryLockType tl (modelLock);
        if (tl.isLocked()) audio = shared;
    }
    if (dry.getNumSamples() < n || dry.getNumChannels() < ch) dry.setSize (ch, n, false, false, true);   // (a host's larger block)
    if ((int) mono.size() < n) { mono.resize ((size_t) n); pitchPer.resize ((size_t) n); formantPer.resize ((size_t) n); }
    for (int c = 0; c < ch; ++c) dry.copyFrom (c, 0, buffer, c, 0, n);
    for (int i = 0; i < n; ++i)
    {
        float s = 0.0f;
        for (int c = 0; c < ch; ++c) s += buffer.getSample (c, i);
        mono[(size_t) i] = s / (float) ch;
    }

    // the transport: the song position for synced rates (counted from the start of processing without one)
    double bpm = 120.0, ppq = 0.0;
    bool havePpq = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto q = pos->getPpqPosition()) { ppq = *q; havePpq = true; }
        }
    if (! havePpq) ppq = (double) processed / sr * bpm / 60.0;

    // the controls' own values (normalised): Pitch and Formant whole semitones without Fine, cents with it
    std::array<float, numTargets> base {};
    for (int t = 0; t < numTargets; ++t) base[(size_t) t] = targetParams[(size_t) t]->getValue();
    for (int t : { 0, 1 })
    {
        const float st = targetParams[(size_t) t]->convertFrom0to1 (base[(size_t) t]);
        base[(size_t) t] = targetParams[(size_t) t]->convertTo0to1 (audio.fine ? std::round (st * 100.0f) / 100.0f : std::round (st));
    }
    auto real = [this] (int t, float norm) { return targetParams[(size_t) t]->convertFrom0to1 (jlimit (0.0f, 1.0f, norm)); };

    // the slots: depth / 100 x the source on the control's normalised value, clipped to its range; Pitch's to twice its
    // range, +-24 semitones (measured: depth 100 of a +1 source moves it +24, Formant stops at +12)
    auto applySlots = [&] (const double* v)
    {
        for (int t = 0; t < numTargets; ++t)
        {
            double sum = 0.0;
            for (int s = 0; s < numSlots; ++s)
            {
                const auto& sl = audio.slots[(size_t) (t * numSlots + s)];
                if (sl.source > None && sl.source < numSources) sum += sl.depth / 100.0 * v[sl.source];
            }
            modSum[(size_t) t] = sum;
            modNorm[(size_t) t] = t == 0 ? jlimit (-0.5f, 1.5f, base[0] + (float) sum) : jlimit (0.0f, 1.0f, base[(size_t) t] + (float) sum);
            modulated[(size_t) t] = (float) modulatedValue (t);   // (the knobs show Pitch at their end past +-12)
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
        if (c.synced) return (ppq + (i + 1 - syncedDelay * sr) * beatsPerSample) / c.beats;
        if (! l.counting) { l.freeCycles = -c.cps * freeDelay; l.counting = true; }
        l.freeCycles += c.cps / sr;
        return l.freeCycles;
    };
    auto setupSources = [&]
    {
    for (int m = 0; m < 2; ++m)
    {
        const auto& d = fwpitchbend::mod[m];
        const int t0 = 3 + 4 * m;                                  // its Rate, Warp, Phase, Level in targets[]
        auto& c = src[m];
        c.seq = get (d.type) > 0.5f;
        c.synced = get (d.sync) < 0.5f;
        c.steps = jlimit (2, maxSteps, roundToInt (get (d.steps)));
        // a modulated Rate scales the synced rate too (measured: 1 beat at 120 bpm with x1.68 runs at 3.37 Hz), up to 30 Hz
        const double rateScale = oddsScale (modSum[(size_t) t0], 1.5, 3.905);
        const int index = roundToInt (apvts.getParameter (d.rateSynced)->getValue() * 23.0f);
        c.beats = std::max (syncedBeats (index) / rateScale, bpm / 60.0 / 30.0) * (c.seq ? c.steps : 1);
        c.baseBeats = syncedBeats (index);
        c.baseHz = freeRate (get (d.rate), sr);
        c.baseWarp = get (d.warp);
        c.basePhase = get (d.phase);
        c.baseLevel = get (d.level);
        c.ctlMod = false;
        for (int s = 0; s < 4 * numSlots; ++s)
            c.ctlMod = c.ctlMod || audio.slots[(size_t) (t0 * numSlots + s)].source > None;
        // a free rate: its period in whole ticks first, then the modulation's scale (measured: 1 Hz x1.6845 runs at
        // 1.6843 Hz, not at the 1.6848 of a whole-tick period)
        c.cps = jlimit (0.06, 30.0, freeRate (get (d.rate), sr) * rateScale) / (c.seq ? c.steps : 1);
        c.warp = modulatedValue (t0 + 1);
        c.phase = modulatedValue (t0 + 2);
        c.gain = levelGain (modulatedValue (t0 + 3));
        c.pole = poleFor (get (d.smooth), sr);
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
    const double attackMs = modulatedValue (11), amA = Follower::attackPole (attackMs, sr);
    const double amR = Follower::releasePole (attackMs, modulatedValue (12), sr), amGain = modulatedValue (13);
    // PT holds its pitch where the voice's pitch is unclear (the detector's aperiodicity over 0.15): on speech the
    // reference's holds through such stretches, where an octave error would otherwise come through
    const float ptHz = shifter.aperiodicity() <= 0.15f ? shifter.detectedHz() : 0.0f;
    const double ptV = (pt.next (ptHz, modulatedValue (14), n, sr) + get (fwpitchbend::ptOffset)) * modulatedValue (15);
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
                const double scale = oddsScale (sum[0], 1.5, 3.905);
                c.cps = jlimit (0.06, 30.0, c.baseHz * scale) / (c.seq ? c.steps : 1);
                c.beats = std::max (c.baseBeats / scale, bpm / 60.0 / 30.0) * (c.seq ? c.steps : 1);
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
        for (int t = 0; t < 2; ++t)
            for (int s = 0; s < numSlots; ++s)
            {
                const auto& sl = audio.slots[(size_t) (t * numSlots + s)];
                if (sl.source > None && sl.source < numSources) pf[t] += sl.depth / 100.0 * value[sl.source];
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
    const float flattenHz = get (fwpitchbend::flatten) > 0.5f ? 130.8128f * (float) fwpitchbend::pbmath::exp2 (std::round (get (fwpitchbend::flattenNote)) / 12.0) : 0.0f;
    // the modulated totals set the pitch and the formant, followed in steps of 16 samples (measured: a stepped Pitch
    // lands about 2 ms after the step wherever it falls in the host's block, at blocks of 64 and 512); the knobs' own
    // values (rounded as they act) set the envelope's share of Pitch, which level table applies and whether noise is
    // re-granulated, as on the reference
    float* io[2] {};
    for (int i0 = 0; i0 < n; i0 += 16)
    {
        for (int c = 0; c < ch; ++c) io[c] = buffer.getWritePointer (c, i0);
        shifter.process (io, ch, jmin (16, n - i0), pitchPer[(size_t) i0], formantPer[(size_t) i0], flattenHz,
                         real (0, base[0]), real (1, base[1]));
    }
    for (int i = 0; i < n; ++i)
    {
        const double m = mono[(size_t) i];
        for (int c = 0; c < ch; ++c)
        {
            const double dr = dry.getSample (c, i), w = buffer.getSample (c, i);
            buffer.setSample (c, i, (float) (dr + m * (w - dr)));
        }
    }
    processed += n;
}

AudioProcessorEditor* FwPitchBendProcessor::createEditor() { return new FwPitchBendEditor (*this); }

void FwPitchBendProcessor::getStateInformation (MemoryBlock& d)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, d);
}

void FwPitchBendProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType())) { apvts.replaceState (ValueTree::fromXml (*xml)); ensureState(); }
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new FwPitchBendProcessor(); }
