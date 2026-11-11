// Unit tests for Flatwater Bass Leveler: the parameters as the reference's, the components' latency, the audio path, the ride
// against the reference's measured behaviour (law, look-ahead, hold, glide, stereo mid, Idle), the state, the editor,
// the factory presets.
// Run: FwBassLevelerTests (exit code 0 = all passed).
#include <functional>
#include <cmath>
#include <cstdio>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

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

int main()
{
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
    testPresets();
    std::printf ("%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
