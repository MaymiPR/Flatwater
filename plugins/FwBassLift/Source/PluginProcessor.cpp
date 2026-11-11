#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

FwBassLiftProcessor::FwBassLiftProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, &undo, "FwBassLift", bf::createLayout())
{
}

bool FwBassLiftProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    return in == out && (in == AudioChannelSet::mono() || in == AudioChannelSet::stereo());
}

void FwBassLiftProcessor::prepareToPlay (double sampleRate, int)
{
    dsp.prepare (sampleRate, getTotalNumInputChannels());
}

void FwBassLiftProcessor::processBlock (AudioBuffer<float>& b, MidiBuffer&)
{
    ScopedNoDenormals nd;
    const int n = b.getNumSamples(), ch = jmin (2, b.getNumChannels());
    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    dsp.setFreq (get (bf::id::freq));
    dsp.setIntensityDb (get (bf::id::intensity));
    dsp.setOriginalBass (get (bf::id::origBass) > 0.5f);
    dsp.setGainDb (get (bf::id::gain));
    dsp.setDecay (get (bf::id::decay));
    dsp.process (b.getArrayOfWritePointers(), ch, n);

    auto toDb = [] (float v) { return v > 1.0e-4f ? Decibels::gainToDecibels (v) : -80.0f; };
    const float peakL = b.getMagnitude (0, 0, n), peakR = ch > 1 ? b.getMagnitude (1, 0, n) : peakL;
    origDb.push (toDb (dsp.lastOrigPeak));
    harmDb.push (toDb (dsp.lastHarmPeak));
    outLDb.push (toDb (peakL));
    outRDb.push (toDb (peakR));
    if (jmax (peakL, peakR) >= (float) dsp.t.ceiling) clipped.store (true);
}

AudioProcessorEditor* FwBassLiftProcessor::createEditor() { return new FwBassLiftEditor (*this); }

void FwBassLiftProcessor::getStateInformation (MemoryBlock& d)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, d);
}

void FwBassLiftProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType())) apvts.replaceState (ValueTree::fromXml (*xml));
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new FwBassLiftProcessor(); }
