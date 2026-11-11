#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "shared/ui/FlatUI.h"

using namespace juce;

FwBassXLiftProcessor::FwBassXLiftProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, &undo, "FwBassXLift", bx::createLayout())
{
}

bool FwBassXLiftProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    return in == out && (in == AudioChannelSet::mono() || in == AudioChannelSet::stereo());
}

void FwBassXLiftProcessor::prepareToPlay (double sampleRate, int)
{
    dsp.prepare (sampleRate, jmin (2, getTotalNumInputChannels()));
}

void FwBassXLiftProcessor::processBlock (AudioBuffer<float>& b, MidiBuffer&)
{
    ScopedNoDenormals nd;
    const int n = b.getNumSamples(), ch = jmin (2, b.getNumChannels());
    dsp.setInputDb (get (bx::id::input));
    dsp.setFreq (get (bx::id::freq));
    dsp.setDecayDb (get (bx::id::decay));
    dsp.setResponseMs (get (bx::id::response));
    dsp.setRatio (get (bx::id::ratio));
    dsp.setHighPass ((int) get (bx::id::highpass));
    dsp.setOrigBassDb (get (bx::id::origBass));
    dsp.setHarmonicsDb (get (bx::id::harmonics));
    dsp.setMonitor ((int) get (bx::id::monitor));
    dsp.process (b.getArrayOfWritePointers(), ch, n);
    auto toDb = [] (float v) { return v > 1.0e-5f ? Decibels::gainToDecibels (v) : -100.0f; };
    const float peakL = b.getMagnitude (0, 0, n), peakR = ch > 1 ? b.getMagnitude (1, 0, n) : peakL;
    outLDb.push (toDb (peakL));
    outRDb.push (toDb (peakR));
    if (jmax (peakL, peakR) >= 0.999999f) clipped.store (true);       // the reference's clip level
    blocks.fetch_add (1, std::memory_order_relaxed);
}

AudioProcessorEditor* FwBassXLiftProcessor::createEditor() { return new FwBassXLiftEditor (*this); }

void FwBassXLiftProcessor::getStateInformation (MemoryBlock& d)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, d);
}

void FwBassXLiftProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType())) apvts.replaceState (ValueTree::fromXml (*xml));
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new FwBassXLiftProcessor(); }
