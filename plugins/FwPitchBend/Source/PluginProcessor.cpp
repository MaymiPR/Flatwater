#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

namespace
{
// a range lo .. hi, exponential in its normalised value (the reference's tapers: x = lo * (hi / lo)^v); computed the
// same on every platform, the value rounded to 1e-5 so that a value set comes back as itself (a free rate of 4 Hz a
// hair over 4 runs a tick short)
NormalisableRange<float> expRange (float lo, float hi)
{
    return { lo, hi,
             [] (float a, float b, float v) { return (float) (std::round (a * pb::pbmath::pow (b / a, v) * 1e5) / 1e5); },
             [] (float a, float b, float x) { return (float) (pb::pbmath::log (x / a) / pb::pbmath::log (b / a)); } };
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
        const auto& d = pb::mod[m];
        const String p = "Mod" + String (m + 1) + " ";
        addF (d.warp, p + "Warp", expRange (0.01f, 100.0f), 1.0f, fix (2));
        addF (d.level, p + "Level", { 0.0f, 1.0f }, 1.0f, fix (2));
        addC (d.rateSynced, p + "Rate Synced", pb::syncedRates(), 5);
        addF (d.rate, p + "Rate", expRange (0.06f, 30.0f), 0.5f, [] (float v) { return String (v, 3) + " Hz"; });
        addC (d.sync, p + "Sync On/Off", StringArray { "Note", "ms" }, 0);
        addF (d.phase, p + "Phase", { 0.0f, 1.0f }, 0.0f, fix (3));
        addC (d.type, p + "Type", StringArray { "LFO", "Seq" }, 0);
        addF (d.smooth, p + "Smooth", expRange (1.0f, 1000.0f), 1.5f, fix (1));
        addF (d.steps, p + "Steps", { 2.0f, 16.0f, 1.0f }, 16.0f, fix (0));
    }
    addF (pb::amAttack, "AM Attack", expRange (0.1f, 1000.0f), 10.0f, ms);
    addF (pb::amRelease, "AM Release", expRange (0.1f, 1000.0f), 100.0f, ms);
    addF (pb::amLevel, "AM Level", { 0.0f, 2.0f }, 1.0f, fix (2));
    addF (pb::ptSmooth, "PT Smooth", expRange (1.0f, 1000.0f), 10.0f, fix (1));
    addF (pb::ptOffset, "PT Offset", { -1.0f, 1.0f }, 0.0f, fix (2));
    addF (pb::ptLevel, "PT Level", { 0.0f, 2.0f }, 1.0f, fix (2));
    auto semis = [] (float v) { return String (roundToInt (v)) + " semitones"; };
    addF (pb::pitch, "Pitch", { -12.0f, 12.0f }, 0.0f, semis);
    addF (pb::formant, "Formant", { -12.0f, 12.0f }, 0.0f, semis);
    addF (pb::mix, "Mix", { 0.0f, 100.0f }, 100.0f, fix (1));
    addC (pb::flattenNote, "Flatten Note", pb::noteNames(), 12);
    addC (pb::flatten, "Flatten On/Off", StringArray { "Off", "On" }, 0);
    addC (pb::mode, "Mode", pb::modeNames(), 0);
    return l;
}

FwPitchBendProcessor::FwPitchBendProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, &undo, "FwPitchBend", layout())
{
    ensureState();
    for (int t = 0; t < pb::numTargets; ++t)
    {
        targetParams[(size_t) t] = apvts.getParameter (pb::targets[t].param);
        modulated[(size_t) t] = get (pb::targets[t].param);
        engine.modNorm[(size_t) t] = targetParams[(size_t) t]->getValue();
    }
    engine.setTargets (targetParams);
    apvts.state.addListener (this);
    refreshModel();
}

void FwPitchBendProcessor::ensureState()
{
    using namespace pb;
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
    using namespace pb;
    Engine::Setup m;
    auto& st = apvts.state;
    for (int t = 0; t < numTargets; ++t)
        for (int s = 0; s < numSlots; ++s) m.slots[(size_t) (t * numSlots + s)] = getSlot (st, t, s);
    m.tables[0] = getTable (st, 0);
    m.tables[1] = getTable (st, 1);
    m.fine = fine();
    const SpinLock::ScopedLockType sl (modelLock);
    shared = m;
}

// a factory preset: its host parameters (flatwater::applyPreset), then the rest (applyPresetExtras, when its values land in
// the state); index 0 the defaults
void FwPitchBendProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= getNumPrograms()) return;
    currentProgram = index;
    static const flatwater::FactoryPreset defaults { "", "Default", {} };
    flatwater::applyPreset (apvts, index == 0 ? defaults : pb::factoryPresets()[(size_t) index - 1], apvts.getParameter (pb::mode));
}

void FwPitchBendProcessor::valueTreeChildAdded (ValueTree&, ValueTree& child)
{
    if (child.hasType (flatwater::presetExactTag)) applyPresetExtras (child);
    refreshModel();
}

void FwPitchBendProcessor::applyPresetExtras (const ValueTree& exact)
{
    using namespace pb;
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

bool FwPitchBendProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    return in == out && (in == AudioChannelSet::mono() || in == AudioChannelSet::stereo());
}

void FwPitchBendProcessor::prepareToPlay (double sampleRate, int block)
{
    engine.prepare (sampleRate, jmax (1, block), jmax (1, getTotalNumInputChannels()));
}

void FwPitchBendProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals nd;
    const int n = buffer.getNumSamples(), ch = jmin (buffer.getNumChannels(), getTotalNumInputChannels());
    if (n == 0 || ch == 0) return;
    {
        const SpinLock::ScopedTryLockType tl (modelLock);
        if (tl.isLocked()) audio = shared;
    }
    // the controls once per block, and the transport
    pb::Engine::Params p;
    for (int t = 0; t < pb::numTargets; ++t) p.norm[(size_t) t] = targetParams[(size_t) t]->getValue();
    for (int m = 0; m < 2; ++m)
    {
        const auto& d = pb::mod[m];
        auto& c = p.mod[m];
        c.type = get (d.type); c.sync = get (d.sync); c.steps = get (d.steps); c.rate = get (d.rate); c.warp = get (d.warp);
        c.phase = get (d.phase); c.level = get (d.level); c.smooth = get (d.smooth);
        c.rateSynced = apvts.getParameter (d.rateSynced)->getValue();
    }
    p.ptOffset = get (pb::ptOffset);
    p.flatten = get (pb::flatten);
    p.flattenNote = get (pb::flattenNote);
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) p.bpm = *b;
            if (auto q = pos->getPpqPosition()) { p.ppq = *q; p.havePpq = true; }
        }
    engine.process (buffer.getArrayOfWritePointers(), ch, n, p, audio);
    for (int t = 0; t < pb::numTargets; ++t) modulated[(size_t) t] = engine.modulated[(size_t) t];
    for (int m = 0; m < 2; ++m) cycle[m] = engine.cycle[m];
    amValue = engine.amValue;
    ptValue = engine.ptValue;
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
