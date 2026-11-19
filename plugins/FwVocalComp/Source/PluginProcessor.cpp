#include "PluginProcessor.h"
#include "shared/dsp/Finite.h"
#include "PluginEditor.h"

using namespace juce;

FwVocalCompProcessor::FwVocalCompProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, &undo, "FwVocalComp", vc::createLayout())
{
    pComp = apvts.getRawParameterValue (vc::id::comp);
    pGate = apvts.getRawParameterValue (vc::id::gate);
    pGain = apvts.getRawParameterValue (vc::id::gain);
    pMode = apvts.getRawParameterValue (vc::id::mode);
}

bool FwVocalCompProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    return in == out && (in == AudioChannelSet::mono() || in == AudioChannelSet::stereo());
}

void FwVocalCompProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    prepared = false;
    configure();
}

// Everything both layouts need, made here (it allocates): prepareToPlay calls it, the audio thread only for a host
// that sends a block without having prepared. The latency is the same in both layouts.
void FwVocalCompProcessor::configure()
{
    const int ch = getTotalNumInputChannels();
    procs[0].prepare (sr, ch);
    procs[1].prepare (sr, 1);
    setLatencySamples (procs[0].getLatency());
    bypassLine.prepare (ch, getLatencySamples());
    prepared = true;
    setLayout (ch > 1 && pMode->load() > 0.5f);
}

// Stereo bus + Mode "Stereo": one linked gain. Stereo bus + Mode "Mono": two independent mono processors. Mono bus: one.
// A Mode change on the audio thread comes here: a full reset (the processors, their delay lines, the bypass line),
// no allocation and no word to the host.
void FwVocalCompProcessor::setLayout (bool dual)
{
    layoutDual = dual;
    procs[0].setChannels (dual ? 1 : getTotalNumInputChannels());
    procs[1].reset();
    bypassLine.reset();
}

void FwVocalCompProcessor::processBlock (AudioBuffer<float>& b, MidiBuffer&)
{
    ScopedNoDenormals nd;
    flatwater::silenceNonFinite (b.getArrayOfWritePointers(), b.getNumChannels(), b.getNumSamples());
    const int n = b.getNumSamples(), ch = b.getNumChannels();
    const bool wantDual = ch > 1 && pMode->load() > 0.5f;
    if (! prepared) configure();
    if (wantDual != layoutDual) setLayout (wantDual);
    bypassLine.write (b.getArrayOfReadPointers(), ch, n);

    // input energy meter
    auto toDb = [] (float v) { return v > 1.0e-4f ? Decibels::gainToDecibels (v) : -80.0f; };
    float inPeak = 0.0f;
    for (int c = 0; c < ch; ++c) inPeak = jmax (inPeak, b.getMagnitude (c, 0, n));

    vc::dsp::Processor::Params p;
    p.compDb = pComp->load(); p.gateDb = pGate->load(); p.gainDb = pGain->load();
    float* const* w = b.getArrayOfWritePointers();
    if (! layoutDual)
    {
        procs[0].setParams (p);
        const float* const* r = w;
        procs[0].process (r, w, n);
        attnDepth.push (-procs[0].getBlockMinReductionDb());
    }
    else
    {
        for (int c = 0; c < 2; ++c)
        {
            procs[c].setParams (p);
            const float* in[1] = { w[c] };
            float* out[1] = { w[c] };
            procs[c].process (in, out, n);
        }
        attnDepth.push (-jmin (procs[0].getBlockMinReductionDb(), procs[1].getBlockMinReductionDb()));
    }

    const float peakL = b.getMagnitude (0, 0, n), peakR = ch > 1 ? b.getMagnitude (1, 0, n) : peakL;
    energyDb.push (toDb (inPeak));
    outLDb.push (toDb (peakL));
    outRDb.push (toDb (peakR));
}

void FwVocalCompProcessor::processBlockBypassed (AudioBuffer<float>& b, MidiBuffer&)
{
    if (! prepared) configure();
    // the processors' delay lines take the input too, so the release of the bypass goes on with the audio of now,
    // not with what the lines held when the bypass came on
    const int n = b.getNumSamples();
    if (const float* const* r = b.getArrayOfReadPointers(); ! layoutDual) { if (b.getNumChannels() >= jmin (2, getTotalNumInputChannels())) procs[0].feed (r, n); }
    else if (b.getNumChannels() >= 2)
        for (int c = 0; c < 2; ++c) { const float* one[1] = { r[c] }; procs[c].feed (one, n); }
    bypassLine.process (b.getArrayOfWritePointers(), b.getNumChannels(), n);
}

AudioProcessorEditor* FwVocalCompProcessor::createEditor() { return new FwVocalCompEditor (*this); }

void FwVocalCompProcessor::getStateInformation (MemoryBlock& d)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, d);
}

void FwVocalCompProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType())) apvts.replaceState (ValueTree::fromXml (*xml));
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new FwVocalCompProcessor(); }
