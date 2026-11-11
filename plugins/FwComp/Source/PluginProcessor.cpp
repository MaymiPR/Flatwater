#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

float co::ratioFromNorm (float v) { return std::exp (4.40703f * v - 0.494827f * v * v); }
float co::ratioToNorm (float r)
{
    // solve 0.494827 v^2 - 4.40703 v + ln r = 0 for the root in 0 .. 1
    const float a = 0.494827f, b = -4.40703f, c = std::log (jlimit (1.0f, 50.0f, r));
    return jlimit (0.0f, 1.0f, (-b - std::sqrt (b * b - 4.0f * a * c)) / (2.0f * a));
}

String co::timeText (float ms)
{
    if (ms < 10.0f) return String (ms, 2) + " ms";
    if (ms < 100.0f) return String (ms, 1) + " ms";
    return String (roundToInt (ms)) + " ms";
}

FwCompProcessor::FwCompProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                                       .withInput ("Side Chain", AudioChannelSet::stereo(), false)),
      apvts (*this, &undo, "FwComp", co::createLayout())
{
}

bool FwCompProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    if (in != out || (in != AudioChannelSet::mono() && in != AudioChannelSet::stereo())) return false;
    const auto sc = l.inputBuses.size() > 1 ? l.inputBuses[1] : AudioChannelSet::disabled();
    return sc.isDisabled() || sc == AudioChannelSet::mono() || sc == AudioChannelSet::stereo();
}

// the release time of a note value at a tempo; as the reference, a note longer than about 3.4 s (2 bars: 3.96 s) falls
// back to the next shorter straight or dotted note
float co::syncedReleaseMs (int note, double tempo)
{
    static constexpr double beats[] = { 1.0 / 24, 1.0 / 16, 3.0 / 32, 1.0 / 12, 1.0 / 8, 3.0 / 16, 1.0 / 6, 1.0 / 4, 3.0 / 8,
                                        1.0 / 3, 1.0 / 2, 3.0 / 4, 2.0 / 3, 1.0, 1.5, 4.0 / 3, 2.0, 3.0, 4.0, 8.0 };
    tempo = tempo > 0.0 ? tempo : 120.0;
    note = jlimit (0, 19, note);
    for (;;)
    {
        const double ms = beats[note] * 60000.0 / tempo;
        if (note <= 2 || ms <= (note == 19 ? 3960.0 : 3400.0)) return (float) ms;
        note = note == 19 ? 18 : note == 18 ? 17 : note - (note % 3 == 1 ? 2 : 1);   // 2 bars, 1 bar, then x/D, x, (x/2)D ...
    }
}

void FwCompProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    prepared = false;
    configure();
}

// Stereo bus + Mode "Stereo": one linked compressor. Stereo bus + Mode "Mono": two independent mono ones. Mono bus: one.
void FwCompProcessor::configure()
{
    const int ch = getMainBusNumInputChannels();
    const bool dual = ch > 1 && get (co::id::mode) > 0.5f;
    layoutDual = dual;
    comps[0].prepare (sr, dual ? 1 : ch);
    comps[1].prepare (sr, 1);
    analogStage.prepare (sr, ch);
    setLatencySamples (comps[0].latency());
    prepared = true;
}

co::dsp::Compressor::Settings FwCompProcessor::settings() const
{
    co::dsp::Compressor::Settings s;
    s.thresholdDb = get (co::id::threshold);
    s.ratio = get (co::id::ratio);
    s.attackMs = get (co::id::attack);
    s.punch = get (co::id::punch);
    s.mix = get (co::id::mix) * 0.01f;
    s.outputDb = get (co::id::output);
    s.limiter = get (co::id::limiter) > 0.5f;
    const int sync = roundToInt (get (co::id::sync));
    if (sync == 2) s.releaseMs = get (co::id::release);
    else
    {
        const double host = hostBpm.load (std::memory_order_relaxed);
        s.releaseMs = co::syncedReleaseMs (roundToInt (get (co::id::releaseNote)), sync == 0 ? (double) get (co::id::bpm) : host);
    }
    return s;
}

void FwCompProcessor::processBlock (AudioBuffer<float>& b, MidiBuffer&)
{
    ScopedNoDenormals nd;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            hostBpm.store (pos->getBpm() ? *pos->getBpm() : 0.0);
    const int n = b.getNumSamples(), ch = jmin (2, getMainBusNumInputChannels());
    const bool wantDual = ch > 1 && get (co::id::mode) > 0.5f;
    if (! prepared || wantDual != layoutDual) configure();
    // a side-chain bus, if the host enabled it, is not part of the output (and the reference ignores it)
    for (int c = getMainBusNumOutputChannels(); c < b.getNumChannels(); ++c) b.clear (c, 0, n);

    auto toDb = [] (float v) { return v > 1.0e-5f ? Decibels::gainToDecibels (v) : -100.0f; };
    inLDb.push (toDb (b.getMagnitude (0, 0, n))); inRDb.push (toDb (b.getMagnitude (ch > 1 ? 1 : 0, 0, n)));

    const auto s = settings();
    float* const* w = b.getArrayOfWritePointers();
    if (! layoutDual)
    {
        comps[0].setSettings (s);
        comps[0].process (w, n);
        grDb.push ((float) comps[0].blockGainReductionDb());
        limDb.push ((float) comps[0].blockLimiterDb());
    }
    else
    {
        for (int c = 0; c < 2; ++c)
        {
            comps[c].setSettings (s);
            float* one[1] = { w[c] };
            comps[c].process (one, n);
        }
        grDb.push ((float) jmax (comps[0].blockGainReductionDb(), comps[1].blockGainReductionDb()));
        limDb.push ((float) jmax (comps[0].blockLimiterDb(), comps[1].blockLimiterDb()));
    }
    // the Analog stage is the last one, after the output gain and the limiter (measured)
    analogStage.setMode (roundToInt (get (co::id::analog)));
    analogStage.process (w, n);
    outLDb.push (toDb (b.getMagnitude (0, 0, n))); outRDb.push (toDb (b.getMagnitude (ch > 1 ? 1 : 0, 0, n)));
    blocks.fetch_add (1, std::memory_order_relaxed);
}

AudioProcessorEditor* FwCompProcessor::createEditor() { return new FwCompEditor (*this); }

void FwCompProcessor::getStateInformation (MemoryBlock& d)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, d);
}

void FwCompProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType())) apvts.replaceState (ValueTree::fromXml (*xml));
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new FwCompProcessor(); }
