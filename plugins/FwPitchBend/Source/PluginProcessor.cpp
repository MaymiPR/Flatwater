#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

FwPitchBendProcessor::FwPitchBendProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, &undo, "FwPitchBend", pb::createLayout())
{
    ensureState();
    for (int t = 0; t < pb::dsp::numTargets; ++t)
    {
        targetParams[(size_t) t] = apvts.getParameter (pb::dsp::targets[t].param);
        modulated[(size_t) t] = get (pb::dsp::targets[t].param);
        engine.modNorm[(size_t) t] = targetParams[(size_t) t]->getValue();
    }
    apvts.state.addListener (this);
    refreshModel();
}

void FwPitchBendProcessor::ensureState()
{
    using namespace pb;
    using namespace pb::dsp;
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
    using namespace pb::dsp;
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
    flatwater::applyPreset (apvts, index == 0 ? defaults : pb::factoryPresets()[(size_t) index - 1], apvts.getParameter (pb::id::mode));
}

void FwPitchBendProcessor::valueTreeChildAdded (ValueTree&, ValueTree& child)
{
    if (child.hasType (flatwater::presetExactTag)) applyPresetExtras (child);
    refreshModel();
}

void FwPitchBendProcessor::applyPresetExtras (const ValueTree& exact)
{
    using namespace pb;
    using namespace pb::dsp;
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
    std::array<float, pb::dsp::numTargets> norm {};
    for (int t = 0; t < pb::dsp::numTargets; ++t) norm[(size_t) t] = targetParams[(size_t) t]->getValue();
    engine.prepare (sampleRate, jmax (1, block), jmax (1, getTotalNumInputChannels()), norm);
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
    pb::dsp::Engine::Params p;
    for (int t = 0; t < pb::dsp::numTargets; ++t) p.norm[(size_t) t] = targetParams[(size_t) t]->getValue();
    for (int m = 0; m < 2; ++m)
    {
        const auto& d = pb::mod[m];
        auto& c = p.mod[m];
        c.type = get (d.type); c.sync = get (d.sync); c.steps = get (d.steps); c.rate = get (d.rate); c.warp = get (d.warp);
        c.phase = get (d.phase); c.level = get (d.level); c.smooth = get (d.smooth);
        c.rateSynced = apvts.getParameter (d.rateSynced)->getValue();
    }
    p.ptOffset = get (pb::id::ptOffset);
    p.flatten = get (pb::id::flatten);
    p.flattenNote = get (pb::id::flattenNote);
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) p.bpm = *b;
            if (auto q = pos->getPpqPosition()) { p.ppq = *q; p.havePpq = true; }
        }
    engine.process (buffer.getArrayOfWritePointers(), ch, n, p, audio);
    for (int t = 0; t < pb::dsp::numTargets; ++t) modulated[(size_t) t] = engine.modulated[(size_t) t];
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
