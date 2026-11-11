#include "PluginEditor.h"
#include "shared/ui/Widgets.h"
#include "shared/ui/Theme.h"

using namespace juce;
using namespace flatwater::hybrid;
namespace theme = flatwater::theme;

namespace
{
// design grid (the measured layout): knob centres
const Point<float> kDelay (204.63f, 101.63f), kFeedback (516.03f, 101.51f), kDepth (181.42f, 223.15f), kRate (286.16f, 222.61f),
    kHiPass (407.16f, 222.61f), kLoPass (542.25f, 222.56f), kMix (698.69f, 62.07f), kOutput (698.63f, 142.0f),
    kAnalog (698.47f, 226.89f), kLink (473.75f, 220.25f);
const Rectangle<int> kDisplay (290, 64, 145, 78);         // the window 292 .. 430 x 66 .. 139 and its bevel
const Rectangle<float> kMeterFrame (598.0f, 67.0f, 50.0f, 198.0f);      // the housing 600 .. 646 x 69 .. 262 and around it
const Point<float> kClipLamp (621.3f, 48.6f);
const Rectangle<float> kTapPad (24.0f, 59.0f, 93.0f, 95.0f);
const Colour kSeg (theme::cyan);                               // the display's segments

float rad (float deg) { return deg * MathConstants<float>::pi / 180.0f; }

// the knobs' drag law (measured on the reference's display and Link wheel, as on its knobs): 0.001 of the range per px
// on mouse steps up to 2 px, 0.002 from 3 px, a quarter of either with Ctrl
float dragRate (float stepPx) { return (std::abs (stepPx) <= 2.0f ? 0.001f : 0.002f) * (flatwater::ctrlHeldNow() ? 0.25f : 1.0f); }
float wheelNotches (const MouseWheelDetails& w)
{
    return (std::abs (w.deltaX) > std::abs (w.deltaY) ? -w.deltaX : w.deltaY) * (w.isReversed ? -1.0f : 1.0f) / (60.0f / 256.0f);
}
float refToJuce (float deg) { return rad (deg + 180.0f); }

// the meter's scale (measured on the reference's ticks): y = 83.70 + 157.82 (1 - 10^(-dB / 40)), within 0.3 px
float meterY (float db) { return 83.70f + 157.82f * (1.0f - std::pow (10.0f, -jmax (0.0f, -db) / 40.0f)); }

// Ping Pong's start side (measured place and size): a cyan box with corners of 2.1, leaning 0.04 px right per px up;
// its letter (L or R) in straight dark strokes that stop short of each other, as on a segment display. Selected: an
// amber line round it.
void drawStartBox (Graphics& g, bool right, bool selected)
{
    const float lean = 0.04f;
    const auto shear = AffineTransform (1.0f, -lean, lean * 123.0f, 0.0f, 1.0f, 0.0f);
    if (selected)
    {
        Path ring;
        ring.addRoundedRectangle (390.22f, 94.0f, 23.6f, 29.0f, 2.8f);
        g.setColour (theme::amber);
        g.strokePath (ring, PathStrokeType (1.0f), shear);
    }
    Path box;
    box.addRoundedRectangle (391.22f, 95.0f, 21.6f, 27.0f, 2.1f);
    g.setColour (kSeg);
    g.fillPath (box, shear);
    static const float strokesL[][4] = { { 400.63f, 102.05f, 400.14f, 107.07f }, { 399.75f, 110.39f, 399.34f, 115.58f },
                                         { 401.20f, 116.16f, 406.52f, 116.14f } };
    static const float strokesR[][4] = { { 400.03f, 101.56f, 405.14f, 101.58f }, { 399.57f, 103.70f, 399.02f, 107.54f },
                                         { 407.32f, 101.97f, 407.01f, 107.24f }, { 400.71f, 108.82f, 405.41f, 108.77f },
                                         { 398.78f, 110.25f, 398.32f, 115.52f }, { 401.07f, 111.01f, 405.57f, 116.13f } };
    Path ink;
    auto add = [&ink] (const float (*st)[4], int n)
    {
        for (int i = 0; i < n; ++i) { ink.startNewSubPath (st[i][0], st[i][1]); ink.lineTo (st[i][2], st[i][3]); }
    };
    if (right) add (strokesR, 6); else add (strokesL, 3);
    g.setColour (theme::well);
    g.strokePath (ink, PathStrokeType (1.5f, PathStrokeType::mitered, PathStrokeType::square));
}
}

FwDelayEditor::FwDelayEditor (FwDelayProcessor& p)
    : Editor (p, p.apvts, p.undo, p.apvts.getParameter (de::id::mode), "Flatwater Delay", JucePlugin_VersionString, W, H,
              flatwater::Toolbar::Style::Hybrid, { "Stereo", "Mono", "M>S" }),
      proc (p),
      delay (make<Knob> (Knob::Grand, 39.0f, 321.0f)), delayNote (make<Knob> (Knob::Grand, 39.0f, 321.0f)),
      delayR (make<Knob> (Knob::Grand, 39.0f, 321.0f)), delayNoteR (make<Knob> (Knob::Grand, 39.0f, 321.0f)),
      feedback (make<Knob> (Knob::Grand, 39.0f, 321.0f)),
      depth (make<Knob> (Knob::Knurled, 39.0f, 321.0f)), rate (make<Knob> (Knob::Knurled, 39.0f, 321.0f)),
      hipass (make<Knob> (Knob::Knurled, 39.0f, 321.0f)), lopass (make<Knob> (Knob::Knurled, 39.0f, 321.0f)),
      mix (make<Knob> (Knob::Mini, 39.0f, 321.0f)), output (make<Knob> (Knob::Mini, 39.0f, 321.0f)),
      analog (make<Knob> (Knob::Mini, 39.0f, 321.0f)),
      display (make<Display> (*this)), tap (make<TapPad> (*this)), link (make<LinkWheel> (*this)), meter (make<Meter>()),
      rMark (make<RMark>())
{
    setAuxLook (std::make_unique<HybridLook>());
    for (auto* c : getChildren())
        if (dynamic_cast<ResizableCornerComponent*> (c) != nullptr) c->setAlpha (0.0f);
    setFactoryPresets (de::factoryPresets());
    setKeepOnPresetLoad ({ p.apvts.getParameter (de::id::mix) });        // presets leave Mix alone, as the reference

    auto place = [this] (Knob& k, const char* id, Rectangle<int> r, Point<float> c, double reset, std::function<String (double)> fmt)
    {
        // the component covers the knob, its shadow and the value box (shown on the knob while it is set, as the
        // reference); clicks outside the knob fall through (hitTest)
        const float e = k.paintExtent();
        const auto drawn = Rectangle<float> (c.x - e, c.y - e, 2.0f * e, 2.0f * e).getSmallestIntegerContainer();
        const auto box = Rectangle<float> (37.0f, 12.0f).withCentre (c.translated (0.0f, 2.5f)).getSmallestIntegerContainer();
        const auto b = r.getUnion (drawn).getUnion (box);
        k.centre = c - b.getPosition().toFloat();
        k.valueArea = box - b.getPosition();
        k.format = std::move (fmt); k.selection = &selection;
        attachSlider (k, id, b, reset);
        if (k.format) flatwater::flat::applyFormat (k, k.format, false);
    };
    auto noteText = [] (double v) { return de::noteNames()[jlimit (0, 19, roundToInt (v))]; };
    place (delay, de::id::delay, { 130, 27, 152, 154 }, kDelay, 375.0, [] (double v) { return String (roundToInt (v)); });
    place (delayR, de::id::delayR, { 130, 27, 152, 154 }, kDelay, 375.0, [] (double v) { return String (roundToInt (v)); });
    place (delayNote, de::id::delayNote, { 130, 27, 152, 154 }, kDelay, 11.0, {});
    place (delayNoteR, de::id::delayNoteR, { 130, 27, 152, 154 }, kDelay, 11.0, {});
    for (auto* k : { &delayNote, &delayNoteR })
    {
        k->textFromValueFunction = noteText;
        k->steps = 20;
        const float px[4] = { 30.0f, 20.0f, 120.0f, 200.0f / 3.0f };     // as Flatwater Comp's note knob (to be measured)
        std::copy (px, px + 4, k->feel.stepPx);
    }
    place (feedback, de::id::feedback, { 442, 27, 152, 154 }, kFeedback, 60.0, [] (double v) { return String (roundToInt (v)); });
    place (depth, de::id::depth, { 134, 175, 96, 106 }, kDepth, 0.0, [] (double v) { return String (roundToInt (v)); });
    place (rate, de::id::rate, { 239, 175, 96, 106 }, kRate, 0.1, [] (double v) { return de::rateText ((float) v).upToFirstOccurrenceOf (" ", false, false); });
    place (hipass, de::id::hipass, { 360, 175, 96, 106 }, kHiPass, 19.0, [] (double v) { return de::freqText ((float) v, true); });
    place (lopass, de::id::lopass, { 495, 175, 96, 106 }, kLoPass, 20138.77, [] (double v) { return de::freqText ((float) v, false); });
    place (mix, de::id::mix, { 663, 26, 73, 76 }, kMix, 100.0, [] (double v) { return String (roundToInt (v)); });
    place (output, de::id::output, { 663, 106, 73, 76 }, kOutput, 0.0, [] (double v) { return String (v, 1); });
    place (analog, de::id::analog, { 663, 191, 73, 76 }, kAnalog, 2.0, {});
    analog.steps = 5;
    for (auto& px : analog.feel.stepPx) px = 50.0f;           // as Flatwater Comp's Analog (to be measured)

    // Phase L, Ping Pong, Dual, Phase R in one frame (the reference's rectangles: 23, 62, 34, 22 px); the mono
    // component shows a single phase button
    auto* um = &p.undo;
    auto par = [&p] (const char* id) { return p.apvts.getParameter (id); };
    // (the Delay layout: frames 2 px, faces of the measured widths; the component holds the frame, 4 px around it and
    // the shadow 9 px under it)
    const String slash (CharPointer_UTF8 ("\xc3\x98")), slashSmall (CharPointer_UTF8 ("\xc3\xb8"));     // "Ø", "ø"
    auto around = [] (Rectangle<int> frame) { return frame.withTrimmedLeft (-4).withTrimmedTop (-4).withTrimmedRight (-4).withTrimmedBottom (-9); };
    stereoRow = &make<ToggleRow> (std::vector<ToggleRow::Item> { { par (de::id::phaseL), slash + "L", 20.0f },
                                                                 { par (de::id::pingPong), "PING PONG", 59.0f },
                                                                 { par (de::id::dual), "DUAL", 34.0f },
                                                                 { par (de::id::phaseR), slash + "R", 20.0f } }, um, ButtonLook::delay);
    addToContent (*stereoRow, around ({ 290, 38, 143, 22 }));
    monoRow = &make<ToggleRow> (std::vector<ToggleRow::Item> { { par (de::id::phaseL), slashSmall, 20.0f } }, um, ButtonLook::delay);
    addToContent (*monoRow, around ({ 290, 38, 24, 22 }));
    syncGroup = &make<ButtonGroup> (*par (de::id::sync), um, StringArray { "BPM", "HOST", "MS" }, false, false, ButtonLook::delay);
    addToContent (*syncGroup, around ({ 292, 146, 141, 21 }));
    lofiButton = &make<ButtonGroup> (*par (de::id::lofi), um, StringArray { "LoFi" }, false, true, ButtonLook::delay);
    addToContent (*lofiButton, around ({ 35, 188, 70, 22 }));
    // one control is selected at a time (the reference): a knob (its ring), a button group or a button (an amber line),
    // the tap pad
    for (auto* r : { stereoRow, monoRow }) r->onClick = [this] (Component* c) { selectOnly (c); };
    for (auto* b : { syncGroup, lofiButton }) b->onClick = [this] (Component* c) { selectOnly (c); };

    addToContent (display, kDisplay);
    addToContent (tap, kTapPad.expanded (6.0f).getSmallestIntegerContainer());
    const auto linkBox = Rectangle<float> (40.0f, 44.0f).withCentre (kLink.translated (0.0f, -4.0f)).getSmallestIntegerContainer();
    link.centre = kLink - linkBox.getPosition().toFloat();
    addToContent (link, linkBox);
    addToContent (meter, kMeterFrame.getSmallestIntegerContainer());
    addToContent (rMark, { 233, 168, 14, 13 });
    updateDelayKnobs();
}

float FwDelayEditor::value (const char* id) const { return proc.apvts.getRawParameterValue (id)->load(); }

void FwDelayEditor::setValue (const char* id, float v)
{
    auto* p = proc.apvts.getParameter (id);
    p->beginChangeGesture();
    p->setValueNotifyingHost (p->convertTo0to1 (v));
    p->endChangeGesture();
}

double FwDelayEditor::tempo() const
{
    if (roundToInt (value (de::id::sync)) == 0) return value (de::id::bpm);
    const double h = proc.hostBpm.load();
    return h > 0.0 ? h : 120.0;
}

// the Delay knob shows ms or note values (Sync), and in Dual the side chosen on the display
void FwDelayEditor::updateDelayKnobs()
{
    const int s = roundToInt (value (de::id::sync)), m = modeIndex();
    const bool dualOn = value (de::id::dual) > 0.5f;
    const bool dual = dualOn && m != 1, ping = value (de::id::pingPong) > 0.5f && m != 1;
    // as the reference: the right side starts from the left's time when Dual is switched on
    if (lastDualOn == 0 && dualOn)
    {
        setValue (de::id::delayR, value (de::id::delay));
        setValue (de::id::delayNoteR, value (de::id::delayNote));
    }
    lastDualOn = (int) dualOn;
    // the right side's knob while Dual's layout shows (Ping Pong's takes over the display and shows the left)
    const bool right = rightSide && dual && ! ping;
    if (s != lastSync && lastSync >= 0)
    {
        // as the reference: switching between ms and note values keeps the time, as near as the new unit allows
        const double t = tempo();
        if (s == 2)
        {
            setValue (de::id::delay, (float) (de::noteBeats (roundToInt (value (de::id::delayNote))) * 60000.0 / t));
            setValue (de::id::delayR, (float) (de::noteBeats (roundToInt (value (de::id::delayNoteR))) * 60000.0 / t));
        }
        else if (lastSync == 2)
        {
            setValue (de::id::delayNote, (float) de::nearestNote (value (de::id::delay), t));
            setValue (de::id::delayNoteR, (float) de::nearestNote (value (de::id::delayR), t));
        }
    }
    if (s == lastSync && m == lastMode && (int) dual == lastDual && (int) ping == lastPing && right == lastRight) return;
    lastSync = s; lastMode = m; lastDual = (int) dual; lastPing = (int) ping; lastRight = right;
    delay.setVisible (s == 2 && ! right);
    delayR.setVisible (s == 2 && right);
    delayNote.setVisible (s != 2 && ! right);
    delayNoteR.setVisible (s != 2 && right);
    rMark.setVisible (right);
    stereoRow->setVisible (m != 1);
    monoRow->setVisible (m == 1);
    selection.clear();
    content().repaint();
}

// the selection outline on one button control (or none); a knob's selection or the tap pad's go
void FwDelayEditor::selectOnly (Component* c)
{
    selection.clear();
    tap.deselect();
    display.selectBox (false);
    syncGroup->setOutlined (c == syncGroup);
    lofiButton->setOutlined (c == lofiButton);
    if (c != stereoRow) stereoRow->setOutlinedItem (-1);
    if (c != monoRow) monoRow->setOutlinedItem (-1);
}

void FwDelayEditor::tick()
{
    updateDelayKnobs();
    // as the reference: a note value whose time at the tempo passes 3500 ms is not taken (2 bars at 120 BPM: it stops at
    // 1 bar); the longest one that fits instead
    for (auto* id : { de::id::delayNote, de::id::delayNoteR })
    {
        int n = roundToInt (value (id));
        const int n0 = n;
        while (n > 0 && de::noteBeats (n) * 60000.0 / tempo() > 3500.0) --n;
        if (n != n0) setValue (id, (float) n);
    }
    if (selection.primary != nullptr && (lastPrimary != selection.primary))      // a knob was clicked: the others go
    {
        lastPrimary = selection.primary;
        tap.deselect();
        display.selectBox (false);
        syncGroup->setOutlined (false); lofiButton->setOutlined (false);
        stereoRow->setOutlinedItem (-1); monoRow->setOutlinedItem (-1);
    }
    if (selection.primary == nullptr) lastPrimary = nullptr;
    display.repaint();
    const double now = Time::getMillisecondCounterHiRes() * 1.0e-3;
    const double dt = lastTick > 0.0 ? jlimit (0.0, 0.1, now - lastTick) : 1.0 / 60.0;
    lastTick = now;
    const float l = proc.outLDb.take(), r = proc.outRDb.take();
    meter.set (meterL.update (l, dt), meterR.update (r, dt), modeIndex() != 1);
    // the clip light: lit at 0 dBFS (the reference's ClipLevel 0.999999), held a moment
    clipAge = jmax (l, r) >= -8.7e-6f ? 0.0 : clipAge + dt;
    const double lit = clipAge < 1.0 ? 1.0 : 0.0;
    if (! juce::exactlyEqual (lit, clipLevel))
    {
        clipLevel = lit;
        content().repaint (Rectangle<float> (24.0f, 24.0f).withCentre (kClipLamp).toNearestInt());
    }
}

//==============================================================================
// Dual's right side on the Delay knob (measured place: a box 11 x 10 px after DELAY): a cyan box, the R cut out of it
// (its stem 2.7 px, the bowl, the leg leaning right)
void FwDelayEditor::RMark::paint (Graphics& g)
{
    g.addTransform (AffineTransform::translation ((float) -getX(), (float) -getY()));
    g.setColour (kSeg);
    g.fillRoundedRectangle (Rectangle<float> (235.0f, 169.0f, 11.0f, 10.0f), 1.5f);
    Path r;
    r.addRectangle (237.0f, 171.0f, 2.7f, 4.0f);                  // the stem, narrower under the bowl
    r.addRectangle (237.0f, 175.0f, 2.4f, 2.5f);
    r.addRectangle (237.0f, 171.0f, 5.5f, 1.0f);                  // the top
    r.addRoundedRectangle (240.55f, 171.0f, 3.3f, 4.0f, 1.5f);    // the bowl's right side
    r.addRectangle (237.0f, 174.0f, 5.0f, 1.0f);                  // the middle
    r.startNewSubPath (240.4f, 175.0f);                           // the leg
    r.lineTo (243.2f, 175.0f); r.lineTo (244.35f, 177.5f); r.lineTo (241.9f, 177.5f);
    r.closeSubPath();
    r.setUsingNonZeroWinding (true);
    g.setColour (theme::panel);
    g.fillPath (r);
}

//==============================================================================
void FwDelayEditor::Display::paint (Graphics& g)
{
    // a window sunk in the card (measured: 292 .. 430 x 66 .. 139): a dark rim, a lit lip along its foot, the glass
    // dark with a faint sheen from the upper left and a few fine streaks falling to the right
    g.addTransform (AffineTransform::translation ((float) -kDisplay.getX(), (float) -kDisplay.getY()));
    const Rectangle<float> win (292.0f, 66.0f, 139.0f, 74.0f);
    g.setColour (theme::line.brighter (0.3f));
    g.drawRoundedRectangle (win.translated (0.0f, 1.0f).reduced (0.5f), 5.0f, 1.0f);
    g.setColour (theme::bg);
    g.fillRoundedRectangle (win, 5.0f);
    const Rectangle<float> pane (294.0f, 68.0f, 135.0f, 70.0f);
    {
        g.saveState();
        Path clip;
        clip.addRoundedRectangle (pane, 3.0f);
        g.reduceClipRegion (clip);
        g.setColour (theme::well);
        g.fillRect (pane);
        ColourGradient sheen (theme::raised.withAlpha (0.9f), 330.0f, 66.0f, theme::raised.withAlpha (0.0f), 330.0f + 60.0f, 66.0f + 60.0f, true);
        g.setGradientFill (sheen);
        g.fillRect (pane);
        Random rnd (4321);
        for (int i = 0; i < 9; ++i)
        {
            const float x = pane.getX() + rnd.nextFloat() * pane.getWidth(), y = pane.getY() + rnd.nextFloat() * 40.0f;
            g.setColour (Colours::white.withAlpha (0.012f + 0.02f * rnd.nextFloat()));
            g.drawLine (x, y, x + 40.0f, y + 12.0f, 0.8f + rnd.nextFloat());
        }
        g.setColour (theme::line.withAlpha (0.8f));
        g.drawRoundedRectangle (pane.reduced (0.5f), 3.0f, 1.0f);
        g.restoreState();
    }
    // the characters (measured: design coordinates, each text from its first cell's bottom-left x and its top y): the
    // fonts, and where each piece goes
    static const SegFont big { 12.85f, 22.65f, 2.45f, 2.26f, 1.56f, 0.175f, 17.05f, true },   // the tempo and the time
        noteF { 9.85f, 16.0f, 2.0f, 1.45f, 1.0f, 0.19f, 12.0f },                          // a note's figures and T / D
        dashF { 9.93f, 16.0f, 1.85f, 0.88f, 1.1f, 0.14f, 13.6f },                         // the fraction's bar, a dash per digit
        barF { 8.0f, 13.44f, 1.89f, 1.28f, 0.88f, 0.20f, 10.25f },                         // "BAR" after a 1
        label { 5.26f, 8.69f, 1.18f, 0.66f, 0.63f, 0.21f, 6.8f },                         // "BPM"
        dualMs { 9.88f, 17.05f, 1.85f, 0.72f, 0.79f, 0.10f, 14.02f },                     // Dual: the two times
        dualTempo { 10.03f, 17.01f, 2.01f, 1.02f, 0.83f, 0.17f, 13.89f },
        dualLabel { 5.26f, 8.68f, 0.99f, 0.6f, 0.42f, 0.18f, 6.42f },
        side { 6.1f, 9.4f, 1.1f, 0.6f, 0.4f, 0.17f, 7.0f },                                // Dual's "L" and "R"
        dualNote { 6.8f, 11.2f, 1.3f, 0.8f, 0.8f, 0.21f, 8.5f },
        dualBar { 5.59f, 8.72f, 1.23f, 0.51f, 0.5f, 0.17f, 6.48f };
    g.setColour (kSeg);
    const int s = roundToInt (ed.value (de::id::sync)), m = ed.modeIndex();
    const bool ping = ed.value (de::id::pingPong) > 0.5f && m != 1, dual = ed.value (de::id::dual) > 0.5f && m != 1 && ! ping;
    const String tempoText = String (roundToInt (s == 0 ? (double) ed.value (de::id::bpm) : ed.tempo()));
    // digits right-aligned in a field whose first cell starts at x (the reference's 3-digit tempo, 4-digit time)
    auto field = [&] (const SegFont& f, float x, float y, const String& t, int cells)
    {
        drawSegmentText (g, f, x + f.pitch * (float) (cells - t.length()), y, t);
    };
    // a note as the reference prints it: "1" over a dash per digit over the denominator, then T or D; a bar as "1BAR"
    // (positions measured on each of the reference's notes: its figures are not evenly spaced)
    auto note = [&] (int n, float dx)
    {
        const String t = de::noteNames()[jlimit (0, 19, n)];
        if (t.endsWith ("B"))
        {
            drawSegmentText (g, SegFont { 10.92f, 17.02f, 1.89f, 1.28f, 0.88f, 0.20f, 12.0f }, 298.67f + dx, 96.13f, t.substring (0, 1));
            drawSegmentText (g, barF, 313.85f + dx, 99.88f, "BAR");
            return;
        }
        const String den = t.fromFirstOccurrenceOf ("/", false, false).retainCharacters ("0123456789");
        const bool two = den.length() > 1;
        drawSegmentText (g, noteF, (two ? 308.0f : 302.0f) + dx, 80.95f, "1");
        drawSegmentText (g, dashF, 302.7f + dx, 97.0f, two ? "--" : "-");
        struct Pos { const char* d; float x0, x1; };
        static const Pos pos[] = { { "2", 300.7f, 0 }, { "4", 299.04f, 0 }, { "8", 300.42f, 0 }, { "16", 299.6f, 310.94f },
                                   { "32", 301.85f, 314.28f }, { "64", 301.94f, 312.54f } };
        for (auto& q : pos)
            if (den == q.d)
            {
                drawSegmentText (g, noteF, q.x0 + dx, 112.0f, den.substring (0, 1));
                if (two) drawSegmentText (g, noteF, q.x1 + dx, 112.0f, den.substring (1, 2));
            }
        if (t.endsWithChar ('T') || t.endsWithChar ('D'))
        {
            // T and D measured on their own: T's strokes run nearer the corners
            static const SegFont tF { 9.84f, 16.03f, 1.98f, 0.64f, 0.58f, 0.19f, 12.0f }, dF { 9.7f, 16.03f, 2.0f, 1.24f, 0.95f, 0.20f, 12.0f };
            drawSegmentText (g, t.endsWithChar ('T') ? tF : dF, (two ? 331.7f : 321.7f) + dx, 96.9f, t.getLastCharacters (1));
        }
    };
    // Dual's smaller notes, at a side's offset (measured on 1/8D and 1 bar)
    auto smallNote = [&] (int n, float dx)
    {
        const String t = de::noteNames()[jlimit (0, 19, n)];
        if (t.endsWith ("B"))
        {
            drawSegmentText (g, dualNote, 307.65f + dx, 95.8f, t.substring (0, 1));
            drawSegmentText (g, dualBar, 302.76f + dx, 117.8f, "BAR");
            return;
        }
        const String den = t.fromFirstOccurrenceOf ("/", false, false).retainCharacters ("0123456789");
        const float shift = den.length() > 1 ? 4.0f : 0.0f;
        drawSegmentText (g, dualNote, 303.74f + shift + dx, 96.42f, "1");
        g.fillRect (Rectangle<float> (303.0f + dx, 111.0f, 6.0f, 1.4f));        // the bar: two short strokes
        g.fillRect (Rectangle<float> (311.0f + dx, 111.0f, 6.0f, 1.4f));
        drawSegmentText (g, dualNote, 304.23f + dx - (den.length() > 1 ? 4.0f : 0.0f), 115.66f, den);
        if (t.endsWithChar ('T') || t.endsWithChar ('D'))
            drawSegmentText (g, dualNote, 319.33f + dx + shift, 106.32f, t.getLastCharacters (1));
    };
    auto msText = [] (float v) { return String (roundToInt (v)).paddedLeft ('0', 4); };
    if (ping)
    {
        // the left half of Dual's layout (the time, or the note and the tempo) and
        // the start side's box
        if (s == 2) drawSegmentText (g, dualMs, 299.31f, 101.89f, msText (ed.value (de::id::delay)));
        else
        {
            smallNote (roundToInt (ed.value (de::id::delayNote)), 0.0f);
            field (dualTempo, 338.85f, 95.95f, tempoText, 3);
            drawSegmentText (g, dualLabel, 359.93f, 118.25f, "BPM");
        }
        drawStartBox (g, ed.value (de::id::pingPongStart) > 0.5f, boxSelected);
        return;
    }
    if (dual)
    {
        drawSegmentText (g, side, 309.32f, 76.95f, "L");
        drawSegmentText (g, side, 405.71f, 76.75f, "R");
        if (s == 2)
        {
            drawSegmentText (g, dualMs, 299.31f, 101.89f, msText (ed.value (de::id::delay)));
            drawSegmentText (g, dualMs, 369.39f, 101.92f, msText (ed.value (de::id::delayR)));
            g.setColour (kSeg.withAlpha (0.37f));                                      // the divider between them
            g.fillRect (Rectangle<float> (360.0f, 80.0f, 2.0f, 50.0f));
        }
        else
        {
            smallNote (roundToInt (ed.value (de::id::delayNote)), 0.0f);
            smallNote (roundToInt (ed.value (de::id::delayNoteR)), 94.0f);
            field (dualTempo, 338.85f, 95.95f, tempoText, 3);
            drawSegmentText (g, dualLabel, 359.93f, 118.25f, "BPM");
        }
        return;
    }
    if (s == 2)
    {
        drawSegmentText (g, big, 327.16f, 92.15f, msText (ed.value (de::id::delay)));
        return;
    }
    note (roundToInt (ed.value (de::id::delayNote)), 0.0f);
    field (big, 366.71f, 93.4f, tempoText, 3);
    drawSegmentText (g, label, 398.65f, 119.76f, "BPM");
}

void FwDelayEditor::Display::mouseDown (const MouseEvent& e)
{
    ed.selectOnly (nullptr);
    const auto h = hit (e.position + kDisplay.getPosition().toFloat());          // design coordinates
    if (h.box)
    {
        // the start side flips at each click, and the box is the selected control
        ed.setValue (de::id::pingPongStart, ed.value (de::id::pingPongStart) > 0.5f ? 0.0f : 1.0f);
        selectBox (true);
        return;
    }
    if (h.side >= 0)
    {
        ed.rightSide = h.side == 1;
        ed.updateDelayKnobs();
    }
    dragParam = h.param;
    noteDrag = h.note;
    stepAcc = 0.0f;
    if (dragParam == nullptr) return;
    lastY = e.position.y;
    e.source.enableUnboundedMouseMovement (true, false);
    dragParam->beginChangeGesture();
}

// what a click, a vertical drag or the wheel sets at a point of the display (design coordinates): the reference's
// control rectangles (its layout; the first listed takes a point two share), checked with clicks and drags on it
FwDelayEditor::Display::Hit FwDelayEditor::Display::hit (Point<float> p) const
{
    const int s = roundToInt (ed.value (de::id::sync)), m = ed.modeIndex();
    const bool ping = ed.value (de::id::pingPong) > 0.5f && m != 1, dual = ed.value (de::id::dual) > 0.5f && m != 1 && ! ping;
    auto par = [this] (const char* id) { return ed.proc.apvts.getParameter (id); };
    auto in = [p] (float x, float y, float w, float h) { return Rectangle<float> (x, y, w, h).contains (p); };
    Hit h;
    if (ping && in (390.0f, 94.0f, 26.0f, 29.0f)) { h.box = true; return h; }
    if (ping || dual)
    {
        // the left time or note, the right one (Dual), the tempo between them (BPM)
        if (s == 2)
        {
            if (in (293.0f, 100.0f, 66.0f, 22.0f)) { h.param = par (de::id::delay); h.side = 0; }
            else if (dual && in (321.0f, 78.0f, 104.0f, 55.0f)) { h.param = par (de::id::delayR); h.side = 1; }
        }
        else if (in (298.0f, 95.0f, 34.0f, 34.0f)) { h.param = par (de::id::delayNote); h.side = 0; h.note = true; }
        else if (dual && in (392.0f, 95.0f, 34.0f, 34.0f)) { h.param = par (de::id::delayNoteR); h.side = 1; h.note = true; }
        else if (s == 0 && in (338.0f, 94.0f, 42.0f, 21.0f)) h.param = par (de::id::bpm);
        if (! dual) h.side = -1;
        return h;
    }
    if (s == 2) { if (in (294.0f, 62.0f, 137.0f, 74.0f)) h.param = par (de::id::delay); }
    else if (in (300.0f, 80.0f, 45.0f, 49.0f)) { h.param = par (de::id::delayNote); h.note = true; }
    else if (s == 0 && in (292.0f, 57.0f, 137.0f, 74.0f)) h.param = par (de::id::bpm);
    return h;
}

void FwDelayEditor::Display::mouseDrag (const MouseEvent& e)
{
    if (dragParam == nullptr) return;
    const float dy = e.position.y - lastY;
    lastY = e.position.y;
    if (noteDrag)
    {
        // a note value steps as on the Delay knob: 30 px per value on slow mouse steps (up to 2 px), 20 on quicker ones;
        // with Ctrl 120 and 66.7 (measured: 40 px moved the note one value, on the display as on the knob)
        const bool slow = std::abs (dy) <= 2.0f, fine = flatwater::ctrlHeldNow();
        stepAcc -= dy / (fine ? (slow ? 120.0f : 200.0f / 3.0f) : (slow ? 30.0f : 20.0f));
        const int steps = (int) stepAcc;
        if (steps == 0) return;
        stepAcc -= (float) steps;
        const float v = jlimit (0.0f, 19.0f, (float) roundToInt (dragParam->convertFrom0to1 (dragParam->getValue())) + (float) steps);
        dragParam->setValueNotifyingHost (dragParam->convertTo0to1 (v));
        return;
    }
    dragParam->setValueNotifyingHost (jlimit (0.0f, 1.0f, dragParam->getValue() - dy * dragRate (dy)));
}

void FwDelayEditor::Display::mouseUp (const MouseEvent& e)
{
    if (dragParam == nullptr) return;
    e.source.enableUnboundedMouseMovement (false);
    dragParam->endChangeGesture();
    dragParam = nullptr;
}

void FwDelayEditor::Display::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w)
{
    const auto h = hit (e.position + kDisplay.getPosition().toFloat());
    auto* p = h.param;
    if (p == nullptr || e.mods.isAnyMouseButtonDown()) return;
    p->beginChangeGesture();
    if (h.note)                                                     // a note value: one per notch
    {
        const int n = roundToInt (wheelNotches (w));
        const float v = jlimit (0.0f, 19.0f, (float) roundToInt (p->convertFrom0to1 (p->getValue())) + (float) n);
        p->setValueNotifyingHost (p->convertTo0to1 (v));
    }
    else p->setValueNotifyingHost (jlimit (0.0f, 1.0f, p->getValue() + wheelNotches (w) / 72.0f));
    p->endChangeGesture();
}

//==============================================================================
void FwDelayEditor::TapPad::paint (Graphics& g)
{
    // the pad (measured: 24 .. 118 x 59 .. 155, corners of 6; its face 36.5 .. 103.5 x 72.5 .. 140.5, corners of 5): a
    // raised rubber block lit from above with its shadow below, the face standing on it; pressed, the face sinks and its
    // rim lights in cyan. Our mark on the face (33 x 26 px). Selected (after a click, until another control is): an
    // amber line just outside the pad.
    g.addTransform (AffineTransform::translation (-getBounds().toFloat().getPosition()));
    const Rectangle<float> pad (24.0f, 59.0f, 94.0f, 96.0f), face (36.5f, 72.5f, 67.0f, 68.0f);
    for (int k = 4; k >= 1; --k)
    {
        g.setColour (Colours::black.withAlpha (0.12f));
        g.fillRoundedRectangle (pad.translated (0.24f, 0.6f * (float) k).expanded (0.4f * (float) k), 6.0f + 0.4f * (float) k);
    }
    g.setGradientFill (ColourGradient (theme::raisedTop, 0.0f, pad.getY(), theme::raised.darker (0.3f), 0.0f, pad.getBottom(), false));
    g.fillRoundedRectangle (pad, 6.0f);
    g.setColour (theme::line.brighter (0.2f));
    g.drawRoundedRectangle (pad.reduced (0.5f), 6.0f, 1.0f);
    // the face: a step down into a groove round it, then the face itself
    theme::drawWell (g, face.expanded (2.5f), 6.5f);
    if (pressed) g.setGradientFill (ColourGradient (theme::raised.darker (0.4f), 0.0f, face.getY(), theme::raised, 0.0f, face.getBottom(), false));
    else g.setGradientFill (ColourGradient (theme::raisedTop.brighter (0.1f), 0.0f, face.getY(), theme::raised, 0.0f, face.getBottom(), false));
    g.fillRoundedRectangle (pressed ? face.translated (0.0f, 1.0f) : face, 5.0f);
    g.setColour (pressed ? theme::cyan : theme::line.brighter (0.35f));
    g.drawRoundedRectangle ((pressed ? face.translated (0.0f, 1.0f) : face).reduced (0.5f), 5.0f, 1.0f);
    flatwater::drawMark (g, Rectangle<float> (53.0f, 93.0f, 33.5f, 26.5f).translated (0.0f, pressed ? 1.0f : 0.0f), false, false,
                  pressed ? theme::cyan.withAlpha (0.7f) : theme::textFaint, 1.4f);
    if (selected)
    {
        g.setColour (theme::amber);
        g.drawRoundedRectangle (Rectangle<float> (22.5f, 57.5f, 97.0f, 99.0f), 7.0f, 1.0f);
    }
}

void FwDelayEditor::TapPad::mouseDown (const MouseEvent&)
{
    ed.selectOnly (nullptr);
    pressed = selected = true;
    repaint();
    const double now = Time::getMillisecondCounterHiRes();
    const double ms = now - lastTap;
    lastTap = now;
    if (ms < 1.0 || ms > 3500.0) return;                  // the first tap, or too long ago
    const int s = roundToInt (ed.value (de::id::sync));
    const bool right = ed.rightSide;
    if (s == 2) ed.setValue (right ? de::id::delayR : de::id::delay, (float) ms);
    else if (s == 1) ed.setValue (right ? de::id::delayNoteR : de::id::delayNote, (float) de::nearestNote (ms, ed.tempo()));
    else ed.setValue (de::id::bpm, jlimit (30.0f, 300.0f, (float) (60000.0 / ms)));
}

//==============================================================================
void FwDelayEditor::LinkWheel::paint (Graphics& g)
{
    // measured: a thin ring of radius 19.2 about (473.8, 219.2); the wheel (8 square teeth, 16 px outside, 13.5 at the
    // roots) about (473.2, 218.3) with a soft shadow down and right; an arrowed arc above, its top at y 195. The cap's
    // amber index turns with the wheel.
    const auto c = centre + Point<float> (0.05f, -1.0f), wc = centre + Point<float> (-0.55f, -1.9f);
    g.setColour (theme::textFaint);
    g.drawEllipse (c.x - 19.2f, c.y - 19.2f, 38.4f, 38.4f, 1.3f);
    Path arc;
    arc.addCentredArc (c.x, c.y + 10.0f, 34.0f, 34.0f, 0.0f, rad (-24.0f), rad (24.0f), true);
    g.strokePath (arc, PathStrokeType (1.1f));
    for (float side : { -1.0f, 1.0f })
    {
        const float a = rad (25.0f * side);
        const Point<float> tip (c.x + 34.0f * std::sin (a), c.y + 10.0f - 34.0f * std::cos (a));
        Path head;
        head.addTriangle (tip.translated (2.5f * side, 1.2f), tip.translated (-2.5f * side, -1.8f), tip.translated (-1.4f * side, 2.6f));
        g.fillPath (head);
    }
    for (int k = 4; k >= 1; --k)
    {
        g.setColour (Colours::black.withAlpha (0.14f));
        const float r = 14.5f + 0.8f * (float) k;
        g.fillEllipse (wc.x + 1.5f - r, wc.y + 3.0f - r, 2.0f * r, 2.0f * r);
    }
    Path gear;
    const int teeth = 8;
    for (int i = 0; i < teeth; ++i)
    {
        // a tooth's flat top over 20 degrees, its sides falling over 4, the root between over 17
        const float a0 = turn + MathConstants<float>::twoPi * (float) i / (float) teeth;
        const float pts[4][2] = { { -12.0f, 13.5f }, { -9.5f, 16.0f }, { 9.5f, 16.0f }, { 12.0f, 13.5f } };
        for (auto& q : pts)
        {
            const float a = a0 + rad (q[0]);
            const Point<float> v (wc.x + q[1] * std::sin (a), wc.y - q[1] * std::cos (a));
            if (i == 0 && &q == &pts[0]) gear.startNewSubPath (v); else gear.lineTo (v);
        }
    }
    gear.closeSubPath();
    g.setGradientFill (ColourGradient (theme::raisedTop.brighter (0.15f), wc.x - 8.0f, wc.y - 12.0f, theme::raised.darker (0.3f), wc.x + 8.0f, wc.y + 12.0f, false));
    g.fillPath (gear);
    g.setColour (theme::line.brighter (0.3f));
    g.strokePath (gear, PathStrokeType (0.8f));
    g.setGradientFill (ColourGradient (theme::raisedTop, wc.x, wc.y - 9.5f, theme::raised, wc.x, wc.y + 9.5f, false));
    g.fillEllipse (wc.x - 9.5f, wc.y - 9.5f, 19.0f, 19.0f);
    g.setColour (theme::line);
    g.drawEllipse (wc.x - 9.5f, wc.y - 9.5f, 19.0f, 19.0f, 0.8f);
    const Point<float> d (std::sin (turn), -std::cos (turn));
    g.setColour (theme::amber);
    g.drawLine (Line<float> (wc + d * 3.0f, wc + d * 8.0f), 2.0f);
}

void FwDelayEditor::LinkWheel::mouseDown (const MouseEvent& e)
{
    lastY = e.position.y;
    e.source.enableUnboundedMouseMovement (true, false);
    for (auto* id : { de::id::hipass, de::id::lopass }) ed.proc.apvts.getParameter (id)->beginChangeGesture();
}

void FwDelayEditor::LinkWheel::mouseDrag (const MouseEvent& e)
{
    const float dy = e.position.y - lastY;
    lastY = e.position.y;
    move (-dy * dragRate (dy));
}

void FwDelayEditor::LinkWheel::mouseUp (const MouseEvent& e)
{
    e.source.enableUnboundedMouseMovement (false);
    for (auto* id : { de::id::hipass, de::id::lopass }) ed.proc.apvts.getParameter (id)->endChangeGesture();
}

void FwDelayEditor::LinkWheel::mouseWheelMove (const MouseEvent&, const MouseWheelDetails& w)
{
    for (auto* id : { de::id::hipass, de::id::lopass }) ed.proc.apvts.getParameter (id)->beginChangeGesture();
    move (wheelNotches (w) / 72.0f);
    for (auto* id : { de::id::hipass, de::id::lopass }) ed.proc.apvts.getParameter (id)->endChangeGesture();
}

// both filters by the same amount of their (equal) ranges, as far as the one nearer its end can go
void FwDelayEditor::LinkWheel::move (float d)
{
    auto* hp = ed.proc.apvts.getParameter (de::id::hipass);
    auto* lp = ed.proc.apvts.getParameter (de::id::lopass);
    const float a = hp->getValue(), b = lp->getValue();
    d = jlimit (-jmin (a, b), 1.0f - jmax (a, b), d);
    if (juce::exactlyEqual (d, 0.0f)) return;
    hp->setValueNotifyingHost (a + d);
    lp->setValueNotifyingHost (b + d);
    turn += d * 12.0f;
    repaint();
}

//==============================================================================
void FwDelayEditor::Meter::set (float lDb, float rDb, bool stereo)
{
    if (std::abs (lDb - l) < 0.05f && std::abs (rDb - r) < 0.05f && stereo == two) return;
    l = lDb; r = rDb; two = stereo;
    repaint();
}

void FwDelayEditor::Meter::paint (Graphics& g)
{
    // the housing (measured: 600 .. 646 x 69 .. 262, its window 607 .. 639 x 76 .. 254): a raised frame round a well;
    // the scale: ticks 7 px long from both sides of the window, the numbers between them at their measured centres
    g.addTransform (AffineTransform::translation (-kMeterFrame.getX(), -kMeterFrame.getY()));
    const Rectangle<float> frame (600.0f, 69.0f, 46.0f, 193.0f), window (607.0f, 76.0f, 32.0f, 178.0f);
    g.setGradientFill (ColourGradient (theme::raisedTop, 0.0f, frame.getY(), theme::raised.darker (0.25f), 0.0f, frame.getBottom(), false));
    g.fillRoundedRectangle (frame, 4.0f);
    g.setColour (theme::line.brighter (0.2f));
    g.drawRoundedRectangle (frame.reduced (0.5f), 4.0f, 1.0f);
    theme::drawWell (g, window, 2.5f);
    struct Mark { float db; const char* t; float cx, cy; };
    static const Mark marks[] = { { 0, "0", 622.5f, 83.5f }, { -3, "3", 623.0f, 107.5f }, { -6, "6", 622.5f, 129.5f },
                                  { -12, "12", 623.0f, 162.5f }, { -24, "24", 623.0f, 200.5f },
                                  { -36, "36", 622.5f, 220.5f }, { -48, "48", 622.5f, 232.0f } };
    for (auto m : marks)
    {
        const float y = meterY (m.db);
        g.setColour (theme::textFaint);
        g.fillRect (Rectangle<float> (608.0f, y - 1.0f, 6.0f, 2.0f));
        g.fillRect (Rectangle<float> (632.0f, y - 1.0f, 6.0f, 2.0f));
        g.setColour (m.db > -30.0f ? theme::text : theme::textDim);
        printText (g, m.t, { m.cx, m.cy }, 11.5f, 1.0f, true);
    }
    // an amber pointer per channel pointing inward (measured at rest: 608 .. 618 x 239 .. 246, its point 1 px below the
    // scale's -inf), a thin shadow under it
    auto tri = [&] (float db, bool left)
    {
        const float y = (db <= -99.0f ? meterY (-200.0f) : meterY (db)) + 1.0f;
        const float xb = left ? 608.0f : 638.5f, xt = left ? 618.0f : 628.5f;
        Path p;
        p.addTriangle (xb, y - 3.6f, xt, y - 0.1f, xb, y + 3.6f);
        g.setColour (Colours::black.withAlpha (0.5f));
        g.fillPath (p, AffineTransform::translation (0.4f, 1.4f));
        g.setGradientFill (ColourGradient (theme::amberLight, 0.0f, y - 3.6f, theme::amber.darker (0.3f), 0.0f, y + 3.6f, false));
        g.fillPath (p);
    };
    tri (l, true);
    tri (two ? r : l, false);
}

//==============================================================================
void FwDelayEditor::paintContent (Graphics& g)
{
    drawPanel (g, { 0, 19, W, H - 19 });
    theme::drawCard (g, { 3.0f, 22.0f, (float) W - 6.0f, (float) H - 25.0f });

    // the scales round the knobs (measured: 21 ticks of 14.1 degrees; the radii of each size)
    const float s0 = refToJuce (39.0f), s1 = refToJuce (321.0f);
    auto ticks = [&] (Point<float> c, int n, float rIn, float rOut, float thick)
    {
        Scale s; s.ticks = n; s.rIn = rIn; s.rOut = rOut; s.thick = thick;
        drawScale (g, c, s0, s1, s);
    };
    ticks (kDelay, 21, 64.3f, 72.2f, 1.9f);
    ticks (kFeedback, 21, 64.3f, 72.2f, 1.9f);
    for (auto c : { kDepth, kRate, kHiPass, kLoPass }) ticks (c, 21, 33.5f, 38.7f, 1.8f);
    ticks (kMix, 21, 24.7f, 30.6f, 1.6f);
    ticks (kOutput, 21, 24.7f, 30.6f, 1.6f);
    ticks (kAnalog, 5, 23.9f, 31.3f, 1.7f);

    // the labels and numbers, each centred at its measured place; the section headings in amber
    auto label = [&] (const char* t, float x, float y, float em, float hs) { g.setColour (theme::textDim); printText (g, t, { x, y }, em, hs, true); };
    auto heading = [&] (const char* t, float x, float y, float em, float hs) { g.setColour (theme::amber); printText (g, t, { x, y }, em, hs, true); };
    auto num = [&] (const char* t, float x, float y, float em = 9.4f) { g.setColour (theme::textFaint.brighter (0.35f)); printText (g, t, { x, y }, em, 1.0f, false); };
    label ("TAP", 68.5f, 164.0f, 11.5f, 1.05f);
    label ("DELAY", 207.5f, 174.5f, 10.75f, 1.1f); label ("FEEDBACK", 516.0f, 174.5f, 10.75f, 1.1f);
    label ("DEPTH", 181.0f, 265.0f, 9.75f, 1.05f); label ("RATE", 286.0f, 265.0f, 9.75f, 1.05f);
    label ("HiPASS", 407.5f, 265.0f, 9.75f, 1.05f); label ("LoPASS", 543.0f, 265.0f, 9.75f, 1.05f);
    label ("LINK", 474.5f, 247.0f, 7.2f, 1.1f);
    heading ("MODULATION", 231.0f, 279.0f, 10.75f, 1.1f); heading ("FILTERS", 477.0f, 279.5f, 10.75f, 1.1f);
    label ("DRY/WET", 700.5f, 97.0f, 8.6f, 1.1f); label ("OUTPUT", 700.5f, 177.0f, 8.6f, 1.1f); label ("ANALOG", 700.5f, 265.0f, 8.6f, 1.1f);
    if (roundToInt (value (de::id::sync)) == 2) { num ("1", 147.5f, 162.5f); num ("3500", 258.0f, 162.5f); }
    else { num ("1/64T", 149.0f, 162.5f); num ("2Bar", 259.0f, 163.0f); }
    num ("0", 469.5f, 162.5f); num ("200", 568.5f, 162.5f);
    num ("0", 155.5f, 254.0f); num ("100", 212.0f, 254.0f);
    num ("0.1", 255.0f, 254.5f); num ("6K", 315.0f, 254.0f);
    num ("20Hz", 374.0f, 254.0f); num ("20kHz", 442.0f, 254.0f);
    num ("20Hz", 509.0f, 254.5f); num ("20kHz", 577.0f, 254.5f);
    num ("0", 676.0f, 89.0f, 8.6f); num ("100", 728.5f, 89.0f, 8.6f);
    num ("-18", 676.0f, 167.0f, 8.6f); num ("+18", 724.5f, 167.0f, 8.6f);
    num ("OFF", 667.0f, 253.5f, 8.0f); num ("1", 666.5f, 214.5f, 8.6f); num ("2", 699.0f, 190.5f, 8.6f);
    num ("3", 731.5f, 214.5f, 8.6f); num ("4", 721.7f, 253.3f, 8.6f);

    // the brackets under MODULATION and FILTERS (the strokes end 2 px from the words), the lines from the filters to the
    // Link wheel
    {
        auto bracket = [&] (float x0, float x1, float t0, float t1)
        {
            Path p;
            p.startNewSubPath (x0, 274.0f); p.lineTo (x0, 279.8f); p.lineTo (t0, 279.8f);
            p.startNewSubPath (t1, 279.8f); p.lineTo (x1, 279.8f); p.lineTo (x1, 274.0f);
            g.strokePath (p, PathStrokeType (1.4f));
        };
        g.setColour (theme::amber.withAlpha (0.55f));
        bracket (163.5f, 298.5f, 175.0f, 287.0f);
        bracket (427.5f, 525.5f, 442.0f, 511.0f);
        g.setColour (theme::textFaint);
        g.drawLine (441.0f, 221.0f, 452.5f, 221.0f, 1.3f);
        g.drawLine (490.5f, 221.0f, 509.0f, 221.0f, 1.3f);
    }

    // the clip light above the meter (measured: a bezel to radius 12.3, a dome of 7.8): dark until the output reaches
    // 0 dBFS, then lit red
    {
        const auto c = kClipLamp;
        for (int k = 3; k >= 1; --k)
        {
            g.setColour (Colours::black.withAlpha (0.13f));
            const float r = 12.3f + 0.5f * (float) k;
            g.fillEllipse (c.x - r, c.y + 0.8f * (float) k - r, 2.0f * r, 2.0f * r);
        }
        g.setGradientFill (ColourGradient (theme::raisedTop.brighter (0.2f), c.x, c.y - 12.3f, theme::raised.darker (0.3f), c.x, c.y + 12.3f, false));
        g.fillEllipse (c.x - 12.3f, c.y - 12.3f, 24.6f, 24.6f);
        g.setColour (theme::line);
        g.drawEllipse (c.x - 12.3f, c.y - 12.3f, 24.6f, 24.6f, 0.8f);
        g.setColour (theme::well);
        g.fillEllipse (c.x - 9.0f, c.y - 9.0f, 18.0f, 18.0f);
        if (clipLevel > 0.0)
        {
            g.setColour (theme::red.withAlpha (0.3f));
            g.fillEllipse (c.x - 10.5f, c.y - 10.5f, 21.0f, 21.0f);
            g.setGradientFill (ColourGradient (theme::red.brighter (0.4f), c.x, c.y - 1.0f, theme::red.darker (0.4f), c.x, c.y + 7.8f, true));
        }
        else g.setGradientFill (ColourGradient (theme::raised, c.x, c.y + 2.6f, theme::well, c.x, c.y + 10.0f, true));
        g.fillEllipse (c.x - 7.8f, c.y - 7.8f, 15.6f, 15.6f);
        g.setColour (Colours::white.withAlpha (0.3f));
        g.fillEllipse (c.x - 5.2f, c.y - 5.0f, 2.6f, 2.0f);
    }
    // the name plate: the Flatwater mark and the plug-in's name
    drawNamePlate (g, { 16.0f, 228.0f, 130.0f, 36.0f }, "Flatwater Delay", true);
}
