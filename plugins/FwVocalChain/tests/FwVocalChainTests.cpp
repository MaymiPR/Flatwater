// Unit tests for Flatwater Vocal Chain (Phase 2): the parameters as the reference's (ranges, defaults, the colours' texts), the
// components' buses, the latency, the audio path (the gains, the delay), the de-esser, and the editor (its size, the Mono/Stereo
// component's layout, the colour selectors' clicks, the faders' grab area and the selection, a snapshot paints).
// Run: FwVocalChainTests (exit code 0 = all passed).
#include <limits>
#include <algorithm>
#include <cmath>
#include <vector>
#include <cstdio>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "shared/test/AllocCounter.h"
#include "shared/test/PresetChecks.h"

static int failures = 0, checks = 0;
#define CHECK(cond) do { ++checks; if (! (cond)) { ++failures; std::printf ("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } } while (0)
#define CHECK_NEAR(a, b, tol) do { ++checks; const double va = (a), vb = (b); if (! (std::fabs (va - vb) <= (tol))) { ++failures; std::printf ("FAIL %s:%d  %s = %g, expected %g +- %g\n", __FILE__, __LINE__, #a, va, vb, (double) (tol)); } } while (0)

static void set (FwVocalChainProcessor& p, const char* id, float v)
{
    auto* q = p.apvts.getParameter (id);
    q->setValueNotifyingHost (q->convertTo0to1 (v));
}

// the reference's VST3 parameters (RESEARCH.md, Flatwater Vocal Chain): the faders -10 .. +10 in 0.1 (Balance -6 .. +6) from 0, the
// colours four choices from the second
static void testParameters()
{
    FwVocalChainProcessor p;
    for (auto id : { ch::id::sensitivity, ch::id::bass, ch::id::treble, ch::id::compress, ch::id::reverb,
                     ch::id::delay, ch::id::pitch, ch::id::output })
    {
        auto* q = dynamic_cast<juce::AudioParameterFloat*> (p.apvts.getParameter (id));
        CHECK (q != nullptr);
        if (q == nullptr) continue;
        CHECK_NEAR (q->range.start, -10.0, 1e-6);
        CHECK_NEAR (q->range.end, 10.0, 1e-6);
        CHECK_NEAR (q->range.interval, 0.1, 1e-6);
        CHECK_NEAR (q->get(), 0.0, 1e-6);
    }
    CHECK_NEAR (p.apvts.getParameter (ch::id::balance)->convertFrom0to1 (1.0f), 6.0, 1e-6);
    for (const auto& st : ch::stages())
    {
        auto* c = dynamic_cast<juce::AudioParameterChoice*> (p.apvts.getParameter (st.colour));
        CHECK (c != nullptr && c->getIndex() == 1 && c->choices.size() == 4);
    }
    CHECK (p.apvts.getParameter (ch::id::compressColor)->getCurrentValueAsText() == "Push");
    CHECK (p.apvts.getParameter (ch::id::reverbColor)->getText (0.0f, 64) == "Mute");
    CHECK (p.apvts.getParameter (ch::id::pitchColor)->getText (1.0f, 64) == "Spreader");
}

// Stereo (2 in, 2 out) and Mono/Stereo (1 in, 2 out); the reference's latency (193 samples at 48 kHz, 385 at 96)
static void testBusesAndLatency()
{
    FwVocalChainProcessor p;
    juce::AudioProcessor::BusesLayout l;
    l.inputBuses.add (juce::AudioChannelSet::stereo()); l.outputBuses.add (juce::AudioChannelSet::stereo());
    CHECK (p.checkBusesLayoutSupported (l));
    l.inputBuses.getReference (0) = juce::AudioChannelSet::mono();
    CHECK (p.checkBusesLayoutSupported (l));
    l.outputBuses.getReference (0) = juce::AudioChannelSet::quadraphonic();
    CHECK (! p.checkBusesLayoutSupported (l));
    p.prepareToPlay (48000.0, 512);
    CHECK (p.getLatencySamples() == 193);
    p.prepareToPlay (96000.0, 512);
    CHECK (p.getLatencySamples() == 385);
    CHECK (ch::latencySamples (44100.0) == 193);
}

// the audio: nothing before the latency, then the reference's fixed filter (with every colour clear its impulse
// response starts +0.278, -0.721, -0.130: MEASUREMENTS.md), Sensitivity and Output as exact gains
static void testPath()
{
    FwVocalChainProcessor p;
    p.setPlayConfigDetails (2, 2, 48000.0, 512);
    for (const auto& st : ch::stages()) set (p, st.colour, 0.0f);
    set (p, ch::id::sensitivity, 6.0f);
    set (p, ch::id::output, -6.0f);
    p.prepareToPlay (48000.0, 512);
    juce::AudioBuffer<float> b (2, 512);
    juce::MidiBuffer m;
    b.clear(); b.setSample (0, 10, 0.01f); b.setSample (1, 10, 0.01f);  // (a small impulse: the chain's slight distortion)
    p.processBlock (b, m);
    CHECK_NEAR (b.getSample (0, 10 + 192) / 0.01, 0.0, 1e-3);
    CHECK_NEAR (b.getSample (0, 10 + 193) / 0.01, 0.278, 0.002);        // (+6 then -6 dB: unity)
    CHECK_NEAR (b.getSample (0, 10 + 194) / 0.01, -0.721, 0.002);
    CHECK_NEAR (b.getSample (1, 10 + 195) / 0.01, -0.130, 0.002);
}

// the main de-esser (every colour clear): a steady tone's level out at 0 dBFS against at -30 dBFS (dB, its share above
// -30's): the reference turns 10 kHz down 13.2 dB and 6 kHz 11.0, leaves 1 kHz alone (MEASUREMENTS.md, Phase 4)
static double toneGain (double freq, double peakDb)
{
    FwVocalChainProcessor p;
    p.setPlayConfigDetails (2, 2, 48000.0, 512);
    for (const auto& st : ch::stages()) set (p, st.colour, 0.0f);
    p.prepareToPlay (48000.0, 512);
    juce::AudioBuffer<float> b (2, 512);
    juce::MidiBuffer m;
    const double a = std::pow (10.0, peakDb / 20.0);
    double in = 0.0, out = 0.0;
    for (int blk = 0, t = 0; blk < 48; ++blk)
    {
        for (int i = 0; i < 512; ++i, ++t)
        {
            const float v = (float) (a * std::sin (2.0 * juce::MathConstants<double>::pi * freq * t / 48000.0));
            b.setSample (0, i, v); b.setSample (1, i, v);
            if (blk >= 32) in += (double) v * v;
        }
        p.processBlock (b, m);
        if (blk >= 32) for (int i = 0; i < 512; ++i) out += (double) b.getSample (0, i) * b.getSample (0, i);
    }
    return 10.0 * std::log10 (out / in);
}

static void testDeEsser()
{
    CHECK_NEAR (toneGain (10000.0, 0.0) - toneGain (10000.0, -30.0), -13.2, 1.0);
    CHECK_NEAR (toneGain (6000.0, 0.0) - toneGain (6000.0, -30.0), -11.0, 1.0);
    CHECK_NEAR (toneGain (1000.0, 0.0) - toneGain (1000.0, -30.0), 0.0, 0.2);
}

static void testEditor()
{
    FwVocalChainProcessor p;
    std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
    CHECK (ed != nullptr);
    if (ed == nullptr) return;
    ed->setSize (FwVocalChainEditor::W, FwVocalChainEditor::H);
    auto img = ed->createComponentSnapshot (ed->getLocalBounds());
    CHECK (img.getWidth() == FwVocalChainEditor::W);
    // a colour selector's button steps the colour; a click on the selected name turns it off
    std::vector<ch::ui::ColourSelector*> cs;
    std::function<void (juce::Component&)> find = [&] (juce::Component& c)
    {
        for (auto* k : c.getChildren())
        {
            if (auto* s = dynamic_cast<ch::ui::ColourSelector*> (k)) cs.push_back (s);
            find (*k);
        }
    };
    find (*ed);
    CHECK (cs.size() == 6);
    if (cs.size() == 6)
    {
        cs[0]->setState ((cs[0]->state() + 1) % 4);
        CHECK (p.apvts.getParameter (ch::id::bassColor)->getCurrentValueAsText() == "Lower");
        cs[0]->setState (0);
        CHECK (p.apvts.getParameter (ch::id::bassColor)->getCurrentValueAsText() == "Bypass");
    }
    // the selection (filmed on the reference): a fader is grabbed on its cap only (34 x 58, at 0 from 75.5 px down its
    // 215 px control); one control marked at a time; its value shows only while the mouse is on it
    std::vector<ch::ui::Fader*> fs;
    std::function<void (juce::Component&)> findF = [&] (juce::Component& c)
    {
        for (auto* k : c.getChildren())
        {
            if (auto* f = dynamic_cast<ch::ui::Fader*> (k)) fs.push_back (f);
            findF (*k);
        }
    };
    findF (*ed);
    CHECK (fs.size() == 8);
    if (fs.size() == 8)
    {
        CHECK_NEAR (fs[1]->capTop(), 75.5, 1e-3);
        CHECK (fs[1]->onCap ({ 17.0f, 100.0f }));
        CHECK (! fs[1]->onCap ({ 17.0f, 60.0f }) && ! fs[1]->onCap ({ 17.0f, 140.0f }) && ! fs[1]->onCap ({ 38.0f, 100.0f }));
        auto& mk = dynamic_cast<FwVocalChainEditor&> (*ed).marks;
        mk.select (fs[1]);
        CHECK (! mk.showsValue (fs[1]));
        mk.hover = fs[1];
        CHECK (mk.showsValue (fs[1]) && ! mk.showsValue (fs[2]));
        mk.select (fs[2]);
        CHECK (! mk.showsValue (fs[1]));
        mk.clear();
        CHECK (mk.selected == nullptr);
        mk.hover = nullptr;
    }
    set (p, ch::id::mode, 1.0f);                                      // Mono/Stereo: no Balance
    img = ed->createComponentSnapshot (ed->getLocalBounds());
    CHECK (img.getHeight() == FwVocalChainEditor::H);
}

// allIds lists every parameter in the layout's order; presetIds every one of them but Mode
static void testParamIds()
{
    FwVocalChainProcessor p;
    const auto& prms = p.getParameters();
    CHECK (prms.size() == ch::numParams);
    int same = 0;
    for (int i = 0; i < juce::jmin (prms.size(), ch::numParams); ++i)
        if (auto* hp = dynamic_cast<juce::HostedAudioProcessorParameter*> (prms[i]); hp != nullptr && hp->getParameterID() == ch::allIds[i]) ++same;
    CHECK (same == ch::numParams);
    int preset = 0;
    for (auto* pid : ch::presetIds)
        for (auto* all : ch::allIds)
            if (juce::String (pid) == all && juce::String (pid) != ch::id::mode) ++preset;
    CHECK (preset == ch::numParams - 1);
}

// The host's bypass, engaged and released in mid-stream: every bypassed sample is the input of `latency` samples
// before, from the first one on, because the delay line takes the input while the bypass is off too.
static void testBypassIsDelayedInput()
{
    FwVocalChainProcessor p;
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

// A sample that is not a number, or infinite, is taken as silence: nothing of it stays in a filter, and the blocks
// after it are finite again.
static void testNonFiniteInput()
{
    FwVocalChainProcessor p;
    p.prepareToPlay (48000.0, 512);
    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    bool finite = true;
    for (int b = 0; b < 30; ++b)
    {
        for (int i = 0; i < 512; ++i) { const float v = 0.2f * std::sin (0.05f * (float) (b * 512 + i)); buf.setSample (0, i, v); buf.setSample (1, i, v); }
        if (b == 3) { buf.setSample (0, 100, std::numeric_limits<float>::quiet_NaN()); buf.setSample (1, 200, std::numeric_limits<float>::infinity()); }
        p.processBlock (buf, midi);
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < 512; ++i) finite = finite && std::isfinite (buf.getSample (c, i));
    }
    CHECK (finite);
}

// The release of the host's bypass: the audio after it is the audio of now. Noise is processed, then silence is
// bypassed for two seconds, then silence is processed: nothing of the noise comes out (under -120 dB: what is left
// is the filters' decay), because the chain went on running on the input while the bypass was on. The Noise is off
// and the three sends are muted, so the output is the dry path alone.
static void testBypassReleaseHasNoOldAudio()
{
    FwVocalChainProcessor p;
    auto set = [&p] (const char* id, float v) { auto* q = p.apvts.getParameter (id); q->setValueNotifyingHost (q->convertTo0to1 (v)); };
    set (ch::id::reverbColor, 0.0f); set (ch::id::delayColor, 0.0f); set (ch::id::pitchColor, 0.0f);
    p.setNoise (false);
    p.prepareToPlay (48000.0, 512);
    const int lat = p.getLatencySamples();
    CHECK (lat > 0);
    const int block = 512, quiet = 2 * 48000 / block;
    juce::AudioBuffer<float> buf (2, block);
    juce::MidiBuffer midi;
    unsigned int seed = 777u;
    float loud = 0.0f;
    for (int b = 0; b < 20; ++b)                               // noise, processed
    {
        for (int i = 0; i < block; ++i)
        {
            seed = seed * 1664525u + 1013904223u;
            const float v = ((float) (seed >> 8) / 8388608.0f - 1.0f) * 0.25f;
            buf.setSample (0, i, v); buf.setSample (1, i, -v);
        }
        p.processBlock (buf, midi);
        loud = buf.getMagnitude (0, block);
    }
    CHECK (loud > 0.01f);
    for (int b = 0; b < quiet; ++b) { buf.clear(); p.processBlockBypassed (buf, midi); }   // silence, bypassed
    buf.clear();
    p.processBlock (buf, midi);                                // silence, processed: the delayed silence
    CHECK (buf.getMagnitude (0, block) < 1.0e-6f);
}

// A block larger than the size given to prepareToPlay (a host may send one): it allocates nothing on the calling
// thread and gives the audio of the same input in blocks of the prepared size, to the bit.
static void testOversizedBlock()
{
    auto setup = [] (FwVocalChainProcessor& q) { set (q, ch::id::sensitivity, 3.0f); set (q, ch::id::reverb, 4.0f); set (q, ch::id::delay, 2.0f); };
    FwVocalChainProcessor big, small;
    setup (big); setup (small);
    big.prepareToPlay (48000.0, 256);
    small.prepareToPlay (48000.0, 256);
    juce::AudioBuffer<float> a (2, 1024), b (2, 256);
    juce::MidiBuffer midi;
    unsigned int seed = 4242u;
    int allocations = 0, differing = 0;
    for (int blk = 0; blk < 12; ++blk)
    {
        for (int i = 0; i < 1024; ++i)
        {
            seed = seed * 1664525u + 1013904223u;
            const float v = ((float) (seed >> 8) / 8388608.0f - 1.0f) * 0.25f;
            a.setSample (0, i, v); a.setSample (1, i, 0.5f * v * v - v);
        }
        juce::AudioBuffer<float> x;
        x.makeCopyOf (a);
        {
            flatwater::test::AllocScope scope;
            if (blk % 4 == 3) big.processBlockBypassed (a, midi); else big.processBlock (a, midi);
            allocations += scope.count();
        }
        for (int part = 0; part < 4; ++part)
        {
            for (int c = 0; c < 2; ++c) b.copyFrom (c, 0, x, c, part * 256, 256);
            if (blk % 4 == 3) small.processBlockBypassed (b, midi); else small.processBlock (b, midi);
            for (int c = 0; c < 2; ++c)
                for (int i = 0; i < 256; ++i)
                    if (! juce::exactlyEqual (b.getSample (c, i), a.getSample (c, part * 256 + i))) ++differing;
        }
    }
    CHECK (allocations == 0);
    CHECK (differing == 0);
}

// The reverb's responses load on a background thread. prepareToPlay waits for them: after it they are there and no
// time-out is recorded. With no wait allowed, the time-out is recorded, and until the responses are there the reverb
// is silent: the output is that of the chain with Reverb muted, to the bit, not the send handed on unconvolved.
// Then the reverb comes in, and the record of the late start stays.
static void testReverbLoadTimeOut()
{
    auto setup = [] (FwVocalChainProcessor& q, float reverbColour)
    {
        for (const auto& st : ch::stages()) set (q, st.colour, 0.0f);
        set (q, ch::id::reverbColor, reverbColour);
        set (q, ch::id::reverb, 10.0f);
        q.setNoise (false);
    };
    FwVocalChainProcessor waited;
    setup (waited, 1.0f);
    waited.prepareToPlay (48000.0, 256);
    CHECK (waited.reverbLoaded() && ! waited.reverbLoadTimedOut());

    FwVocalChainProcessor late, muted;
    setup (late, 1.0f); setup (muted, 0.0f);
    late.setReverbLoadTimeout (0);
    late.prepareToPlay (48000.0, 256);
    muted.prepareToPlay (48000.0, 256);
    CHECK (late.reverbLoadTimedOut() && ! late.reverbLoaded());
    juce::AudioBuffer<float> a (2, 256), b (2, 256);
    juce::MidiBuffer midi;
    unsigned int seed = 99u;
    int differingWhileLoading = 0, blocksWhileLoading = 0, blocksAfter = 0;
    bool finite = true;
    double wetAfter = 0.0;
    for (int blk = 0; blk < 20000 && blocksAfter < 40; ++blk)
    {
        for (int i = 0; i < 256; ++i)
        {
            seed = seed * 1664525u + 1013904223u;
            const float v = ((float) (seed >> 8) / 8388608.0f - 1.0f) * 0.25f;
            a.setSample (0, i, v); a.setSample (1, i, -v);
        }
        b.makeCopyOf (a);
        const bool loading = ! late.reverbLoaded();
        late.processBlock (a, midi);
        muted.processBlock (b, midi);
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < 256; ++i)
            {
                finite = finite && std::isfinite (a.getSample (c, i));
                if (loading && ! juce::exactlyEqual (a.getSample (c, i), b.getSample (c, i))) ++differingWhileLoading;
                if (! loading) wetAfter = std::max (wetAfter, (double) std::fabs (a.getSample (c, i) - b.getSample (c, i)));
            }
        if (loading) { ++blocksWhileLoading; juce::Thread::sleep (1); } else ++blocksAfter;
    }
    CHECK (blocksWhileLoading > 0);
    CHECK (differingWhileLoading == 0);
    CHECK (finite);
    CHECK (late.reverbLoaded());
    CHECK (late.reverbLoadTimedOut());
    CHECK (wetAfter > 1.0e-3);
}

// presets, programs and the saved state (shared/test/PresetChecks.h)
static void testPresetState()
{
    const auto check = [] (bool ok, const char* what) { ++checks; if (! ok) { ++failures; std::printf ("FAIL preset state: %s\n", what); } };
    flatwater::test::programChecks<FwVocalChainProcessor> (ch::id::mode, check);
    flatwater::test::presetChecks<FwVocalChainProcessor> (ch::id::mode, check);
    flatwater::test::windowChecks<FwVocalChainProcessor> (check);
}

int main()
{
    testReverbLoadTimeOut();
    testBypassIsDelayedInput();
    testBypassReleaseHasNoOldAudio();
    juce::ScopedJuceInitialiser_GUI init;
    testNonFiniteInput();
    testParamIds();
    testParameters();
    testBusesAndLatency();
    testPath();
    testDeEsser();
    testOversizedBlock();
    testEditor();
    testPresetState();
    std::printf ("%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
