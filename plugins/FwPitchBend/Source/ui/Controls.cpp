#include "Controls.h"
#include "shared/ui/Theme.h"

using namespace juce;
namespace theme = flatwater::theme;
using namespace pb;
using namespace pb::dsp;

namespace pb::ui
{
Colour sourceColour (int source)
{
    switch (source)
    {
        case M1: case M2: return theme::cyan;
        case AM: case PT: return theme::amberLight;
        default:          return theme::textFaint;
    }
}

namespace
{
constexpr float degToRad = MathConstants<float>::pi / 180.0f;

// a closed outline of `lobes` shallow flutes round c (radius r, depth d), turned by `turn`
Path fluted (Point<float> c, float r, float d, int lobes, float turn)
{
    Path p;
    const int n = lobes * 16;
    for (int i = 0; i <= n; ++i)
    {
        const float a = MathConstants<float>::twoPi * (float) i / (float) n;
        const float rr = r - d * (0.5f + 0.5f * std::cos ((float) lobes * (a - turn)));
        const Point<float> q (c.x + rr * std::sin (a), c.y - rr * std::cos (a));
        if (i == 0) p.startNewSubPath (q); else p.lineTo (q);
    }
    p.closeSubPath();
    return p;
}

void arc (Graphics& g, Point<float> c, float r, float a0, float a1, float width, Colour col)
{
    if (std::abs (a1 - a0) < 1.0e-3f) return;
    Path p;
    p.addCentredArc (c.x, c.y, r, r, 0.0f, jmin (a0, a1), jmax (a0, a1), true);
    g.setColour (col);
    g.strokePath (p, PathStrokeType (width, PathStrokeType::curved, PathStrokeType::rounded));
}

// the sizes of a knob's parts: the body, its fluted cap, the scale's dots, the value arc, the white dot's orbit, the
// first slot ring and the rings' spacing and width. Pitch and Formant keep their four rings inside their own boxes,
// clear of Link between them (the outermost ring at 68 px from the centre)
struct Parts { float body, cap, ticks, valueArc, dot, ring0, ringStep, ringWidth; };
Parts partsOf (ModKnob::Kind k)
{
    switch (k)
    {
        case ModKnob::Big:    return { 50.0f, 41.0f, 54.0f, 59.0f, 63.0f, 67.5f, 3.2f, 2.4f };
        case ModKnob::Medium: return { 29.0f, 0.0f, 0.0f, 33.0f, 37.0f, 40.5f, 2.2f, 1.6f };
        case ModKnob::Small:
        default:              return { 17.0f, 0.0f, 0.0f, 20.5f, 25.0f, 28.5f, 2.0f, 1.5f };
    }
}
}

//==============================================================================
ModKnob::ModKnob (Ui& u, Kind k, int t, String l) : kind (k), target (t), label (std::move (l)), ui (u)
{
    setSliderStyle (RotaryVerticalDrag);
    setTextBoxStyle (NoTextBox, true, 0, 0);
    const float end = (kind == Big ? 125.8f : 131.6f) * degToRad;
    setRotaryParameters (MathConstants<float>::twoPi - end, MathConstants<float>::twoPi + end, true);
    setWantsKeyboardFocus (false);
    setMouseClickGrabsKeyboardFocus (false);
}

float ModKnob::angleOf (double proportion) const
{
    const auto rp = getRotaryParameters();
    return rp.startAngleRadians + (float) proportion * (rp.endAngleRadians - rp.startAngleRadians) - MathConstants<float>::twoPi;
}

String ModKnob::valueText() const { return format ? format (getValue()) : String (getValue(), 2); }

void ModKnob::paint (Graphics& g)
{
    const auto c = centre().translated (0.0f, kind == Small ? 4.0f : 0.0f);
    const auto P = partsOf (kind);
    const auto rp = getRotaryParameters();
    const float a0 = rp.startAngleRadians - MathConstants<float>::twoPi, a1 = rp.endAngleRadians - MathConstants<float>::twoPi;
    const double prop = valueToProportionOfLength (getValue());
    const float a = angleOf (prop);
    float from = a0;
    if (getMinimum() < 0.0 && getMaximum() > 0.0) from = angleOf (valueToProportionOfLength (0.0));

    if (kind == Small)
    {
        g.setColour (theme::textDim);
        g.setFont (theme::font (9.5f, true));
        g.drawText (label, getLocalBounds().removeFromTop (11), Justification::centred);
    }

    // the scale: the track and the value's arc
    const float arcWidth = kind == Big ? 3.2f : kind == Medium ? 2.8f : 2.2f;
    arc (g, c, P.valueArc, a0, a1, arcWidth, theme::line);
    arc (g, c, P.valueArc, from, a, arcWidth, theme::cyan);
    if (kind == Big)                                             // tick dots round the scale
        for (int i = 0; i <= 24; ++i)
        {
            const float ta = a0 + (a1 - a0) * (float) i / 24.0f;
            const Point<float> d (std::sin (ta), -std::cos (ta));
            g.setColour (i % 6 == 0 ? theme::textDim : theme::textFaint);
            g.fillEllipse (Rectangle<float> (i % 6 == 0 ? 2.6f : 1.8f, i % 6 == 0 ? 2.6f : 1.8f).withCentre (c + d * P.ticks));
        }

    // the slots' arcs: one ring each, from the value, as long as the depth (depth 100: the whole range), in its source's colour
    if (target >= 0)
        for (int s = 0; s < numSlots; ++s)
        {
            const auto sl = getSlot (ui.state(), target, s);
            if (sl.source == None || exactlyEqual (sl.depth, 0.0f)) continue;
            const double to = jlimit (0.0, 1.0, prop + sl.depth / 100.0);
            arc (g, c, P.ring0 + (float) s * P.ringStep, a, angleOf (to), P.ringWidth, sourceColour (sl.source));
        }

    // the body
    const auto disc = Rectangle<float> (P.body * 2.0f, P.body * 2.0f).withCentre (c);
    if (kind == Big)
    {
        for (int k = 3; k >= 1; --k)
        {
            g.setColour (Colours::black.withAlpha (0.14f));
            g.fillEllipse (disc.translated (0.0f, 0.8f * (float) k).expanded (0.4f * (float) k));
        }
        g.setGradientFill (ColourGradient (theme::raisedTop, 0.0f, disc.getY(), theme::raised.darker (0.25f), 0.0f, disc.getBottom(), false));
        g.fillEllipse (disc);
        g.setColour (Colours::black);
        g.drawEllipse (disc.reduced (0.5f), 1.0f);
        g.setGradientFill (ColourGradient (theme::rim, 0.0f, disc.getY(), theme::rim.withAlpha (0.0f), 0.0f, disc.getCentreY(), false));
        g.drawEllipse (disc.reduced (1.5f), 1.0f);
        g.setColour (theme::line.withAlpha (0.55f));
        for (float r = P.cap + 4.0f; r < P.body - 2.0f; r += 3.0f)
            g.drawEllipse (Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c), 0.6f);
        const auto cap = fluted (c, P.cap, P.cap * 0.07f, 10, a - a0);
        g.setColour (Colours::black.withAlpha (0.45f));
        g.fillPath (cap, AffineTransform::translation (0.0f, 2.5f));
        g.setGradientFill (ColourGradient (theme::raisedTop.brighter (0.12f), 0.0f, c.y - P.cap, theme::raised, 0.0f, c.y + P.cap, false));
        g.fillPath (cap);
        g.setColour (theme::line.brighter (0.25f));
        g.strokePath (cap, PathStrokeType (1.0f));
        // the pointer: an amber triangle on the cap's edge
        const Point<float> d (std::sin (a), -std::cos (a)), n (d.y, -d.x);
        Path tri;
        tri.addTriangle (c + d * (P.cap - 1.0f), c + d * (P.cap - 11.0f) + n * 5.0f, c + d * (P.cap - 11.0f) - n * 5.0f);
        g.setColour (theme::amber);
        g.fillPath (tri);
    }
    else
    {
        g.setColour (Colours::black.withAlpha (0.35f));
        g.fillEllipse (disc.translated (0.0f, 1.5f));
        g.setGradientFill (ColourGradient (theme::raisedTop, 0.0f, disc.getY(), theme::raised, 0.0f, disc.getBottom(), false));
        g.fillEllipse (disc);
        g.setColour (Colours::black);
        g.drawEllipse (disc.reduced (0.5f), 1.0f);
        g.setGradientFill (ColourGradient (theme::rim, 0.0f, disc.getY(), theme::rim.withAlpha (0.0f), 0.0f, disc.getCentreY(), false));
        g.drawEllipse (disc.reduced (1.5f), 1.0f);
        const Point<float> d (std::sin (a), -std::cos (a));
        g.setColour (theme::amber);
        g.drawLine (Line<float> (c + d * (P.body * 0.25f), c + d * (P.body - 2.5f)), 2.2f);
    }
    if (framed)
    {
        const auto glow = ui.dropTarget == this ? theme::amber : theme::cyan;
        for (int k = 3; k >= 1; --k)
        {
            g.setColour (glow.withAlpha (0.10f));
            g.drawEllipse (disc.expanded (2.0f + 2.0f * (float) k), 2.5f);
        }
        g.setColour (glow.withAlpha (ui.dropTarget == this ? 0.22f : 0.12f));
        g.fillEllipse (disc);
        g.setColour (glow);
        g.drawEllipse (disc.expanded (2.0f), ui.dropTarget == this ? 2.5f : 1.6f);
    }
    else if (ui.selected == this)
    {
        g.setColour (theme::amber);
        g.drawEllipse (disc.expanded (2.5f), 1.5f);
    }

    // the white dot: the value with its modulation, while a slot acts on it
    if (target >= 0)
    {
        bool active = false;
        for (int s = 0; s < numSlots && ! active; ++s) { const auto sl = getSlot (ui.state(), target, s); active = sl.source != None && ! exactlyEqual (sl.depth, 0.0f); }
        if (active)
        {
            const float v = ui.proc.modulated[(size_t) target].load();
            const float da = angleOf (valueToProportionOfLength (jlimit (getMinimum(), getMaximum(), (double) v)));
            const Point<float> d (std::sin (da), -std::cos (da));
            g.setColour (theme::text.withAlpha (0.85f));
            const float dot = kind == Big ? 5.5f : kind == Medium ? 4.5f : 3.6f;
            g.fillEllipse (Rectangle<float> (dot, dot).withCentre (c + d * P.dot));
        }
    }
}

void ModKnob::mouseDown (const MouseEvent& e)
{
    if (e.mods.isPopupMenu()) return;
    ui.select (this);
    if (e.mods.isAltDown()) { setValue (defaultValue, sendNotificationSync); return; }
    startValue = getValue();
    accum = valueToProportionOfLength (startValue);
    lastY = e.position.y;
    if (isShowing()) e.source.enableUnboundedMouseMovement (true);  // the cursor hides and comes back where it was pressed
    if (onDragStart) onDragStart();
}

void ModKnob::mouseDrag (const MouseEvent& e)
{
    if (e.mods.isPopupMenu() || e.mods.isAltDown()) return;
    const float dy = lastY - e.position.y;
    lastY = e.position.y;
    // measured on the reference's knobs (40 px drags in 1, 2, 4 and 8 px mouse steps): a part of the range per px,
    // slow steps (1 .. 2 px) and fast ones (4 px and more): the 68 and 96 px knobs 0.001 / 0.002; Pitch and Formant
    // 0.00053 / 0.0016 with Fine off, 1 cent (1 / 2400) / 0.002 with it on; Ctrl a quarter of either
    const float step = std::abs (dy);
    const bool fine = kind == Big && ui.proc.fine();
    const double slow = kind != Big ? 1.0e-3 : fine ? 1.0 / 2400.0 : 0.53e-3, fast = kind != Big || fine ? 2.0e-3 : 1.6e-3;
    double perPx = step <= 2.0f ? slow : step >= 4.0f ? fast : slow + (fast - slow) * (step - 2.0f) / 2.0f;
    if (e.mods.isCtrlDown()) perPx *= 0.25;
    accum = jlimit (0.0, 1.0, accum + dy * perPx);
    double v = proportionOfLengthToValue (accum);
    if (kind == Big && ! fine) v = std::round (v);                  // Fine off: the knob sticks on each semitone
    if (! exactlyEqual (v, getValue())) setValue (v, sendNotificationSync);
}

void ModKnob::mouseUp (const MouseEvent&)
{
    if (onDragEnd) onDragEnd();
}

void ModKnob::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w)
{
    if (exactlyEqual (w.deltaY, 0.0f)) return;
    const int dir = w.deltaY > 0.0f ? 1 : -1;
    ui.select (this);
    if (kind == Big && ! ui.proc.fine())                            // a semitone per notch, onto whole semitones
        setValue (jlimit (getMinimum(), getMaximum(), std::round (getValue()) + dir), sendNotificationSync);
    else if (kind == Big)                                           // Fine: 32 cents per notch (measured)
        setValue (jlimit (getMinimum(), getMaximum(), getValue() + dir * 0.32), sendNotificationSync);
    else if (stepped)
        setValue (jlimit (getMinimum(), getMaximum(), getValue() + dir), sendNotificationSync);
    else
    {
        const double p = valueToProportionOfLength (getValue()) + dir * (wheelStep > 0.0 ? wheelStep : 0.0135);
        setValue (proportionOfLengthToValue (jlimit (0.0, 1.0, p)), sendNotificationSync);
    }
    ignoreUnused (e);
}

//==============================================================================
SlotPad::SlotPad (Ui& u, ModKnob& o, int s, bool b) : owner (o), slot (s), ui (u), big (b)
{
    setWantsKeyboardFocus (false);
    setMouseClickGrabsKeyboardFocus (false);
}

void SlotPad::paint (Graphics& g)
{
    const auto sl = getSlot (ui.state(), owner.target, slot);
    const auto r = getLocalBounds().toFloat().reduced (0.5f);
    if (sl.source == None)
    {
        if (ui.dragging == None) return;                            // empty slots show while an assignment is dragged
        g.setColour (theme::line);
        g.drawRoundedRectangle (r, 3.0f, 1.0f);
        return;
    }
    theme::drawWell (g, r, 3.0f);
    g.setColour (ui.selected == this ? theme::amber : sourceColour (sl.source).withAlpha (0.6f));
    g.drawRoundedRectangle (r, 3.0f, 1.0f);
    g.setColour (sourceColour (sl.source));
    g.setFont (theme::font (big ? 12.5f : (getHeight() < 15 ? 7.5f : 9.5f), true));
    g.drawText (sourceName (sl.source), getLocalBounds(), Justification::centred);
}

void SlotPad::mouseDown (const MouseEvent& e)
{
    auto sl = getSlot (ui.state(), owner.target, slot);
    if (e.mods.isPopupMenu())                                       // the source menu, the current one ticked
    {
        PopupMenu m;
        m.setLookAndFeel (ui.menuLook);
        m.addSectionHeader (String ("Slot ") + String (slot + 1));
        for (int s = None; s < numSources; ++s)
            m.addItem (s + 1, s == None ? "None" : sourceName (s), s == None || s == sl.source || ! holds (ui.state(), owner.target, s, slot), s == sl.source);
        m.showMenuAsync (PopupMenu::Options().withTargetComponent (this).withMinimumWidth (70).withStandardItemHeight (18), [this] (int r)
        {
            if (r <= 0) return;
            auto cur = getSlot (ui.state(), owner.target, slot);
            ui.um()->beginNewTransaction();
            setSlot (ui.state(), owner.target, slot, r - 1 == None ? Slot {} : Slot { r - 1, cur.depth }, ui.um());
            if (ui.changed) ui.changed();
        });
        return;
    }
    if (sl.source == None) return;
    ui.select (this);
    ui.um()->beginNewTransaction();
    if (e.mods.isAltDown()) { setSlot (ui.state(), owner.target, slot, { sl.source, 0.0f }, ui.um()); if (ui.changed) ui.changed(); return; }
    startDepth = sl.depth;
    lastY = e.position.y;
    draggingDepth = true;
    if (isShowing()) e.source.enableUnboundedMouseMovement (true);
}

void SlotPad::mouseDrag (const MouseEvent& e)
{
    if (! draggingDepth) return;
    const auto sl = getSlot (ui.state(), owner.target, slot);
    const float depth = jlimit (-100.0f, 100.0f, startDepth + (lastY - e.position.y) * 0.2f);   // 0.2 per px, up positive
    if (! exactlyEqual (std::round (depth), sl.depth)) setSlot (ui.state(), owner.target, slot, { sl.source, depth }, ui.um());
    if (ui.readout) ui.readout (String (sourceName (sl.source)) + " | " + String (roundToInt (depth)), owner.getBounds());
    if (ui.changed) ui.changed();
}

void SlotPad::mouseUp (const MouseEvent&)
{
    draggingDepth = false;
    if (ui.readout) ui.readout ({}, {});
}

//==============================================================================
Screen::Screen (Ui& u) : ui (u)
{
    setWantsKeyboardFocus (false);
    setMouseClickGrabsKeyboardFocus (false);
    startTimerHz (30);
}

void Screen::show (int t) { tab = t; history.clear(); repaint(); }

bool Screen::isSeq() const { return tab < 2 && ui.proc.get (mod[tab].type) > 0.5f; }
int Screen::steps() const { return tab < 2 ? jlimit (2, maxSteps, roundToInt (ui.proc.get (mod[tab].steps))) : 0; }
Rectangle<float> Screen::plot() const { return getLocalBounds().toFloat().reduced (1.5f); }

void Screen::timerCallback()
{
    if (tab >= 2)
    {
        history.push_back (tab == 2 ? ui.proc.amValue.load() : ui.proc.ptValue.load());
        if ((int) history.size() > getWidth()) history.erase (history.begin());
    }
    if (isShowing()) repaint();
}

void Screen::paint (Graphics& g)
{
    theme::drawWell (g, getLocalBounds().toFloat(), 3.0f);
    const auto r = plot();
    const auto col = tab < 2 ? theme::cyan : theme::amberLight;
    auto yOf = [&r] (float v) { return r.getCentreY() - v * (r.getHeight() * 0.5f - 2.0f); };   // v -1 .. 1
    g.setColour (theme::line);
    g.drawHorizontalLine (roundToInt (r.getCentreY()), r.getX(), r.getRight());

    if (tab < 2)
    {
        const auto t = getTable (ui.state(), tab);
        if (isSeq())
        {
            const int n = steps();
            const float w = r.getWidth() / (float) n;
            const int playing = jlimit (0, n - 1, (int) (ui.proc.cycle[tab].load() * (float) n));
            for (int i = 0; i < n; ++i)
            {
                const float v = (float) stepValue (t, i) / 24.0f, x = r.getX() + w * (float) i;
                auto bar = Rectangle<float>::leftTopRightBottom (x + 1.5f, jmin (yOf (v), r.getCentreY()), x + w - 1.5f, jmax (yOf (v), r.getCentreY()));
                g.setColour (col.withAlpha (i == playing ? 0.75f : 0.45f));
                g.fillRect (bar.withHeight (jmax (1.5f, bar.getHeight())));
                g.setColour (theme::line);
                if (i > 0) g.drawVerticalLine (roundToInt (x), r.getY(), r.getBottom());
            }
            if (editStep >= 0 && editStep < n)                      // the step's value above it while it is dragged
            {
                const float x = r.getX() + w * (float) editStep;
                g.setColour (theme::text);
                g.setFont (theme::font (10.0f, true));
                g.drawText (String (stepValue (t, editStep)), Rectangle<float> (x - 6.0f, r.getY(), w + 12.0f, 12.0f), Justification::centred);
            }
        }
        else
        {
            Path curve, fill;
            for (int i = 0; i <= lfoPoints; ++i)
            {
                const Point<float> p (r.getX() + r.getWidth() * (float) i / lfoPoints, yOf (t[(size_t) (i % lfoPoints)]));
                if (i == 0) { curve.startNewSubPath (p); fill.startNewSubPath (p.x, r.getCentreY()); }
                else curve.lineTo (p);
                fill.lineTo (p);
            }
            fill.lineTo (r.getRight(), r.getCentreY());
            fill.closeSubPath();
            g.setColour (col.withAlpha (0.22f));
            g.fillPath (fill);
            g.setColour (col);
            g.strokePath (curve, PathStrokeType (1.6f));
            g.setColour (theme::text.withAlpha (0.8f));                // the play-head
            g.drawVerticalLine (roundToInt (r.getX() + r.getWidth() * ui.proc.cycle[tab].load()), r.getY(), r.getBottom());
            if (drawMode)
            {
                g.setColour (theme::amber);
                g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 3.0f, 1.0f);
            }
        }
    }
    else if (! history.empty())                                     // AM (0 .. 1, from the bottom) or PT (-1 .. 1)
    {
        Path curve;
        const float x0 = r.getRight() - (float) history.size();
        for (size_t i = 0; i < history.size(); ++i)
        {
            const float v = tab == 2 ? jlimit (0.0f, 1.0f, history[i]) * 2.0f - 1.0f : jlimit (-1.0f, 1.0f, history[i]);
            const Point<float> p (x0 + (float) i, yOf (v));
            if (i == 0) curve.startNewSubPath (p); else curve.lineTo (p);
        }
        g.setColour (col);
        g.strokePath (curve, PathStrokeType (1.5f));
    }
}

void Screen::mouseDown (const MouseEvent& e)
{
    if (tab >= 2) return;
    const auto r = plot();
    ui.um()->beginNewTransaction();
    if (isSeq())
    {
        const int n = steps();
        editStep = jlimit (0, n - 1, (int) ((e.position.x - r.getX()) / r.getWidth() * (float) n));
        startStep = stepValue (getTable (ui.state(), tab), editStep);
        startY = e.position.y;
        repaint();
    }
    else if (drawMode)
    {
        lastIndex = -1;
        mouseDrag (e);
    }
}

void Screen::mouseDrag (const MouseEvent& e)
{
    if (tab >= 2) return;
    const auto r = plot();
    auto t = getTable (ui.state(), tab);
    if (isSeq() && editStep >= 0)                                   // about 20 px per step of value
    {
        const int v = jlimit (-24, 24, startStep + roundToInt ((startY - e.position.y) / 20.0f));
        if (v != stepValue (t, editStep))
        {
            t[(size_t) (seqFirst + editStep)] = (float) v / 24.0f;
            setTable (ui.state(), tab, t, ui.um());
        }
        repaint();
        return;
    }
    if (! drawMode) return;
    // draw: the cycle follows the mouse's path, the points between two events joined by a straight line
    const int i = jlimit (0, lfoPoints - 1, (int) ((e.position.x - r.getX()) / r.getWidth() * lfoPoints));
    const float v = jlimit (-1.0f, 1.0f, (r.getCentreY() - e.position.y) / (r.getHeight() * 0.5f - 2.0f));
    if (lastIndex < 0) t[(size_t) i] = v;
    else
    {
        const int a = jmin (lastIndex, i), b = jmax (lastIndex, i);
        for (int k = a; k <= b; ++k)
        {
            const float f = b == a ? 1.0f : (float) (k - lastIndex) / (float) (i - lastIndex);
            t[(size_t) k] = lastValue + (v - lastValue) * f;
        }
    }
    lastIndex = i;
    lastValue = v;
    setTable (ui.state(), tab, t, ui.um());
    repaint();
}

void Screen::mouseUp (const MouseEvent&)
{
    editStep = -1;
    lastIndex = -1;
    repaint();
}

//==============================================================================
Library::Library (Ui& u) : ui (u)
{
    setWantsKeyboardFocus (false);
    setMouseClickGrabsKeyboardFocus (false);
}

void Library::open (int m, bool s)
{
    modulator = m;
    seq = s;
    scroll = 0.0f;
    deleteMode = false;
    rebuild();
    setVisible (true);
}

void Library::rebuild()
{
    tiles.clear();
    if (seq)
        for (const auto& p : patterns) { Table t {}; setSteps (t, p.steps); tiles.push_back ({ p.name, t, {} }); }
    else
        for (int s = 0; s < shapeNames().size(); ++s) { Table t {}; setLfo (t, (Shape) s); tiles.push_back ({ shapeNames()[s], t, {} }); }
    for (const auto& f : userShapeDir().findChildFiles (File::findFiles, false, seq ? "*.steps" : "*.shape"))
    {
        Table t {};
        auto vals = StringArray::fromTokens (f.loadFileAsString(), " \n\r\t", "");
        vals.removeEmptyStrings();
        if (seq) for (int i = 0; i < jmin (maxSteps, vals.size()); ++i) t[(size_t) (seqFirst + i)] = (float) jlimit (-24, 24, vals[i].getIntValue()) / 24.0f;
        else for (int i = 0; i < jmin (lfoPoints, vals.size()); ++i) t[(size_t) i] = jlimit (-1.0f, 1.0f, vals[i].getFloatValue());
        tiles.push_back ({ f.getFileNameWithoutExtension(), t, f });
    }
    repaint();
}

Rectangle<float> Library::tileRect (int i) const
{
    constexpr int cols = 4;
    const float w = ((float) getWidth() - 4.0f) / cols, h = 46.0f;
    return { 2.0f + w * (float) (i % cols), 2.0f + h * (float) (i / cols) - scroll, w - 3.0f, h - 3.0f };
}

void Library::paint (Graphics& g)
{
    theme::drawWell (g, getLocalBounds().toFloat(), 3.0f);
    Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (getLocalBounds().reduced (1));
    for (int i = 0; i < (int) tiles.size(); ++i)
    {
        const auto r = tileRect (i);
        if (r.getBottom() < 0.0f || r.getY() > (float) getHeight()) continue;
        const bool user = tiles[(size_t) i].file != File();
        theme::drawCard (g, r, 3.0f);
        const auto col = user ? theme::amberLight : theme::cyan;
        const auto plotArea = r.reduced (3.0f).withTrimmedBottom (10.0f);
        Path p;
        const auto& t = tiles[(size_t) i].table;
        const int n = seq ? maxSteps : lfoPoints;
        for (int k = 0; k <= n; ++k)
        {
            const float v = seq ? t[(size_t) (seqFirst + jmin (k, n - 1))] : t[(size_t) (k % n)];
            const Point<float> q (plotArea.getX() + plotArea.getWidth() * (float) k / (float) n, plotArea.getCentreY() - v * plotArea.getHeight() * 0.5f);
            if (k == 0) p.startNewSubPath (q); else p.lineTo (q);
        }
        g.setColour (col);
        g.strokePath (p, PathStrokeType (1.2f));
        g.setColour (deleteMode && user ? theme::red : theme::textDim);
        g.setFont (theme::font (8.0f));
        g.drawText (tiles[(size_t) i].name, r.withTop (r.getBottom() - 11.0f), Justification::centred, true);
    }
}

void Library::mouseDown (const MouseEvent& e)
{
    for (int i = 0; i < (int) tiles.size(); ++i)
        if (tileRect (i).contains (e.position))
        {
            const auto& tile = tiles[(size_t) i];
            if (deleteMode)
            {
                if (tile.file != File()) { tile.file.deleteFile(); rebuild(); }
                return;
            }
            auto t = getTable (ui.state(), modulator);
            if (seq) for (int k = 0; k < maxSteps; ++k) t[(size_t) (seqFirst + k)] = tile.table[(size_t) (seqFirst + k)];
            else for (int k = 0; k < lfoPoints; ++k) t[(size_t) k] = tile.table[(size_t) k];
            ui.um()->beginNewTransaction();
            setTable (ui.state(), modulator, t, ui.um());
            setVisible (false);
            if (closed) closed();
            return;
        }
}

void Library::mouseWheelMove (const MouseEvent&, const MouseWheelDetails& w)
{
    const float rows = std::ceil ((float) tiles.size() / 4.0f), maxScroll = jmax (0.0f, rows * 46.0f + 4.0f - (float) getHeight());
    scroll = jlimit (0.0f, maxScroll, scroll - w.deltaY * 60.0f);
    repaint();
}

void Library::saveCurrent()
{
    const auto t = getTable (ui.state(), modulator);
    auto dir = userShapeDir();
    dir.createDirectory();
    String text;
    if (seq) for (int i = 0; i < maxSteps; ++i) text << stepValue (t, i) << ' ';
    else for (int i = 0; i < lfoPoints; ++i) text << String (t[(size_t) i], 4) << (i % 16 == 15 ? "\n" : " ");
    int n = 1;
    while (dir.getChildFile ("User " + String (n) + (seq ? ".steps" : ".shape")).exists()) ++n;
    dir.getChildFile ("User " + String (n) + (seq ? ".steps" : ".shape")).replaceWithText (text);
    rebuild();
}

//==============================================================================
Toggle::Toggle (Ui& u, String l, std::function<bool()> gt, std::function<void()> c, bool left)
    : ui (u), label (std::move (l)), get (std::move (gt)), click (std::move (c)), lampLeft (left)
{
    setWantsKeyboardFocus (false);
    setMouseClickGrabsKeyboardFocus (false);
}

void Toggle::paint (Graphics& g)
{
    const bool on = get ? get() : false;
    const auto r = getLocalBounds().toFloat();
    if (look == Look::Pair)                                         // LFO | SEQ: the chosen half lit
    {
        const auto halves = StringArray::fromTokens (label, "|", "");
        for (int i = 0; i < 2; ++i)
        {
            const auto h = r.withWidth (r.getWidth() * 0.5f).translated (r.getWidth() * 0.5f * (float) i, 0.0f).reduced (1.0f);
            const bool lit = (i == 1) == on;
            if (lit) { g.setColour (theme::raisedTop); g.fillRoundedRectangle (h, 3.0f); }
            g.setColour (lit ? theme::text : theme::textFaint);
            g.setFont (theme::font (9.0f, true));
            g.drawText (halves[i], h, Justification::centred);
        }
        if (ui.selected == this) { g.setColour (theme::amber); g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f); }
        return;
    }
    if (look == Look::Icon)
    {
        const auto b = r.reduced (4.0f);
        g.setColour (on ? theme::amber : theme::textDim);
        Path p;
        if (icon == "draw")            // a pencil
        {
            p.startNewSubPath (b.getX() + 2.0f, b.getBottom() - 1.0f);
            p.lineTo (b.getRight() - 2.0f, b.getY() + 3.0f);
            g.strokePath (p, PathStrokeType (3.0f, PathStrokeType::mitered, PathStrokeType::butt));
            g.fillEllipse (Rectangle<float> (3.0f, 3.0f).withCentre ({ b.getX() + 1.5f, b.getBottom() - 0.5f }));
        }
        else if (icon == "erase")      // an eraser: a tilted block
        {
            p.addRoundedRectangle (b.reduced (2.0f, 5.0f), 2.0f);
            p.applyTransform (AffineTransform::rotation (-0.6f, b.getCentreX(), b.getCentreY()));
            g.strokePath (p, PathStrokeType (1.5f));
        }
        else if (icon == "browse")     // a folder
        {
            p.startNewSubPath (b.getX(), b.getY() + 3.0f);
            p.lineTo (b.getX() + b.getWidth() * 0.4f, b.getY() + 3.0f);
            p.lineTo (b.getX() + b.getWidth() * 0.5f, b.getY() + 6.0f);
            p.lineTo (b.getRight(), b.getY() + 6.0f);
            p.lineTo (b.getRight(), b.getBottom() - 2.0f);
            p.lineTo (b.getX(), b.getBottom() - 2.0f);
            p.closeSubPath();
            g.strokePath (p, PathStrokeType (1.5f));
        }
        else if (icon == "save")       // a disk
        {
            g.drawRoundedRectangle (b.reduced (1.0f), 1.5f, 1.4f);
            g.fillRect (b.reduced (4.0f, 1.0f).withHeight (b.getHeight() * 0.35f));
        }
        else if (icon == "delete")     // a bin
        {
            g.drawRoundedRectangle (b.reduced (3.0f, 2.0f).withTrimmedTop (3.0f), 1.5f, 1.4f);
            g.fillRect (b.reduced (1.0f, 0.0f).withHeight (1.5f).translated (0.0f, 2.5f));
        }
        else if (icon == "sync")       // a note
        {
            g.fillEllipse (Rectangle<float> (6.0f, 5.0f).withCentre ({ b.getX() + 4.0f, b.getBottom() - 3.0f }));
            g.fillRect (b.getX() + 6.0f, b.getY() + 1.0f, 1.4f, b.getHeight() - 4.0f);
            g.fillRect (b.getX() + 6.0f, b.getY() + 1.0f, 5.0f, 1.6f);
        }
        if (ui.selected == this) { g.setColour (theme::amber); g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f); }
        return;
    }
    // a lamp with its label (Fine: below it; Flatten: the lamp on the left)
    const auto lamp = lampLeft ? Rectangle<float> (12.0f, 12.0f).withCentre ({ r.getX() + 9.0f, r.getCentreY() })
                               : Rectangle<float> (12.0f, 12.0f).withCentre ({ r.getCentreX(), r.getY() + 10.0f });
    theme::drawLamp (g, lamp, on, theme::amber);
    if (ui.selected == this) { g.setColour (theme::amber); g.drawEllipse (lamp.expanded (3.0f), 1.0f); }
    g.setColour (theme::textDim);
    g.setFont (theme::font (12.0f, true));
    if (! lampLeft) g.drawText (label, r.withTop (lamp.getBottom() + 4.0f).withHeight (15.0f), Justification::centred);
}

void Toggle::mouseDown (const MouseEvent& e)
{
    if (e.mods.isPopupMenu()) return;
    ui.select (this);
    if (click) click();
    repaint();
}
}
