// Unit tests for Flatwater Pitch Bend (Phase 0): the parameters as the reference's host parameters (names, ranges,
// defaults, texts), the buses (mono or stereo, no latency), the Mix law on the pass-through path, the state, the editor.
// Run: FwPitchBendTests (exit code 0 = all passed).
#include <cmath>
#include <cstdio>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

static int failures = 0, checks = 0;
#define CHECK(cond) do { ++checks; if (! (cond)) { ++failures; std::printf ("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } } while (0)
#define CHECK_NEAR(a, b, tol) do { ++checks; const double va = (a), vb = (b); if (! (std::fabs (va - vb) <= (tol))) { ++failures; std::printf ("FAIL %s:%d  %s = %g, expected %g +- %g\n", __FILE__, __LINE__, #a, va, vb, (double) (tol)); } } while (0)

static void set (FwPitchBendProcessor& p, const char* id, float v)
{
    auto* q = p.apvts.getParameter (id);
    q->setValueNotifyingHost (q->convertTo0to1 (v));
}

// the reference's defaults, normalised values and texts (ParamDump; RESEARCH.md, Flatwater Pitch Bend)
static void testParameters()
{
    FwPitchBendProcessor p;
    auto norm = [&p] (const char* id) { return (double) p.apvts.getParameter (id)->getValue(); };
    auto text = [&p] (const char* id, float v) { return p.apvts.getParameter (id)->getText (v, 32); };
    for (int m = 0; m < 2; ++m)
    {
        const auto& d = pb::mod[m];
        CHECK_NEAR (norm (d.warp), 0.5, 1e-6);
        CHECK_NEAR (norm (d.level), 1.0, 1e-6);
        CHECK_NEAR (norm (d.rateSynced), 5.0 / 23.0, 1e-6);
        CHECK_NEAR (norm (d.rate), 0.341174, 1e-5);
        CHECK_NEAR (norm (d.smooth), 0.058697, 1e-5);
        CHECK_NEAR (norm (d.steps), 1.0, 1e-6);
        CHECK (text (d.rateSynced, 5.0f / 23.0f) == "1B");
        CHECK (text (d.rateSynced, 1.0f) == "1/64T");
        CHECK (text (d.rate, 0.5f) == "1.342 Hz");
        CHECK (text (d.smooth, 0.5f) == "31.6");
        CHECK (text (d.warp, 0.25f) == "0.10");
    }
    CHECK_NEAR (norm (pb::id::amAttack), 0.5, 1e-6);
    CHECK_NEAR (norm (pb::id::amRelease), 0.75, 1e-6);
    CHECK_NEAR (norm (pb::id::amLevel), 0.5, 1e-6);
    CHECK_NEAR (norm (pb::id::ptSmooth), 1.0 / 3.0, 1e-6);
    CHECK_NEAR (norm (pb::id::ptOffset), 0.5, 1e-6);
    CHECK_NEAR (norm (pb::id::pitch), 0.5, 1e-6);
    CHECK_NEAR (norm (pb::id::formant), 0.5, 1e-6);
    CHECK_NEAR (norm (pb::id::mix), 1.0, 1e-6);
    CHECK_NEAR (norm (pb::id::flattenNote), 12.0 / 23.0, 1e-6);
    CHECK_NEAR (norm (pb::id::flatten), 0.0, 1e-6);
    CHECK (text (pb::id::amAttack, 0.25f) == "1.0 ms");
    CHECK (text (pb::id::flattenNote, 0.0f) == "C2");
    CHECK (text (pb::id::flattenNote, 1.0f) == "B3");
    CHECK (text (pb::id::pitch, 0.0f) == "-12 semitones");
    CHECK (p.getParameters().size() == 30);          // the reference's 29 (without its Bypass) and Mode
}

// a main input and output, mono or stereo, the same on both sides; no latency
static void testBuses()
{
    FwPitchBendProcessor p;
    CHECK (p.getLatencySamples() == 0);
    auto layout = p.getBusesLayout();
    CHECK (p.checkBusesLayoutSupported (layout));
    layout.inputBuses.getReference (0) = juce::AudioChannelSet::mono();
    CHECK (! p.checkBusesLayoutSupported (layout));
    layout.outputBuses.getReference (0) = juce::AudioChannelSet::mono();
    CHECK (p.checkBusesLayoutSupported (layout));
}

// the pass-through path is the input at every Mix (Phase 0: the wet path is the dry one)
// at no shift: Mix 0 is the input exactly; Mix 100 is the input delayed by the wet path's 105.53 samples (48 kHz, as
// the reference's 2.2 ms), Mix 50 their average
static void testPassThrough()
{
    for (float mix : { 0.0f, 50.0f, 100.0f })
    {
        FwPitchBendProcessor p;
        set (p, pb::id::mix, mix);
        p.setPlayConfigDetails (2, 2, 48000.0, 256);
        p.prepareToPlay (48000.0, 256);
        juce::AudioBuffer<float> b (2, 256);
        juce::MidiBuffer midi;
        auto x = [] (double t, int c) { return std::sin (0.01 * t + c); };
        double err = 0.0;
        for (int blk = 0; blk < 8; ++blk)
        {
            for (int c = 0; c < 2; ++c)
                for (int i = 0; i < 256; ++i) b.setSample (c, i, (float) x (blk * 256 + i, c));
            p.processBlock (b, midi);
            if (blk < 2) continue;                                   // (the delay line fills)
            for (int c = 0; c < 2; ++c)
                for (int i = 0; i < 256; ++i)
                {
                    const double t = blk * 256 + i, want = x (t, c) + mix / 100.0 * (x (t - 105.53125, c) - x (t, c));
                    err = std::max (err, std::fabs (b.getSample (c, i) - want));
                }
        }
        CHECK (err < (mix == 0.0f ? 1e-7 : 1e-4));
    }
}

static void testState()
{
    FwPitchBendProcessor a, b;
    set (a, pb::id::pitch, -7.0f);
    set (a, pb::id::mix, 35.0f);
    set (a, pb::mod[1].rate, 4.0f);
    juce::MemoryBlock m;
    a.getStateInformation (m);
    b.setStateInformation (m.getData(), (int) m.getSize());
    CHECK_NEAR (b.get (pb::id::pitch), -7.0, 1e-5);
    CHECK_NEAR (b.get (pb::id::mix), 35.0, 1e-4);
    CHECK_NEAR (b.get (pb::mod[1].rate), 4.0, 1e-4);
}

// the slots: a source takes the first free one at depth 0; a fifth is refused; depths are whole numbers within ±100;
// they, Fine and the shape tables survive the state and undo
static void testModel()
{
    using namespace pb;
    using namespace pb::dsp;
    FwPitchBendProcessor p;
    auto& st = p.apvts.state;
    for (int s = 0; s < numSources - 1; ++s) CHECK (assign (st, 0, M1 + s, &p.undo));
    CHECK (! assign (st, 0, M1, &p.undo));
    CHECK (assign (st, 2, AM, &p.undo));
    CHECK (! assign (st, 2, AM, &p.undo));                       // the same source twice on one control: refused
    CHECK (holds (st, 2, AM) && ! holds (st, 2, AM, 0));
    CHECK (getSlot (st, 0, 2).source == AM);
    setSlot (st, 0, 1, { M2, 123.4f }, &p.undo);
    CHECK_NEAR (getSlot (st, 0, 1).depth, 100.0, 1e-6);
    setSlot (st, 0, 1, { M2, -30.4f }, &p.undo);
    CHECK_NEAR (getSlot (st, 0, 1).depth, -30.0, 1e-6);
    CHECK (targetIndex ("ptLevel") == numTargets - 1);
    auto t = getTable (st, 0);
    CHECK_NEAR (t[64], 1.0, 1e-6);                              // M1's default: a sine (a quarter cycle in: 1)
    CHECK_NEAR (getTable (st, 1)[64], 1.0, 1e-6);                // M2's: a triangle (also 1 there)
    CHECK_NEAR (getTable (st, 1)[32], 0.5, 1e-6);
    setSteps (t, patterns[2].steps);
    setTable (st, 0, t, &p.undo);
    st.setProperty (fineId, true, &p.undo);
    juce::MemoryBlock m;
    p.getStateInformation (m);
    FwPitchBendProcessor q;
    q.setStateInformation (m.getData(), (int) m.getSize());
    CHECK (getSlot (q.apvts.state, 0, 3).source == PT);
    CHECK_NEAR (getSlot (q.apvts.state, 0, 1).depth, -30.0, 1e-6);
    CHECK (stepValue (getTable (q.apvts.state, 0), 2) == 12);
    CHECK (q.fine());
    p.undo.undo();                                              // the last transaction: Fine back off
    CHECK (! p.fine());
    // whole semitones while Fine is off, cents while it is on
    set (q, pb::id::pitch, 3.456f);
    CHECK_NEAR (q.shiftSemitones (pb::id::pitch), 3.46, 1e-5);
    q.apvts.state.setProperty (fineId, false, nullptr);
    CHECK_NEAR (q.shiftSemitones (pb::id::pitch), 3.0, 1e-6);
    // our own shapes span -1 .. 1 and are not all the same
    for (int s = 0; s < shapeNames().size(); ++s)
    {
        float lo = 1.0f, hi = -1.0f;
        for (int i = 0; i < 256; ++i) { lo = juce::jmin (lo, shapeValue ((Shape) s, i / 256.0)); hi = juce::jmax (hi, shapeValue ((Shape) s, i / 256.0)); }
        CHECK (lo >= -1.0f && hi <= 1.0f);
    }
}

// the editor: it never takes the keyboard; it opens with and without the panel and on every tab. With
// FWPITCHBEND_SNAPSHOTS set to a folder, it also writes PNGs of a modulated state there (for the visual checks)
static void testEditor()
{
    using namespace pb;
    using namespace pb::dsp;
    FwPitchBendProcessor p;
    {
        std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
        CHECK (ed != nullptr);
        CHECK (! ed->getWantsKeyboardFocus());
    }
    auto& st = p.apvts.state;
    setSlot (st, 0, 0, { M1, 50.0f }, nullptr);
    setSlot (st, 0, 1, { M2, -30.0f }, nullptr);
    setSlot (st, 0, 2, { AM, 100.0f }, nullptr);
    setSlot (st, 0, 3, { PT, -70.0f }, nullptr);
    setSlot (st, 2, 0, { M1, 50.0f }, nullptr);
    setSlot (st, 3, 0, { AM, 20.0f }, nullptr);
    p.modulated[0] = 6.0f;
    p.modulated[2] = 60.0f;
    set (p, pb::mod[0].type, 1.0f);
    set (p, pb::mod[0].steps, 8.0f);
    auto t = getTable (st, 0);
    setSteps (t, patterns[3].steps);
    setTable (st, 0, t, nullptr);
    const auto dir = juce::SystemStats::getEnvironmentVariable ("FWPITCHBEND_SNAPSHOTS", {});
    for (int tab = 0; tab < 5; ++tab)                           // the four panels, then the panel closed
    {
        st.setProperty (expandedId, tab < 4, nullptr);
        st.setProperty (tabId, tab, nullptr);
        st.setProperty (fineId, tab == 1, nullptr);
        std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
        CHECK (ed != nullptr);
        CHECK_NEAR (ed->getHeight() / (double) ed->getWidth(), (tab < 4 ? FwPitchBendEditor::HOpen : FwPitchBendEditor::HClosed) / (double) FwPitchBendEditor::W, 0.01);
        if (dir.isNotEmpty())
        {
            auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
            juce::File f (juce::File (dir).getChildFile ("pb_tab" + juce::String (tab) + ".png"));
            f.deleteFile();
            juce::FileOutputStream os (f);
            juce::PNGImageFormat().writeImageToStream (img, os);
        }
    }
}

// the behaviour checklist on our editor (RESEARCH.md, "The editor"), by mouse events sent to its parts at the
// reference's measured places (design px); the editor is never on screen, so no real cursor moves
namespace pb::mouse
{
juce::MouseEvent ev (juce::Component& c, juce::Point<float> local, juce::ModifierKeys mods)
{
    const auto now = juce::Time::getCurrentTime();
    return juce::MouseEvent (juce::Desktop::getInstance().getMainMouseSource(), local, mods, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                             &c, &c, now, local, now, 1, false);
}
// a press at `from` (content coordinates), moves to each point, a release; on whatever component is at `from`
void drag (juce::Component& content, juce::Point<float> from, std::initializer_list<juce::Point<float>> path, juce::ModifierKeys extra = {})
{
    auto* c = content.getComponentAt (from.roundToInt());
    const auto down = juce::ModifierKeys (juce::ModifierKeys::leftButtonModifier | extra.getRawFlags());
    auto local = [&] (juce::Point<float> p) { return c->getLocalPoint (&content, p); };
    c->mouseDown (ev (*c, local (from), down));
    for (auto p : path) c->mouseDrag (ev (*c, local (p), down));
    c->mouseUp (ev (*c, local (path.size() > 0 ? *(path.end() - 1) : from), extra));
}
void wheel (juce::Component& content, juce::Point<float> at, float dy)
{
    auto* c = content.getComponentAt (at.roundToInt());
    juce::MouseWheelDetails w { 0.0f, dy, false, false, false };
    c->mouseWheelMove (ev (*c, c->getLocalPoint (&content, at), {}), w);
}
}

static void testBehaviour()
{
    using namespace pb;
    using namespace pb::dsp;
    FwPitchBendProcessor p;
    auto& st = p.apvts.state;
    std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
    auto& content = *ed->getChildComponent (0);
    const juce::Point<float> tabM1 { 189.0f, 272.0f }, tabM2 { 251.0f, 272.0f }, tabAM { 313.0f, 272.0f }, tabPT { 375.0f, 272.0f },
        mixAt { 515.0f, 141.0f }, pitchAt { 170.0f, 150.0f }, linkAt { 278.5f, 150.5f }, plate { 40.0f, 240.0f };

    // while a tab is dragged, the knobs it can go on light up (the one under it in amber): a snapshot with
    // FWPITCHBEND_SNAPSHOTS set
    if (const auto dir = juce::SystemStats::getEnvironmentVariable ("FWPITCHBEND_SNAPSHOTS", {}); dir.isNotEmpty())
    {
        const auto down = juce::ModifierKeys (juce::ModifierKeys::leftButtonModifier);
        content.mouseDown (pb::mouse::ev (content, tabM1, down));
        content.mouseDrag (pb::mouse::ev (content, { 300.0f, 200.0f }, down));
        content.mouseDrag (pb::mouse::ev (content, mixAt, down));
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::File f (juce::File (dir).getChildFile ("pb_dragging.png"));
        f.deleteFile();
        juce::FileOutputStream os (f);
        juce::PNGImageFormat().writeImageToStream (img, os);
        content.mouseUp (pb::mouse::ev (content, { 40.0f, 240.0f }, {}));       // dropped on the plate: nothing
    }
    // a tab dragged onto Mix fills its first slot at depth 0; dropped on the plate, nothing
    pb::mouse::drag (content, tabM1, { { 300.0f, 200.0f }, mixAt });
    CHECK (getSlot (st, 2, 0).source == M1);
    CHECK_NEAR (getSlot (st, 2, 0).depth, 0.0, 1e-6);
    pb::mouse::drag (content, tabAM, { { 300.0f, 200.0f }, plate });
    CHECK (getSlot (st, 2, 1).source == None);
    // a source already on a control is refused there (M1 on Mix again); all four on Pitch, each once
    pb::mouse::drag (content, tabM1, { { 300.0f, 200.0f }, mixAt });
    CHECK (getSlot (st, 2, 1).source == None);
    for (auto tabAt : { tabM1, tabM2, tabAM, tabPT }) pb::mouse::drag (content, tabAt, { { 250.0f, 200.0f }, pitchAt });
    CHECK (getSlot (st, 0, 0).source == M1 && getSlot (st, 0, 1).source == M2 && getSlot (st, 0, 2).source == AM && getSlot (st, 0, 3).source == PT);
    pb::mouse::drag (content, tabAM, { { 250.0f, 200.0f }, pitchAt });
    int am = 0;
    for (int s = 0; s < 4; ++s) am += getSlot (st, 0, s).source == AM ? 1 : 0;
    CHECK (am == 1);

    // a slot's depth: 0.2 per px upwards (250 px: 50), clamped at 100; Alt-click 0, the source kept
    const juce::Point<float> mixSlot0 { 481.0f, 198.0f };
    pb::mouse::drag (content, mixSlot0, { { 481.0f, 148.0f }, { 481.0f, -52.0f } });
    CHECK_NEAR (getSlot (st, 2, 0).depth, 50.0, 1e-6);
    pb::mouse::drag (content, mixSlot0, { { 481.0f, -500.0f } });
    CHECK_NEAR (getSlot (st, 2, 0).depth, 100.0, 1e-6);
    pb::mouse::drag (content, mixSlot0, {}, juce::ModifierKeys::altModifier);
    CHECK_NEAR (getSlot (st, 2, 0).depth, 0.0, 1e-6);
    CHECK (getSlot (st, 2, 0).source == M1);

    // Link: both move by the same amount and stop when either reaches its end (Pitch 5 / Formant 0 up: 12 / 7);
    // Alt-click: both 0
    set (p, pb::id::pitch, 5.0f);
    set (p, pb::id::formant, 0.0f);
    std::vector<juce::Point<float>> up;
    for (int i = 1; i <= 200; ++i) up.push_back ({ linkAt.x, linkAt.y - 5.0f * (float) i });
    {
        auto* c = content.getComponentAt (linkAt.roundToInt());
        const auto down = juce::ModifierKeys (juce::ModifierKeys::leftButtonModifier);
        c->mouseDown (pb::mouse::ev (*c, c->getLocalPoint (&content, linkAt), down));
        for (auto q : up) c->mouseDrag (pb::mouse::ev (*c, c->getLocalPoint (&content, q), down));
        c->mouseUp (pb::mouse::ev (*c, c->getLocalPoint (&content, up.back()), {}));
    }
    CHECK_NEAR (p.get (pb::id::pitch), 12.0, 1e-4);
    CHECK_NEAR (p.get (pb::id::formant), 7.0, 1e-4);
    pb::mouse::drag (content, linkAt, {}, juce::ModifierKeys::altModifier);
    CHECK_NEAR (p.get (pb::id::pitch), 0.0, 1e-4);
    CHECK_NEAR (p.get (pb::id::formant), 0.0, 1e-4);

    // knobs: Alt-click the default; the wheel moves Pitch a whole semitone; a slow drag on Mix: 0.001 of the range per px
    set (p, pb::id::mix, 40.0f);
    pb::mouse::drag (content, mixAt, {}, juce::ModifierKeys::altModifier);
    CHECK_NEAR (p.get (pb::id::mix), 100.0, 1e-4);
    set (p, pb::id::pitch, 2.4f);
    pb::mouse::wheel (content, pitchAt, 1.0f);
    CHECK_NEAR (p.get (pb::id::pitch), 3.0, 1e-4);
    set (p, pb::id::mix, 50.0f);
    std::vector<juce::Point<float>> slow;
    for (int i = 1; i <= 40; ++i) slow.push_back ({ mixAt.x, mixAt.y - (float) i });
    {
        auto* c = content.getComponentAt (mixAt.roundToInt());
        const auto down = juce::ModifierKeys (juce::ModifierKeys::leftButtonModifier);
        c->mouseDown (pb::mouse::ev (*c, c->getLocalPoint (&content, mixAt), down));
        for (auto q : slow) c->mouseDrag (pb::mouse::ev (*c, c->getLocalPoint (&content, q), down));
        c->mouseUp (pb::mouse::ev (*c, c->getLocalPoint (&content, slow.back()), {}));
    }
    CHECK_NEAR (p.get (pb::id::mix), 54.0, 1e-3);

    // Pitch sticks on each semitone while Fine is off (a slow drag: 0.53e-3 of the range per px, so 20 px stays on 0,
    // 60 px reaches 1); with Fine on, a cent per px
    auto slowDrag = [&content] (juce::Point<float> at, int px)
    {
        auto* c = content.getComponentAt (at.roundToInt());
        const auto down = juce::ModifierKeys (juce::ModifierKeys::leftButtonModifier);
        c->mouseDown (pb::mouse::ev (*c, c->getLocalPoint (&content, at), down));
        for (int i = 1; i <= px; ++i) c->mouseDrag (pb::mouse::ev (*c, c->getLocalPoint (&content, at.translated (0.0f, (float) -i)), down));
        c->mouseUp (pb::mouse::ev (*c, c->getLocalPoint (&content, at.translated (0.0f, (float) -px)), {}));
    };
    set (p, pb::id::pitch, 0.0f);
    slowDrag (pitchAt, 20);
    CHECK_NEAR (p.get (pb::id::pitch), 0.0, 1e-6);
    slowDrag (pitchAt, 60);
    CHECK_NEAR (p.get (pb::id::pitch), 1.0, 1e-6);
    p.apvts.state.setProperty (pb::fineId, true, nullptr);
    slowDrag (pitchAt, 20);
    CHECK_NEAR (p.get (pb::id::pitch), 1.2, 1e-3);
    pb::mouse::wheel (content, pitchAt, 1.0f);                               // Fine: 32 cents per notch
    CHECK_NEAR (p.get (pb::id::pitch), 1.52, 1e-3);
    p.apvts.state.setProperty (pb::fineId, false, nullptr);
    set (p, pb::id::pitch, 0.0f);
    set (p, pb::id::formant, 2.0f);
    slowDrag (linkAt, 60);                                               // Link: whole semitones, the offset kept
    CHECK_NEAR (p.get (pb::id::pitch), 1.0, 1e-6);
    CHECK_NEAR (p.get (pb::id::formant), 3.0, 1e-6);
    set (p, pb::id::formant, 0.0f);

    // Fine: a click turns it on (undoable)
    pb::mouse::drag (content, { 278.0f, 200.0f }, {});
    CHECK (p.fine());
    // the Flatten lamp and note: a click on the lamp turns Flatten on; a drag up on the note, a semitone per 10 px
    pb::mouse::drag (content, { 19.0f, 159.0f }, {});
    CHECK (p.get (pb::id::flatten) > 0.5f);
    pb::mouse::drag (content, { 60.0f, 158.0f }, { { 60.0f, 148.0f }, { 60.0f, 138.0f } });
    CHECK (juce::roundToInt (p.get (pb::id::flattenNote)) == 14);
    pb::mouse::drag (content, { 50.0f, 158.0f }, {});                      // a click on the box: the next key
    CHECK (juce::roundToInt (p.get (pb::id::flattenNote)) == 15);
    pb::mouse::drag (content, { 50.0f, 158.0f }, {}, juce::ModifierKeys::altModifier);   // Alt-click: C3
    CHECK (juce::roundToInt (p.get (pb::id::flattenNote)) == 12);
}

// the factory presets: their host values, then their slots, Fine, shapes and steps; Default clears them
static void testPresets()
{
    using namespace pb;
    using namespace pb::dsp;
    FwPitchBendProcessor p;
    CHECK (p.getNumPrograms() == 41);
    int robot = -1;
    for (int i = 1; i < p.getNumPrograms(); ++i) if (p.getProgramName (i) == "Robot: Stepping Robot") robot = i;
    CHECK (robot > 0);
    p.setCurrentProgram (robot);
    auto& st = p.apvts.state;
    CHECK (p.get (pb::id::flatten) > 0.5f);
    CHECK (getSlot (st, 0, 0).source == M1);
    CHECK_NEAR (getSlot (st, 0, 0).depth, 100.0, 1e-6);
    CHECK (getSlot (st, 1, 0).source == M2);
    CHECK (p.get (pb::mod[0].type) > 0.5f);                 // a sequence, its steps kept
    bool anyStep = false;
    for (int k = 0; k < maxSteps; ++k) anyStep = anyStep || stepValue (getTable (st, 0), k) != 0;
    CHECK (anyStep);
    CHECK_NEAR (getTable (st, 1)[0], shapeValue (Shape::Steps4, 0.0), 1e-6);   // its drawn M2 shape: our Four Steps
    p.setCurrentProgram (0);
    CHECK (getSlot (st, 0, 0).source == None);
    CHECK (! p.fine());
    CHECK_NEAR (getTable (st, 0)[64], 1.0, 1e-6);

    // the toolbar's Default Preset resets the slots, Fine and the shapes too
    p.setCurrentProgram (robot);
    {
        std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
        std::function<flatwater::Toolbar* (juce::Component&)> find = [&] (juce::Component& c) -> flatwater::Toolbar*
        {
            if (auto* t = dynamic_cast<flatwater::Toolbar*> (&c)) return t;
            for (auto* k : c.getChildren()) if (auto* t = find (*k)) return t;
            return nullptr;
        };
        auto* tb = find (*ed);
        CHECK (tb != nullptr && tb->onLoadDefault != nullptr);       // (the toolbar calls it after the parameters)
        if (tb != nullptr && tb->onLoadDefault) tb->onLoadDefault();
    }
    CHECK (getSlot (st, 0, 0).source == None);
    CHECK (getSlot (st, 1, 0).source == None);
    CHECK_NEAR (getTable (st, 1)[64], defaultTable (1)[64], 1e-6);
}

// the modulators' laws as measured (MEASUREMENTS.md, Flatwater Pitch Bend, Phase 3)
static void testModLaws()
{
    using namespace pb;
    using namespace pb::dsp;
    CHECK_NEAR (freeRate (3.7, 48000.0), 48000.0 / 12968.0, 1e-9);          // whole 8-sample ticks, rounded down
    CHECK_NEAR (freeRate (7.3, 48000.0), 48000.0 / 6568.0, 1e-9);
    CHECK_NEAR (freeRate (1.0, 48000.0), 1.0, 1e-12);
    CHECK_NEAR (oddsScale (0.1, 1.5, 3.905), 1.6845, 2e-3);             // the scaling controls' laws, as measured
    CHECK_NEAR (oddsScale (0.25, 0.5, 1.0), 3.0, 1e-9);
    CHECK_NEAR (oddsScale (-0.25, 0.5, 1.0), 1.0 / 3.0, 1e-9);
    CHECK_NEAR (oddsScale (0.1, 1.0, 1.0), 1.1 / 0.9, 1e-9);
    CHECK (oddsScale (-0.5, 0.5, 1.0) < 1e-5);
    Table t {};
    for (int i = 0; i < lfoPoints; ++i) t[(size_t) i] = (float) i / lfoPoints;
    Lfo lfo;
    CHECK_NEAR (lfo.next (t, false, 16, 10.5 / lfoPoints, 0.0, 1.0, 1.0, 0.0), 10.0 / lfoPoints, 1e-7);   // each point held
    CHECK_NEAR (lfo.next (t, false, 16, 10.99 / lfoPoints, 0.0, 1.0, 1.0, 0.0), 10.0 / lfoPoints, 1e-7);
    CHECK_NEAR (lfo.next (t, false, 16, 11.0 / lfoPoints, 0.0, 1.0, 1.0, 0.0), 11.0 / lfoPoints, 1e-7);

    // a constant +1 (a sequence of one value) on Pitch at depth 100 reaches +24 semitones; on Formant it stops at +12
    for (int target : { 0, 1 })
    {
        FwPitchBendProcessor p;
        auto& st = p.apvts.state;
        set (p, mod[0].type, 1.0f);
        Table c {};
        for (int k = 0; k < maxSteps; ++k) c[(size_t) (seqFirst + k)] = 1.0f;
        setTable (st, 0, c, nullptr);
        setSlot (st, target, 0, { M1, 100.0f }, nullptr);
        p.setPlayConfigDetails (2, 2, 48000.0, 256);
        p.prepareToPlay (48000.0, 256);
        juce::AudioBuffer<float> b (2, 256);
        b.clear();
        juce::MidiBuffer midi;
        for (int blk = 0; blk < 4; ++blk) p.processBlock (b, midi);
        CHECK_NEAR (p.modulated[(size_t) target].load(), 12.0, 1e-4);       // (the knob shows its end)
        if (target == 0) CHECK_NEAR (p.pitchShifted(), 24.0, 1e-4);
    }
}

// once the grains have run they keep running at no shift, at the input's level (MEASUREMENTS.md, Phase 3)
static void testShifterLatch()
{
    pb::dsp::Shifter s;
    s.prepare (48000.0, 256, 1);
    std::vector<float> buf (256);
    float* ptr[1] { buf.data() };
    auto x = [] (double t) { double v = 0.0; for (int k = 1; k <= 8; ++k) v += std::sin (2.0 * 3.14159265358979 * 150.0 * k * t / 48000.0) / k; return 0.1 * v; };
    // the residual against the input 105.53 samples late, and the output's level against the input's, over a stretch
    auto run = [&] (long long& t, int blocks, float pitch, double* resid, double* level)
    {
        double e = 0.0, out = 0.0, in = 0.0;
        for (int b = 0; b < blocks; ++b)
        {
            for (int i = 0; i < 256; ++i) buf[(size_t) i] = (float) x ((double) (t + i));
            s.process (ptr, 1, 256, pitch, 0.0f, 0.0f, 0.0f, 0.0f);
            if (b >= blocks / 2)
                for (int i = 0; i < 256; ++i)
                {
                    const double want = x ((double) (t + i) - 105.53125);
                    e += (buf[(size_t) i] - want) * (buf[(size_t) i] - want); out += buf[(size_t) i] * buf[(size_t) i]; in += want * want;
                }
            t += 256;
        }
        *resid = 10.0 * std::log10 (std::max (e, 1e-30) / in);
        *level = 10.0 * std::log10 (out / in);
    };
    long long t = 0;
    double r = 0.0, lv = 0.0;
    run (t, 100, 0.0f, &r, &lv);
    CHECK (r < -60.0);                                                  // the pass-through
    run (t, 100, 3.0f, &r, &lv);
    run (t, 200, 0.0f, &r, &lv);
    CHECK (r > -20.0);                                                  // the grains still run
    CHECK (std::abs (lv) < 0.5);
    // digital silence after the input played: the grains stop and the pass-through is 39 whole samples late
    for (int b = 0; b < 8; ++b) { std::fill (buf.begin(), buf.end(), 0.0f); s.process (ptr, 1, 256, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f); t += 256; }
    double e = 0.0, in = 0.0;
    for (int b = 0; b < 40; ++b)
    {
        for (int i = 0; i < 256; ++i) buf[(size_t) i] = (float) x ((double) (b * 256 + i));
        s.process (ptr, 1, 256, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        if (b >= 2)
            for (int i = 0; i < 256; ++i) { const double want = x ((double) (b * 256 + i - 39)); e += (buf[(size_t) i] - want) * (buf[(size_t) i] - want); in += want * want; }
    }
    CHECK (e < 1e-12 * in);
    s.reset();
    t = 0;
    run (t, 100, 0.0f, &r, &lv);
    CHECK (r < -60.0);
}

// the DSP's ranges of the modulatable controls (dsp/Model.h targets) convert as their host parameters do, to the bit
static void testTargetRanges()
{
    FwPitchBendProcessor p;
    int same = 0, total = 0;
    for (int t = 0; t < pb::dsp::numTargets; ++t)
    {
        auto* q = dynamic_cast<juce::RangedAudioParameter*> (p.apvts.getParameter (pb::dsp::targets[t].param));
        CHECK (q != nullptr);
        if (q == nullptr) continue;
        const auto& r = pb::dsp::targets[t].range;
        for (int k = -20; k <= 1020; ++k)
        {
            const float v = (float) k / 1000.0f, x = r.start + (r.end - r.start) * v;
            total += 2;
            if (juce::exactlyEqual (q->convertFrom0to1 (v), r.from0to1 (v))) ++same;
            if (juce::exactlyEqual (q->convertTo0to1 (x), r.to0to1 (x))) ++same;
        }
    }
    CHECK (same == total);
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    testTargetRanges();
    testParameters();
    testBuses();
    testPassThrough();
    testState();
    testModel();
    testEditor();
    testBehaviour();
    testModLaws();
    testPresets();
    testShifterLatch();
    std::printf ("%d checks, %d failed\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
