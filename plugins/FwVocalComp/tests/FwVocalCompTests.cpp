// Unit tests for Flatwater Vocal Comp: the DSP (dsp/Engine.h) and the plug-in processor. Run: FwVocalCompTests (exit code 0 = all passed).
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <vector>
#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/Engine.h"
#include "PluginProcessor.h"
#include "shared/test/AllocCounter.h"
#include "shared/test/PresetChecks.h"
#include "shared/test/ResetChecks.h"

static int failures = 0, checks = 0;
#define CHECK(cond) do { ++checks; if (! (cond)) { ++failures; std::printf ("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } } while (0)
#define CHECK_NEAR(a, b, tol) do { ++checks; const double va = (a), vb = (b); if (! (std::fabs (va - vb) <= (tol))) { ++failures; std::printf ("FAIL %s:%d  %s = %g, expected %g +- %g\n", __FILE__, __LINE__, #a, va, vb, (double) (tol)); } } while (0)

static const double kPi = 3.14159265358979323846;

static std::vector<float> tone (double fs, double f, double secs, double db)
{
    std::vector<float> x ((size_t) (fs * secs));
    for (size_t i = 0; i < x.size(); ++i) x[i] = (float) (std::pow (10.0, db / 20.0) * std::sin (2.0 * kPi * f * (double) i / fs));
    return x;
}

// renders x through the DSP, latency compensated (the output lines up with the input)
static std::vector<float> run (const std::vector<float>& x, double fs, vc::dsp::Processor::Params p, int block = 512,
                               const vc::dsp::Tuning& t = {})
{
    vc::dsp::Processor proc;
    proc.prepare (fs, 1, t);
    proc.setParams (p);
    const int lat = proc.getLatency();
    std::vector<float> in (x); in.resize (x.size() + (size_t) lat, 0.0f);
    std::vector<float> out (in.size());
    for (size_t s = 0; s < in.size(); s += (size_t) block)
    {
        const int n = (int) std::min ((size_t) block, in.size() - s);
        const float* ip[1] = { in.data() + s }; float* op[1] = { out.data() + s };
        proc.process (ip, op, n);
    }
    return { out.begin() + lat, out.end() };
}

static double rmsDb (const std::vector<float>& x, size_t from)
{
    double s = 0; for (size_t i = from; i < x.size(); ++i) s += (double) x[i] * x[i];
    return 10.0 * std::log10 (s / (double) (x.size() - from) + 1.0e-30);
}

static double steadyGainDb (double comp, double level, double gate = -80.0, double gain = 0.0)
{
    const double fs = 48000.0;
    auto x = tone (fs, 1000.0, 1.6, level);
    vc::dsp::Processor::Params p; p.compDb = (float) comp; p.gateDb = (float) gate; p.gainDb = (float) gain;
    auto y = run (x, fs, p);
    return rmsDb (y, (size_t) (fs * 1.2)) - rmsDb (x, (size_t) (fs * 1.2));
}

static void testLatency()
{
    CHECK (vc::dsp::latencyFor (44100.0) == 64);  CHECK (vc::dsp::latencyFor (48000.0) == 64);
    CHECK (vc::dsp::latencyFor (88200.0) == 128); CHECK (vc::dsp::latencyFor (96000.0) == 128);
    CHECK (vc::dsp::latencyFor (176400.0) == 256); CHECK (vc::dsp::latencyFor (192000.0) == 256);
    // an impulse comes out exactly `latency` samples later
    vc::dsp::Processor proc; proc.prepare (48000.0, 1);
    std::vector<float> in (1000, 0.0f), out (1000);
    in[100] = 0.05f;
    const float* ip[1] = { in.data() }; float* op[1] = { out.data() };
    proc.process (ip, op, 1000);
    size_t pk = 0; for (size_t i = 0; i < out.size(); ++i) if (std::fabs (out[i]) > std::fabs (out[pk])) pk = i;
    CHECK ((int) pk == 100 + proc.getLatency());
}

static void testStaticLaw()
{
    // measured law: gain = 0.99 M / (1 + (M - 1) x), M = 10^(-comp/20), x = peak amplitude of a steady sine
    for (double comp : { 0.0, -6.0, -12.0, -24.0, -36.0 })
        for (double level : { -60.0, -30.0, -20.0, -10.0, -3.0 })
        {
            const double M = std::pow (10.0, -comp / 20.0), x = std::pow (10.0, level / 20.0);
            const double expect = 20.0 * std::log10 (0.99 * M / (1.0 + (M - 1.0) * x));
            CHECK_NEAR (steadyGainDb (comp, level), expect, 0.05);
        }
    CHECK_NEAR (steadyGainDb (0.0, -20.0), -0.0873, 0.01);                                              // the fixed x0.99
    CHECK_NEAR (steadyGainDb (-24.0, -20.0, -80.0, -6.0) - steadyGainDb (-24.0, -20.0), -6.0, 0.01);    // Gain is a trim
}

static void testGate()
{
    // hard-knee 2:1 expander: gain = level - threshold below the threshold, 0 above (comp 0 adds the fixed -0.0873 dB)
    const double fixedDb = -0.0873;
    CHECK_NEAR (steadyGainDb (0.0, -50.0, -40.0), -10.0 + fixedDb, 0.3);
    CHECK_NEAR (steadyGainDb (0.0, -70.0, -40.0), -30.0 + fixedDb, 0.3);
    CHECK_NEAR (steadyGainDb (0.0, -30.0, -40.0), fixedDb, 0.05);
    CHECK_NEAR (steadyGainDb (0.0, -20.0, -80.0), fixedDb, 0.05);               // -80 = gate off

    // gate and compressor detect the input in parallel; their gains multiply (measured on the reference to 0.01 dB)
    for (double comp : { -12.0, -36.0 })
        for (double level : { -50.0, -32.0, -22.0 })
        {
            const double M = std::pow (10.0, -comp / 20.0), x = std::pow (10.0, level / 20.0);
            const double expect = std::min (0.0, level + 20.0) + 20.0 * std::log10 (0.99 * M / (1.0 + (M - 1.0) * x));
            CHECK_NEAR (steadyGainDb (comp, level, -20.0), expect, 0.05);
        }
}

static void testBehaviour()
{
    const double fs = 48000.0;
    vc::dsp::Processor::Params p; p.compDb = -24.0f;
    // attack is immediate: a short loud burst after silence gets its full reduction at once
    std::vector<float> x (24000, 0.0f);
    auto b = tone (fs, 3000.0, 0.01, -10.0);
    for (size_t i = 0; i < b.size(); ++i) x[4800 + i] = b[i];
    auto y = run (x, fs, p);
    float pkIn = 0, pkOut = 0;
    for (size_t i = 4800; i < 4800 + b.size(); ++i) { pkIn = std::max (pkIn, std::fabs (x[i])); pkOut = std::max (pkOut, std::fabs (y[i])); }
    CHECK_NEAR (20.0 * std::log10 (pkOut / pkIn), 8.8, 0.5);

    // release is slow: 100 ms after a long loud tone ends the gain is still far from its quiet value
    auto loud = tone (fs, 3000.0, 1.0, -10.0), quiet = tone (fs, 3000.0, 1.0, -50.0);
    loud.insert (loud.end(), quiet.begin(), quiet.end());
    auto z = run (loud, fs, p);
    auto g = [&] (size_t at) { double a = 0, c = 0; for (size_t i = at; i < at + 480; ++i) { a += (double) z[i] * z[i]; c += (double) loud[i] * loud[i]; } return 10.0 * std::log10 (a / c); };
    CHECK (g ((size_t) (fs * 1.1)) < g ((size_t) (fs * 1.9)) - 3.0);

    // output stays finite on silence, denormals, huge and odd input
    std::vector<float> weird (4096, 0.0f);
    for (size_t i = 0; i < weird.size(); ++i) weird[i] = (i % 7 == 0) ? 1.0e30f : (i % 5 == 0 ? 1.0e-40f : (i % 3 == 0 ? -3.0f : 0.0f));
    auto w = run (weird, fs, p);
    bool finite = true; for (float v : w) finite = finite && std::isfinite (v);
    CHECK (finite);
}

static void testBlockSizeInvariance()
{
    const double fs = 48000.0;
    auto x = tone (fs, 440.0, 1.0, -20.0);
    for (size_t i = 0; i < x.size(); ++i) x[i] += 0.05f * std::sin (2.0f * 3.14159f * 61.0f * (float) i / 48000.0f) * ((i / 9000) % 2 ? 1.0f : 0.1f);
    vc::dsp::Processor::Params p; p.compDb = -18.0f; p.gateDb = -50.0f;
    auto a = run (x, fs, p, 512), b = run (x, fs, p, 64), c = run (x, fs, p, 37);
    double d1 = 0, d2 = 0;
    for (size_t i = 0; i < a.size(); ++i) { d1 = std::max (d1, (double) std::fabs (a[i] - b[i])); d2 = std::max (d2, (double) std::fabs (a[i] - c[i])); }
    CHECK (d1 == 0.0);
    CHECK (d2 == 0.0);
}

static void testStereoLink()
{
    vc::dsp::Processor proc; proc.prepare (48000.0, 2);
    vc::dsp::Processor::Params p; p.compDb = -24.0f; proc.setParams (p);
    auto l = tone (48000.0, 1000.0, 1.0, -10.0), r = tone (48000.0, 1300.0, 1.0, -40.0);
    std::vector<float> ol (l.size()), orr (l.size());
    const float* ip[2] = { l.data(), r.data() }; float* op[2] = { ol.data(), orr.data() };
    proc.process (ip, op, (int) l.size());
    // the quiet right channel gets the gain of the loud left one (+8.8 dB), not its own (+22 dB)
    double a = 0, c = 0; for (size_t i = 30000; i < 48000; ++i) { a += (double) orr[i] * orr[i]; c += (double) r[i - 64] * r[i - 64]; }
    CHECK_NEAR (10.0 * std::log10 (a / c), 8.8, 0.3);
}

static void testProcessor()
{
    FwVocalCompProcessor proc;
    auto* comp = proc.apvts.getParameter (vc::id::comp);
    auto* gate = proc.apvts.getParameter (vc::id::gate);
    auto* gain = proc.apvts.getParameter (vc::id::gain);
    CHECK (comp != nullptr && gate != nullptr && gain != nullptr);
    CHECK_NEAR (comp->convertFrom0to1 (comp->getDefaultValue()), 0.0, 1e-4);
    CHECK_NEAR (gate->convertFrom0to1 (gate->getDefaultValue()), -80.0, 1e-4);
    CHECK_NEAR (gain->convertFrom0to1 (gain->getDefaultValue()), 0.0, 1e-4);
    CHECK_NEAR (comp->convertFrom0to1 (0.0f), -36.0, 1e-4);
    CHECK (comp->getText (comp->convertTo0to1 (-12.0f), 10).contains ("-12.0"));

    proc.setPlayConfigDetails (2, 2, 48000.0, 512);
    proc.prepareToPlay (48000.0, 512);
    CHECK (proc.getLatencySamples() == 64);
    proc.prepareToPlay (96000.0, 512);
    CHECK (proc.getLatencySamples() == 128);
    proc.prepareToPlay (48000.0, 512);

    // processBlock gives the same audio as the bare DSP
    comp->setValueNotifyingHost (comp->convertTo0to1 (-24.0f));
    auto x = tone (48000.0, 1000.0, 0.5, -20.0);
    juce::AudioBuffer<float> buf (2, (int) x.size());
    for (int c = 0; c < 2; ++c) std::copy (x.begin(), x.end(), buf.getWritePointer (c));
    juce::MidiBuffer midi;
    proc.processBlock (buf, midi);
    vc::dsp::Processor::Params p; p.compDb = -24.0f;
    vc::dsp::Processor ref; ref.prepare (48000.0, 2); ref.setParams (p);
    std::vector<float> o0 (x.size()), o1 (x.size());
    const float* ip[2] = { x.data(), x.data() }; float* op[2] = { o0.data(), o1.data() };
    ref.process (ip, op, (int) x.size());
    double d = 0; for (size_t i = 0; i < x.size(); ++i) d = std::max (d, (double) std::fabs (buf.getSample (0, (int) i) - o0[i]));
    CHECK (d < 1.0e-6);

    // state round trip
    juce::MemoryBlock mb; proc.getStateInformation (mb);
    FwVocalCompProcessor other; other.setStateInformation (mb.getData(), (int) mb.getSize());
    CHECK_NEAR (other.apvts.getRawParameterValue (vc::id::comp)->load(), -24.0, 1e-3);

    // Mono mode on a stereo bus: independent channels
    auto* mode = proc.apvts.getParameter (vc::id::mode);
    mode->setValueNotifyingHost (1.0f);
    juce::AudioBuffer<float> b2 (2, 24000);
    auto loud = tone (48000.0, 1000.0, 0.5, -10.0), quiet = tone (48000.0, 1300.0, 0.5, -40.0);
    std::copy (loud.begin(), loud.end(), b2.getWritePointer (0)); std::copy (quiet.begin(), quiet.end(), b2.getWritePointer (1));
    proc.prepareToPlay (48000.0, 512);
    proc.processBlock (b2, midi);
    double a = 0, c = 0; for (int i = 15000; i < 24000; ++i) { a += (double) b2.getSample (1, i) * b2.getSample (1, i); c += (double) quiet[(size_t) i - 64] * quiet[(size_t) i - 64]; }
    CHECK (10.0 * std::log10 (a / c) > 15.0);          // the quiet channel keeps its own, larger, gain
}

static void testFactoryPresets()
{
    FwVocalCompProcessor proc;
    const auto& list = vc::factoryPresets();
    CHECK (! list.empty());
    CHECK (proc.getNumPrograms() == 1 + (int) list.size());
    for (size_t i = 0; i < list.size(); ++i)
    {
        for (const auto& v : list[i].values)
        {
            auto* p = proc.apvts.getParameter (v.id);
            CHECK (p != nullptr);                                        // every id exists
            if (p == nullptr) continue;
            const auto& r = p->getNormalisableRange();
            CHECK (v.value >= r.start - 1.0e-4f && v.value <= r.end + 1.0e-4f);
        }
        proc.setCurrentProgram ((int) i + 1);
        for (const auto& v : list[i].values)
            if (auto* p = proc.apvts.getParameter (v.id))
                CHECK (std::abs (p->convertFrom0to1 (p->getValue()) - v.value) <= 0.051f);   // applied (1 Hz / 0.1 dB steps)
    }
    proc.setCurrentProgram (0);
    for (auto* p : proc.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            CHECK (std::abs (rp->getValue() - rp->getDefaultValue()) < 1.0e-6f);   // program 0 = defaults
}

// the learned slow-path correction is guarded: steady tones and Compression 0 come out exactly as without it, while
// on noise at heavy compression it is active
static void testLearnedGuards()
{
    const double fs = 48000.0;
    vc::dsp::Tuning off; off.learned = false;
    auto maxDiff = [] (const std::vector<float>& a, const std::vector<float>& b, size_t from)
    { double m = 0; for (size_t i = from; i < a.size(); ++i) m = std::max (m, (double) std::fabs (a[i] - b[i])); return m; };
    for (double comp : { -12.0, -24.0, -36.0 })
        for (auto [f, level] : { std::pair<double, double> { 1000.0, -20.0 }, { 8000.0, -6.0 }, { 3000.0, -40.0 } })
        {
            const auto x = tone (fs, f, 2.5, level);
            vc::dsp::Processor::Params p; p.compDb = (float) comp;
            CHECK (maxDiff (run (x, fs, p), run (x, fs, p, 512, off), (size_t) (fs * 1.2)) < 1.0e-5);
        }
    std::vector<float> z ((size_t) fs * 2); unsigned s = 1;
    for (auto& v : z) { s = s * 1664525u + 1013904223u; v = 0.3f * ((float) (s >> 8) / 16777216.0f - 0.5f); }
    vc::dsp::Processor::Params p0; p0.compDb = 0.0f;
    CHECK (maxDiff (run (z, fs, p0), run (z, fs, p0, 512, off), 0) < 1.0e-7);
    vc::dsp::Processor::Params p1; p1.compDb = -24.0f;
    const auto a = run (z, fs, p1), b = run (z, fs, p1, 512, off);
    bool finite = true; for (float v : a) finite = finite && std::isfinite (v);
    CHECK (finite);
    CHECK (maxDiff (a, b, (size_t) (fs * 0.2)) > 1.0e-4);
}

// allIds lists every parameter in the layout's order; presetIds every one of them but Mode
static void testParamIds()
{
    FwVocalCompProcessor p;
    const auto& prms = p.getParameters();
    CHECK (prms.size() == vc::numParams);
    int same = 0;
    for (int i = 0; i < juce::jmin (prms.size(), vc::numParams); ++i)
        if (auto* hp = dynamic_cast<juce::HostedAudioProcessorParameter*> (prms[i]); hp != nullptr && hp->getParameterID() == vc::allIds[i]) ++same;
    CHECK (same == vc::numParams);
    int preset = 0;
    for (auto* pid : vc::presetIds)
        for (auto* all : vc::allIds)
            if (juce::String (pid) == all && juce::String (pid) != vc::id::mode) ++preset;
    CHECK (preset == vc::numParams - 1);
}

// The host's bypass, engaged and released in mid-stream: every bypassed sample is the input of `latency` samples
// before, from the first one on, because the delay line takes the input while the bypass is off too.
static void testBypassIsDelayedInput()
{
    FwVocalCompProcessor p;
    p.prepareToPlay (48000.0, 512);
    const int lat = p.getLatencySamples();
    CHECK (lat > 0);
    const int block = 512, blocks = 60;
    std::vector<float> x ((size_t) (block * blocks));
    unsigned int seed = 12345u;
    for (auto& v : x) { seed = seed * 1664525u + 1013904223u; v = ((float) (seed >> 8) / 8388608.0f - 1.0f) * 0.25f; }
    auto bypassedAt = [] (int b) { return (b >= 10 && b < 20) || (b >= 21 && b < 22) || (b >= 40 && b < 55); };
    juce::AudioBuffer<float> buf (2, block);
    double err = 0.0;
    int compared = 0;
    for (int b = 0; b < blocks; ++b)
    {
        for (int i = 0; i < block; ++i)
        {
            buf.setSample (0, i, x[(size_t) (b * block + i)]);
            buf.setSample (1, i, -x[(size_t) (b * block + i)]);
        }
        juce::MidiBuffer midi;
        if (! bypassedAt (b)) { p.processBlock (buf, midi); continue; }
        p.processBlockBypassed (buf, midi);
        for (int i = 0; i < block; ++i)
        {
            const float want = x[(size_t) (b * block + i - lat)];
            err = std::max (err, (double) std::fabs (buf.getSample (0, i) - want));
            err = std::max (err, (double) std::fabs (buf.getSample (1, i) + want));
            ++compared;
        }
    }
    CHECK (compared == 26 * block);
    CHECK (err == 0.0);
}

// The release of the host's bypass: the audio after it is the audio of now. Noise is processed, then silence is
// bypassed for longer than the latency, then silence is processed: nothing of the noise comes out, because the
// processor's delay line took the input while the bypass was on.
static void testBypassReleaseHasNoOldAudio()
{
    for (int dual = 0; dual < 2; ++dual)                       // Mode Stereo (one linked gain), Mode Mono (two)
    {
        FwVocalCompProcessor p;
        auto* mode = p.apvts.getParameter (vc::id::mode);
        mode->setValueNotifyingHost (mode->convertTo0to1 ((float) dual));
        p.prepareToPlay (48000.0, 512);
        const int lat = p.getLatencySamples();
        CHECK (lat > 0);
        const int block = 512, quiet = lat / block + 2;
        juce::AudioBuffer<float> buf (2, block);
        juce::MidiBuffer midi;
        unsigned int seed = 777u;
        for (int b = 0; b < 20; ++b)                           // noise, processed
        {
            for (int i = 0; i < block; ++i)
            {
                seed = seed * 1664525u + 1013904223u;
                const float v = ((float) (seed >> 8) / 8388608.0f - 1.0f) * 0.25f;
                buf.setSample (0, i, v); buf.setSample (1, i, -v);
            }
            p.processBlock (buf, midi);
        }
        for (int b = 0; b < quiet; ++b) { buf.clear(); p.processBlockBypassed (buf, midi); }   // silence, bypassed
        buf.clear();
        p.processBlock (buf, midi);                            // silence, processed: the delayed silence
        CHECK (buf.getMagnitude (0, block) == 0.0f);
    }
}

// Mode changed between two blocks (a host automates it): the block that takes the change allocates nothing on the
// calling thread, the latency stays, and the audio from the change on is that of a processor prepared in the new
// Mode (the change is a full reset).
static void testModeChangeOnTheAudioThread()
{
    auto set = [] (FwVocalCompProcessor& q, const char* id, float v) { auto* a = q.apvts.getParameter (id); a->setValueNotifyingHost (a->convertTo0to1 (v)); };
    FwVocalCompProcessor p;
    set (p, vc::id::comp, -24.0f);
    p.prepareToPlay (48000.0, 512);
    juce::AudioBuffer<float> buf (2, 512), in (2, 512);
    juce::MidiBuffer midi;
    unsigned int seed = 4242u;
    auto fill = [&seed] (juce::AudioBuffer<float>& b)
    {
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            seed = seed * 1664525u + 1013904223u;
            const float v = ((float) (seed >> 8) / 8388608.0f - 1.0f) * 0.25f;
            b.setSample (0, i, v); b.setSample (1, i, 0.5f * v * v - v);
        }
    };
    auto same = [] (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
    {
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < a.getNumSamples(); ++i)
                if (! juce::exactlyEqual (a.getSample (c, i), b.getSample (c, i))) return false;
        return true;
    };
    for (int b = 0; b < 8; ++b) { fill (buf); p.processBlock (buf, midi); }
    for (int mode : { 1, 0, 1 })
    {
        set (p, vc::id::mode, (float) mode);
        FwVocalCompProcessor fresh;
        set (fresh, vc::id::comp, -24.0f); set (fresh, vc::id::mode, (float) mode);
        fresh.prepareToPlay (48000.0, 512);
        for (int b = 0; b < 4; ++b)
        {
            fill (buf);
            in.makeCopyOf (buf);
            int allocations = 0;
            {
                flatwater::test::AllocScope scope;
                p.processBlock (buf, midi);
                allocations = scope.count();
            }
            CHECK (allocations == 0);
            fresh.processBlock (in, midi);
            CHECK (same (buf, in));
        }
        CHECK (p.getLatencySamples() == 64);
    }
}

// reset(), which a host calls to clear the tails (shared/test/ResetChecks.h): Comp at -12, so it compresses
static void testReset()
{
    const auto check = [] (bool ok, const char* what) { ++checks; if (! ok) { ++failures; std::printf ("FAIL reset: %s\n", what); } };
    flatwater::test::resetChecks<FwVocalCompProcessor> ([] (FwVocalCompProcessor& p)
    {
        auto set = [&p] (const char* id, float v) { auto* q = p.apvts.getParameter (id); q->setValueNotifyingHost (q->convertTo0to1 (v)); };
        set (vc::id::comp, -12.0f);
        p.setRateAndBufferSizeDetails (48000.0, 512);
        p.prepareToPlay (48000.0, 512);
    }, check);
}

// presets, programs and the saved state (shared/test/PresetChecks.h)
static void testPresetState()
{
    const auto check = [] (bool ok, const char* what) { ++checks; if (! ok) { ++failures; std::printf ("FAIL preset state: %s\n", what); } };
    flatwater::test::programChecks<FwVocalCompProcessor> (vc::id::mode, check);
    flatwater::test::presetChecks<FwVocalCompProcessor> (vc::id::mode, check);
    flatwater::test::windowChecks<FwVocalCompProcessor> (check);
}

int main()
{
    testBypassIsDelayedInput();
    testBypassReleaseHasNoOldAudio();
    juce::ScopedJuceInitialiser_GUI init;
    testParamIds();
    testLatency();
    testStaticLaw();
    testGate();
    testBehaviour();
    testBlockSizeInvariance();
    testStereoLink();
    testProcessor();
    testFactoryPresets();
    testModeChangeOnTheAudioThread();
    testLearnedGuards();
    testReset();
    testPresetState();
    std::printf ("%d checks, %d failed\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
