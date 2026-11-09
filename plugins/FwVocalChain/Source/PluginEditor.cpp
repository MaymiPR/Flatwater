#include "PluginEditor.h"
#include "shared/ui/Theme.h"
#include "shared/ui/Logo.h"

using namespace juce;
namespace th = fw::theme;

namespace fwvocalchain::ui
{
namespace
{
float physicalScale (Graphics& g)
{
    return jlimit (0.25f, 8.0f, std::round (g.getInternalContext().getPhysicalPixelScaleFactor() * 100.0f) / 100.0f);
}

// the faders' travel: the control's 209 px less the cap's 58 (7.55 px per step of the -10 .. +10 scale, measured)
constexpr float kTravel = 209.0f - 58.0f;
constexpr int selectorX[6] = { 176, 243, 309, 376, 442, 508 };
// the faders' left edges: Input Sens, the six stages, Output (each control 43 x 215 from y 210)
constexpr int faderX[8] = { 49, 186, 253, 319, 386, 452, 519, 655 };
// the Balance knob's pointer angles at -6 / +6 dB (measured), degrees clockwise from straight up
constexpr float kBalanceFrom = -130.0f, kBalanceTo = 130.0f;

// a fader's scale: the cap's centre at +10 (y 239) to -10 (y 390), 7.55 px per step
float scaleY (float v) { return 239.0f + (10.0f - v) * 7.55f; }

Font italicFont (float size)
{
    return Font (FontOptions ("Segoe UI", size, Font::bold | Font::italic));
}

// a screw: a raised face in a dark ring, its slot across
void drawScrew (Graphics& g, Point<float> c, float r)
{
    g.setColour (Colours::black.withAlpha (0.4f));
    g.fillEllipse (Rectangle<float> (2.0f * r + 1.0f, 2.0f * r + 1.0f).withCentre (c.translated (0.0f, 1.0f)));
    g.setGradientFill (ColourGradient (th::raisedTop, 0.0f, c.y - r, th::raised, 0.0f, c.y + r, false));
    g.fillEllipse (Rectangle<float> (2.0f * r, 2.0f * r).withCentre (c));
    g.setColour (th::well);
    g.drawEllipse (Rectangle<float> (2.0f * r, 2.0f * r).withCentre (c), 1.2f);
    Path slot;
    slot.addRectangle (-r * 0.6f, -0.6f, r * 1.2f, 1.2f);
    slot.applyTransform (AffineTransform::rotation (0.6f).translated (c));
    g.setColour (th::well.withAlpha (0.9f));
    g.fillPath (slot);
}

// a stage's push button (the face 32 x 20 at the selector's (11, 50) in a dark bezel): raised and dark while the
// stage's colour is off, lit amber while it is on; an arrow on its face (it steps the colour)
void drawPushButton (Graphics& g, float sx, bool on)
{
    const Rectangle<float> face (sx + 11.0f, 167.0f, 32.0f, 20.0f);
    th::drawWell (g, face.expanded (3.0f), 4.0f);
    if (on)
    {
        g.setColour (th::amber.withAlpha (0.22f));
        g.fillRoundedRectangle (face.expanded (2.0f), 3.0f);
        g.setGradientFill (ColourGradient (th::amberLight, 0.0f, face.getY(), th::amber.darker (0.15f), 0.0f, face.getBottom(), false));
    }
    else
        g.setGradientFill (ColourGradient (th::raisedTop, 0.0f, face.getY(), th::raised, 0.0f, face.getBottom(), false));
    g.fillRoundedRectangle (face, 2.5f);
    g.setColour (on ? th::amber.darker (0.5f) : th::line);
    g.drawRoundedRectangle (face.reduced (0.5f), 2.5f, 1.0f);
    const auto c = face.getCentre();
    Path arrow;
    arrow.addTriangle (c.x - 4.0f, c.y - 0.5f, c.x + 4.0f, c.y - 0.5f, c.x, c.y - 5.0f);
    arrow.addRectangle (c.x - 1.0f, c.y - 0.5f, 2.0f, 5.5f);
    g.setColour (on ? th::bg.withAlpha (0.85f) : th::textFaint);
    g.fillPath (arrow);
}

// Flatwater Vocal Chain's name plate on the header (where the original's sits: cap height 18 px from y 70 at x 63):
// the name in bold italic, a bar under each word in amber, a soft shadow down and to the right
void drawNamePlate (Graphics& g)
{
    const float top = 70.0f, base = 88.0f, x0 = 63.0f;
    auto word = [&] (const String& w, float x)
    {
        GlyphArrangement ga;
        ga.addLineOfText (italicFont (24.0f), w, 0.0f, 0.0f);
        Path p;
        ga.createPath (p);
        const auto b = p.getBounds();
        const float sy = (base - top) / b.getHeight(), sx = sy * 0.95f;
        p.applyTransform (AffineTransform::translation (-b.getX(), -b.getY()).scaled (sx, sy).translated (x, top));
        return p;
    };
    const auto first = word ("Vocal", x0);
    const auto second = word ("Chain", first.getBounds().getRight() + 6.5f);
    Path name (first);
    name.addPath (second);
    Path bars;                                                       // 3 px under each word, the ends slanted as the letters
    for (const auto* w : { &first, &second })
    {
        const auto b = w->getBounds();
        const float l = b.getX() - 3.0f, r = b.getRight() - 1.0f, y0 = 91.0f, y1 = 94.0f;
        Path bar;
        bar.startNewSubPath (l + 1.0f, y0); bar.lineTo (r + 1.0f, y0); bar.lineTo (r, y1); bar.lineTo (l, y1); bar.closeSubPath();
        bars.addPath (bar);
    }
    Path all (name);
    all.addPath (bars);
    for (auto o : { Point<float> (2.0f, 2.4f), Point<float> (1.2f, 1.5f) })
    {
        g.setColour (Colours::black.withAlpha (0.45f));
        g.fillPath (all, AffineTransform::translation (o));
    }
    g.setGradientFill (ColourGradient (th::text, 0.0f, top, th::textDim, 0.0f, base, false));
    g.fillPath (name);
    g.setGradientFill (ColourGradient (th::amberLight, 0.0f, 91.0f, th::amber.darker (0.2f), 0.0f, 94.0f, false));
    g.fillPath (bars);
}

// the static art in design coordinates: the cheeks, the cards, the header's plate, the grille, the stages' window,
// the meters' wells, the faders' slots and scales, the labels, the screws, the name plate and the mark
void paintStatic (Graphics& g, bool stereo)
{
    g.fillAll (th::bg);

    // the side cheeks (the original's rails), x 0 .. 22 and 716 .. 738
    for (bool left : { true, false })
    {
        const Rectangle<float> r (left ? 2.0f : 718.0f, 21.0f, 18.0f, 451.0f);
        g.setGradientFill (ColourGradient (th::raised, left ? r.getX() : r.getRight(), 0.0f, th::panel, left ? r.getRight() : r.getX(), 0.0f, false));
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (th::line);
        g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.0f);
    }

    // the header (the original's plate, y 19 .. 107): a card lit from above, with fine scratches (our own seeded pattern)
    const Rectangle<float> header (24.0f, 22.0f, 690.0f, 83.0f);
    th::drawCard (g, header);
    {
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (header.reduced (1.0f).toNearestInt());
        g.setGradientFill (ColourGradient (th::raisedTop.withAlpha (0.55f), 0.0f, header.getY(), th::raisedTop.withAlpha (0.0f), 0.0f, header.getY() + 46.0f, false));
        g.fillRect (header);
        Random rnd (20261101);
        for (int i = 0; i < 260; ++i)
        {
            const float x = header.getX() + rnd.nextFloat() * header.getWidth(), y = header.getY() + rnd.nextFloat() * header.getHeight();
            const float ang = degreesToRadians ((rnd.nextBool() ? 62.0f : 118.0f) + (rnd.nextFloat() - 0.5f) * 30.0f);
            const float len = 3.0f + rnd.nextFloat() * 16.0f;
            g.setColour (th::textFaint.withAlpha (0.04f + 0.07f * rnd.nextFloat()));
            g.drawLine (x, y, x + len * std::cos (ang), y + len * std::sin (ang), 0.6f);
        }
    }
    drawScrew (g, { 40.0f, 82.5f }, 5.5f);
    drawScrew (g, { 696.5f, 82.5f }, 5.5f);
    drawNamePlate (g);
    // the maker's place on the header (top right): the Flatwater mark and the suite's name
    {
        const Rectangle<float> mark (654.0f, 28.0f, 38.0f, 27.0f);
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (mark.toNearestInt());
        fw::drawMark (g, mark, false, false);
    }
    g.setColour (th::textDim);
    g.setFont (th::font (10.0f, true).withExtraKerningFactor (0.18f));
    g.drawText ("FLATWATER", Rectangle<float> (560.0f, 35.0f, 88.0f, 13.0f), Justification::centredRight);

    // three columns: the input (x 22 .. 164), the stages (164 .. 577), the output (577 .. 716)
    th::drawCard (g, { 24.0f, 109.0f, 138.0f, 361.0f });
    th::drawCard (g, { 166.0f, 109.0f, 409.0f, 361.0f });
    th::drawCard (g, { 579.0f, 109.0f, 135.0f, 361.0f });
    g.setColour (th::line);
    g.fillRect (32.0f, 181.0f, 122.0f, 1.0f);
    g.fillRect (587.0f, 181.0f, 119.0f, 1.0f);

    // the grille (x 24 .. 162, y 114 .. 174): a raised plate, its holes 11 px apart in rows 9.53 px apart, every other
    // row offset by half a hole (measured)
    {
        const Rectangle<float> plate (28.0f, 113.0f, 130.0f, 63.0f);
        g.setGradientFill (ColourGradient (th::raisedTop, 0.0f, plate.getY(), th::raised, 0.0f, plate.getBottom(), false));
        g.fillRoundedRectangle (plate, 3.0f);
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (plate.reduced (1.0f).toNearestInt());
        for (int row = 0; row < 6; ++row)
            for (int col = -1; col < 13; ++col)
            {
                const Point<float> c ((row % 2 == 0 ? 35.65f : 30.15f) + 11.0f * (float) col, 120.41f + 9.53f * (float) row);
                g.setColour (th::line.withAlpha (0.7f));
                g.fillEllipse (Rectangle<float> (8.0f, 8.0f).withCentre (c.translated (0.0f, 0.7f)));
                g.setColour (th::well);
                g.fillEllipse (Rectangle<float> (7.6f, 7.6f).withCentre (c));
            }
    }
    g.setColour (th::line);
    g.drawRoundedRectangle (Rectangle<float> (28.0f, 113.0f, 130.0f, 63.0f).reduced (0.5f), 3.0f, 1.0f);

    // the stages' window (x 172 .. 568, y 113 .. 157): a dark well, fine scan lines, a divider between the stages
    {
        const Rectangle<float> window (172.0f, 113.0f, 396.0f, 44.0f);
        th::drawWell (g, window, 4.0f);
        g.setColour (th::panel.withAlpha (0.55f));
        for (float y = window.getY() + 2.0f; y < window.getBottom() - 1.0f; y += 2.0f)
            g.fillRect (window.getX() + 2.0f, y, window.getWidth() - 4.0f, 1.0f);
        g.setColour (th::line.withAlpha (0.8f));
        for (int i = 0; i < 5; ++i)
        {
            const float x = 0.5f * ((float) selectorX[i] + 55.0f + (float) selectorX[i + 1]);
            g.fillRect (x - 0.5f, window.getY() + 6.0f, 1.0f, window.getHeight() - 12.0f);
        }
    }

    // the badge's place (x 577 .. 716, y 107 .. 181): the Flatwater mark pressed into the panel
    {
        const Rectangle<float> b (614.0f, 121.0f, 64.0f, 46.0f);
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (b.expanded (1.0f, 2.0f).toNearestInt());
        fw::drawMark (g, b.translated (0.0f, 1.0f), false, false, th::raisedTop, 1.4f);
        fw::drawMark (g, b, false, false, th::well, 1.4f);
    }

    // the meters' wells (x 101 / 591, y 186, 48 x 238) and their scales between the bars (0 .. 24 dB, measured)
    for (float x : { 101.0f, 591.0f })
    {
        th::drawWell (g, { x, 186.0f, 48.0f, 238.0f }, 5.0f);
        struct Num { const char* t; float cy; };
        g.setColour (th::textDim);
        g.setFont (th::font (8.5f, true));
        for (auto n : { Num { "0", 229.5f }, Num { "6", 271.5f }, Num { "12", 308.0f }, Num { "18", 343.5f }, Num { "24", 379.5f } })
            g.drawText (n.t, Rectangle<float> (x + 17.0f, n.cy - 5.0f, 14.0f, 10.0f), Justification::centred);
    }
    if (stereo)                                                     // the Balance knob's L and R
    {
        g.setColour (th::textDim);
        g.setFont (th::font (8.5f, true));
        g.drawText ("L", Rectangle<float> (102.0f, 409.0f, 9.0f, 10.0f), Justification::centred);
        g.drawText ("R", Rectangle<float> (138.0f, 409.0f, 9.0f, 10.0f), Justification::centred);
    }

    // the faders' slots: a shallow trough (the original's track) and the slot down its middle, between its screws
    for (size_t i = 0; i < 8; ++i)
    {
        const float x = (float) faderX[i], cx = x + 17.0f, top = (i == 0 || i == 7) ? 216.0f : 196.0f;
        g.setColour (th::well.withAlpha (0.45f));
        g.fillRoundedRectangle (x + 5.0f, top, 24.0f, 430.0f - top, 7.0f);
        th::drawWell (g, { cx - 3.0f, 216.0f, 6.0f, 196.0f }, 3.0f);
    }
    // the scales (measured): +10, 0 and -10 printed with a dash beside each fader they belong to (left of Input Sens,
    // right of Output, both sides of the gaps between the stages), a faint tick at every step in between
    {
        const float majors[3] = { 10.0f, 0.0f, -10.0f };
        auto ticks = [&] (float x0, float x1, bool towardRight)
        {
            for (int v = -10; v <= 10; ++v)
            {
                const float y = scaleY ((float) v);
                const bool major = v % 10 == 0, mid = v % 5 == 0;
                const float len = major ? 7.0f : mid ? 5.0f : 3.0f;
                g.setColour (major ? th::textDim.withAlpha (0.8f) : th::textFaint.withAlpha (mid ? 0.9f : 0.6f));
                g.fillRect (towardRight ? x1 - len : x0, std::round (y) - 0.5f, len, 1.0f);
            }
        };
        auto number = [&] (Rectangle<float> box, float v, Justification j)
        {
            g.setColour (th::textDim);
            g.setFont (th::font (9.0f, true));
            g.drawText (v > 0.0f ? "+10" : v < 0.0f ? "-10" : "0", box.withY (std::round (scaleY (v)) - 6.0f).withHeight (12.0f), j);
        };
        ticks (42.0f, 49.0f, true);                                  // Input Sens
        ticks (689.0f, 696.0f, false);                               // Output
        for (float v : majors)
        {
            number ({ 22.0f, 0.0f, 19.0f, 12.0f }, v, Justification::centredRight);
            number ({ 698.0f, 0.0f, 18.0f, 12.0f }, v, Justification::centredLeft);
        }
        for (int i = 1; i < 6; ++i)                                  // the gaps between the stages
        {
            const float a = (float) faderX[i] + 35.0f, b = (float) faderX[i + 1];
            ticks (a, a + 7.0f, false);
            ticks (b - 7.0f, b, true);
            for (float v : majors)
                number ({ a + 7.0f, 0.0f, b - a - 14.0f, 12.0f }, v, Justification::centred);
        }
    }

    // the labels under the faders (centred where the original's are) and the link's
    {
        struct Lab { const char* t; float cx; };
        g.setColour (th::textDim);
        g.setFont (th::font (10.0f, true));
        for (auto l : { Lab { "INPUT SENS", 92.5f }, Lab { "BASS", 202.0f }, Lab { "TREBLE", 268.5f }, Lab { "COMPRESS", 337.0f },
                        Lab { "REVERB", 404.0f }, Lab { "DELAY", 469.5f }, Lab { "PITCH", 535.5f }, Lab { "OUTPUT", 646.5f } })
            g.drawText (l.t, Rectangle<float> (l.cx - 40.0f, 430.0f, 80.0f, 13.0f), Justification::centred);
        g.setFont (th::font (8.5f, true));
        g.drawText ("LINK", Rectangle<float> (690.0f, 195.0f, 24.0f, 12.0f), Justification::centredLeft);
    }

    // the screws at their measured places: the ends of the stages' slots, the foot of Input Sens' and Output's, the
    // meters' wells (not under the Balance knob in Stereo)
    for (int i = 1; i < 7; ++i)
        for (float y : { 210.5f, 417.5f })
            drawScrew (g, { (float) faderX[i] + 16.5f, y }, 4.5f);
    drawScrew (g, { 66.5f, 417.5f }, 4.5f);
    drawScrew (g, { 672.5f, 417.5f }, 4.5f);
    for (float x : { 125.5f, 615.5f })
    {
        drawScrew (g, { x, 203.5f }, 4.5f);
        if (! (stereo && x < 300.0f)) drawScrew (g, { x, 411.5f }, 4.5f);
    }
}

// a value under the marked control: a raised box in an amber frame
void hoverBox (Graphics& g, Rectangle<float> r, const String& text)
{
    g.setColour (th::raised);
    g.fillRect (r);
    g.setColour (th::amber);
    g.drawRect (r, 1.0f);
    g.setColour (th::text);
    g.setFont (th::font (10.0f));
    g.drawText (text, r, Justification::centred);
}

// an LED in its pocket (28 x 25): a dark bezel, the lamp in it
void drawLed (Graphics& g, bool on, Colour colour)
{
    th::drawWell (g, { 0.0f, 0.0f, 28.0f, 25.0f }, 5.0f);
    th::drawLamp (g, { 7.0f, 6.0f, 14.0f, 13.0f }, on, colour);
}
}

//==============================================================================
Fader::Fader (bool sensitivityCap, Marks& m) : sens (sensitivityCap), marks (m)
{
    setSliderStyle (LinearVertical);
    setTextBoxStyle (NoTextBox, true, 0, 0);
    setSliderSnapsToMousePosition (false);                          // (a click on the cap leaves the value: measured)
    setWantsKeyboardFocus (false);
    setMouseClickGrabsKeyboardFocus (false);
}

float Fader::capTop() const
{
    const double f = (getValue() - getMinimum()) / (getMaximum() - getMinimum());
    return (float) ((1.0 - f) * kTravel);
}

void Fader::paint (Graphics& g)
{
    const float y = std::round (capTop());
    // the value in the slot: a cyan line from the scale's 0 to the cap's centre
    const float zero = scaleY (0.0f) - 210.0f, at = y + 29.0f;
    if (std::abs (at - zero) > 0.5f)
    {
        g.setColour (th::cyan.withAlpha (0.85f));
        g.fillRect (Rectangle<float>::leftTopRightBottom (16.0f, jmin (zero, at), 18.0f, jmax (zero, at)));
    }
    // the cap (34 x 58): the theme's, its amber index across the middle (Input Sens' with fewer grips)
    th::drawCap (g, Rectangle<float> (1.0f, y + 1.0f, 32.0f, 56.0f), sens ? 3 : 4);
    if (marks.selected == this)                                     // the marked cap's frame
    {
        g.setColour (activeColour);
        g.drawRoundedRectangle (Rectangle<float> (0.0f, y, 34.0f, 58.0f).reduced (0.5f), 3.0f, 1.2f);
    }
}

void Fader::mouseDown (const MouseEvent& e)
{
    if (e.mods.isPopupMenu()) return;
    if (! onCap (e.position)) { marks.clear(); return; }            // the track off the cap: nothing (but unmarks)
    if (e.mods.isShiftDown() && marks.selected == this) { marks.clear(); return; }
    marks.select (this);
    if (e.mods.isAltDown()) { userSet (getDoubleClickReturnValue()); repaint(); return; }
    dragging = true;
    start = getValue();
    origin = last = virt = e.position;
    e.source.enableUnboundedMouseMovement (true, false);
    Slider::mouseDown (e);
    repaint();
}

void Fader::mouseDrag (const MouseEvent& e)
{
    if (! dragging) return;
    const auto d = e.position - last;
    last = e.position;
    const float k = fw::ctrlHeldNow() ? (std::abs (d.y) <= 2.0f ? 0.037f : 0.19f) : 1.0f;
    virt += d * k;
    const double perUnit = kTravel / (getMaximum() - getMinimum());
    setValue (jlimit (getMinimum(), getMaximum(), start + (double) (origin.y - virt.y) / perUnit), sendNotificationSync);
    if (onUserMove) onUserMove (start, getValue());
}

void Fader::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w)
{
    const double from = getValue();
    fw::wavesWheel (*this, e, w, wheelAcc, false);
    if (onUserMove && ! juce::exactlyEqual (getValue(), from)) onUserMove (from, getValue());
}

void Fader::userSet (double v)
{
    const double from = getValue();
    setValue (v, sendNotificationSync);
    if (onUserMove && ! juce::exactlyEqual (getValue(), from)) onUserMove (from, getValue());
}

void Fader::mouseUp (const MouseEvent& e)
{
    if (! dragging) return;
    dragging = false;
    Slider::mouseUp (e);
    e.source.enableUnboundedMouseMovement (false);
    const double perUnit = kTravel / (getMaximum() - getMinimum());
    const auto at = Point<float> (origin.x, origin.y - (float) ((getValue() - start) * perUnit));
    Desktop::setMousePosition (localPointToGlobal (at).roundToInt());
    if (marks.changed) marks.changed();
    repaint();
}

void Fader::mouseDoubleClick (const MouseEvent& e)
{
    if (onCap (e.position) && marks.enter) marks.enter (this);
}

//==============================================================================
ColourSelector::ColourSelector (RangedAudioParameter& param, const Stage& st, Marks& m) : p (param), stage (st), marks (m)
{
    setWantsKeyboardFocus (false);
    setMouseClickGrabsKeyboardFocus (false);
}

int ColourSelector::state() const { return roundToInt (p.convertFrom0to1 (p.getValue())); }

void ColourSelector::setState (int st)
{
    p.beginChangeGesture();
    p.setValueNotifyingHost (p.convertTo0to1 ((float) jlimit (0, 3, st)));
    p.endChangeGesture();
    repaint();
}

void ColourSelector::paint (Graphics& g)
{
    const int st = state();
    // the flavours in the window, the third at the top (rows 12 px apart from y 2); the selected one in cyan
    g.setFont (th::font (9.0f, true));
    for (int row = 0; row < 3; ++row)
    {
        const int k = 3 - row;
        g.setColour (st == k ? th::cyan : th::textFaint);
        g.drawFittedText (String (stage.names[k]).toUpperCase(), Rectangle<int> (1, row * 12, 53, 11), Justification::centred, 1, 0.8f);
    }
    if (marks.selected == this)                                     // the marked button's ring, round its bezel
    {
        g.setColour (activeColour);
        g.drawRoundedRectangle (Rectangle<float> (6.5f, 45.5f, 42.0f, 29.0f), 5.0f, 1.2f);
    }
}

void ColourSelector::mouseDown (const MouseEvent& e)
{
    if (e.mods.isPopupMenu()) return;
    if (e.y < 40)
    {
        // the names (the reference's fixed-state rows, 12 px each, the third at the top): a click selects one, a click
        // on the selected one turns the colour off; the marked control is unmarked
        marks.clear();
        const int k = jlimit (1, 3, 3 - (e.y - 1) / 12);
        setState (state() == k ? 0 : k);
        return;
    }
    if (e.mods.isShiftDown() && marks.selected == this) { marks.clear(); return; }
    marks.select (this);
    if (e.mods.isAltDown()) { setState (1); return; }                  // (the default colour)
    setState ((state() + 1) % 4);
}

//==============================================================================
void Meter::set (float l, float r, double dt)
{
    const float v[2] = { l, r };
    for (int c = 0; c < 2; ++c)
    {
        shown[c] = ballistics[c].update (v[c], dt);
        if (v[c] >= 0.0f) clip[c] = true;
    }
    repaint();
}

void Meter::paint (Graphics& g)
{
    // the bar: 155 px for 0 .. -24 dB below a top margin (the scale's 0 at y 10 and 24 at y 149 of the bar; linear in dB),
    // in segments 3 px tall, cyan into amber up the bar; the clip light above it
    auto lit = [] (float db) { return jlimit (0.0f, 155.0f, 149.0f - (jlimit (-26.0f, 0.0f, db) + 24.0f) / 24.0f * 139.0f); };
    auto bar = [&] (float x, float db, bool clipped)
    {
        const Rectangle<float> r (x, 10.0f, 9.0f, 155.0f);
        th::drawWell (g, r, 1.5f);
        const auto inner = r.reduced (1.5f);
        const float top = 10.0f + lit (db);
        const auto grad = th::meterGradient (inner, true);
        for (float yb = inner.getBottom(); yb > inner.getY() + 0.5f; yb -= 4.0f)
        {
            const auto seg = Rectangle<float>::leftTopRightBottom (inner.getX(), jmax (inner.getY(), yb - 3.0f), inner.getRight(), yb);
            const bool on = seg.getCentreY() >= top;
            g.setColour (on ? grad.getColourAtPosition ((inner.getBottom() - seg.getCentreY()) / inner.getHeight()) : th::line.withAlpha (0.35f));
            g.fillRect (seg);
        }
        th::drawLamp (g, { x, 0.0f, 9.0f, 5.0f }, clipped, th::red);
    };
    // (Mono/Stereo: the one input shown on both bars, the scale between them left clear)
    for (int c = 0; c < 2; ++c)
        bar (c == 0 ? 0.0f : 22.0f, shown[mono ? 0 : c], clip[mono ? 0 : c]);
}

//==============================================================================
void LinkButton::paintButton (Graphics& g, bool, bool)
{
    // the Sensitivity LED's body: dark while unlinked, lit amber while linked
    drawLed (g, getToggleState(), th::amber);
}

//==============================================================================
void SensLed::set (float levelDb)
{
    // the reference's LED (filmed, MEASUREMENTS.md): the input's rms level through the curve its state holds (X -144 /
    // -20 / -12 / -4 / 0 dB to Y -144 / -109.56 / -72 / -34.43 / 0, linear between), frame round (23 (Y + 144) / 144)
    static const float X[5] = { -144.0f, -20.0f, -12.0f, -4.0f, 0.0f }, Y[5] = { -144.0f, -109.56f, -72.0f, -34.43f, 0.0f };
    float y = levelDb >= X[4] ? Y[4] : Y[0];
    for (int i = 0; i < 4; ++i)
        if (levelDb >= X[i] && levelDb < X[i + 1]) y = Y[i] + (Y[i + 1] - Y[i]) * (levelDb - X[i]) / (X[i + 1] - X[i]);
    const int f = jlimit (0, 23, roundToInt ((y + 144.0f) / 144.0f * 23.0f));
    if (f != frame) { frame = f; repaint(); }
}

void SensLed::paint (Graphics& g)
{
    // its 24 steps: off, then cyan, amber and red as the level rises, brighter within each colour
    const float t = (float) frame / 23.0f;
    const Colour c = t < 0.55f ? th::cyan : t < 0.8f ? th::amber : th::red;
    drawLed (g, frame > 0, c.withAlpha (0.55f + 0.45f * t));
}

//==============================================================================
BalanceKnob::BalanceKnob (Marks& m) : marks (m)
{
    setSliderStyle (RotaryVerticalDrag);
    setTextBoxStyle (NoTextBox, true, 0, 0);
    setRotaryParameters (degreesToRadians (360.0f + kBalanceFrom), degreesToRadians (360.0f + kBalanceTo), true);
    setMouseDragSensitivity (500);                                    // (halved on small mouse steps: 1/1000 of the range per px)
    setVelocityModeParameters (1.0, 1, 0.0, false);
    setWantsKeyboardFocus (false);
    setMouseClickGrabsKeyboardFocus (false);
}

void BalanceKnob::mouseDown (const MouseEvent& e)
{
    if (e.mods.isPopupMenu()) return;
    if (e.mods.isShiftDown() && marks.selected == this) { marks.clear(); return; }
    marks.select (this);
    feel.down (*this, e, {}, nullptr);
    repaint();
}

void BalanceKnob::paint (Graphics& g)
{
    // the theme's knob round (15, 14): the cyan arc from the centre (0 dB) to the value, the amber pointer
    const float from = degreesToRadians (kBalanceFrom), to = degreesToRadians (kBalanceTo);
    const float angle = from + (to - from) * (float) valueToProportionOfLength (getValue());
    th::drawKnob (g, { 15.0f, 14.0f }, 8.0f, angle, from, to, 0.0f);
    if (marks.selected == this)                                     // the marked knob's ring, at its rim
    {
        g.setColour (activeColour);
        g.drawEllipse (Rectangle<float> (19.6f, 19.6f).withCentre ({ 15.0f, 14.0f }), 1.2f);
    }
}
}

//==============================================================================
using namespace fwvocalchain::ui;

FwVocalChainEditor::FwVocalChainEditor (FwVocalChainProcessor& p)
    : Editor (p, p.apvts, p.undo, p.apvts.getParameter (fwvocalchain::mode), "Flatwater Vocal Chain", JucePlugin_VersionString, W, H,
              fw::Toolbar::Style::Classic, fwvocalchain::modeNames()),
      proc (p),
      inMeter (make<Meter> (marks)), outMeter (make<Meter> (marks)), led (make<SensLed> (marks)), balance (make<BalanceKnob> (marks)),
      linkButton (make<LinkButton> (marks))
{
    for (auto* c : getChildren())
        if (dynamic_cast<ResizableCornerComponent*> (c) != nullptr) c->setAlpha (0.0f);
    setFactoryPresets (fwvocalchain::factoryPresets());
    setKeepOnPresetLoad ({ p.apvts.getParameter (fwvocalchain::sensitivity), p.apvts.getParameter (fwvocalchain::output),   // (IgnoreOnPresetLoad)
                           p.apvts.getParameter (fwvocalchain::link) });
    marks.changed = [this] { content().repaint(); };
    marks.enter = [this] (Slider* sl) { showEntry (sl); };

    // the faders: the control rectangles (34 x 209 from y 210), the cap's shadow reaching 9 px right of them
    const std::array<std::pair<const char*, int>, 8> fd { { { fwvocalchain::sensitivity, 49 }, { fwvocalchain::bass, 186 }, { fwvocalchain::treble, 253 },
        { fwvocalchain::compress, 319 }, { fwvocalchain::reverb, 386 }, { fwvocalchain::delay, 452 }, { fwvocalchain::pitch, 519 }, { fwvocalchain::output, 655 } } };
    for (size_t i = 0; i < fd.size(); ++i)
    {
        auto& f = make<Fader> (i == 0, marks);
        attachSlider (f, fd[i].first, { fd[i].second, 210, 43, 215 }, 0.0);
        faders.push_back (&f);
    }
    // the colour selectors (55 x 81 from y 117, over the six channel faders)
    for (size_t i = 0; i < 6; ++i)
    {
        const auto& st = fwvocalchain::stages()[i];
        auto& c = make<fwvocalchain::ui::ColourSelector> (*p.apvts.getParameter (st.colour), st, marks);
        addToContent (c, { selectorX[i], 117, 55, 81 });
        auto* att = new ParameterAttachment (*p.apvts.getParameter (st.colour), [this, &c] (float) { c.repaint(); content().repaint(); }, &p.undo);
        keepAttachment (std::unique_ptr<ParameterAttachment> (att));
        colours.push_back (&c);
    }
    addToContent (inMeter, { 109, 227, 31, 175 });
    addToContent (outMeter, { 599, 227, 31, 175 });
    addToContent (led, { 52, 189, 28, 25 });
    attachSlider (balance, fwvocalchain::balance, { 109, 395, 30, 30 }, 0.0);
    attach (linkButton, fwvocalchain::link, { 658, 189, 28, 25 });         // (where the input's LED sits, mirrored)
    linkButton.setTooltip ("Link Input Sens and Output: Output mirrors Input Sens (+3 in, -3 out)");
    linkButton.onClick = [this] { proc.syncLink(); };                   // (on: Output jumps to the mirror at once)
    linkFaders (*faders.front(), *faders.back());
    linkFaders (*faders.back(), *faders.front());
    for (auto* f : faders) f->setDoubleClickReturnValue (true, 0.0);
    balance.setDoubleClickReturnValue (true, 0.0);
    layoutForMode();
}

// the In/Out link: while it is on, Output is the mirror of Input Sens (the processor holds it there); the user's moves
// of either fader set the other to its mirror at once, so the two move together in the same gesture
void FwVocalChainEditor::linkFaders (fwvocalchain::ui::Fader& mine, fwvocalchain::ui::Fader& other)
{
    mine.onUserMove = [this, &other] (double, double to)
    {
        if (! proc.linked()) return;
        auto* param = proc.apvts.getParameter (&other == faders.back() ? fwvocalchain::output : fwvocalchain::sensitivity);
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 ((float) -to));
        param->endChangeGesture();
    };
}

FwVocalChainEditor::~FwVocalChainEditor()
{
    marks.changed = nullptr;
    entry.reset();
}

void FwVocalChainEditor::layoutForMode()
{
    const bool mono = fwvocalchain::isMonoIn (proc.modeIndex());
    if ((int) mono == laidOut) return;
    laidOut = (int) mono;
    balance.setVisible (! mono);
    if (mono && marks.selected == &balance) marks.clear();
    inMeter.mono = mono;
    content().repaint();
}

Rectangle<int> FwVocalChainEditor::valueBox (const Slider* sl) const
{
    if (sl == &balance) return { 106, 454, 37, 13 };
    for (size_t i = 0; i < faders.size(); ++i)
        if (faders[i] == sl) return { faders[i]->getX() - 1, 454, 37, 13 };
    return {};
}

void FwVocalChainEditor::paintContent (Graphics& g)
{
    const bool stereo = ! fwvocalchain::isMonoIn (proc.modeIndex());
    const float s = physicalScale (g);
    if (background.isNull() || std::abs (s - backgroundScale) > 1.0e-3f || backgroundMode != (int) stereo)
    {
        // the static art, rendered once per physical scale (the window's size times the display's) and component
        background = Image (Image::RGB, roundToInt ((float) W * s), roundToInt ((float) H * s), true);
        Graphics bg (background);
        bg.addTransform (AffineTransform::scale (s));
        paintStatic (bg, stereo);
        backgroundScale = s;
        backgroundMode = (int) stereo;
    }
    {
        Graphics::ScopedSaveState ss (g);
        g.addTransform (AffineTransform::scale (1.0f / s));
        g.setOpacity (1.0f);
        g.drawImageAt (background, 0, 0);
    }
    // the push buttons (lit while the stage's colour is on)
    for (size_t i = 0; i < 6; ++i)
        drawPushButton (g, (float) selectorX[i], colours[i]->state() != 0);
    // the value under the marked control while the mouse is on it (or while it is dragged)
    auto box = [&] (Slider* sl, const String& text) { hoverBox (g, valueBox (sl).toFloat(), text); };
    for (auto* f : faders)
        if ((marks.showsValue (f) || f->dragging) && f != entryFor) box (f, String (f->getValue(), 1));
    if (balance.isVisible() && (marks.showsValue (&balance) || balance.dragging()) && entryFor != &balance) box (&balance, String (balance.getValue(), 1));
}

void FwVocalChainEditor::tick()
{
    layoutForMode();
    const double now = Time::getMillisecondCounterHiRes() * 0.001;
    const double dt = lastTick > 0.0 ? now - lastTick : 1.0 / 60.0;
    lastTick = now;
    inMeter.set (proc.inL.take(), proc.inR.take(), dt);
    outMeter.set (proc.outL.take(), proc.outR.take(), dt);
    led.set (proc.sensLevelDb.load());
}

void FwVocalChainEditor::showEntry (Slider* sl)
{
    entry = std::make_unique<TextEditor>();
    entryFor = sl;
    auto& e = *entry;
    e.setJustification (Justification::centred);
    e.setFont (th::font (10.0f));
    e.setColour (TextEditor::backgroundColourId, th::well);
    e.setColour (TextEditor::textColourId, th::text);
    e.setColour (TextEditor::highlightColourId, th::cyan.withAlpha (0.35f));
    e.setColour (TextEditor::highlightedTextColourId, th::text);
    e.setColour (CaretComponent::caretColourId, th::amber);
    e.setColour (TextEditor::outlineColourId, th::amber);
    e.setColour (TextEditor::focusedOutlineColourId, th::amber);
    e.setText (String (sl->getValue(), 1), false);
    e.selectAll();
    addToContent (e, valueBox (sl));
    auto close = [this, sl] (bool apply)
    {
        if (apply && entry != nullptr)
        {
            const auto t = entry->getText().trim();
            if (t.containsAnyOf ("0123456789"))
            {
                const double v = t.getDoubleValue();
                if (v >= sl->getMinimum() && v <= sl->getMaximum())       // (out of range: ignored)
                {
                    if (auto* f = dynamic_cast<fwvocalchain::ui::Fader*> (sl)) f->userSet (v);
                    else sl->setValue (v, sendNotificationSync);
                }
            }
        }
        MessageManager::callAsync ([this] { entry.reset(); entryFor = nullptr; content().repaint(); });
    };
    e.onReturnKey = [close] { close (true); };
    e.onEscapeKey = [close] { close (false); };
    e.onFocusLost = [close] { close (true); };
    e.grabKeyboardFocus();
}
