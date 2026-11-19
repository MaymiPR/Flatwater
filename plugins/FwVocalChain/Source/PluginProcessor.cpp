#include "PluginProcessor.h"
#include "shared/dsp/Finite.h"
#include "PluginEditor.h"

using namespace juce;

FwVocalChainProcessor::FwVocalChainProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, &undo, "FwVocalChain", ch::createLayout()),
      presets (apvts, undo, ch::factoryPresets(), apvts.getParameter (ch::id::mode),
               { apvts.getParameter (ch::id::sensitivity), apvts.getParameter (ch::id::output),   // (presets leave these
                 apvts.getParameter (ch::id::link) })                                             // alone, as the reference)
{
    chain.setReverb (&reverb);
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
    maxBlock = jmax (block, 16);
    chain.prepare (sr, maxBlock);
    work.setSize (2, maxBlock);
    bypassLine.prepare (jmax (1, getTotalNumInputChannels()), getLatencySamples());
}

ch::dsp::Params FwVocalChainProcessor::chainParams()
{
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
    return p;
}

void FwVocalChainProcessor::processBlockBypassed (AudioBuffer<float>& b, MidiBuffer&)
{
    const int n = b.getNumSamples(), ins = jmin (getTotalNumInputChannels(), b.getNumChannels());
    // The chain's delays stand between its stages (the input delay, Compress's look-ahead, the de-esser's band), so
    // they cannot be fed apart from the stages: the chain runs on the input as in processBlock and its output is
    // dropped. The release of the bypass then goes on with the audio of now, the echoes and the reverb of it too,
    // not with what the chain held when the bypass came on.
    if (ins > 0 && maxBlock > 0)
    {
        ScopedNoDenormals nd;
        const auto prm = chainParams();
        for (int at = 0; at < n; at += maxBlock)                        // (in pieces: see processBlock)
        {
            const int m = jmin (maxBlock, n - at);
            for (int c = 0; c < 2; ++c) work.copyFrom (c, 0, b, jmin (c, ins - 1), at, m);
            flatwater::silenceNonFinite (work.getArrayOfWritePointers(), 2, m);
            chain.process (work.getWritePointer (0), work.getWritePointer (1), m, prm);
        }
    }
    bypassLine.process (b.getArrayOfWritePointers(), ins, n);
    for (int c = jmax (1, ins); c < b.getNumChannels(); ++c) b.copyFrom (c, 0, b, 0, 0, n);   // (Mono/Stereo)
}

void FwVocalChainProcessor::processBlock (AudioBuffer<float>& b, MidiBuffer&)
{
    ScopedNoDenormals nd;
    flatwater::silenceNonFinite (b.getArrayOfWritePointers(), b.getNumChannels(), b.getNumSamples());
    const int n = b.getNumSamples(), ins = getTotalNumInputChannels(), outs = b.getNumChannels();
    if (maxBlock <= 0) return;                                          // (a host that never prepared)
    bypassLine.write (b.getArrayOfReadPointers(), jmin (ins, outs), n);
    float pin[2] { 0.0f, 0.0f }, pout[2] { 0.0f, 0.0f };
    const auto prm = chainParams();
    // The chain and its buffers are made for blocks of up to the size prepareToPlay was given. A larger block goes
    // through in pieces of that size, so the audio thread never resizes a buffer; the audio is that of the host
    // sending those pieces as blocks.
    for (int at = 0; at < n; at += maxBlock)
    {
        const int m = jmin (maxBlock, n - at);
        for (int c = 0; c < 2; ++c)                                     // (Mono/Stereo: the input on both sides)
        {
            work.copyFrom (c, 0, b, jmin (c, jmax (0, ins - 1)), at, m);
            pin[c] = jmax (pin[c], work.getMagnitude (c, 0, m));
        }
        chain.process (work.getWritePointer (0), work.getWritePointer (1), m, prm);
        for (int c = 0; c < outs; ++c)
        {
            b.copyFrom (c, at, work, outs == 1 ? 0 : jmin (c, 1), 0, m);
            if (outs == 1) { b.addFrom (0, at, work, 1, 0, m); b.applyGain (0, at, m, 0.5f); }
        }
        for (int c = 0; c < 2; ++c) pout[c] = jmax (pout[c], work.getMagnitude (c, 0, m));
    }
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
