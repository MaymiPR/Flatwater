#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

AudioProcessorValueTreeState::ParameterLayout FwBassLevelerProcessor::layout()
{
    AudioProcessorValueTreeState::ParameterLayout l;
    auto addF = [&] (const char* id, const char* name, NormalisableRange<float> r, float def, std::function<String (float)> text)
    {
        auto a = AudioParameterFloatAttributes().withStringFromValueFunction ([text] (float v, int) { return text (v); });
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, r, def, a));
    };
    auto db1 = [] (float v) { return String (v, 1) + " dB"; };
    auto plain1 = [] (float v) { return String (v, 1); };
    addF (bl::target, "Target", { -42.0f, 0.0f, 0.1f }, -14.0f, db1);
    addF (bl::sensitivity, "Sensitivity", { 0.0f, 100.0f, 0.1f }, 50.0f, plain1);
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { bl::response, 1 }, "Response", StringArray { "Slow", "Fast" }, 0));
    addF (bl::spill, "Spill", { -50.0f, -3.0f, 0.1f }, -40.0f, db1);
    addF (bl::artifacts, "Artifacts", { 0.0f, 100.0f, 0.1f }, 50.0f, plain1);
    addF (bl::rangeMax, "Range Max", { -11.0f, 12.0f, 0.1f }, 12.0f, db1);
    addF (bl::rangeMin, "Range Min", { -12.0f, 11.0f, 0.1f }, -12.0f, db1);
    // the Idle arrow is stored as its place in the range (as the reference's preset arrays); the host sees it in dB
    // over a fixed +-12 dB (as the reference's: its text stays 0.0 at 0.5 whatever the range)
    addF (bl::idle, "Idle", { 0.02f, 0.98f, 0.01f }, 0.5f, [] (float v) { return String (bl::idleDb (v, -12.0f, 12.0f), 1); });
    addF (bl::output, "Output", { -12.0f, 12.0f, 0.1f }, 0.0f, db1);
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { bl::mode, 1 }, "Mode", bl::modeNames(), 0));
    return l;
}

FwBassLevelerProcessor::FwBassLevelerProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, &undo, "FwBassLeveler", layout())
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
    outGain = Decibels::decibelsToGain ((double) get (bl::output));
    heldDb = 0.0;
    setLatencySamples (delaySamples);
}

void FwBassLevelerProcessor::processBlock (AudioBuffer<float>& b, MidiBuffer&)
{
    ScopedNoDenormals nd;
    if (modeIndex() != configuredMode || (int) line.size() < b.getNumChannels()) configure();
    const int n = b.getNumSamples(), ch = jmin (b.getNumChannels(), (int) line.size());
    bl::RideSettings st;
    st.target = get (bl::target); st.rangeMin = get (bl::rangeMin); st.rangeMax = get (bl::rangeMax);
    st.idleDb = idleDb(); st.artifacts = get (bl::artifacts); st.sensitivity = get (bl::sensitivity);
    st.fast = get (bl::response) > 0.5f;
    st.spill = get (bl::spill); st.live = bl::isLive (modeIndex());
    const double outTo = Decibels::decibelsToGain ((double) get (bl::output));
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
