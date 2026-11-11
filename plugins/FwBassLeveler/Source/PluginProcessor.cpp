#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

FwBassLevelerProcessor::FwBassLevelerProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, &undo, "FwBassLeveler", bl::createLayout())
{
    setLatencySamples (roundToInt (bl::latencyMs (0) * 0.001 * sr));
}

bool FwBassLevelerProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    return in == out && (in == AudioChannelSet::mono() || in == AudioChannelSet::stereo());
}

void FwBassLevelerProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    configuredMode = -1;
    configure();
}

void FwBassLevelerProcessor::configure()
{
    configuredMode = modeIndex();
    delaySamples = roundToInt (bl::latencyMs (configuredMode) * 0.001 * sr);
    line.assign ((size_t) jmax (1, getMainBusNumInputChannels()), std::vector<float> ((size_t) delaySamples + 1, 0.0f));
    writePos = 0;
    ride.prepare (sr);
    ride.preHold = delaySamples;
    smooth5 = std::exp (-1.0 / (0.005 * sr));
    outGain = Decibels::decibelsToGain ((double) get (bl::id::output));
    heldDb = 0.0;
    setLatencySamples (delaySamples);
}

void FwBassLevelerProcessor::processBlock (AudioBuffer<float>& b, MidiBuffer&)
{
    ScopedNoDenormals nd;
    if (modeIndex() != configuredMode || (int) line.size() < b.getNumChannels()) configure();
    const int n = b.getNumSamples(), ch = jmin (b.getNumChannels(), (int) line.size());
    bl::dsp::RideSettings st;
    st.target = get (bl::id::target); st.rangeMin = get (bl::id::rangeMin); st.rangeMax = get (bl::id::rangeMax);
    st.idleDb = idleDb(); st.artifacts = get (bl::id::artifacts); st.sensitivity = get (bl::id::sensitivity);
    st.fast = get (bl::id::response) > 0.5f;
    st.spill = get (bl::id::spill); st.live = bl::isLive (modeIndex());
    const double outTo = Decibels::decibelsToGain ((double) get (bl::id::output));
    const bool held = userRide.load();
    const double heldTo = (double) userRideDb.load();
    if (! held && wasHeld) { returning = true; wasHeld = false; }

    // the ride listens to the mid of the input as it comes in; its gain meets the audio the component's latency later,
    // so a note's gain is in place when the note is heard (the reference's look-ahead)
    const int len = delaySamples + 1;
    int pos = writePos;
    float energyMax = -100.0f;
    double r = 0.0;
    for (int i = 0; i < n; ++i)
    {
        const double mid = ch > 1 ? 0.5 * ((double) b.getSample (0, i) + (double) b.getSample (1, i)) : (double) b.getSample (0, i);
        r = ride.push (mid, st);
        energyMax = jmax (energyMax, (float) ride.energy);
        // a held Rider: its gain, reached over 5 ms; on release the ride is rejoined over 5 ms, then followed exactly
        if (held || returning)
        {
            const double to = held ? heldTo : r;
            heldDb = to + smooth5 * (heldDb - to);
            returning = ! held && std::abs (heldDb - r) > 0.005;
            wasHeld = true;
        }
        else heldDb = r;
        outGain = outTo + smooth5 * (outGain - outTo);
        const float gainNow = (float) (Decibels::decibelsToGain (heldDb) * outGain);
        const int rd = pos - delaySamples < 0 ? pos - delaySamples + len : pos - delaySamples;
        for (int c = 0; c < ch; ++c)
        {
            auto& d = line[(size_t) c];
            d[(size_t) pos] = b.getSample (c, i);
            b.setSample (c, i, d[(size_t) rd] * gainNow);
        }
        pos = pos + 1 == len ? 0 : pos + 1;
    }
    writePos = pos;
    energyDb.push (energyMax);
    rideDb.store ((float) heldDb);
    const float pk = b.getMagnitude (0, n);
    this->outDb.push (pk > 1.0e-5f ? Decibels::gainToDecibels (pk) : -100.0f);
}

AudioProcessorEditor* FwBassLevelerProcessor::createEditor() { return new FwBassLevelerEditor (*this); }

void FwBassLevelerProcessor::getStateInformation (MemoryBlock& d)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, d);
}

void FwBassLevelerProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType())) apvts.replaceState (ValueTree::fromXml (*xml));
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new FwBassLevelerProcessor(); }
