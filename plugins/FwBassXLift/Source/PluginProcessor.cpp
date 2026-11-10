#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "shared/ui/FlatUI.h"

using namespace juce;

AudioProcessorValueTreeState::ParameterLayout FwBassXLiftProcessor::layout()
{
    AudioProcessorValueTreeState::ParameterLayout l;
    auto addF = [&] (const char* id, const char* name, NormalisableRange<float> r, float def, std::function<String (float)> text)
    {
        auto a = AudioParameterFloatAttributes().withStringFromValueFunction ([text] (float v, int) { return text (v); });
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, r, def, a));
    };
    auto addC = [&] (const char* id, const char* name, StringArray items, int def)
    {
        l.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, items, def));
    };
    auto logRange = [] (float lo, float hi, float step)
    {
        return NormalisableRange<float> (lo, hi,
            [] (float s, float e, float n) { return s * std::pow (e / s, n); },
            [] (float s, float e, float v) { return std::log (v / s) / std::log (e / s); },
            [step] (float, float, float v) { return std::round (v / step) * step; });
    };
    auto db = [] (float v) { return flatwater::flat::gainText (v) + " dB"; };

    addF (bx::input, "Input", flatwater::flat::gainRange (0.0f), 0.0f, db);
    addF (bx::freq, "Frequency", logRange (32.0f, 256.0f, 1.0f), 80.0f, [] (float v) { return String (roundToInt (v)) + " Hz"; });
    addF (bx::decay, "Decay", { -24.0f, -9.0f, 0.1f }, -15.0f, [] (float v) { return String (v, 1) + " dB"; });
    addF (bx::response, "Response", { 10.0f, 30.0f, 0.1f }, 20.0f, [] (float v) { return String (v, 1) + " ms"; });
    addF (bx::ratio, "Ratio", logRange (1.0f, 4.0f, 0.01f), 1.0f, [] (float v) { return String (v, 2) + ":1"; });
    addC (bx::monitor, "Monitor", { "Audio", "Harmonics", "Original Bass" }, 0);
    addC (bx::highpass, "High Pass", { "6 dB/oct", "12 dB/oct", "24 dB/oct" }, 1);
    addF (bx::origBass, "Original Bass", flatwater::flat::gainRange (12.0f), 0.0f, db);
    addF (bx::harmonics, "Harmonics", flatwater::flat::gainRange (12.0f), -100.0f, db);
    addC (bx::mode, "Mode", { "Stereo", "Mono" }, 0);
    return l;
}

FwBassXLiftProcessor::FwBassXLiftProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, &undo, "FwBassXLift", layout())
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
    dsp.setInputDb (get (bx::input));
    dsp.setFreq (get (bx::freq));
    dsp.setDecayDb (get (bx::decay));
    dsp.setResponseMs (get (bx::response));
    dsp.setRatio (get (bx::ratio));
    dsp.setHighPass ((int) get (bx::highpass));
    dsp.setOrigBassDb (get (bx::origBass));
    dsp.setHarmonicsDb (get (bx::harmonics));
    dsp.setMonitor ((int) get (bx::monitor));
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
