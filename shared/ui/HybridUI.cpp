#include "shared/ui/HybridUI.h"
#include "shared/ui/Theme.h"
#include "shared/ui/Logo.h"
#include <cmath>

using namespace juce;
namespace th = fw::theme;

namespace fw::hybrid
{
Font font (float h, bool bold) { return th::font (h, bold); }

static float rad (float deg) { return deg * MathConstants<float>::pi / 180.0f; }
// a fixed array as a range, so two of different lengths can be chosen between
template <typename T> struct Items
{
    template <size_t N> Items (const T (&a)[N]) : b (a), e (a + N) {}
    const T* begin() const { return b; }
    const T* end() const { return e; }
    const T* b; const T* e;
};
// the measured angles: 0 = straight down, clockwise; JUCE: 0 = up, clockwise
static float refToJuce (float deg) { return rad (deg + 180.0f); }

//==============================================================================
void drawPanel (Graphics& g, Rectangle<int> area)
{
    g.setColour (th::bg);
    g.fillRect (area);
}

void drawScale (Graphics& g, Point<float> c, float a0, float a1, const Scale& s)
{
    g.setColour (s.colour.isTransparent() ? th::textFaint : s.colour);
    for (int i = 0; i < s.ticks; ++i)
    {
        const float a = s.ticks > 1 ? a0 + (a1 - a0) * (float) i / (float) (s.ticks - 1) : a0;
        const Point<float> d (std::sin (a), -std::cos (a));
        g.drawLine (Line<float> (c + d * s.rIn, c + d * s.rOut), s.thick);
    }
    g.setFont (font (s.labelSize));
    for (const auto& l : s.labels)
    {
        const float a = s.ticks > 1 ? a0 + (a1 - a0) * (float) l.tick / (float) (s.ticks - 1) : a0;
        const Point<float> p = c + Point<float> (std::sin (a), -std::cos (a)) * l.radius + Point<float> (l.dx, l.dy);
        g.drawText (l.text, Rectangle<float> (p.x - 20.0f, p.y - 7.0f, 40.0f, 14.0f), Justification::centred);
    }
}

void printText (Graphics& g, const String& t, Point<float> centre, float em, float hs, bool bold, float angle)
{
    const Font f = Font (FontOptions ("Segoe UI", 10.0f, bold ? Font::bold : Font::plain).withPointHeight (em)).withHorizontalScale (hs);
    GlyphArrangement ga;
    ga.addLineOfText (f, t, 0.0f, 0.0f);
    Path ink;
    ga.createPath (ink);
    const auto b = ink.getBounds();
    g.fillPath (ink, AffineTransform::translation (-b.getCentreX(), -b.getCentreY()).rotated (angle).translated (centre));
}

// a value in a raised box with an amber frame (a selected knob's value)
static void valueBox (Graphics& g, Rectangle<float> r, const String& text)
{
    g.setColour (th::raised);
    g.fillRect (r);
    g.setColour (th::amber);
    g.drawRect (r, 1.0f);
    g.setColour (th::text);
    g.setFont (font (11.0f, false));
    g.drawText (text, r, Justification::centred);
}

void drawScrew (Graphics& g, Point<float> c, float r, bool slot)
{
    const float rr = r + 1.8f;
    g.setColour (th::well);
    g.fillEllipse (c.x - rr, c.y - rr, rr * 2.0f, rr * 2.0f);
    g.setColour (th::line.withAlpha (0.7f));
    g.drawEllipse (c.x - rr + 0.5f, c.y - rr + 0.5f, rr * 2.0f - 1.0f, rr * 2.0f - 1.0f, 1.0f);
    g.setGradientFill (ColourGradient (th::raisedTop, c.x - r * 0.5f, c.y - r * 0.7f, th::raised, c.x + r * 0.5f, c.y + r * 0.8f, false));
    g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
    g.setColour (th::line);
    g.drawEllipse (c.x - r + 0.4f, c.y - r + 0.4f, r * 2.0f - 0.8f, r * 2.0f - 0.8f, 0.8f);
    g.setColour (th::well);
    const float w = jmax (1.2f, r * 0.18f);
    if (slot) g.drawLine (c.x - r * 0.75f, c.y + r * 0.27f, c.x + r * 0.75f, c.y - r * 0.27f, w);
    else
    {
        g.drawLine (c.x - r * 0.55f, c.y, c.x + r * 0.55f, c.y, w);
        g.drawLine (c.x, c.y - r * 0.55f, c.x, c.y + r * 0.55f, w);
    }
}

//==============================================================================
Knob::Knob (Kind k, float startDeg, float endDeg) : kind (k)
{
    setSliderStyle (RotaryVerticalDrag);
    setTextBoxStyle (TextBoxBelow, false, 37, 13);      // laid out on the value slot by HybridLook, invisible until typing
    setRotaryParameters (refToJuce (startDeg), refToJuce (endDeg), true);
    setMouseDragSensitivity (500);
    setVelocityModeParameters (1.0, 1, 0.0, false);
    setScrollWheelEnabled (true);
    // measured (MeterCapture drag scripts): 1000 px for the range on mouse steps up to 2 px, 500 px above; Ctrl: a
    // quarter of either
    feel.slowMaxStep = 2.0f;
    feel.ctrlFactor = 0.25f;
    feel.ctrlScalesSpeed = true;
}

float Knob::pointerAngle() const
{
    const auto rp = getRotaryParameters();
    float frac = (float) const_cast<Knob*> (this)->valueToProportionOfLength (getValue());
    if (steps > 1) frac = std::round (frac * (float) (steps - 1)) / (float) (steps - 1);
    return rp.startAngleRadians + frac * (rp.endAngleRadians - rp.startAngleRadians);
}

bool Knob::hitTest (int x, int y) { return centre.getDistanceFrom ({ (float) x, (float) y }) <= radius() + 5.0f; }

void Knob::mouseDown (const MouseEvent& e)
{
    if (selection != nullptr) selection->select (this);
    if (! feel.down (*this, e, {}, selection)) Slider::mouseDown (e);
}
void Knob::mouseDrag (const MouseEvent& e) { if (feel.active) feel.drag (*this, e, selection); else Slider::mouseDrag (e); }
void Knob::mouseUp (const MouseEvent& e)
{
    if (! feel.up (*this, e, centre, selection)) Slider::mouseUp (e);
}
void Knob::mouseDoubleClick (const MouseEvent&) { showTextBox(); }

// the wheel: the measured rule (shared/ui/Wheel.h), one step per notch on the stepped knobs
void Knob::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w) { wavesWheel (*this, e, w, wheelAcc, steps > 0); }

float Knob::paintExtent() const { return std::ceil (radius() + 4.0f); }

namespace
{
// each kind's parts: the body's radius, the value arc's (between the body and the printed scale), the fluted cap's
// (0: none), the pointer's width
struct KnobParts { float body, arc, cap, pointer; };
KnobParts partsOf (Knob::Kind k)
{
    switch (k)
    {
        // the fluted caps are the original's grooved caps' size (measured: 40, 53 and 48 px)
        case Knob::Large:   return { 48.0f, 52.0f, 40.0f, 3.0f };
        case Knob::Huge:    return { 65.0f, 69.5f, 53.0f, 3.5f };
        case Knob::Grand:   return { 57.0f, 61.0f, 48.0f, 3.0f };
        case Knob::Mini:    return { 18.0f, 21.3f, 0.0f, 2.2f };
        case Knob::Knurled: return { 28.0f, 31.3f, 0.0f, 2.5f };
        default:            return { 26.0f, 29.8f, 0.0f, 2.5f };   // Small
    }
}

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
}

void Knob::paint (Graphics& g)
{
    const auto c = centre;
    const auto P = partsOf (kind);
    const auto rp = getRotaryParameters();
    const float a = pointerAngle();
    const float turn = a - rp.startAngleRadians;            // the flutes and knurls turn with the value

    // the track and the value's arc (from the start, or from 0 on a knob that runs either side of it)
    float from = rp.startAngleRadians;
    if (getMinimum() < 0.0 && getMaximum() > 0.0)
        from = rp.startAngleRadians + (float) valueToProportionOfLength (0.0) * (rp.endAngleRadians - rp.startAngleRadians);
    const float aw = P.body > 40.0f ? 3.0f : 2.5f;
    {
        Path track;
        track.addCentredArc (c.x, c.y, P.arc, P.arc, 0.0f, rp.startAngleRadians, rp.endAngleRadians, true);
        g.setColour (th::line);
        g.strokePath (track, PathStrokeType (aw, PathStrokeType::curved, PathStrokeType::rounded));
        if (std::abs (a - from) > 1.0e-3f)
        {
            Path value;
            value.addCentredArc (c.x, c.y, P.arc, P.arc, 0.0f, jmin (from, a), jmax (from, a), true);
            g.setColour (th::cyan);
            g.strokePath (value, PathStrokeType (aw, PathStrokeType::curved, PathStrokeType::rounded));
        }
    }

    // the body: a soft shadow below, the raised disc lit from above, its rim
    const auto disc = Rectangle<float> (P.body * 2.0f, P.body * 2.0f).withCentre (c);
    for (int k = 3; k >= 1; --k)
    {
        g.setColour (Colours::black.withAlpha (0.14f));
        g.fillEllipse (disc.translated (0.0f, 0.8f * (float) k).expanded (0.4f * (float) k));
    }
    g.setGradientFill (ColourGradient (th::raisedTop, 0.0f, disc.getY(), th::raised.darker (0.25f), 0.0f, disc.getBottom(), false));
    g.fillEllipse (disc);
    g.setColour (th::line.brighter (0.15f));
    g.drawEllipse (disc.reduced (0.5f), 1.0f);

    if (P.cap > 0.0f)
    {
        // the skirt's fine turned rings, then the fluted cap (ten flutes) standing on it with its own shadow
        g.setColour (th::line.withAlpha (0.55f));
        for (float r = P.cap + 4.0f; r < P.body - 2.0f; r += 3.0f)
            g.drawEllipse (Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c), 0.6f);
        const auto cap = fluted (c, P.cap, P.cap * 0.07f, 10, turn);
        g.setColour (Colours::black.withAlpha (0.45f));
        g.fillPath (cap, AffineTransform::translation (0.0f, 2.5f));
        g.setGradientFill (ColourGradient (th::raisedTop.brighter (0.12f), 0.0f, c.y - P.cap, th::raised, 0.0f, c.y + P.cap, false));
        g.fillPath (cap);
        g.setColour (th::line.brighter (0.25f));
        g.strokePath (cap, PathStrokeType (1.0f));
        // the cap's lit top: a faint disc a little inside the flutes
        g.setColour (Colours::white.withAlpha (0.035f));
        g.fillEllipse (Rectangle<float> (P.cap * 1.5f, P.cap * 1.5f).withCentre (c.translated (0.0f, -P.cap * 0.08f)));
    }
    else if (kind == Knurled)
    {
        // the knurled rim: short grooves all round, turning with the value
        g.setColour (th::line.brighter (0.2f));
        for (int i = 0; i < 40; ++i)
        {
            const float t = turn + MathConstants<float>::twoPi * (float) i / 40.0f;
            const Point<float> d (std::sin (t), -std::cos (t));
            g.drawLine (Line<float> (c + d * (P.body - 4.0f), c + d * (P.body - 1.0f)), 1.0f);
        }
        g.setColour (th::line);
        g.drawEllipse (Rectangle<float> ((P.body - 4.5f) * 2.0f, (P.body - 4.5f) * 2.0f).withCentre (c), 0.8f);
    }

    // the pointer: an amber line from near the centre out to the rim
    {
        const Point<float> d (std::sin (a), -std::cos (a));
        const float r0 = P.cap > 0.0f ? P.cap * 0.35f : P.body * 0.25f, r1 = P.body - 3.0f;
        g.setColour (Colours::black.withAlpha (0.35f));
        const Point<float> drop (0.0f, 1.2f);
        g.drawLine (Line<float> (c + d * r0 + drop, c + d * r1 + drop), P.pointer + 1.0f);
        g.setColour (th::amber);
        g.drawLine (Line<float> (c + d * r0, c + d * r1), P.pointer);
    }

    // selected: a ring on the cap (amber, or cyan for a member of a group), and the value on the value slot
    if (selection != nullptr && selection->contains (this))
    {
        const float rr = ringRadius();
        g.setColour (selection->is (this) ? th::amber : th::cyan);
        g.drawEllipse (c.x - rr, c.y - rr, rr * 2.0f, rr * 2.0f, 1.6f);
        if (! valueArea.isEmpty()) valueBox (g, valueArea.toFloat(), getTextFromValue (getValue()));
    }
}

//==============================================================================
ButtonGroup::ButtonGroup (RangedAudioParameter& p, UndoManager* um, StringArray l, bool v, bool t, ButtonLook lk)
    : labels (std::move (l)), vertical (v), toggle (t), look (lk)
{
    attachment = std::make_unique<ParameterAttachment> (p, [this] (float x) { selected = roundToInt (x); repaint(); }, um);
    attachment->sendInitialUpdate();
}

// The frame sits kShadow px inside the component, which also holds the frame's soft shadow.
static constexpr float kShadow = 4.0f;

Rectangle<float> ButtonGroup::buttonRect (int i) const
{
    const int n = toggle ? 1 : labels.size();
    if (look == ButtonLook::delay)                      // a 2 px frame, 2 px dividers; the faces (whole pixels) share the rest
    {
        const auto in = frameRect().reduced (2.0f);
        const float w = (in.getWidth() - 2.0f * (float) (n - 1)) / (float) n;
        const float x0 = std::round (in.getX() + (float) i * (w + 2.0f)), x1 = std::round (in.getX() + (float) i * (w + 2.0f) + w);
        return { x0, in.getY(), x1 - x0, in.getHeight() };
    }
    // measured: a 1 px frame, the buttons 1 px apart
    const auto b = getLocalBounds().toFloat().reduced (kShadow).reduced (1.0f);
    if (vertical)
    {
        const float h = (b.getHeight() - (float) (n - 1)) / (float) n;
        return { b.getX(), b.getY() + (float) i * (h + 1.0f), b.getWidth(), h };
    }
    const float w = (b.getWidth() - (float) (n - 1)) / (float) n;
    return { b.getX() + (float) i * (w + 1.0f), b.getY(), w, b.getHeight() };
}

// the frame: a well with a soft shadow under it
static void drawButtonFrame (Graphics& g, Rectangle<float> f)
{
    for (int k = 3; k >= 1; --k)
    {
        g.setColour (Colours::black.withAlpha (0.12f));
        g.fillRoundedRectangle (f.translated (0.0f, 0.8f * (float) k).expanded (0.4f * (float) k), 3.0f + 0.4f * (float) k);
    }
    th::drawWell (g, f, 3.0f);
}

void drawButtonFace (Graphics& g, Rectangle<float> r, const String& label, bool on)
{
    // off: a raised face lit from above, its label in the label colour; on: pressed in (darker), the label and a line
    // along its foot in cyan
    if (on) g.setGradientFill (ColourGradient (th::well, 0.0f, r.getY(), th::raised, 0.0f, r.getBottom(), false));
    else g.setGradientFill (ColourGradient (th::raisedTop, 0.0f, r.getY(), th::raised, 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r, 2.0f);
    g.setColour (on ? th::cyan.withAlpha (0.45f) : th::line.brighter (0.2f));
    g.drawRoundedRectangle (r.reduced (0.5f), 2.0f, 1.0f);
    if (on)
    {
        g.setColour (th::cyan);
        g.fillRect (r.getX() + 3.0f, r.getBottom() - 2.5f, r.getWidth() - 6.0f, 1.5f);
    }
    g.setColour (on ? th::cyan : th::textDim);
    printText (g, label, r.getCentre().translated (0.0f, -0.5f), 9.75f, 1.0f, true);
}

Rectangle<float> ButtonGroup::frameRect() const
{
    return getLocalBounds().toFloat().withTrimmedLeft (kShadow).withTrimmedTop (kShadow).withTrimmedRight (kShadow)
                                     .withTrimmedBottom (look == ButtonLook::delay ? 9.0f : kShadow);
}

void ButtonGroup::paint (Graphics& g)
{
    const int n = toggle ? 1 : labels.size();
    const auto fr = look == ButtonLook::delay ? frameRect() : getLocalBounds().toFloat().reduced (kShadow);
    drawButtonFrame (g, fr);
    for (int i = 0; i < n; ++i)
        drawButtonFace (g, buttonRect (i), labels[toggle ? 0 : i], toggle ? selected != 0 : selected == i);
    if (outlined)
    {
        g.setColour (th::amber);
        g.drawRoundedRectangle (fr.reduced (0.5f), 3.0f, 1.0f);
    }
}

//==============================================================================
ToggleRow::ToggleRow (std::vector<Item> it, UndoManager* um, ButtonLook lk)
    : items (std::move (it)), on (items.size(), false), shown ((int) items.size()), look (lk)
{
    for (size_t i = 0; i < items.size(); ++i)
    {
        attachments.push_back (std::make_unique<ParameterAttachment> (*items[i].param, [this, i] (float x) { on[i] = x > 0.5f; repaint(); }, um));
        attachments.back()->sendInitialUpdate();
    }
}

// the buttons inside the frame, as the ButtonGroup's; each takes its width from the measured rectangles
Rectangle<float> ToggleRow::frameRect() const
{
    float w = look == ButtonLook::delay ? 2.0f : 1.0f;
    for (int i = 0; i < shown; ++i) w += items[(size_t) i].width + (look == ButtonLook::delay ? 2.0f : 0.0f);
    return getLocalBounds().toFloat().withTrimmedLeft (kShadow).withTrimmedTop (kShadow).withWidth (w)
                                     .withTrimmedBottom (look == ButtonLook::delay ? 9.0f : kShadow);
}

Rectangle<float> ToggleRow::buttonRect (int i) const
{
    if (look == ButtonLook::delay)                      // faces of the given widths between 2 px dividers in a 2 px frame
    {
        const auto in = frameRect().reduced (2.0f);
        float x = in.getX();
        for (int k = 0; k < i; ++k) x += items[(size_t) k].width + 2.0f;
        return { x, in.getY(), items[(size_t) i].width, in.getHeight() };
    }
    const auto b = getLocalBounds().toFloat().reduced (kShadow).reduced (1.0f);
    float x = b.getX();
    for (int k = 0; k < i; ++k) x += items[(size_t) k].width;
    return { x, b.getY(), items[(size_t) i].width - 1.0f, b.getHeight() };
}

void ToggleRow::paint (Graphics& g)
{
    drawButtonFrame (g, frameRect());
    for (int i = 0; i < shown; ++i) drawButtonFace (g, buttonRect (i), items[(size_t) i].label, on[(size_t) i]);
    if (outlinedItem >= 0 && outlinedItem < shown)
    {
        // a single button of the row: the line runs just around its face
        g.setColour (th::amber);
        g.drawRoundedRectangle (buttonRect (outlinedItem).expanded (1.0f).reduced (0.5f), 2.5f, 1.0f);
    }
}

void ToggleRow::mouseDown (const MouseEvent& e)
{
    for (int i = 0; i < shown; ++i)
        if (buttonRect (i).expanded (1.0f).contains (e.position))
        {
            if (onClick) onClick (this);
            outlinedItem = i;
            attachments[(size_t) i]->setValueAsCompleteGesture (on[(size_t) i] ? 0.0f : 1.0f);
            repaint();
        }
}

void ButtonGroup::mouseDown (const MouseEvent& e)
{
    if (onClick) onClick (this);
    const int n = toggle ? 1 : labels.size();
    for (int i = 0; i < n; ++i)
        if (buttonRect (i).contains (e.position))
        {
            if (toggle) attachment->setValueAsCompleteGesture (selected != 0 ? 0.0f : 1.0f);
            else attachment->setValueAsCompleteGesture ((float) i);
        }
}

//==============================================================================
// a rounded rectangle with one radius for the top corners and another for the bottom ones
static Path roundRect (Rectangle<float> r, float top, float bottom)
{
    Path p;
    const float x0 = r.getX(), y0 = r.getY(), x1 = r.getRight(), y1 = r.getBottom(), hp = MathConstants<float>::halfPi;
    p.startNewSubPath (x0 + top, y0);
    p.lineTo (x1 - top, y0);
    p.addCentredArc (x1 - top, y0 + top, top, top, 0.0f, 0.0f, hp);
    p.lineTo (x1, y1 - bottom);
    p.addCentredArc (x1 - bottom, y1 - bottom, bottom, bottom, 0.0f, hp, 2.0f * hp);
    p.lineTo (x0 + bottom, y1);
    p.addCentredArc (x0 + bottom, y1 - bottom, bottom, bottom, 0.0f, 2.0f * hp, 3.0f * hp);
    p.lineTo (x0, y0 + top);
    p.addCentredArc (x0 + top, y0 + top, top, top, 0.0f, 3.0f * hp, 4.0f * hp);
    p.closeSubPath();
    return p;
}

VuMeter::VuMeter() { setInterceptsMouseClicks (false, false); }

// The printed scale (measured): an arc of radius 99.8 about the pivot point from -57.7 to 60.1 degrees. The needle turns
// about a point 2.72, -0.04 px from that centre (measured); where it points is set by the meter (plugins/FwComp/Source/ui/Needle.h), not by
// this print.
static constexpr float kArcR = 99.8f, kNeedleDx = 2.72f, kNeedleDy = -0.04f;
static constexpr float kNeedleWidth = 2.0f;

// The face (component coordinates; the component covers the housing's window, 197 .. 448 x 73 .. 221, and the pivot's
// bump below it)
static const Rectangle<float> kFace (7.5f, 7.5f, 236.0f, 135.0f);
static constexpr float kFrameTop = 46.0f, kFrameBottom = 13.0f;      // the window's corner radii (the face: 7 px less)

void VuMeter::paint (Graphics& g)
{
    const auto c = pivot;
    const auto faceShape = roundRect (kFace, kFrameTop - 7.0f, kFrameBottom - 7.0f);
    g.saveState();
    g.reduceClipRegion (faceShape);
    // the dark face: a soft glow behind the scale, darker toward the edges
    {
        ColourGradient face (th::raised, c.x, c.y - 70.0f, th::well, c.x, kFace.getY() - 90.0f, true);
        face.addColour (0.55, th::panel);
        g.setGradientFill (face);
        g.fillPath (faceShape);
    }

    const float R = kArcR;
    auto at = [&] (float deg, float r) { return c + Point<float> (std::sin (rad (deg)), -std::cos (rad (deg))) * r; };
    const bool io = face == Face::io;
    // the band from 6 to 0 outside the arc (to radius 107), its dividers at the dots
    {
        Path band;
        band.addCentredArc (c.x, c.y, 107.0f, 107.0f, 0.0f, rad (0.2f), rad (60.0f), true);
        band.addCentredArc (c.x, c.y, R, R, 0.0f, rad (60.0f), rad (0.2f), false);
        band.closeSubPath();
        g.setColour (th::cyan.withAlpha (0.22f));
        g.fillPath (band);
        g.setColour (th::cyan.withAlpha (0.55f));
        g.strokePath (band, PathStrokeType (1.0f));
        static const float grDiv[] = { 7.7f, 15.6f, 24.6f, 35.6f, 46.5f }, ioDiv[] = { 14.55f, 29.45f, 44.3f, 51.25f };
        for (float a : io ? Items (ioDiv) : Items (grDiv))
            g.drawLine (Line<float> (at (a, R), at (a, 107.0f)), 1.0f);
    }
    // the arc, its ticks standing outward on it (minor 4 px, majors 8 px), the stop at the 48 end
    const Colour ink = th::text.withAlpha (0.85f);
    {
        Path arc;
        arc.addCentredArc (c.x, c.y, R, R, 0.0f, rad (-57.7f), rad (60.1f), true);
        g.setColour (ink);
        g.strokePath (arc, PathStrokeType (1.4f));
        g.drawLine (Line<float> (at (-57.7f, R - 0.7f), at (-57.7f, 107.9f)), 1.4f);
        // GR: majors at 24 and 12; In / Out: a major at 36, minors between the numbers
        static const float grTicks[] = { -51.8f, -46.4f, -40.5f, -34.5f, -28.9f, -21.6f, -14.7f, -7.9f },
                           ioTicks[] = { -48.9f, -39.2f, -29.8f, -20.0f, -10.3f };
        for (float a : io ? Items (ioTicks) : Items (grTicks))
        {
            const bool major = io ? juce::exactlyEqual (a, -29.8f) : (juce::exactlyEqual (a, -51.8f) || juce::exactlyEqual (a, -28.9f));
            g.drawLine (Line<float> (at (a, R), at (a, major ? 107.9f : 103.8f)), 1.0f);
        }
    }
    static const float grDots[] = { -39.3f, -19.6f, -10.0f, 7.8f, 15.7f, 34.7f, 46.9f }, ioDots[] = { -49.1f, -39.3f, -19.6f, -10.0f };
    g.setColour (th::textDim);
    for (float a : io ? Items (ioDots) : Items (grDots))
    {
        const auto p = at (a, 117.2f);
        g.drawEllipse (p.x - 2.3f, p.y - 2.3f, 4.6f, 4.6f, 1.0f);
    }
    // the numbers, each turned along the scale (measured centres)
    {
        g.setColour (th::text);
        struct Num { float a, r; const char* t; };
        static const Num grNums[] = { { -58.6f, 116.3f, "48" }, { -49.6f, 116.4f, "24" }, { -29.9f, 116.0f, "12" }, { 0.2f, 115.5f, "6" },
                                      { 24.9f, 116.8f, "3" }, { 59.0f, 117.1f, "0" } },
                         // In / Out: dBFS, 24 at the top
                         ioNums[] = { { -58.6f, 116.3f, "48" }, { -30.8f, 116.2f, "36" }, { 0.0f, 116.3f, "24" }, { 15.5f, 116.2f, "18" },
                                      { 30.0f, 116.4f, "12" }, { 43.4f, 115.5f, "6" }, { 51.7f, 116.5f, "3" }, { 59.0f, 117.1f, "0" } };
        for (auto n : io ? Items (ioNums) : Items (grNums))
            printText (g, n.t, at (n.a, n.r), 13.0f, 1.0f, true, rad (n.a));
    }
    // our mark printed on the face, and what the needle shows
    drawMark (g, Rectangle<float> (c.x - 18.4f, c.y - 69.1f, 38.0f, 29.0f), false, false, th::textFaint, 1.2f);
    g.setColour (th::textFaint);
    g.setFont (font (9.0f, true));
    g.drawText (io ? "dBFS" : "GR dB", Rectangle<float> (c.x - 30.0f, c.y - 40.0f, 60.0f, 12.0f), Justification::centred);

    // the needle and its shadow: 2 px, to 111.5 px from its pivot, a rivet at 104.7 px; the shadow straight down under it
    {
        const Point<float> np (c.x + kNeedleDx, c.y + kNeedleDy), dir (std::sin (rad (needle)), -std::cos (rad (needle)));
        const Point<float> sh (-0.05f, 5.02f);
        for (auto [w, alpha] : { std::pair { 5.14f, 0.05f }, std::pair { 3.19f, 0.16f }, std::pair { 1.53f, 0.3f } })
        {
            Path p;
            p.startNewSubPath (np + sh + dir * 22.0f);
            p.lineTo (np + sh + dir * 110.3f);
            g.setColour (Colours::black.withAlpha (alpha));
            g.strokePath (p, PathStrokeType (w, PathStrokeType::curved, PathStrokeType::rounded));
        }
        g.setColour (th::amber);
        g.drawLine (Line<float> (np + dir * 20.0f, np + dir * 111.5f), kNeedleWidth);
        const auto rv = np + dir * 104.7f;
        g.fillEllipse (rv.x - 2.85f, rv.y - 2.85f, 5.7f, 5.7f);
        g.setColour (th::well);
        g.fillEllipse (rv.x - 1.15f, rv.y - 1.15f, 2.3f, 2.3f);
    }
    // the glass: a faint sheen across the top
    g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.05f), 0.0f, kFace.getY(), Colours::white.withAlpha (0.0f), 0.0f, kFace.getY() + 45.0f, false));
    g.fillPath (faceShape);
    g.restoreState();

    // the housing's bump into the window over the pivot, with the slotted pivot screw
    const Point<float> bc (c.x + 0.4f, c.y - 0.9f);
    g.saveState();
    g.reduceClipRegion (Rectangle<float> (0.0f, 0.0f, (float) getWidth(), 147.0f).toNearestInt());     // the recess edge only in the window
    g.setColour (th::bg);
    g.fillEllipse (bc.x - 21.3f, bc.y - 21.3f, 42.6f, 42.6f);
    g.restoreState();
    g.setGradientFill (ColourGradient (th::raisedTop, 0.0f, bc.y - 18.3f, th::raised, 0.0f, bc.y + 18.3f, false));
    g.fillEllipse (bc.x - 18.3f, bc.y - 18.3f, 36.6f, 36.6f);
    g.setColour (th::line);
    g.drawEllipse (bc.x - 18.3f, bc.y - 18.3f, 36.6f, 36.6f, 1.0f);
    drawScrew (g, { c.x + 0.5f, c.y - 2.15f }, 7.5f, true);
}

void drawVuHousing (Graphics& g, Rectangle<float> h, Point<float> archCentre, float archRadius)
{
    // the housing's outline: a rounded rectangle whose bottom edge arches up around the Threshold knob, the arch
    // joining the edge in small round fillets (measured)
    const float R = 12.0f, F = 6.0f, x0 = h.getX(), y0 = h.getY(), x1 = h.getRight(), y1 = h.getBottom(), hp = MathConstants<float>::halfPi;
    const auto C = archCentre;
    const float fy = y1 - F, fdx = std::sqrt (jmax (0.0f, square (archRadius + F) - square (fy - C.y)));
    const Point<float> fR (C.x + fdx, fy), fL (C.x - fdx, fy);                  // the fillets' centres
    auto towards = [] (Point<float> from, Point<float> to) { return std::atan2 (to.x - from.x, -(to.y - from.y)); };
    Path p;
    p.startNewSubPath (x0 + R, y0);
    p.lineTo (x1 - R, y0);
    p.addCentredArc (x1 - R, y0 + R, R, R, 0.0f, 0.0f, hp);
    p.lineTo (x1, y1 - R);
    p.addCentredArc (x1 - R, y1 - R, R, R, 0.0f, hp, 2.0f * hp);
    p.lineTo (fR.x, y1);
    p.addCentredArc (fR.x, fR.y, F, F, 0.0f, 2.0f * hp, towards (fR, C) + MathConstants<float>::twoPi);
    p.addCentredArc (C.x, C.y, archRadius, archRadius, 0.0f, towards (C, fR), towards (C, fL));
    p.addCentredArc (fL.x, fL.y, F, F, 0.0f, towards (fL, C), 2.0f * hp);
    p.lineTo (x0 + R, y1);
    p.addCentredArc (x0 + R, y1 - R, R, R, 0.0f, 2.0f * hp, 3.0f * hp);
    p.lineTo (x0, y0 + R);
    p.addCentredArc (x0 + R, y0 + R, R, R, 0.0f, 3.0f * hp, 4.0f * hp);
    p.closeSubPath();

    // its shadow on the panel, below and to the right
    for (int k = 4; k >= 1; --k)
    {
        g.setColour (Colours::black.withAlpha (0.10f));
        g.fillPath (p, AffineTransform::translation (0.6f * (float) k, 0.9f * (float) k));
    }
    // a raised block lit from above, a lit line along its top edge, its rim
    g.setGradientFill (ColourGradient (th::raisedTop, 0.0f, y0, th::raised.darker (0.2f), 0.0f, y1, false));
    g.fillPath (p);
    {
        g.saveState();
        g.reduceClipRegion (p);
        g.setColour (Colours::white.withAlpha (0.06f));
        g.strokePath (p, PathStrokeType (3.0f), AffineTransform::translation (0.0f, 1.5f));
        // the arch's edge faces down toward the knob: an even dark lip all round it
        const float lw = 5.6f, rl = archRadius + lw * 0.5f;
        g.setColour (th::panel);
        g.drawEllipse (C.x - rl, C.y - rl, rl * 2.0f, rl * 2.0f, lw);
        g.restoreState();
    }
    g.setColour (th::line.brighter (0.15f));
    g.strokePath (p, PathStrokeType (1.2f));

    // the face's window: a dark recess, its lower lip catching the light
    const auto frame = roundRect ({ 197.5f, 73.5f, 250.0f, 147.0f }, kFrameTop, kFrameBottom);
    g.setColour (th::line.brighter (0.3f));
    g.strokePath (frame, PathStrokeType (1.0f), AffineTransform::translation (0.0f, 1.0f));
    g.setColour (th::bg);
    g.fillPath (frame);
    for (auto s : { Point<float> (198.4f, 75.95f), Point<float> (196.6f, 261.05f), Point<float> (449.2f, 260.95f) })
        drawScrew (g, s, 7.0f, false);
}

// one character of the display's segment font in the cell r (segments a..g as a 7-segment digit, i: the upper middle
// stroke of M), thickness t, leaning right by `slant` px per px of height
static void segChar (Graphics& g, Rectangle<float> r, juce_wchar ch, float t, float slant)
{
    enum { A = 1, B = 2, C = 4, D = 8, E = 16, F = 32, G = 64, I = 128, J = 256 };
    int m = 0;
    switch (CharacterFunctions::toUpperCase (ch))
    {
        case '0': case 'O': m = A | B | C | D | E | F; break;
        case '1': m = B | C; break;
        case '2': m = A | B | G | E | D; break;
        case '3': m = A | B | G | C | D; break;
        case '4': m = F | G | B | C; break;
        case '5': case 'S': m = A | F | G | C | D; break;
        case '6': m = A | F | G | E | C | D; break;
        case '7': m = A | B | C; break;
        case '8': case 'B': m = A | B | C | D | E | F | G; break;
        case '9': m = A | B | C | D | F | G; break;
        case 'P': m = A | B | F | G | E; break;
        case 'M': m = A | B | C | E | F | I; break;
        case 'F': m = A | F | G | E; break;
        case 'H': m = B | C | E | F | G; break;
        case 'T': m = A | I; break;
        case 'L': m = D | E | F; break;
        case 'R': m = A | B | E | F | G | J; break;           // J: the leg from the middle to the lower right
        case 'D': m = A | B | C | D | E | F; break;
        case '-': m = G; break;
        default: break;
    }
    const float w = r.getWidth(), h = r.getHeight(), x0 = r.getX(), y0 = r.getY(), hm = std::round (h * 0.5f - t * 0.5f), gap = t * 0.35f;
    auto rect = [&] (float x, float y, float rw, float rh)
    {
        // a segment, sheared by the slant
        Path p;
        p.addRectangle (x0 + x, y0 + y, rw, rh);
        g.fillPath (p, AffineTransform::shear (-slant, 0.0f).translated (slant * (y0 + h), 0.0f));
    };
    if (m & A) rect (gap, 0.0f, w - 2.0f * gap, t);
    if (m & D) rect (gap, h - t, w - 2.0f * gap, t);
    if (m & G) rect (gap, hm, w - 2.0f * gap, t);
    if (m & F) rect (0.0f, gap, t, hm - gap);
    if (m & E) rect (0.0f, hm + t, t, h - hm - t - gap);
    if (m & B) rect (w - t, gap, t, hm - gap);
    if (m & C) rect (w - t, hm + t, t, h - hm - t - gap);
    if (m & I) rect (std::round (w * 0.5f - t * 0.5f), gap, t, hm - gap);
    if (m & J)
    {
        Path p;
        p.startNewSubPath (x0 + w * 0.45f, y0 + hm + t);
        p.lineTo (x0 + w - t * 0.5f, y0 + h - gap);
        g.strokePath (p, PathStrokeType (t), AffineTransform::shear (-slant, 0.0f).translated (slant * (y0 + h), 0.0f));
    }
}

void drawSegmentText (Graphics& g, const SegFont& f, float x, float y, const String& text)
{
    const float w = f.w, h = f.h, t = f.t, hx = f.hx, vy = f.vy;
    Path p;
    auto rect = [&] (float cx, float x0, float y0, float x1, float y1)
    {
        auto at = [&] (float px, float py) { return Point<float> (cx + px + f.slant * (h - py), y + py); };
        p.startNewSubPath (at (x0, y0)); p.lineTo (at (x1, y0)); p.lineTo (at (x1, y1)); p.lineTo (at (x0, y1)); p.closeSubPath();
    };
    for (int i = 0; i < text.length(); ++i)
    {
        const float cx = x + f.pitch * (float) i;
        const char* segs = "";
        switch (text[i])
        {
            case '0': case 'D': segs = "ABCDEF"; break;
            case '1': segs = f.solidOne ? "S" : "BC"; break;
            case '2': segs = "ABGED"; break;
            case '3': segs = "ABGCD"; break;
            case '4': segs = "FGBC"; break;
            case '5': segs = "AFGCD"; break;
            case '6': segs = "AFGEDC"; break;
            case '7': segs = "ABC"; break;
            case '8': case 'B': segs = "ABCDEFG"; break;
            case '9': segs = "ABCDFG"; break;
            case 'A': segs = "ABCEFG"; break;
            case 'R': segs = "ABEFGK"; break;
            case 'T': segs = "AIJ"; break;
            case 'L': segs = "DEF"; break;
            case 'P': segs = "ABEFG"; break;
            case 'M': segs = "ABCEFI"; break;
            case '-': segs = "G"; break;
            default: break;
        }
        for (const char* c = segs; *c != 0; ++c)
            switch (*c)
            {
                case 'A': rect (cx, hx, 0.0f, w - hx, t); break;
                case 'G': rect (cx, hx, h * 0.5f - t * 0.5f, w - hx, h * 0.5f + t * 0.5f); break;
                case 'D': rect (cx, hx, h - t, w - hx, h); break;
                case 'F': rect (cx, 0.0f, vy, t, h * 0.5f - vy); break;
                case 'E': rect (cx, 0.0f, h * 0.5f + vy, t, h - vy); break;
                case 'B': rect (cx, w - t, vy, w, h * 0.5f - vy); break;
                case 'C': rect (cx, w - t, h * 0.5f + vy, w, h - vy); break;
                case 'S': rect (cx, w - t, vy * 0.5f, w, h - vy * 0.5f); break;        // a 1 drawn as one stroke
                case 'I': rect (cx, w * 0.5f - t * 0.5f, vy, w * 0.5f + t * 0.5f, h * 0.5f - vy); break;
                case 'J': rect (cx, w * 0.5f - t * 0.5f, h * 0.5f + vy, w * 0.5f + t * 0.5f, h - vy); break;
                case 'K':                                       // R's leg, from the middle to the lower right
                {
                    auto at = [&] (float px, float py) { return Point<float> (cx + px + f.slant * (h - py), y + py); };
                    const float x0 = w * 0.5f, y0 = h * 0.5f + t * 0.5f, x1 = w - t * 0.5f, y1 = h - vy;
                    p.startNewSubPath (at (x0 - t * 0.5f, y0)); p.lineTo (at (x0 + t * 0.5f, y0));
                    p.lineTo (at (x1 + t * 0.5f, y1)); p.lineTo (at (x1 - t * 0.5f, y1)); p.closeSubPath();
                    break;
                }
                default: break;
            }
    }
    p.setUsingNonZeroWinding (true);
    g.fillPath (p);
}

void drawSegText (Graphics& g, Point<float> origin, const String& text, float cellW, float cellH, float pitch, float t, float slant)
{
    for (int i = 0; i < text.length(); ++i)
        segChar (g, { origin.x + pitch * (float) i, origin.y, cellW, cellH }, text[i], t, slant);
}

void drawLedDisplay (Graphics& g, Rectangle<float> box, const String& text, const String& digits)
{
    // a dark window; 6 x 9 px characters at an 8 px pitch from 7 px in, 13 px from the top; the tempo in 12 x 20 px
    // digits at a 16 px pitch, right-aligned 8.5 px from the right edge, 7 px from the top (measured)
    for (int k = 3; k >= 1; --k)
    {
        g.setColour (Colours::black.withAlpha (0.12f));
        g.fillRoundedRectangle (box.translated (0.0f, 0.8f * (float) k).expanded (0.4f * (float) k), 5.0f);
    }
    th::drawWell (g, box, 5.0f);
    g.setColour (th::line.withAlpha (0.6f));
    g.drawRoundedRectangle (box.reduced (2.5f), 3.5f, 1.0f);
    g.setColour (th::cyan);
    for (int i = 0; i < text.length(); ++i)
        segChar (g, { box.getX() + 7.0f + 8.0f * (float) i, box.getY() + 13.0f, 6.0f, 9.0f }, text[i], 1.0f, 0.11f);
    for (int i = 0; i < digits.length(); ++i)
        segChar (g, { box.getRight() - 8.5f - 12.0f - 16.0f * (float) (digits.length() - 1 - i), box.getY() + 7.0f, 12.0f, 20.0f },
                 digits[i], 2.0f, 0.1f);
}

void drawNamePlate (Graphics& g, Rectangle<float> area, const String& name, bool stacked)
{
    const float h = area.getHeight();
    fw::drawMark (g, area.withWidth (h), false, false);
    const auto text = area.withTrimmedLeft (h + 6.0f);
    if (! stacked)
    {
        g.setColour (th::text);
        g.setFont (font (h * 0.56f, true));
        g.drawText (name, text, Justification::centredLeft);
        return;
    }
    g.setColour (th::textDim);
    g.setFont (font (h * 0.36f, false));
    g.drawText (name.upToFirstOccurrenceOf (" ", false, false), text.withHeight (h * 0.42f), Justification::bottomLeft);
    g.setColour (th::text);
    g.setFont (font (h * 0.56f, true));
    g.drawText (name.fromFirstOccurrenceOf (" ", false, false), text.withTrimmedTop (h * 0.40f), Justification::centredLeft);
}

//==============================================================================
Slider::SliderLayout HybridLook::getSliderLayout (Slider& s)
{
    if (auto* k = dynamic_cast<Knob*> (&s))
    {
        Slider::SliderLayout l;
        l.sliderBounds = s.getLocalBounds();
        l.textBoxBounds = k->valueArea.isEmpty() ? Rectangle<int> (0, 0, 1, 1) : k->valueArea;
        return l;
    }
    return FlatLook::getSliderLayout (s);
}

Label* HybridLook::createSliderTextBox (Slider& s)
{
    auto* l = FlatLook::createSliderTextBox (s);
    if (dynamic_cast<Knob*> (&s) != nullptr) l->setColour (Label::textColourId, Colours::transparentBlack);   // shown only while typing
    return l;
}
}
