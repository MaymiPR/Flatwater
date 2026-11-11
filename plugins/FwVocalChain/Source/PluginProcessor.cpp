#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

AudioProcessorValueTreeState::ParameterLayout FwVocalChainProcessor::layout()
{
    AudioProcessorValueTreeState::ParameterLayout l;
    auto text1 = [] (float v) { return String (v, 1); };
    auto addF = [&] (const char* id, const char* name, float lo, float hi, std::function<String (float)> text, const char* unit = "")
    {
        auto a = AudioParameterFloatAttributes().withStringFromValueFunction ([text] (float v, int) { return text (v); }).withLabel (unit);
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (lo, hi, 0.1f), 0.0f, a));
    };
    // in the reference's host order: Sensitivity, then each fader and its colour, Output, Balance
    addF (ch::id::sensitivity, "Sensitivity", -10.0f, 10.0f, text1, "dB");
    const char* names[6][2] = { { "Bass", "Bass Color" }, { "Treble", "Treble Color" }, { "Compress", "Compress Color" },
                                { "Reverb", "Reverb Color" }, { "Delay", "Delay Color" }, { "Pitch", "Pitch Color" } };
    for (size_t i = 0; i < 6; ++i)
    {
        const auto& s = ch::stages()[i];
        addF (s.fader, names[i][0], -10.0f, 10.0f, text1);
        l.add (std::make_unique<AudioParameterChoice> (ParameterID { s.colour, 1 }, names[i][1],
                                                       StringArray { s.names[0], s.names[1], s.names[2], s.names[3] }, 1));
    }
    addF (ch::id::output, "Output", -10.0f, 10.0f, text1, "dB");
    addF (ch::id::balance, "Balance", -6.0f, 6.0f, text1);
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ch::id::mode, 1 }, "Mode", ch::modeNames(), 0));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { ch::id::link, 1 }, "Link In/Out", false));
    return l;
}

FwVocalChainProcessor::FwVocalChainProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, &undo, "FwVocalChain", layout())
{
    setLatencySamples (ch::latencySamples (48000.0));
    startTimerHz (30);
}

void FwVocalChainProcessor::syncLink()
{
    if (! linked()) return;
    auto* out = apvts.getParameter (ch::id::output);
    const float mirror = -get (ch::id::sensitivity);
    if (std::abs (out->convertFrom0to1 (out->getValue()) - mirror) < 1.0e-3f) return;
    out->beginChangeGesture();
    out->setValueNotifyingHost (out->convertTo0to1 (mirror));
    out->endChangeGesture();
}

// stereo in / out (Stereo), mono in / stereo out (Mono/Stereo); a host offering mono out gets mono
bool FwVocalChainProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    if (out == AudioChannelSet::stereo()) return in == AudioChannelSet::stereo() || in == AudioChannelSet::mono();
    return out == AudioChannelSet::mono() && in == AudioChannelSet::mono();
}

void FwVocalChainProcessor::prepareToPlay (double sampleRate, int block)
{
    sr = sampleRate;
    setLatencySamples (ch::latencySamples (sr));
    chain.prepare (sr, jmax (block, 16));
    work.setSize (2, jmax (block, 16));
}

void FwVocalChainProcessor::processBlock (AudioBuffer<float>& b, MidiBuffer&)
{
    ScopedNoDenormals nd;
    const int n = b.getNumSamples(), ins = getTotalNumInputChannels(), outs = b.getNumChannels();
    if (work.getNumSamples() < n) work.setSize (2, n, false, false, true);
    float pin[2] { 0.0f, 0.0f }, pout[2] { 0.0f, 0.0f };
    for (int c = 0; c < 2; ++c)                                         // (Mono/Stereo: the input on both sides)
    {
        work.copyFrom (c, 0, b, jmin (c, jmax (0, ins - 1)), 0, n);
        pin[c] = work.getMagnitude (c, 0, n);
    }
    ch::dsp::Params p;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm()) p.bpm = *bpm;
    p.sensitivity = get (ch::id::sensitivity); p.output = get (ch::id::output); p.balance = get (ch::id::balance);
    if (linked()) p.output = -p.sensitivity;                            // (the In/Out link: Output mirrors Sensitivity)
    for (size_t i = 0; i < 6; ++i)
    {
        const auto& st = ch::stages()[i];
        p.fader[i] = get (st.fader);
        p.colour[i] = roundToInt (get (st.colour));
    }
    chain.process (work.getWritePointer (0), work.getWritePointer (1), n, p);
    for (int c = 0; c < outs; ++c)
    {
        b.copyFrom (c, 0, work, outs == 1 ? 0 : jmin (c, 1), 0, n);
        if (outs == 1) { b.addFrom (0, 0, work, 1, 0, n); b.applyGain (0, 0, n, 0.5f); }
    }
    for (int c = 0; c < 2; ++c) pout[c] = work.getMagnitude (c, 0, n);
    auto db = [] (float v) { return v > 1.0e-5f ? Decibels::gainToDecibels (v) : -100.0f; };
    inL.push (db (pin[0])); inR.push (db (pin[1]));
    outL.push (db (pout[0])); outR.push (db (pout[1]));
    sensLevelDb.store (chain.levelDb());
}

AudioProcessorEditor* FwVocalChainProcessor::createEditor() { return new FwVocalChainEditor (*this); }

void FwVocalChainProcessor::getStateInformation (MemoryBlock& d)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, d);
}

void FwVocalChainProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType())) apvts.replaceState (ValueTree::fromXml (*xml));
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new FwVocalChainProcessor(); }
