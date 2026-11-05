#include "FwCompEditor.h"
#include "FwTheme.h"

using namespace juce;
using namespace fw::hybrid;
namespace th = fw::theme;

namespace
{
// design grid (the measured layout)
const Rectangle<float> kHousing (174.5f, 52.5f, 295.0f, 228.0f);   // its outline, bevels included
constexpr float kArch = 83.2f;                                  // the housing's arch around Threshold (measured: half a pixel
const Point<float> kArchCentre (320.15f, 333.35f);              // left of the knob's centre)
const Rectangle<int> kDial (197, 73, 251, 170);                 // the face's window, and the pivot's bump below it
const Point<float> kVuPivot (320.7f, 220.9f);                  // the scale's centre
const Rectangle<float> kLed (506.0f, 404.0f, 90.5f, 34.0f);
const Point<float> kLimLamp (444.5f, 75.5f);
// knob centres (measured)
const Point<float> kAnalog (88.23f, 80.10f), kOutput (554.17f, 79.99f), kRatio (86.07f, 218.47f), kMix (552.58f, 218.48f),
    kThreshold (320.65f, 333.35f), kPunch (58.25f, 344.08f), kAttack (163.17f, 344.99f), kRelease (480.18f, 343.95f);

float rad (float deg) { return deg * MathConstants<float>::pi / 180.0f; }
float refToJuce (float deg) { return rad (deg + 180.0f); }
}

FwCompEditor::FwCompEditor (FwCompProcessor& p)
    : Editor (p, p.apvts, p.undo, p.apvts.getParameter (fwcomp::mode), "Flatwater Comp", JucePlugin_VersionString, W, H,
              fw::Toolbar::Style::Hybrid),
      proc (p),
      analog (make<Knob> (Knob::Small, 39.0f, 321.0f)), output (make<Knob> (Knob::Small, 39.0f, 321.0f)),
      ratio (make<Knob> (Knob::Large, 40.0f, 320.0f)), mix (make<Knob> (Knob::Large, 40.0f, 320.0f)),
      threshold (make<Knob> (Knob::Huge, 53.0f, 307.0f)),       // 0 dB on the left, -48 dB on the right (inverted parameter)
      punch (make<Knob> (Knob::Small, 39.0f, 321.0f)), attack (make<Knob> (Knob::Small, 39.0f, 321.0f)),
      release (make<Knob> (Knob::Small, 39.0f, 321.0f)), releaseNote (make<Knob> (Knob::Small, 39.0f, 321.0f)),
      vu (make<VuMeter>())
{
    setAuxLook (std::make_unique<HybridLook>());
    for (auto* c : getChildren())
        if (dynamic_cast<ResizableCornerComponent*> (c) != nullptr) c->setAlpha (0.0f);
    setFactoryPresets (fwcomp::factoryPresets());
    setKeepOnPresetLoad ({ p.apvts.getParameter (fwcomp::mix) });        // presets leave Mix alone, as the reference

    auto place = [this] (Knob& k, const char* id, Rectangle<int> r, Point<float> c, double reset, Rectangle<int> valueArea,
                         std::function<String (double)> fmt)
    {
        // the component covers the knob, its shadow and its value slot (which can lie outside the measured control
        // rectangle); clicks outside the knob fall through (hitTest)
        const float e = k.paintExtent();
        const auto drawn = Rectangle<float> (c.x - e, c.y - e, 2.0f * e, 2.0f * e).getSmallestIntegerContainer();
        const auto b = (valueArea.isEmpty() ? r : r.getUnion (valueArea)).getUnion (drawn);
        k.centre = c - b.getPosition().toFloat();
        k.valueArea = valueArea.isEmpty() ? Rectangle<int>() : valueArea - b.getPosition();
        k.format = std::move (fmt); k.selection = &selection;
        attachSlider (k, id, b, reset);
        // a knob whose range is never positive (Threshold) takes "18" as -18, as the reference reads it
        if (k.format) fw::flat::applyFormat (k, k.format, k.getMaximum() <= 0.0);
    };
    analog.steps = 5;
    for (auto& px : analog.feel.stepPx) px = 50.0f;           // measured: a step per 50 px at any speed, Ctrl or not
    place (analog, fwcomp::analog, { 39, 30, 97, 107 }, kAnalog, 2.0, {}, {});
    place (output, fwcomp::output, { 505, 30, 97, 107 }, kOutput, 0.0, { 599, 85, 37, 13 }, [] (double v) { return String (v, 1); });
    place (ratio, fwcomp::ratio, { 6, 142, 161, 143 }, kRatio, 3.0, { 68, 213, 37, 13 }, [] (double v) { return String (v, 2); });
    place (mix, fwcomp::mix, { 473, 142, 161, 143 }, kMix, 100.0, { 534, 213, 37, 13 }, [] (double v) { return String (v, 1); });
    place (threshold, fwcomp::threshold, { 238, 249, 169, 180 }, kThreshold, -18.0, { 302, 330, 37, 13 }, [] (double v) { return String (v, 1); });
    place (punch, fwcomp::punch, { 9, 294, 97, 107 }, kPunch, 1.0, { 39, 396, 37, 13 }, [] (double v) { return String (v, 1); });
    place (attack, fwcomp::attack, { 114, 295, 96, 106 }, kAttack, 7.0, { 143, 396, 37, 13 },
           [] (double v) { return fwcomp::timeText ((float) v).upToFirstOccurrenceOf (" ", false, false); });
    place (release, fwcomp::release, { 431, 294, 97, 107 }, kRelease, 100.25, { 462, 398, 37, 13 },
           [] (double v) { return fwcomp::timeText ((float) v).upToFirstOccurrenceOf (" ", false, false); });
    place (releaseNote, fwcomp::releaseNote, { 431, 294, 97, 107 }, kRelease, 5.0, { 462, 398, 37, 13 }, {});
    releaseNote.textFromValueFunction = [] (double v) { static const char* n[] = { "1/64T", "1/64", "1/64D", "1/32T", "1/32", "1/32D", "1/16T",
        "1/16", "1/16D", "1/8T", "1/8", "1/8D", "1/4T", "1/4", "1/4D", "1/2T", "1/2", "1/2D", "1 Bar", "2 Bar" }; return String (n[jlimit (0, 19, roundToInt (v))]); };
    releaseNote.steps = 20;
    {                                                         // measured: px per step, slow / fast / Ctrl slow / Ctrl fast
        const float px[4] = { 30.0f, 20.0f, 120.0f, 200.0f / 3.0f };
        std::copy (px, px + 4, releaseNote.feel.stepPx);
    }

    auto& meterSel = make<ButtonGroup> (*p.apvts.getParameter (fwcomp::meter), &p.undo, StringArray { "IN", "GR", "OUT" }, false);
    addToContent (meterSel, Rectangle<int> (245, 27, 151, 21).expanded (4));
    auto& lim = make<ButtonGroup> (*p.apvts.getParameter (fwcomp::limiter), &p.undo, StringArray { "LIMITER" }, false, true);
    addToContent (lim, Rectangle<int> (417, 27, 52, 21).expanded (4));
    auto& sync = make<ButtonGroup> (*p.apvts.getParameter (fwcomp::sync), &p.undo, StringArray { "BPM", "HOST", "MS" }, true);
    addToContent (sync, Rectangle<int> (547, 315, 50, 61).expanded (4));

    // the BPM display takes drags and typed values in BPM mode (an invisible slider over it)
    bpmField.setSliderStyle (Slider::LinearBarVertical);
    bpmField.setTextBoxStyle (Slider::NoTextBox, true, 0, 0);
    bpmField.setAlpha (0.0f);
    attachSlider (bpmField, fwcomp::bpm, kLed.toNearestInt(), 120.0);
    bpmField.setMouseDragSensitivity (270);
    vu.pivot = kVuPivot - kDial.getPosition().toFloat();
    addToContent (vu, kDial);
    updateReleaseKnob();
    showMeter();
}

// the selected meter's face and needle
void FwCompEditor::showMeter()
{
    using namespace fwcomp::needle;
    const int m = roundToInt (value (fwcomp::meter));
    vu.setFace (m == 1 ? VuMeter::Face::gr : VuMeter::Face::io);
    vu.setNeedle (angle (m == 1 ? frame (grNeedle.x2) : frame (ioValue (m == 0 ? inNeedle.shown : outNeedle.shown))));
}

float FwCompEditor::value (const char* id) const { return proc.apvts.getRawParameterValue (id)->load(); }

void FwCompEditor::updateReleaseKnob()
{
    const int s = roundToInt (value (fwcomp::sync));
    if (s == lastSync) return;
    lastSync = s;
    release.setVisible (s == 2);                     // ms
    releaseNote.setVisible (s != 2);                 // BPM / Host: note values
    bpmField.setEnabled (s == 0);                    // the tempo is set here only in BPM mode
    content().repaint();
}

void FwCompEditor::tick()
{
    updateReleaseKnob();
    content().repaint (kLed.toNearestInt().expanded (2));

    const double now = Time::getMillisecondCounterHiRes() * 1.0e-3;
    const double dt = lastTick > 0.0 ? jlimit (0.0, 0.1, now - lastTick) : 1.0 / 60.0;
    lastTick = now;

    // the needle and the lamp move by the measured laws (FwCompNeedle.h). They move only while audio is processed (the
    // reference's needle stays put without it); the time between blocks counts when the next one arrives
    const float gr = proc.grDb.take(), lim = proc.limDb.take();
    const float in = jmax (proc.inLDb.take(), proc.inRDb.take()), out = jmax (proc.outLDb.take(), proc.outRDb.take());
    pendingDt += dt;
    const int nb = proc.blocks.load (std::memory_order_relaxed);
    if (nb != lastBlocks)
    {
        lastBlocks = nb;
        grNeedle.step (Decibels::decibelsToGain (-jmax (0.0f, gr), -200.0f), pendingDt);
        inNeedle.step (Decibels::decibelsToGain (in, -100.0f), pendingDt);
        outNeedle.step (Decibels::decibelsToGain (out, -100.0f), pendingDt);
        if (lim > 0.05f) lamp.trigger();
        pendingDt = 0.0;
    }
    showMeter();

    const double lit = lamp.step (dt);
    if (std::abs (lit - lampLevel) > 0.004 || juce::exactlyEqual (lit, 0.0) != juce::exactlyEqual (lampLevel, 0.0))
    {
        lampLevel = lit;
        content().repaint (Rectangle<float> (kLimLamp.x - 14.0f, kLimLamp.y - 14.0f, 28.0f, 28.0f).toNearestInt());
    }
}

void FwCompEditor::paintContent (Graphics& g)
{
    drawPanel (g, { 0, 19, W, H - 19 });
    th::drawCard (g, { 3.0f, 22.0f, (float) W - 6.0f, (float) H - 25.0f });
    drawVuHousing (g, kHousing, kArchCentre, kArch);
    // the limiter lamp in the housing's corner (measured: a dome of radius 8.6 at (444.5, 75.5) in a bezel to 11.2): dark
    // until the limiter acts, then lit in cyan, fading as it lets go
    {
        const auto c = kLimLamp;
        g.setGradientFill (ColourGradient (th::raisedTop.brighter (0.2f), c.x, c.y - 11.2f, th::raised.darker (0.3f), c.x, c.y + 11.2f, false));
        g.fillEllipse (c.x - 11.2f, c.y - 11.2f, 22.4f, 22.4f);
        g.setColour (th::line);
        g.drawEllipse (c.x - 11.2f, c.y - 11.2f, 22.4f, 22.4f, 0.8f);
        g.setColour (th::well);
        g.fillEllipse (c.x - 8.6f, c.y - 8.6f, 17.2f, 17.2f);
        if (lampLevel > 0.0)
        {
            ColourGradient glow (th::cyan.brighter (0.3f), c.x, c.y, th::cyan.withAlpha (0.25f), c.x + 8.6f, c.y, true);
            g.setGradientFill (glow);
            g.setOpacity ((float) lampLevel);
            g.fillEllipse (c.x - 8.6f, c.y - 8.6f, 17.2f, 17.2f);
            g.setOpacity (1.0f);
        }
        g.setColour (Colours::white.withAlpha (0.22f));
        g.fillEllipse (c.x - 5.0f, c.y - 4.4f, 3.6f, 3.0f);
    }

    // the scales round the knobs (measured: tick rings, radii, thickness; 14 degrees per tick)
    const float s0 = refToJuce (40.0f), s1 = refToJuce (320.0f);
    auto ticks = [&] (Point<float> c, int n, float a0, float a1, float rIn, float rOut, float thick)
    {
        Scale s; s.ticks = n; s.rIn = rIn; s.rOut = rOut; s.thick = thick;
        drawScale (g, c, a0, a1, s);
    };
    const Point<float> smallTicks (-0.3f, -0.4f);              // the small knobs' tick rings sit a little up-left of the knobs
    ticks (kAnalog + smallTicks, 5, s0, s1, 33.8f, 40.0f, 1.7f);
    ticks (kOutput + smallTicks, 21, s0, s1, 33.8f, 40.0f, 1.7f);
    ticks (kPunch + smallTicks, 21, s0, s1, 33.8f, 40.0f, 1.7f);
    ticks (kAttack + smallTicks, 21, s0, s1, 33.8f, 40.0f, 1.7f);
    ticks (kRelease + smallTicks, 21, s0, s1, 33.8f, 40.0f, 1.7f);
    ticks (kRatio + Point<float> (0.0f, 0.4f), 21, s0, s1, 56.2f, 64.3f, 1.65f);
    ticks (kMix + Point<float> (0.0f, 0.4f), 21, s0, s1, 56.2f, 64.3f, 1.65f);
    ticks ({ 320.7f, 332.9f }, 19, refToJuce (54.0f), refToJuce (306.0f), 72.2f, 80.4f, 1.7f);

    // the labels and numbers, each centred at its measured place
    auto label = [&] (const char* t, float x, float y) { g.setColour (th::textDim); printText (g, t, { x, y }, 10.75f, 1.1f, true); };
    auto num = [&] (const char* t, float x, float y, float em = 8.6f) { g.setColour (th::textFaint.brighter (0.35f)); printText (g, t, { x, y }, em, 1.05f, false); };
    label ("ANALOG", 88.5f, 125.9f); label ("OUTPUT", 554.6f, 126.0f);
    label ("RATIO", 88.25f, 282.1f); label ("MIX", 552.0f, 282.0f);
    label ("PUNCH", 58.1f, 389.1f); label ("ATTACK", 162.0f, 389.25f);
    label ("RELEASE", 480.75f, 389.0f); label ("SYNC", 571.0f, 389.4f);
    num ("DRY", 499.0f, 282.5f, 9.75f);
    num ("WET", 606.75f, 282.5f, 9.75f);
    num ("[dB]", 320.5f, 435.5f, 11.0f);

    num ("OFF", 53.2f, 112.5f, 7.0f); num ("1", 45.25f, 65.0f); num ("2", 87.5f, 35.5f); num ("3", 128.6f, 64.9f); num ("4", 116.4f, 110.8f, 7.3f);
    num ("-18", 519.0f, 112.9f); num ("-9", 509.0f, 64.4f); num ("0", 553.75f, 34.9f); num ("+9", 598.6f, 64.25f); num ("+18", 587.1f, 112.75f);
    num ("1", 40.75f, 273.1f); num ("8", 86.25f, 148.0f); num ("50", 134.4f, 272.9f);
    {
        static const char* mixText[] = { "0", "10", "20", "30", "40", "50", "60", "70", "80", "90", "100" };
        static const float mixPos[][2] = { { 507.75f, 272.9f }, { 486.6f, 245.25f }, { 480.4f, 210.5f }, { 490.75f, 177.4f }, { 517.9f, 154.9f },
                                           { 552.9f, 146.9f }, { 587.1f, 154.9f }, { 614.1f, 177.4f }, { 625.75f, 212.0f }, { 620.5f, 244.9f },
                                           { 602.6f, 272.9f } };
        for (int i = 0; i < 11; ++i) num (mixText[i], mixPos[i][0], mixPos[i][1]);
    }
    num ("0", 28.6f, 375.2f); num ("15", 57.9f, 297.9f); num ("30", 90.0f, 375.5f);
    num ("0.5", 126.75f, 375.4f); num ("7", 162.5f, 297.9f); num ("100", 197.4f, 375.25f);
    if (lastSync == 2) { num ("3", 449.25f, 375.4f); num ("100", 478.5f, 297.9f); num ("3S", 511.5f, 375.5f); }
    else { num ("1/64", 442.5f, 375.2f); num ("2 Bar", 518.0f, 375.2f); }
    // "0  THRESHOLD  -48" around the knob: each glyph at its measured angle and radius, its top toward the centre
    {
        auto onArc = [&] (const String& t, float deg, float r, bool bold)
        {
            const float a = rad (deg);
            printText (g, t, { kThreshold.x + r * std::sin (a), kThreshold.y - r * std::cos (a) }, 12.0f, 1.0f, bold, a - MathConstants<float>::pi);
        };
        g.setColour (th::textFaint.brighter (0.35f));
        onArc ("0", 234.35f, 83.2f, false);
        onArc ("-48", 129.0f, 84.0f, false);
        g.setColour (th::amber);
        static const float ang[] = { 214.85f, 205.8f, 197.05f, 189.25f, 181.25f, 172.7f, 163.6f, 155.65f, 147.35f };
        for (int i = 0; i < 9; ++i) onArc (String::charToString (String ("THRESHOLD")[i]), ang[i], 85.0f, true);
    }
    // the connector from RELEASE to the BPM display (measured: down from RELEASE, then right to the display), the
    // display, the name plate
    {
        Path link;
        link.startNewSubPath (478.75f, 396.0f);
        link.lineTo (478.75f, 406.25f);
        link.quadraticTo (478.75f, 409.25f, 481.75f, 409.25f);
        link.lineTo (502.5f, 409.25f);
        g.setColour (th::textFaint);
        g.strokePath (link, PathStrokeType (1.5f));
    }
    const int sync = roundToInt (value (fwcomp::sync));
    if (sync == 2) drawLedDisplay (g, kLed, "BPM OFF");
    else drawLedDisplay (g, kLed, "BPM", sync == 0 ? String (roundToInt (value (fwcomp::bpm)))
                                                    : proc.hostBpm.load() > 0.0 ? String (roundToInt (proc.hostBpm.load())) : String ("---"));
    drawNamePlate (g, { 16.0f, 407.0f, 210.0f, 30.0f }, "Flatwater Comp");
}
