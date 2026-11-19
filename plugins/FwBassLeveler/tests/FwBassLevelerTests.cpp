// Unit tests for Flatwater Bass Leveler: the parameters as the reference's, the components' latency, the audio path, the ride
// against the reference's measured behaviour (law, look-ahead, hold, glide, stereo mid, Idle), the state, the editor,
// the factory presets.
// Run: FwBassLevelerTests (exit code 0 = all passed).
#include <functional>
#include <algorithm>
#include <cmath>
#include <vector>
#include <cstdio>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "shared/test/AllocCounter.h"

static int failures = 0, checks = 0;
#define CHECK(cond) do { ++checks; if (! (cond)) { ++failures; std::printf ("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } } while (0)
#define CHECK_NEAR(a, b, tol) do { ++checks; const double va = (a), vb = (b); if (! (std::fabs (va - vb) <= (tol))) { ++failures; std::printf ("FAIL %s:%d  %s = %g, expected %g +- %g\n", __FILE__, __LINE__, #a, va, vb, (double) (tol)); } } while (0)

// the defaults, ranges and texts of the reference's VST3 parameters (RESEARCH.md, Flatwater Bass Leveler)
static void testParameters()
{
    FwBassLevelerProcessor p;
    auto v = [&p] (const char* id) { return p.apvts.getRawParameterValue (id)->load(); };
    auto range = [&p] (const char* id) { return p.apvts.getParameter (id)->getNormalisableRange(); };
    CHECK_NEAR (v (bl::id::target), -14.0, 1e-6);
    CHECK_NEAR (v (bl::id::sensitivity), 50.0, 1e-6);
    CHECK_NEAR (v (bl::id::response), 0.0, 1e-6);
    CHECK_NEAR (v (bl::id::spill), -40.0, 1e-6);
    CHECK_NEAR (v (bl::id::artifacts), 50.0, 1e-6);
    CHECK_NEAR (v (bl::id::rangeMax), 12.0, 1e-6);
    CHECK_NEAR (v (bl::id::rangeMin), -12.0, 1e-6);
    CHECK_NEAR (v (bl::id::idle), 0.5, 1e-6);
    CHECK_NEAR (v (bl::id::output), 0.0, 1e-6);
    CHECK_NEAR (range (bl::id::target).start, -42.0, 1e-6);
    CHECK_NEAR (range (bl::id::target).end, 0.0, 1e-6);
    CHECK_NEAR (range (bl::id::spill).start, -50.0, 1e-6);
    CHECK_NEAR (range (bl::id::spill).end, -3.0, 1e-6);
    CHECK_NEAR (range (bl::id::rangeMax).start, -11.0, 1e-6);
    CHECK_NEAR (range (bl::id::rangeMin).end, 11.0, 1e-6);
    CHECK_NEAR (range (bl::id::idle).start, 0.02, 1e-6);
    CHECK_NEAR (range (bl::id::idle).end, 0.98, 1e-6);
    // the Idle arrow in dB: its place in the range (a preset with Idle 0.98 and Max 0 sits at -0.24 dB)
    CHECK_NEAR (bl::idleDb (0.5f, -12.0f, 12.0f), 0.0, 1e-6);
    CHECK_NEAR (bl::idleDb (0.98f, -12.0f, 0.0f), -0.24, 1e-5);
    CHECK (p.apvts.getParameter (bl::id::idle)->getText (0.5f, 16) == "0.0");
    CHECK (p.apvts.getParameter (bl::id::response)->getText (1.0f, 16) == "Fast");
    CHECK (bl::modeNames().size() == 4);
}

// the components' latency (measured: 42.00 ms, Live 5.00 ms at any rate) and the audio path: the input delayed by it,
// at the idle gain and Output
static void testLatencyAndPath()
{
    for (double fs : { 44100.0, 48000.0, 96000.0 })
        for (int mode : { 0, 2 })
        {
            FwBassLevelerProcessor p;
            auto* m = p.apvts.getParameter (bl::id::mode);
            m->setValueNotifyingHost (m->convertTo0to1 ((float) mode));
            auto* o = p.apvts.getParameter (bl::id::output);
            o->setValueNotifyingHost (o->convertTo0to1 (-6.0f));
            p.setPlayConfigDetails (2, 2, fs, 256);
            p.prepareToPlay (fs, 256);
            const int expect = juce::roundToInt ((mode == 2 ? 5.0 : 42.0) * 0.001 * fs);
            CHECK (p.getLatencySamples() == expect);
            juce::AudioBuffer<float> b (2, 256);
            juce::MidiBuffer midi;
            int found = -1, pos = 0;
            float peak = 0.0f;
            for (int blk = 0; blk < 40 && found < 0; ++blk, pos += 256)
            {
                b.clear();
                // far under the detector's floor (-48.75 dBFS, measured): no note, the Rider at its idle value (0 dB)
                if (blk == 1) { b.setSample (0, 10, 0.001f); b.setSample (1, 10, 0.001f); }
                p.processBlock (b, midi);
                for (int i = 0; i < 256; ++i)
                    if (std::abs (b.getSample (0, i)) > 0.0001f) { found = pos + i; peak = b.getSample (0, i); break; }
            }
            CHECK (found == 256 + 10 + expect);
            CHECK_NEAR (peak, 0.001 * juce::Decibels::decibelsToGain (-6.0), 2e-6);
        }
}

// a processor at 48 kHz fed with blocks of 480 samples; `make` fills each block (left, right), the output is kept
static std::vector<float> runRide (FwBassLevelerProcessor& p, int blocks, std::function<void (int, float&, float&)> make, int outCh = 0)
{
    const double fs = 48000.0;
    p.setPlayConfigDetails (2, 2, fs, 480);
    p.prepareToPlay (fs, 480);
    juce::AudioBuffer<float> b (2, 480);
    juce::MidiBuffer midi;
    std::vector<float> out;
    for (int blk = 0; blk < blocks; ++blk)
    {
        for (int i = 0; i < 480; ++i) { float l = 0.0f, r = 0.0f; make (blk * 480 + i, l, r); b.setSample (0, i, l); b.setSample (1, i, r); }
        p.processBlock (b, midi);
        for (int i = 0; i < 480; ++i) out.push_back (b.getSample (outCh, i));
    }
    return out;
}

static double sineAt (int n, double f, double peakDb) { return std::pow (10.0, peakDb / 20.0) * std::sin (2.0 * juce::MathConstants<double>::pi * f * n / 48000.0); }

// the ride against the reference's measured law (MEASUREMENTS.md, Flatwater Bass Leveler Phase 3): gain = Target - E + 0.68 dB with
// E the weighted level (an 80 Hz sine reads 4.81 dB under its peak): an 80 Hz tone at -20 dBFS rms rides to +8.48 dB
// with the Energy meter at -21.8; when it stops the gain glides to the Idle value at 32 dB/s (back within 0.4 s)
static void testRideLaw()
{
    FwBassLevelerProcessor p;
    float energy = -100.0f;
    runRide (p, 100, [] (int n, float& l, float& r) { l = r = (float) sineAt (n, 80.0, -16.99); });
    energy = p.energyDb.take();
    CHECK_NEAR (p.rideDb.load(), 8.48, 0.1);
    CHECK_NEAR (energy, -21.8, 0.2);
    // (continue on the same processor: 0.4 s of silence)
    juce::AudioBuffer<float> b (2, 480); juce::MidiBuffer midi;
    for (int blk = 0; blk < 25; ++blk) { b.clear(); p.processBlock (b, midi); }
    const float after250ms = p.rideDb.load();
    for (int blk = 0; blk < 15; ++blk) { b.clear(); p.processBlock (b, midi); }
    CHECK (after250ms < 8.0f && after250ms > 0.5f);                        // gliding (32 dB/s)
    CHECK_NEAR (p.rideDb.load(), 0.0, 0.05);
}

// the look-ahead: a legato step -30 -> -16 dBFS (1 kHz) at 1 s; the gain starts down 42 ms before the louder audio
// leaves the plug-in (measured on the reference), so the output never shows the step at the old gain
static void testRideLookAhead()
{
    FwBassLevelerProcessor p;
    const int step = 48000;
    auto y = runRide (p, 200, [step] (int n, float& l, float& r) { l = r = (float) sineAt (n, 1000.0, n < step ? -30.0 : -16.0); });
    const int lat = p.getLatencySamples();
    CHECK (lat == 2016);
    // the output's envelope just after the step reaches the output: about -16 + the new gain (+2.69), not -16 + 12
    float pk = 0.0f;
    for (int i = step + lat + 100; i < step + lat + 600; ++i) pk = std::max (pk, std::abs (y[(size_t) i]));
    CHECK_NEAR (juce::Decibels::gainToDecibels (pk), -16.0 + 2.69, 0.3);
}

// within a note the gain only goes down: a legato step down (-16 -> -30) keeps the gain of the louder part
static void testRideHold()
{
    FwBassLevelerProcessor p;
    runRide (p, 300, [] (int n, float& l, float& r) { l = r = (float) sineAt (n, 1000.0, n < 48000 ? -16.0 : -30.0); });
    CHECK_NEAR (p.rideDb.load(), 2.69, 0.1);
}

// stereo: one gain for both channels, from the mid: L at -6, R at -30 dBFS (in phase, 1 kHz) ride as their mid (-11.5)
static void testRideStereoMid()
{
    FwBassLevelerProcessor p;
    runRide (p, 100, [] (int n, float& l, float& r) { l = (float) sineAt (n, 1000.0, -6.0); r = (float) sineAt (n, 1000.0, -30.0); });
    CHECK_NEAR (p.rideDb.load(), -1.82, 0.15);
}

static void testStateAndEditor()
{
    FwBassLevelerProcessor p;
    auto* t = p.apvts.getParameter (bl::id::target);
    t->setValueNotifyingHost (t->convertTo0to1 (-20.0f));
    juce::MemoryBlock mb;
    p.getStateInformation (mb);
    FwBassLevelerProcessor q;
    q.setStateInformation (mb.getData(), (int) mb.getSize());
    CHECK_NEAR (q.apvts.getRawParameterValue (bl::id::target)->load(), -20.0, 1e-4);
    std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
    CHECK (ed != nullptr);
    CHECK (ed->getWidth() > 0);
    CHECK (bl::ui::scaleY (12.0f) == 222.0f);
    CHECK (bl::ui::scaleY (-12.0f) == 432.0f);
}

// the selection rules measured on the reference (click, lasso group, Rider / Idle in blue, member drags blocked)
static void testSelection()
{
    using namespace bl::ui;
    juce::Component a, b, rider;
    Marks m;
    m.blueAlone = { &rider };
    CHECK (m.click (&a));
    CHECK (m.of (&a) == Mark::Selected);
    CHECK (m.of (&b) == Mark::None);
    CHECK (m.click (&rider));
    CHECK (m.of (&rider) == Mark::Member);                          // the Rider alone: blue
    m.group = { &rider, &a, &b };
    m.selected = &rider;
    CHECK (m.of (&rider) == Mark::Selected);                        // in a group the first is yellow
    CHECK (m.of (&b) == Mark::Member);
    CHECK (! m.click (&b));                                          // a member: its drag moves nothing
    CHECK (m.of (&b) == Mark::Selected && m.of (&rider) == Mark::Member);
    m.hover = &b;
    CHECK (m.showsValue (&a) && m.showsValue (&b));                 // the primary under the mouse: every member's value
    juce::Component other;
    CHECK (m.click (&other));                                        // outside the group: the group goes
    CHECK (m.group.empty() && m.of (&a) == Mark::None);
    // the host sees Idle over a fixed +-12 dB, whatever the range (as the reference's text)
    FwBassLevelerProcessor p;
    auto* mx = p.apvts.getParameter (bl::id::rangeMax);
    mx->setValueNotifyingHost (mx->convertTo0to1 (6.0f));
    CHECK (p.apvts.getParameter (bl::id::idle)->getText (0.5f, 16) == "0.0");
    CHECK_NEAR (p.idleDb(), -3.0, 1e-4);                             // the arrow itself: the middle of -12 .. 6
}

// the Idle value as the reference applies it (MEASUREMENTS.md, Flatwater Bass Leveler: Idle): the gain starts at 0 dB and, in
// silence before any note, glides to the Idle value at 32 dB/s once the look-ahead has filled; a quiet sound before any
// note (the level read over -77.7 dB) holds it; after a note it glides to a new Idle value at once, as the arrow moves
static void testIdle()
{
    {
        FwBassLevelerProcessor p;
        auto* idle = p.apvts.getParameter (bl::id::idle);
        idle->setValueNotifyingHost (idle->convertTo0to1 (0.88f));      // +9.12 dB at +-12 (0.01 steps)
        runRide (p, 10, [] (int, float& l, float& r) { l = r = 0.0f; });          // 0.1 s
        CHECK (p.rideDb.load() > 1.0f && p.rideDb.load() < 3.5f);       // measured +2.94 at 0.1 s
        runRide (p, 40, [] (int, float& l, float& r) { l = r = 0.0f; });
        CHECK_NEAR (p.rideDb.load(), 9.12, 0.05);
    }
    {
        FwBassLevelerProcessor p;
        auto* idle = p.apvts.getParameter (bl::id::idle);
        idle->setValueNotifyingHost (idle->convertTo0to1 (0.75f));      // +6 dB
        runRide (p, 100, [] (int n, float& l, float& r) { l = r = (float) sineAt (n, 80.0, -72.0); });
        CHECK_NEAR (p.rideDb.load(), 0.0, 0.05);                         // held at 0 (measured)
    }
    {
        FwBassLevelerProcessor p;
        runRide (p, 100, [] (int n, float& l, float& r) { l = r = (float) sineAt (n, 80.0, -20.0); });
        juce::AudioBuffer<float> b (2, 480); juce::MidiBuffer midi;
        for (int blk = 0; blk < 60; ++blk) { b.clear(); p.processBlock (b, midi); }
        CHECK_NEAR (p.rideDb.load(), 0.0, 0.05);
        auto* idle = p.apvts.getParameter (bl::id::idle);
        idle->setValueNotifyingHost (idle->convertTo0to1 (0.25f));      // -6 dB: glides there
        for (int blk = 0; blk < 10; ++blk) { b.clear(); p.processBlock (b, midi); }
        CHECK (p.rideDb.load() < -1.5f && p.rideDb.load() > -4.5f);
        for (int blk = 0; blk < 20; ++blk) { b.clear(); p.processBlock (b, midi); }
        CHECK_NEAR (p.rideDb.load(), -6.0, 0.05);
    }
}

// the factory presets: the reference's 17, with our names; they set their values, and all but the first two leave
// Target where it is
static void testPresets()
{
    FwBassLevelerProcessor p;
    CHECK (p.getNumPrograms() == 18);
    CHECK (p.getProgramName (1) == "Essentials: Default");
    auto* target = p.apvts.getParameter (bl::id::target);
    target->setValueNotifyingHost (target->convertTo0to1 (-24.0f));
    p.setCurrentProgram (16);                                         // "Bass Tamer": Max 0, Idle 0.98, Fast, Sensitivity 75
    CHECK (p.getProgramName (16) == "Essentials: Bass Tamer");
    CHECK_NEAR (p.apvts.getRawParameterValue (bl::id::target)->load(), -24.0, 0.05);
    CHECK_NEAR (p.apvts.getRawParameterValue (bl::id::rangeMax)->load(), 0.0, 0.05);
    CHECK_NEAR (p.apvts.getRawParameterValue (bl::id::sensitivity)->load(), 75.0, 0.05);
    CHECK (p.apvts.getRawParameterValue (bl::id::response)->load() > 0.5f);
    CHECK_NEAR (p.idleDb(), -0.24, 0.01);
    p.setCurrentProgram (2);                                          // "Live PA Start" sets Target, Artifacts 68.2, Spill -30
    CHECK_NEAR (p.apvts.getRawParameterValue (bl::id::target)->load(), -14.0, 0.05);
    CHECK_NEAR (p.apvts.getRawParameterValue (bl::id::artifacts)->load(), 68.2, 0.05);
    CHECK_NEAR (p.apvts.getRawParameterValue (bl::id::spill)->load(), -30.0, 0.05);
}

// allIds lists every parameter in the layout's order; presetIds every one of them but Mode
static void testParamIds()
{
    FwBassLevelerProcessor p;
    const auto& prms = p.getParameters();
    CHECK (prms.size() == bl::numParams);
    int same = 0;
    for (int i = 0; i < juce::jmin (prms.size(), bl::numParams); ++i)
        if (auto* hp = dynamic_cast<juce::HostedAudioProcessorParameter*> (prms[i]); hp != nullptr && hp->getParameterID() == bl::allIds[i]) ++same;
    CHECK (same == bl::numParams);
    int preset = 0;
    for (auto* pid : bl::presetIds)
        for (auto* all : bl::allIds)
            if (juce::String (pid) == all && juce::String (pid) != bl::id::mode) ++preset;
    CHECK (preset == bl::numParams - 1);
}

// The host's bypass, engaged and released in mid-stream: every bypassed sample is the input of `latency` samples
// before, from the first one on, because the delay line takes the input while the bypass is off too.
static void testBypassIsDelayedInput()
{
    FwBassLevelerProcessor p;
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

// The release of the host's bypass: the main delay line took the input while the bypass was on, so the first samples
// after it are the input of the last `latency` samples, not what the line held when the bypass came on.
static void testBypassReleaseHasNoOldAudio()
{
    FwBassLevelerProcessor p;
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
            buf.setSample (0, i, v); buf.setSample (1, i, v);
        }
        p.processBlock (buf, midi);
    }
    for (int b = 0; b < quiet; ++b) { buf.clear(); p.processBlockBypassed (buf, midi); }   // silence, bypassed for longer than the latency
    buf.clear();
    p.processBlock (buf, midi);                            // silence, processed: the delayed silence
    CHECK (buf.getMagnitude (0, block) == 0.0f);
}

// Range Max under Range Min (a host can automate them so; the editor does not allow it): the gain stays finite and
// inside the wider of the two bounds
static void testInvertedRange()
{
    FwBassLevelerProcessor p;
    auto set = [&p] (const char* id, float v) { auto* q = p.apvts.getParameter (id); q->setValueNotifyingHost (q->convertTo0to1 (v)); };
    set (bl::id::rangeMin, 6.0f); set (bl::id::rangeMax, -6.0f);
    p.prepareToPlay (48000.0, 512);
    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    bool finite = true;
    float peak = 0.0f;
    for (int b = 0; b < 40; ++b)
    {
        for (int i = 0; i < 512; ++i) { const float v = 0.1f * std::sin (0.01f * (float) (b * 512 + i)); buf.setSample (0, i, v); buf.setSample (1, i, v); }
        p.processBlock (buf, midi);
        for (int i = 0; i < 512; ++i) finite = finite && std::isfinite (buf.getSample (0, i));
        peak = std::max (peak, buf.getMagnitude (0, 512));
    }
    CHECK (finite);
    CHECK (peak <= 0.1f * 4.1f);                           // +12 dB at most
}

// Mode changed between two blocks (a host automates it): the block that takes the change allocates nothing on the
// calling thread, and the audio from the change on is that of a processor prepared in the new Mode (the change is a
// full reset). Between the Studio and the Live components the latency changes (42 ms, 5 ms): the audio thread does
// not tell the host, the processor's timer does (updateHostLatency), and the bypass then delays by the new latency.
static void testModeChangeOnTheAudioThread()
{
    auto set = [] (FwBassLevelerProcessor& q, const char* id, float v) { auto* a = q.apvts.getParameter (id); a->setValueNotifyingHost (a->convertTo0to1 (v)); };
    FwBassLevelerProcessor p;
    set (p, bl::id::output, -3.0f);
    p.prepareToPlay (48000.0, 512);
    CHECK (p.getLatencySamples() == 2016);
    juce::AudioBuffer<float> buf (2, 512), in (2, 512);
    juce::MidiBuffer midi;
    int at = 0;
    auto fill = [&at] (juce::AudioBuffer<float>& b)
    {
        for (int i = 0; i < b.getNumSamples(); ++i, ++at)
        {
            const float v = (float) sineAt (at, 80.0, -17.0);
            b.setSample (0, i, v); b.setSample (1, i, 0.5f * v);
        }
    };
    auto same = [] (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
    {
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < a.getNumSamples(); ++i)
                if (! juce::exactlyEqual (a.getSample (c, i), b.getSample (c, i))) return false;
        return true;
    };
    for (int b = 0; b < 40; ++b) { fill (buf); p.processBlock (buf, midi); }
    for (int mode : { 1, 2, 0, 3 })
    {
        set (p, bl::id::mode, (float) mode);
        FwBassLevelerProcessor fresh;
        set (fresh, bl::id::output, -3.0f); set (fresh, bl::id::mode, (float) mode);
        fresh.prepareToPlay (48000.0, 512);
        for (int b = 0; b < 40; ++b)
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
        const int latency = mode >= 2 ? 240 : 2016;
        p.updateHostLatency();
        CHECK (p.getLatencySamples() == latency);
        // the bypass after the change: the input of `latency` samples before, the processed blocks' input included
        std::vector<float> x;
        for (int b = 0; b < 6; ++b)
        {
            fill (buf);
            x.insert (x.end(), buf.getReadPointer (0), buf.getReadPointer (0) + 512);
            if (b < 5) { p.processBlock (buf, midi); fresh.processBlock (buf, midi); continue; }
            int allocations = 0;
            {
                flatwater::test::AllocScope scope;
                p.processBlockBypassed (buf, midi);
                allocations = scope.count();
            }
            CHECK (allocations == 0);
            float err = 0.0f;
            for (int i = 0; i < 512; ++i) err = std::max (err, std::abs (buf.getSample (0, i) - x[(size_t) (5 * 512 + i - latency)]));
            CHECK (err == 0.0f);
            fresh.processBlockBypassed (buf, midi);
        }
    }
}

int main()
{
    testBypassIsDelayedInput();
    testBypassReleaseHasNoOldAudio();
    testInvertedRange();
    juce::ScopedJuceInitialiser_GUI init;
    testParamIds();
    testParameters();
    testLatencyAndPath();
    testRideLaw();
    testRideLookAhead();
    testRideHold();
    testRideStereoMid();
    testStateAndEditor();
    testSelection();
    testIdle();
    testModeChangeOnTheAudioThread();
    testPresets();
    std::printf ("%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
