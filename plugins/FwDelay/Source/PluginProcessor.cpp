#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

namespace
{
// the upper half of the reference's Delay taper (ms at normalised 0.5, 0.505, ... 1; read from the reference)
constexpr float kDelayUpper[101] = {
    501, 506, 513, 527, 542, 557, 572, 588, 604, 620, 636, 653, 670, 687, 704, 722, 740, 758, 777, 796, 815, 835, 854, 874,
    895, 916, 937, 958, 980, 1002, 1024, 1047, 1070, 1093, 1116, 1140, 1165, 1189, 1214, 1239, 1265, 1291, 1317, 1344, 1371,
    1398, 1426, 1454, 1482, 1511, 1540, 1570, 1599, 1630, 1660, 1691, 1723, 1754, 1786, 1819, 1852, 1885, 1918, 1952, 1987,
    2021, 2057, 2092, 2128, 2164, 2201, 2238, 2276, 2314, 2352, 2391, 2430, 2470, 2510, 2550, 2591, 2632, 2674, 2716, 2759,
    2802, 2845, 2889, 2933, 2978, 3023, 3069, 3115, 3162, 3209, 3256, 3304, 3352, 3401, 3450, 3500 };
}

double de::noteBeats (int note)
{
    // the triplets a hair short of their thirds (measured: 1/8T at 120 BPM is 7999 samples at 48 kHz, not 8000)
    constexpr double t = 1.0 - 3.0e-9;
    static constexpr double beats[] = { t / 24, 1.0 / 16, 3.0 / 32, t / 12, 1.0 / 8, 3.0 / 16, t / 6, 1.0 / 4, 3.0 / 8,
                                        t / 3, 1.0 / 2, 3.0 / 4, 2.0 * t / 3, 1.0, 1.5, 4.0 * t / 3, 2.0, 3.0, 4.0, 8.0 };
    return beats[jlimit (0, 19, note)];
}

int de::nearestNote (double ms, double tempo)
{
    tempo = tempo > 0.0 ? tempo : 120.0;
    int best = 0;
    for (int i = 1; i < 20; ++i)
        if (std::abs (std::log (noteBeats (i) * 60000.0 / tempo / ms)) < std::abs (std::log (noteBeats (best) * 60000.0 / tempo / ms))) best = i;
    return best;
}

float de::delayFromNorm (float n)
{
    n = jlimit (0.0f, 1.0f, n);
    if (n <= 0.5f) return 1.0f + 1000.0f * n;
    const float f = (n - 0.5f) * 200.0f;
    const int i = jmin (99, (int) f);
    return kDelayUpper[i] + (kDelayUpper[i + 1] - kDelayUpper[i]) * (f - (float) i);
}

float de::delayToNorm (float ms)
{
    ms = jlimit (1.0f, 3500.0f, ms);
    if (ms <= 501.0f) return (ms - 1.0f) / 1000.0f;
    int i = 0;
    while (i < 99 && kDelayUpper[i + 1] < ms) ++i;
    return 0.5f + ((float) i + (ms - kDelayUpper[i]) / (kDelayUpper[i + 1] - kDelayUpper[i])) / 200.0f;
}

// as the reference shows them: "Off", "27Hz", "1.24kHz", "20.1kHz"
String de::freqText (float hz, bool hiPass)
{
    if (hiPass ? hz < kHiPassOff : hz >= kLoPassOff) return "Off";
    if (hz < 1000.0f) return String (roundToInt (hz)) + "Hz";
    return String (hz / 1000.0f, hz < 10000.0f ? 2 : 1) + "kHz";
}

// "0.10 Hz", "24.5 Hz", "128 Hz", "1.15k Hz"
String de::rateText (float hz)
{
    if (hz < 10.0f) return String (hz, 2) + " Hz";
    if (hz < 100.0f) return String (hz, 1) + " Hz";
    if (hz < 1000.0f) return String (roundToInt (hz)) + " Hz";
    return String (hz / 1000.0f, 2) + "k Hz";
}

FwDelayProcessor::FwDelayProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, &undo, "FwDelay", de::createLayout())
{
    analogStage.modes = de::dsp::tables::analog;                   // the reference's Analog modes (4 is Flatwater Comp's)
    apvts.state.addListener (this);
}

void FwDelayProcessor::readExact()
{
    const auto t = apvts.state.getChildWithName (flatwater::presetExactTag);
    for (auto& e : exact)
    {
        const auto* p = apvts.getParameter (e.id);
        const bool has = t.isValid() && t.hasProperty (e.id) && p != nullptr;
        const float v = has ? (float) t.getProperty (e.id) : std::numeric_limits<float>::quiet_NaN();
        e.norm.store (has ? p->convertTo0to1 (p->getNormalisableRange().snapToLegalValue (v)) : -1.0f);
        e.value.store (v);
    }
}

float FwDelayProcessor::value (const char* id, bool* exactFromPreset) const
{
    if (exactFromPreset != nullptr) *exactFromPreset = false;
    for (auto& e : exact)
        if (std::strcmp (e.id, id) == 0)
        {
            const float v = e.value.load (std::memory_order_relaxed);
            if (! std::isnan (v) && std::abs (apvts.getParameter (id)->getValue() - e.norm.load (std::memory_order_relaxed)) < 1.0e-6f)
            {
                if (exactFromPreset != nullptr) *exactFromPreset = true;
                return v;
            }
        }
    return de::atResolution (id, get (id));
}

// mono or stereo, in and out alike, or mono in and stereo out (the reference's mono-to-stereo component)
bool FwDelayProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    if (out != AudioChannelSet::mono() && out != AudioChannelSet::stereo()) return false;
    return in == out || (in == AudioChannelSet::mono() && out == AudioChannelSet::stereo());
}

void FwDelayProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    prepared = false;
    configure();
}

void FwDelayProcessor::configure()
{
    const int in = getMainBusNumInputChannels(), out = getMainBusNumOutputChannels();
    layoutDual = in > 1 && roundToInt (get (de::id::mode)) == 1;
    for (auto& d : delays) d.prepare (sr);
    analogStage.prepare (sr, jmax (in, out));
    prepared = true;
}

// the reference uses its parameters at their resolution (measured: Delay 100.4 ms works as 100): Delay,
// Feedback, Mix, BPM, Depth and the filters in whole units, Output in 0.1 dB, Rate in 0.01 Hz
float de::atResolution (const char* id, float v)
{
    const String p (id);
    if (p == id::delay || p == id::delayR || p == id::feedback || p == id::mix || p == id::bpm || p == id::depth || p == id::hipass || p == id::lopass) return std::round (v);
    if (p == id::output) return std::round (v * 10.0f) / 10.0f;
    if (p == id::rate)                                                 // three significant digits, at least 0.01 (measured:
    {                                                              // 3074 acts as 3070, 281.7 as 282, 47.84 as 47.8)
        const float step = v >= 1000.0f ? 10.0f : v >= 100.0f ? 1.0f : v >= 10.0f ? 0.1f : 0.01f;
        return std::round (v / step) * step;
    }
    return v;
}

de::dsp::Settings FwDelayProcessor::settings (double fs) const
{
    de::dsp::Settings s;
    auto get = [this] (const char* id) { return value (id); };
    const int sync = roundToInt (get (de::id::sync));
    auto samples = [&] (const char* msId, const char* noteId)
    {
        double ms = get (msId);
        if (sync != 2)
        {
            const double host = hostBpm.load (std::memory_order_relaxed);
            const double tempo = sync == 0 ? (double) get (de::id::bpm) : (host > 0.0 ? host : 120.0);
            ms = de::noteBeats (roundToInt (get (noteId))) * 60000.0 / tempo;
        }
        return jlimit (1.0, 3500.0, ms) * fs / 1000.0;
    };
    s.timeL = samples (de::id::delay, de::id::delayNote);
    s.timeR = samples (de::id::delayR, de::id::delayNoteR);
    s.feedback = get (de::id::feedback) * 0.01f;
    s.mix = get (de::id::mix) * 0.01f;
    s.outputDb = get (de::id::output);
    bool lpPreset = false;
    const float hp = get (de::id::hipass), lp = value (de::id::lopass, &lpPreset);
    s.hipassHz = hp < de::kHiPassOff ? 0.0f : hp;
    // set from outside, LoPass is Off from 20 100; a preset's is Off at exactly 20 100 (its 20 139 filters, measured)
    s.lopassHz = (lpPreset ? juce::exactlyEqual (lp, de::kLoPassOff) : lp >= de::kLoPassOff) ? 0.0f : lp;
    s.depth = get (de::id::depth) * 0.01f;
    s.rateHz = get (de::id::rate);
    if (sync != 2)                                                 // synced: Rate is in cycles per beat at 60 BPM (measured)
    {
        const double host = hostBpm.load (std::memory_order_relaxed);
        s.rateHz *= (float) ((sync == 0 ? (double) get (de::id::bpm) : (host > 0.0 ? host : 120.0)) / 60.0);
    }
    s.lofi = get (de::id::lofi) > 0.5f;
    s.pingPong = get (de::id::pingPong) > 0.5f;
    s.startRight = get (de::id::pingPongStart) > 0.5f;
    s.phaseL = get (de::id::phaseL) > 0.5f;
    s.phaseR = get (de::id::phaseR) > 0.5f;
    s.dual = get (de::id::dual) > 0.5f;
    return s;
}

// Stereo: one delay with two lines (linked gain stage, Ping Pong, Dual). Mode Mono on a stereo bus: two independent
// mono delays. M>S (or a mono input on a stereo output): the input on both sides of a stereo delay. The Analog stage
// colours the input, before the dry / wet split (measured: its noise comes back in the echoes).
void FwDelayProcessor::processBlock (AudioBuffer<float>& b, MidiBuffer&)
{
    ScopedNoDenormals nd;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            hostBpm.store (pos->getBpm() ? *pos->getBpm() : 0.0);
    const int n = b.getNumSamples();
    const int in = getMainBusNumInputChannels(), out = getMainBusNumOutputChannels();
    const int mode = roundToInt (get (de::id::mode));
    const bool wantDual = in > 1 && mode == 1;
    if (! prepared || wantDual != layoutDual) configure();
    if (in == 1 && out == 2) b.copyFrom (1, 0, b, 0, 0, n);           // mono in, stereo out: both sides get the input
    else if (out == 2 && mode == 2) b.copyFrom (1, 0, b, 0, 0, n);    // M>S on a stereo bus: the left input on both
    float* const* w = b.getArrayOfWritePointers();
    analogStage.setMode (roundToInt (get (de::id::analog)));
    analogStage.process (w, n);
    auto s = settings (sr);
    if (out == 1) { s.mode = 1; delays[0].process (w, 1, n, s); }
    else if (layoutDual)
    {
        s.mode = 1;
        for (int c = 0; c < 2; ++c) { float* one[1] = { w[c] }; delays[c].process (one, 1, n, s); }
    }
    else { s.mode = 0; delays[0].process (w, 2, n, s); }
    auto toDb = [] (float v) { return v > 1.0e-5f ? Decibels::gainToDecibels (v) : -100.0f; };
    outLDb.push (toDb (b.getMagnitude (0, 0, n)));
    outRDb.push (toDb (b.getMagnitude (out > 1 ? 1 : 0, 0, n)));
    blocks.fetch_add (1, std::memory_order_relaxed);
}

AudioProcessorEditor* FwDelayProcessor::createEditor() { return new FwDelayEditor (*this); }

void FwDelayProcessor::getStateInformation (MemoryBlock& d)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, d);
}

void FwDelayProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType())) apvts.replaceState (ValueTree::fromXml (*xml));
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new FwDelayProcessor(); }
