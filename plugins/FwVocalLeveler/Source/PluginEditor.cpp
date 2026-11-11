#include "PluginEditor.h"
#include "shared/ui/Theme.h"
#include "shared/ui/Widgets.h"

using namespace juce;
using namespace vl::ui;
using flatwater::ren::arial;
namespace theme = flatwater::theme;

namespace
{
// the editor in Flatwater's own look (shared/ui/Theme.h): the layout keeps the reference's measured positions and sizes
const Colour kLabel (theme::textDim), kFrame (theme::line.brighter (0.3f)), kTick (theme::textFaint), kIdle (theme::amber),
    kMarkSelected (theme::amber), kMarkMember (theme::cyan);

// the knobs' pointer at the ends (measured: +-134.5 degrees from straight up); the disabled Music knob's pointer
constexpr float kKnobEnd = 134.5f, kKnobDisabledAngle = -136.5f;
// the knobs' centres in their controls (Vocal / Music 54 x 60, Spill 36 x 36) and their radii
const Point<float> kBigCentre (27.0f, 28.9f), kSmallCentre (17.1f, 16.8f);
constexpr float kBigRadius = 17.0f, kSmallRadius = 10.0f;

// a selected control's value: a raised box in an amber frame (as Flatwater Bass Leveler's)
void hoverBox (Graphics& g, Rectangle<float> r, const String& text)
{
    g.setColour (theme::raised);
    g.fillRect (r);
    g.setColour (theme::amber);
    g.drawRect (r, 1.0f);
    g.setColour (theme::text);
    g.setFont (theme::font (11.0f));
    g.drawText (text, r, Justification::centred);
}

void setMarks (Marks* m, Component* c, bool hover)
{
    if (m == nullptr) return;
    if (hover) m->hover = c; else if (m->hover == c) m->hover = nullptr;
    if (m->changed) m->changed();
}

// text centred on a point
void label (Graphics& g, const String& t, float cx, float cy, float w = 80.0f, float h = 14.0f)
{
    g.drawText (t, Rectangle<float> (w, h).withCentre ({ cx, cy }), Justification::centred);
}

// text from a left edge, centred vertically on y
void labelLeft (Graphics& g, const String& t, float x, float cy)
{
    g.drawText (t, Rectangle<float> (x, cy - 7.0f, 60.0f, 14.0f), Justification::centredLeft);
}

// a knob's scale (measured: a ring round a point a little below the knob's centre, ticks pointing out from it, and
// at each end a short flare outwards)
struct ScaleSpec { float dy, r, end; int n; float angles[6], lengths[6]; float flare; };
constexpr ScaleSpec kBigScale { 3.2f, 27.0f, 124.9f, 5, { 0.0f, 22.8f, 45.2f, 67.7f, 90.3f, 113.6f }, { 8.4f, 3.5f, 5.8f, 3.4f, 6.7f, 3.3f }, 6.0f };
constexpr ScaleSpec kSmallScale { 0.7f, 16.8f, 125.5f, 2, { 0.0f, 46.4f, 91.9f, 0.0f, 0.0f, 0.0f }, { 3.9f, 3.8f, 3.7f, 0.0f, 0.0f, 0.0f }, 5.0f };

void drawScale (Graphics& g, const ScaleSpec& sc, Point<float> c)
{
    const Point<float> rc (c.x, c.y + sc.dy);
    auto dir = [] (float deg) { const float a = degreesToRadians (deg); return Point<float> (std::sin (a), -std::cos (a)); };
    Path ring;
    ring.addCentredArc (rc.x, rc.y, sc.r, sc.r, 0.0f, degreesToRadians (-sc.end), degreesToRadians (sc.end), true);
    g.setColour (kTick);
    g.strokePath (ring, PathStrokeType (1.6f, PathStrokeType::curved, PathStrokeType::butt));
    for (int k = -sc.n; k <= sc.n; ++k)
    {
        const auto u = dir (k < 0 ? -sc.angles[-k] : sc.angles[k]);
        const auto from = rc + u * sc.r;
        g.drawLine (Line<float> (from, from + u * sc.lengths[std::abs (k)]), 1.8f);
    }
    for (float side : { -1.0f, 1.0f })
    {
        const auto e = rc + dir (side * sc.end) * sc.r;
        g.drawLine (Line<float> (e, e + dir (side * (sc.end + 15.0f)) * sc.flare), 1.6f);
    }
}

// a round LED in a dark bezel: lit in its colour with a soft glow (level 0 .. 1)
void drawLed (Graphics& g, Point<float> c, float radius, float level, Colour colour)
{
    const auto r = Rectangle<float> (2.0f * radius, 2.0f * radius).withCentre (c);
    g.setColour (theme::well);
    g.fillEllipse (r.expanded (1.5f));
    g.setColour (theme::line);
    g.drawEllipse (r.expanded (1.0f), 1.0f);
    g.setColour (theme::raised);
    g.fillEllipse (r);
    if (level > 0.0f)
    {
        g.setColour (colour.withAlpha (0.22f * level));
        g.fillEllipse (r.expanded (3.0f));
        g.setGradientFill (ColourGradient (colour.brighter (0.6f).withAlpha (level), c.x, c.y - radius * 0.4f,
                                           colour.withAlpha (level), c.x, c.y + radius, true));
        g.fillEllipse (r);
    }
    g.setColour (Colours::white.withAlpha (0.10f + 0.15f * level));
    g.fillEllipse (Rectangle<float> (radius * 0.9f, radius * 0.6f).withCentre ({ c.x - radius * 0.25f, c.y - radius * 0.45f }));
}

// the layouts (the reference's control rectangles: Studio, Live)
struct Layout { Rectangle<int> vocal, music, spill, attack, leds, musicLed, vocalValue, musicValue, spillValue; };
const Layout kStudio { { 83, 88, 54, 60 }, { 183, 88, 54, 60 }, {}, { 21, 124, 13, 27 }, { 19, 68, 17, 36 }, { 254, 67, 33, 40 },
                       { 91, 165, 37, 13 }, { 192, 165, 37, 13 }, {} };
const Layout kLive { { 133, 88, 54, 60 }, { 230, 88, 54, 60 }, { 71, 111, 36, 36 }, { 16, 136, 13, 27 }, { 14, 69, 17, 36 }, { 281, 65, 33, 40 },
                     { 159, 165, 37, 13 }, { 242, 165, 37, 13 }, { 70, 165, 37, 13 } };
}

//==============================================================================
void vl::ui::drawMark (Graphics& g, Rectangle<float> r, Mark m, bool round)
{
    if (m == Mark::None) return;
    g.setColour (m == Mark::Member ? kMarkMember : kMarkSelected);
    if (round) g.drawEllipse (r, 1.5f);
    else g.drawRoundedRectangle (r, 1.5f, 1.2f);
}

//==============================================================================
bool LinearDrag::down (Slider& s, const MouseEvent& e)
{
    if (e.mods.isPopupMenu()) return false;
    if (e.mods.isAltDown() && s.isDoubleClickReturnEnabled()) { s.setValue (s.getDoubleClickReturnValue(), sendNotificationSync); return true; }
    active = true;
    start = s.getValue();
    origin = last = virt = e.position;
    e.source.enableUnboundedMouseMovement (true, false);
    s.Slider::mouseDown (e);
    return true;
}

void LinearDrag::drag (Slider& s, const MouseEvent& e, float ppu, bool vert)
{
    if (! active) return;
    const auto d = e.position - last;
    last = e.position;
    float k = 1.0f;
    if (flatwater::ctrlHeldNow()) k = std::abs (vert ? d.y : d.x) <= 2.0f ? 0.037f : 0.19f;
    virt += d * k;
    vertical = vert;
    pxPerUnit = ppu;
    const float moved = vert ? origin.y - virt.y : virt.x - origin.x;
    s.setValue (jlimit (s.getMinimum(), s.getMaximum(), start + (double) (moved / ppu)), sendNotificationSync);
}

void LinearDrag::up (Slider& s, const MouseEvent& e)
{
    if (! active) return;
    active = false;
    s.Slider::mouseUp (e);
    e.source.enableUnboundedMouseMovement (false);
    const float moved = (float) (s.getValue() - start) * pxPerUnit;
    const auto at = vertical ? juce::Point<float> (origin.x, origin.y - moved) : juce::Point<float> (origin.x + moved, origin.y);
    juce::Desktop::setMousePosition (s.localPointToGlobal (at).roundToInt());
}

//==============================================================================
Knob::Knob (bool isSmall) : small (isSmall)
{
    setSliderStyle (RotaryVerticalDrag);
    setTextBoxStyle (NoTextBox, true, 0, 0);
    setRotaryParameters (degreesToRadians (360.0f - kKnobEnd), degreesToRadians (360.0f + kKnobEnd), true);
    setMouseDragSensitivity (500);
    setVelocityModeParameters (1.0, 1, 0.0, false);
}

void Knob::mouseEnter (const MouseEvent&) { setMarks (marks, this, true); }
void Knob::mouseExit (const MouseEvent&) { setMarks (marks, this, false); }
void Knob::mouseDown (const MouseEvent& e)
{
    if (disabledLook) { blocked = true; return; }               // (Music without a side-chain: no input)
    blocked = marks != nullptr && ! marks->click (this);
    if (blocked) return;
    if (! feel.down (*this, e, {}, nullptr)) Slider::mouseDown (e);
}
void Knob::mouseDrag (const MouseEvent& e) { if (blocked) return; if (feel.active) feel.drag (*this, e, nullptr); else Slider::mouseDrag (e); }
void Knob::mouseUp (const MouseEvent& e)
{
    if (blocked) { blocked = false; return; }
    if (! feel.up (*this, e, getLocalBounds().getCentre().toFloat(), nullptr)) Slider::mouseUp (e);
}

// the knob in Flatwater's look: Vocal and Music bipolar (the arc from 0 dB, straight up), Spill from its start; Music
// without a side-chain a dimmed disc with its pointer at rest and no arc
void Knob::paint (Graphics& g)
{
    const auto c = small ? kSmallCentre : kBigCentre;
    const float radius = small ? kSmallRadius : kBigRadius;
    const auto& rp = getRotaryParameters();
    if (disabledLook && ! small)
    {
        const auto disc = Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (c);
        Path track;
        track.addCentredArc (c.x, c.y, radius + 3.5f, radius + 3.5f, 0.0f, rp.startAngleRadians, rp.endAngleRadians, true);
        g.setColour (theme::line.withAlpha (0.6f));
        g.strokePath (track, PathStrokeType (2.5f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (theme::raised.withAlpha (0.7f));
        g.fillEllipse (disc);
        g.setColour (theme::line);
        g.drawEllipse (disc.reduced (0.5f), 1.0f);
        const float a = degreesToRadians (kKnobDisabledAngle);
        const Point<float> d (std::sin (a), -std::cos (a));
        g.setColour (theme::textFaint);
        g.drawLine (Line<float> (c + d * (radius * 0.25f), c + d * (radius - 2.5f)), 2.5f);
    }
    else
    {
        const float span = rp.endAngleRadians - rp.startAngleRadians;
        const float a = rp.startAngleRadians + (float) valueToProportionOfLength (getValue()) * span;
        const float from = small ? rp.startAngleRadians : rp.startAngleRadians + (float) valueToProportionOfLength (0.0) * span;
        theme::drawKnob (g, c, radius, a, rp.startAngleRadians, rp.endAngleRadians, from);
    }
    const float rr = radius + 4.5f;
    drawMark (g, Rectangle<float> (2.0f * rr, 2.0f * rr).withCentre (c), marks != nullptr ? marks->of (this) : Mark::None, true);
}

//==============================================================================
namespace
{
// the faders (measured): their movement areas, their caps' boxes (the cap's top moves from the area's top over the
// area's height less the box's, on whole pixels) and the caps' visible parts in those boxes
struct FaderGeom { int x, top, height, boxW, boxH; float lo, hi; Rectangle<float> core; };
const FaderGeom& geom (Fader::Kind k)
{
    static const FaderGeom g[] = { { 231, 219, 236, 47, 26, -12.0f, 12.0f, { 7.0f, 1.0f, 40.0f, 25.0f } },
                                   { 28, 204, 233, 55, 27, -11.0f, 12.0f, { 15.0f, 1.0f, 39.0f, 25.0f } },
                                   { 28, 234, 233, 55, 31, -12.0f, 11.0f, { 15.0f, 5.0f, 39.0f, 25.0f } },
                                   { 114, 197, 279, 72, 69, -12.0f, 12.0f, { 19.0f, 0.0f, 53.0f, 69.0f } } };
    return g[(int) k];
}
}

Fader::Fader (Kind k) : kind (k)
{
    setSliderStyle (LinearVertical);
    setTextBoxStyle (NoTextBox, true, 0, 0);
    setSliderSnapsToMousePosition (false);
    setVelocityModeParameters (1.0, 1, 0.0, false);
}

Rectangle<float> Fader::capBounds() const
{
    const auto& fg = geom (kind);
    const float travel = (float) (fg.height - fg.boxH);
    const float y = std::floor ((float) fg.top + travel * (fg.hi - (float) getValue()) / (fg.hi - fg.lo));
    return { (float) (fg.x - getX()), y - (float) getY(), (float) fg.boxW, (float) fg.boxH };
}

// the cap's visible part (and its selection frame)
Rectangle<float> Fader::capCore() const
{
    const auto b = capBounds();
    return geom (kind).core.translated (b.getX(), b.getY());
}

bool Fader::hitTest (int x, int y) { return capCore().expanded (2.0f).contains ((float) x, (float) y); }
void Fader::mouseEnter (const MouseEvent&) { setMarks (marks, this, true); }
void Fader::mouseExit (const MouseEvent&) { setMarks (marks, this, false); }

void Fader::mouseDown (const MouseEvent& e)
{
    blocked = marks != nullptr && ! marks->click (this);
    if (blocked) return;
    if (kind == Rider && onGrab) onGrab (true);
    feel.down (*this, e);
}
void Fader::mouseDrag (const MouseEvent& e)
{
    if (blocked) return;
    const auto& fg = geom (kind);
    feel.drag (*this, e, (float) (fg.height - fg.boxH) / (fg.hi - fg.lo), true);
}
void Fader::mouseUp (const MouseEvent& e)
{
    if (blocked) { blocked = false; return; }
    feel.up (*this, e);
    if (kind == Rider && onGrab) onGrab (false);
}

// the cap in Flatwater's look (the Rider's large, grooved); the selection frame round its visible part
void Fader::paint (Graphics& g)
{
    const auto core = capCore();
    theme::drawCap (g, core.reduced (kind == Rider ? 2.0f : 1.0f, kind == Rider ? 4.0f : 1.5f), kind == Rider ? 5 : 1);
    drawMark (g, core, marks != nullptr ? marks->of (this) : Mark::None);
}

//==============================================================================
TargetSlider::TargetSlider()
{
    setSliderStyle (LinearHorizontal);
    setTextBoxStyle (NoTextBox, true, 0, 0);
    setSliderSnapsToMousePosition (false);
    setVelocityModeParameters (1.0, 1, 0.0, false);
}

// the control is 208 px wide (59 .. 267); the glass 31 px: its left edge from 0 (-42 dB) to 177 (0 dB), on whole pixels
float TargetSlider::thumbX (double v) const { return std::floor ((float) ((v + 42.0) / 42.0 * 177.0)); }
bool TargetSlider::hitTest (int x, int y) { return Rectangle<float> (thumbX (getValue()), 0.0f, 31.0f, 20.0f).expanded (1.0f).contains ((float) x, (float) y); }
void TargetSlider::mouseEnter (const MouseEvent&) { setMarks (marks, this, true); }
void TargetSlider::mouseExit (const MouseEvent&) { setMarks (marks, this, false); }
void TargetSlider::mouseDown (const MouseEvent& e)
{
    blocked = marks != nullptr && ! marks->click (this);
    if (! blocked) feel.down (*this, e);
}
void TargetSlider::mouseDrag (const MouseEvent& e) { if (! blocked) feel.drag (*this, e, 177.0f / 42.0f, false); }
void TargetSlider::mouseUp (const MouseEvent& e) { if (! blocked) feel.up (*this, e); blocked = false; }

void TargetSlider::paint (Graphics& g)
{
    // the vocal level meter under the glass: 184 x 12 at 12, 5 (local); -42 .. 0 dBFS from left to right
    theme::drawMeter (g, { 12.0f, 5.0f, 184.0f, 12.0f }, (levelDb + 42.0f) / 42.0f, false);
    // the glass on whole pixels: clear, in an amber frame, the amber line down its centre
    const Rectangle<float> t (thumbX (getValue()), 0.0f, 31.0f, 20.0f);
    const auto glass = t.reduced (1.5f, 1.0f);
    g.setColour (theme::text.withAlpha (0.12f));
    g.fillRoundedRectangle (glass, 2.0f);
    g.setColour (theme::amber);
    g.drawRoundedRectangle (glass.reduced (0.5f), 2.0f, 1.0f);
    g.fillRect (std::round (glass.getCentreX()) - 0.5f, glass.getY() + 1.0f, 1.0f, glass.getHeight() - 2.0f);
    drawMark (g, Rectangle<float> (t.getX(), 0.0f, 31.0f, 19.0f), marks != nullptr ? marks->of (this) : Mark::None);
}

//==============================================================================
// the Idle arrow (measured: drawn 6 px left of its control, 6 px wide, 8 px tall)
void IdleColumn::paint (Graphics& g)
{
    const float y = std::round (scaleY (idleDb ? idleDb() : 0.0f) - 5.5f) - (float) getY();
    const float x = 27.0f - (float) getX();
    Path tri;
    tri.addTriangle (x + 6.0f, y + 1.4f, x + 6.0f, y + 9.6f, x + 11.8f, y + 5.5f);
    g.setColour (kIdle);
    g.fillPath (tri);
    const Mark m = marks != nullptr ? marks->of (this) : Mark::None;
    if (m != Mark::None)
    {
        g.setColour (m == Mark::Member ? kMarkMember : kMarkSelected);
        g.strokePath (tri, PathStrokeType (1.0f));
    }
}

void IdleColumn::mouseEnter (const MouseEvent&) { setMarks (marks, this, true); }
void IdleColumn::mouseExit (const MouseEvent&) { setMarks (marks, this, false); }

void IdleColumn::mouseDown (const MouseEvent& e)
{
    blocked = marks != nullptr && ! marks->click (this);
    if (blocked) return;
    if (e.mods.isAltDown()) { if (reset) reset(); return; }
    dragging = true;
    grab = e.position.y + (float) getY() - scaleY (idleDb());
    e.source.enableUnboundedMouseMovement (true, false);
    if (gesture) gesture (true);
}

void IdleColumn::mouseDrag (const MouseEvent& e)
{
    if (dragging && setIdleDb) setIdleDb (dbAtY (e.position.y + (float) getY() - grab));
    repaint();
}

void IdleColumn::mouseUp (const MouseEvent& e)
{
    blocked = false;
    if (! dragging) return;
    dragging = false;
    if (gesture) gesture (false);
    e.source.enableUnboundedMouseMovement (false);
    Desktop::setMousePosition (localPointToGlobal (Point<float> (e.mouseDownPosition.x, scaleY (idleDb()) - (float) getY() + grab)).roundToInt());
    if (marks != nullptr && marks->changed) marks->changed();
}

//==============================================================================
// the Attack switch (13 x 27): a well with the raised block up (Fast) or down (Slow), a cyan line on its outer edge
void Switch::paintButton (Graphics& g, bool, bool)
{
    const bool fast = getToggleState();
    theme::drawWell (g, getLocalBounds().toFloat(), 3.0f);
    const Rectangle<float> block (1.0f, fast ? 1.0f : 13.0f, 11.0f, 13.0f);
    g.setGradientFill (ColourGradient (theme::raisedTop, 0.0f, block.getY(), theme::raised, 0.0f, block.getBottom(), false));
    g.fillRoundedRectangle (block, 2.0f);
    g.setColour (theme::line);
    g.drawRoundedRectangle (block.reduced (0.5f), 2.0f, 1.0f);
    g.setColour (theme::cyan);
    g.fillRect (block.getX() + 3.0f, fast ? block.getY() + 2.0f : block.getBottom() - 3.0f, block.getWidth() - 6.0f, 1.0f);
    drawMark (g, getLocalBounds().toFloat(), marks != nullptr ? marks->of (this) : Mark::None);
}

//==============================================================================
// the Automation buttons (132 x 26, three 44 px buttons): raised, the chosen one pressed into a well in cyan
void AutomationButtons::paint (Graphics& g)
{
    const int chosen = mode ? mode() : 0;
    static const char* names[] = { "OFF", "WRITE", "READ" };
    for (int i = 0; i < 3; ++i)
    {
        const Rectangle<float> b ((float) i * 44.0f + 2.0f, 2.0f, 40.0f, 19.0f);
        if (i == chosen)
        {
            theme::drawWell (g, b.translated (0.0f, 1.0f), 3.0f);
            g.setColour (theme::cyan);
            g.drawRoundedRectangle (b.translated (0.0f, 1.0f).reduced (0.5f), 3.0f, 1.0f);
        }
        else
        {
            g.setColour (Colours::black.withAlpha (0.35f));
            g.fillRoundedRectangle (b.translated (0.0f, 2.0f), 3.0f);
            g.setGradientFill (ColourGradient (theme::raisedTop, 0.0f, b.getY(), theme::raised, 0.0f, b.getBottom(), false));
            g.fillRoundedRectangle (b, 3.0f);
            g.setColour (theme::line);
            g.drawRoundedRectangle (b.reduced (0.5f), 3.0f, 1.0f);
        }
        g.setColour (i == chosen ? theme::cyan : theme::textDim);
        g.setFont (arial (9.5f, true));
        g.drawText (names[i], b.translated (0.0f, i == chosen ? 1.0f : 0.0f), Justification::centred);
    }
    drawMark (g, { 1.0f, 1.0f, 130.0f, 22.0f }, marks != nullptr ? marks->of (this) : Mark::None);
}

void AutomationButtons::mouseDown (const MouseEvent& e)
{
    if (marks != nullptr && ! marks->click (this)) return;
    if (setMode) setMode (jlimit (0, 2, (int) (e.position.x / 44.0f)));
    repaint();
}

//==============================================================================
FwVocalLevelerEditor::FwVocalLevelerEditor (FwVocalLevelerProcessor& p)
    : Editor (p, p.apvts, p.undo, p.apvts.getParameter (vl::id::mode), "Flatwater Vocal Leveler", JucePlugin_VersionString, W, H,
              flatwater::Toolbar::Style::TwoRow, vl::modeNames()),
      lasso (make<flatwater::flat::LassoOverlay>()),
      proc (p),
      vocal (make<Knob> (false)), music (make<Knob> (false)), spill (make<Knob> (true)), attackSwitch (make<Switch>()),
      target (make<TargetSlider>()), rangeMax (make<Fader> (Fader::RangeMax)), rangeMin (make<Fader> (Fader::RangeMin)),
      output (make<Fader> (Fader::Output)), rider (make<Fader> (Fader::Rider)), idleColumn (make<IdleColumn>()),
      automation (make<AutomationButtons>())
{
    setFactoryPresets (vl::factoryPresets());
    marks.blueAlone = { &rider, &idleColumn };
    marks.changed = [this] { content().repaint(); };
    marks.enter = [this] (Slider* sl) { openEntry (sl); };
    for (auto* k : { &vocal, &music, &spill }) k->marks = &marks;
    for (auto* f : { &rangeMax, &rangeMin, &output, &rider }) f->marks = &marks;
    target.marks = &marks;
    attackSwitch.marks = &marks;

    attachSlider (vocal, vl::id::vocalSensitivity, kStudio.vocal, 0.0);
    attachSlider (music, vl::id::musicSensitivity, kStudio.music, 0.0);
    attachSlider (spill, vl::id::spill, kLive.spill, -48.0);
    attach (attackSwitch, vl::id::attack, kStudio.attack);
    attachSlider (target, vl::id::target, { 59, 42, 208, 20 }, -21.0);
    // the faders' components span the column; their caps move inside
    attachSlider (rangeMax, vl::id::rangeMax, { 13, 195, 82, 305 }, 6.0);
    attachSlider (rangeMin, vl::id::rangeMin, { 13, 195, 82, 305 }, -6.0);
    attachSlider (output, vl::id::output, { 225, 205, 72, 290 }, 0.0);
    // the Rider shows the ride (the processor's gain) and can be held by hand; it is the host's Rider Fader parameter
    // only through Automation (Write sends the ride, Read plays the host's back)
    rider.setRange (-12.0, 12.0, 0.1);
    addToContent (rider, { 110, 190, 100, 300 });
    rider.onGrab = [this] (bool held)
    {
        proc.userRideDb.store ((float) rider.getValue());
        proc.userRide.store (held);
        if (! held) content().repaint();
    };
    rider.onValueChange = [this] { if (proc.userRide.load()) proc.userRideDb.store ((float) rider.getValue()); content().repaint (135, 470, 50, 26); };
    // Range Max stays at least 1 dB above Min (the Effective Range is 1 .. 24 dB)
    rangeMax.onValueChange = [this] { if (rangeMin.getValue() > rangeMax.getValue() - 1.0) rangeMin.setValue (rangeMax.getValue() - 1.0, sendNotificationSync); content().repaint(); };
    rangeMin.onValueChange = [this] { if (rangeMax.getValue() < rangeMin.getValue() + 1.0) rangeMax.setValue (rangeMin.getValue() + 1.0, sendNotificationSync); content().repaint(); };
    output.onValueChange = [this] { content().repaint (230, 470, 60, 26); };
    for (auto* k : { &vocal, &music, &spill })
        k->onValueChange = [this, k] { content().repaint (k->getX() - 10, k->getY() + 60, k->getWidth() + 20, 30); };
    target.onValueChange = [this] { content().repaint (266, 40, 50, 26); };
    // the Idle arrow
    auto* idleParam = p.apvts.getParameter (vl::id::idle);
    idleColumn.marks = &marks;
    idleColumn.idleDb = [this] { return proc.idleDb(); };
    idleColumn.setIdleDb = [this, idleParam] (float db)
    {
        const float lo = proc.get (vl::id::rangeMin), hi = proc.get (vl::id::rangeMax);
        idleParam->setValueNotifyingHost (idleParam->convertTo0to1 (jlimit (0.02f, 0.98f, (jlimit (lo, hi, db) - lo) / jmax (1.0f, hi - lo))));
        content().repaint();
    };
    idleColumn.gesture = [idleParam] (bool begin) { if (begin) idleParam->beginChangeGesture(); else idleParam->endChangeGesture(); };
    idleColumn.reset = [this, idleParam] { idleParam->setValueNotifyingHost (idleParam->getDefaultValue()); content().repaint(); };
    addToContent (idleColumn, { 27, 224, 18, 224 });
    // the Automation buttons
    auto* autoParam = p.apvts.getParameter (vl::id::automation);
    automation.marks = &marks;
    automation.mode = [this] { return proc.automationMode(); };
    automation.setMode = [autoParam] (int m) { autoParam->beginChangeGesture(); autoParam->setValueNotifyingHost (autoParam->convertTo0to1 ((float) m)); autoParam->endChangeGesture(); };
    addToContent (automation, { 94, 522, 132, 26 });
    addToContent (lasso, { 0, 0, W, H });
    lasso.setVisible (false);
    layoutControls();
}

FwVocalLevelerEditor::~FwVocalLevelerEditor()
{
    entry.reset();
    for (auto* k : { &vocal, &music, &spill }) k->marks = nullptr;
}

// Studio / Live: the knobs, the switch and the LEDs move; Spill shows on Live only
void FwVocalLevelerEditor::layoutControls()
{
    const bool live = vl::isLive (proc.modeIndex());
    if ((int) live == laidOut) return;
    laidOut = (int) live;
    const auto& l = live ? kLive : kStudio;
    vocal.setBounds (l.vocal);
    music.setBounds (l.music);
    attackSwitch.setBounds (l.attack);
    spill.setVisible (live);
    content().repaint();
}

Rectangle<int> FwVocalLevelerEditor::entryArea (Slider* sl) const
{
    const auto& l = vl::isLive (proc.modeIndex()) ? kLive : kStudio;
    if (sl == &vocal) return l.vocalValue;
    if (sl == &music) return l.musicValue;
    if (sl == &spill) return l.spillValue;
    if (sl == &target) return { 272, 45, 32, 13 };
    if (sl == &output) return { 239, 476, 36, 15 };
    if (sl == &rangeMax || sl == &rangeMin)
    {
        auto* f = static_cast<Fader*> (sl);
        const auto cap = f->capCore().translated ((float) f->getX(), (float) f->getY());
        return { 88, roundToInt (cap.getCentreY()) - 7, 31, 14 };
    }
    return {};
}

void FwVocalLevelerEditor::openEntry (Slider* sl)
{
    const auto area = entryArea (sl);
    if (area.isEmpty()) return;
    closeEntry (false);
    entryFor = sl;
    entry = std::make_unique<TextEditor>();
    // (the look's own text-editor colours would put the selected text in a pale highlight: black on white, the
    // selection a clear blue with white text)
    entry->setFont (arial (12.0f, true));
    entry->setJustification (Justification::centredLeft);
    entry->setIndents (2, 0);
    entry->setColour (TextEditor::backgroundColourId, Colours::white);
    entry->setColour (TextEditor::textColourId, Colours::black);
    entry->setColour (TextEditor::highlightColourId, theme::cyan.withAlpha (0.35f));
    entry->setColour (TextEditor::highlightedTextColourId, Colours::white);
    entry->setColour (CaretComponent::caretColourId, Colours::black);
    entry->setColour (TextEditor::outlineColourId, Colours::black);
    entry->setColour (TextEditor::focusedOutlineColourId, Colours::black);
    entry->setText (sl->getTextFromValue (sl->getValue()).upToFirstOccurrenceOf (" ", false, false), false);
    entry->onReturnKey = [this] { closeEntry (true); };
    entry->onEscapeKey = [this] { closeEntry (false); };
    entry->onFocusLost = [this] { closeEntry (false); };
    content().addAndMakeVisible (*entry);
    entry->setBounds (area);
    entry->selectAll();
    entry->grabKeyboardFocus();                                    // only while typing: the host keeps it otherwise
}

void FwVocalLevelerEditor::closeEntry (bool apply)
{
    if (entry == nullptr) return;
    auto e = std::move (entry);
    auto* sl = entryFor;
    entryFor = nullptr;
    if (apply && sl != nullptr)
    {
        const auto t = e->getText().trim().upToFirstOccurrenceOf (" ", false, false);
        if (t.containsAnyOf ("0123456789"))
        {
            double v = t.getDoubleValue();
            if (sl == &spill) v = -std::abs (v);
            if (v >= sl->getMinimum() - 1.0e-9 && v <= sl->getMaximum() + 1.0e-9)
            {
                Slider::ScopedDragNotification drag (*sl);
                sl->setValue (v, sendNotificationSync);
            }
        }
    }
    e->setVisible (false);
    MessageManager::callAsync ([owned = std::shared_ptr<TextEditor> (std::move (e))] {});
    content().repaint();
}

void FwVocalLevelerEditor::contentMouseDown (const MouseEvent& e)
{
    if (Rectangle<float> (280.0f, 192.0f, 15.0f, 15.0f).contains (e.position))   // the clip LED: a click clears it
    {
        clip = false;
        content().repaint (278, 190, 20, 20);
        return;
    }
    lassoing = true;
    lassoStart = e.position;
    marks.group.clear();
    marks.selected = nullptr;
    if (marks.changed) marks.changed();
}

void FwVocalLevelerEditor::contentMouseDrag (const MouseEvent& e)
{
    if (! lassoing) return;
    const auto r = Rectangle<float> (lassoStart, e.position);
    lasso.rect = r.toNearestInt();
    lasso.setVisible (true);
    lasso.repaint();
    auto at = [] (Component& c, Rectangle<float> local) { return local.translated ((float) c.getX(), (float) c.getY()); };
    std::vector<std::pair<Component*, Rectangle<float>>> items = {
        { &target, at (target, { target.thumbX (target.getValue()), 0.0f, 31.0f, 20.0f }) }, { &attackSwitch, attackSwitch.getBounds().toFloat() },
        { &vocal, vocal.getBounds().toFloat() }, { &music, music.getBounds().toFloat() },
        { &rangeMax, at (rangeMax, rangeMax.capCore()) }, { &idleColumn, Rectangle<float> (32.0f, std::round (scaleY (proc.idleDb()) - 5.5f), 7.0f, 11.0f) },
        { &rangeMin, at (rangeMin, rangeMin.capCore()) }, { &rider, at (rider, rider.capCore()) }, { &output, at (output, output.capCore()) } };
    if (spill.isVisible()) items.insert (items.begin() + 2, { &spill, spill.getBounds().toFloat() });
    marks.group.clear();
    for (auto& [c, b] : items)
        if (b.intersects (r)) marks.group.push_back (c);
    marks.selected = marks.group.empty() ? nullptr : marks.group.front();
    if (marks.changed) marks.changed();
}

void FwVocalLevelerEditor::contentMouseUp (const MouseEvent&)
{
    lassoing = false;
    lasso.setVisible (false);
    if (marks.group.size() < 2) marks.group.clear();
    if (marks.changed) marks.changed();
}

void FwVocalLevelerEditor::tick()
{
    layoutControls();
    const double now = Time::getMillisecondCounterHiRes() * 0.001, dt = lastTick > 0.0 ? now - lastTick : 0.0;
    lastTick = now;
    // the vocal level meter (provisional ballistics, as Flatwater Bass Leveler's Target meter: up at once, down 37.3 dB/s at most)
    const float e = proc.vocalDb.take();
    level = e >= level ? e : jmax (e, level - 37.3f * (float) jmin (dt, 0.25));
    target.levelDb = level;
    target.repaint();
    if (proc.outDb.take() >= 0.0f && ! clip) { clip = true; content().repaint (278, 190, 20, 20); }
    const bool sc = proc.sideChainOn.load();
    if (sc != sideChain) { sideChain = sc; music.disabledLook = ! sc; music.repaint(); }
    music.disabledLook = ! sideChain;
    // the Music LED: brighter as the music is louder (provisional: -40 .. -10 dBFS)
    const float m = sideChain ? jlimit (0.0f, 1.0f, (proc.musicDb.take() + 40.0f) / 30.0f) : 0.0f;
    if (std::abs (m - music01) > 0.02f) { music01 = m; content().repaint(); }
    const bool rd = proc.riding.load();
    if (rd != riding) { riding = rd; content().repaint(); }
    const float r = proc.rideDb.load();
    if (! rider.isMouseButtonDown() && std::abs (r - ride) > 1.0e-4f)
    {
        ride = r;
        rider.setValue (r, dontSendNotification);
    }
    content().repaint (100, 190, 110, 310);
    idleColumn.repaint();
}

//==============================================================================
// the static art, in design coordinates
void FwVocalLevelerEditor::paintStatic (Graphics& g, bool live) const
{
    const auto& lay = live ? kLive : kStudio;
    auto knobCentre = [] (Rectangle<int> r, Point<float> c) { return Point<float> ((float) r.getX() + c.x, (float) r.getY() + c.y); };
    const auto vc = knobCentre (lay.vocal, kBigCentre), mc = knobCentre (lay.music, kBigCentre);

    // ---- the name plate: a dark strip with fine scratches (our own seeded pattern), the Flatwater mark and the
    // plug-in's name
    {
        g.setColour (theme::panel);
        g.fillRect (0.0f, 551.0f, 320.0f, 47.0f);
        g.setColour (theme::line);
        g.fillRect (0.0f, 551.0f, 320.0f, 1.0f);
        Random rnd (20261101);
        for (int i = 0; i < 220; ++i)
        {
            const float x = rnd.nextFloat() * 320.0f, y = 553.0f + rnd.nextFloat() * 44.0f;
            const float ang = degreesToRadians ((rnd.nextBool() ? 62.0f : 118.0f) + (rnd.nextFloat() - 0.5f) * 30.0f);
            const float len = 3.0f + rnd.nextFloat() * 16.0f;
            g.setColour (theme::textFaint.withAlpha (0.05f + 0.08f * rnd.nextFloat()));
            g.drawLine (x, y, x + len * std::cos (ang), jlimit (553.0f, 597.0f, y + len * std::sin (ang)), 0.6f);
        }
    }
    flatwater::drawMark (g, { 14.0f, 559.5f, 30.0f, 30.0f }, false, false);
    g.setColour (theme::text);
    g.setFont (theme::font (17.0f, true));
    g.drawText ("Flatwater Vocal Leveler", Rectangle<float> (50.0f, 557.5f, 220.0f, 34.0f), Justification::centredLeft);
    if (live)
    {
        // the Live components' badge at its measured place
        const Rectangle<float> b (270.0f, 565.0f, 43.0f, 19.0f);
        theme::drawWell (g, b, 3.0f);
        g.setColour (theme::amber);
        g.drawRoundedRectangle (b.reduced (0.5f), 3.0f, 1.0f);
        g.setFont (arial (12.0f, true));
        g.drawText ("LIVE", b, Justification::centred);
    }

    // ---- top panel: the line over the Target meter, the Sensitivity bracket (measured: y 82 in Studio, 81 on Live, its
    // ends turned down 3 px; broken by the knobs' zeros and the heading)
    g.setColour (theme::line);
    g.fillRect (59.0f, 43.0f, 209.0f, 1.0f);
    {
        const float y = live ? 81.5f : 82.5f;
        // each piece: from, to, the end turned down
        struct Piece { float x0, x1, down; };
        const Piece studio[] = { { 67.0f, 96.5f, 96.5f }, { 125.5f, 133.0f, 125.5f }, { 189.0f, 196.5f, 196.5f }, { 224.5f, 254.0f, 224.5f } };
        const Piece livePieces[] = { { 66.0f, 147.5f, 147.5f }, { 173.5f, 181.0f, 173.5f }, { 237.0f, 244.5f, 244.5f }, { 270.5f, 278.0f, 270.5f } };
        Path bracket;
        for (int i = 0; i < 4; ++i)
        {
            const auto& p = (live ? livePieces : studio)[i];
            const float other = juce::exactlyEqual (p.down, p.x0) ? p.x1 : p.x0;
            bracket.startNewSubPath (other, y);
            bracket.lineTo (p.down, y);
            bracket.lineTo (p.down, y + 3.5f);
        }
        g.setColour (kFrame);
        g.strokePath (bracket, PathStrokeType (1.2f));
    }

    // the labels (at their measured places)
    const float lx = (float) lay.leds.getX() + 20.0f;
    g.setColour (kLabel);
    g.setFont (arial (12.0f, true));
    label (g, "Target", 35.0f, 53.0f);
    g.setFont (arial (11.0f, true));
    labelLeft (g, "Ride", lx, 78.5f);
    labelLeft (g, "Idle", lx, 94.5f);
    const float sy = (float) lay.attack.getY();
    labelLeft (g, "Fast", lx, sy + 6.5f);
    labelLeft (g, "Slow", lx, sy + 20.5f);
    label (g, "Vocal", vc.x - 1.0f, 157.0f);
    label (g, "Music", mc.x + 1.0f, 157.0f);
    g.setColour (theme::amber);
    g.setFont (arial (12.5f, true));
    label (g, "Sensitivity", (vc.x + mc.x) * 0.5f + (live ? 0.5f : 1.0f), 82.0f - (live ? 1.0f : 0.0f), 70.0f);
    g.setColour (theme::textFaint);
    g.setFont (arial (11.0f, true));
    for (auto c : { vc, mc })
    {
        label (g, "0", c.x, 80.5f, 12.0f);
        label (g, "-", c.x - 30.0f, 144.5f, 12.0f);
        label (g, "+", c.x + 32.0f, 143.5f, 12.0f);
    }
    drawScale (g, kBigScale, vc);
    drawScale (g, kBigScale, mc);
    if (live)
    {
        const auto sc = knobCentre (lay.spill, kSmallCentre);
        drawScale (g, kSmallScale, sc);
        g.setColour (theme::textFaint);
        label (g, "-", sc.x - 20.5f, 144.5f, 12.0f);
        label (g, "+", sc.x + 21.5f, 143.5f, 12.0f);
        g.setColour (kLabel);
        label (g, "Spill", sc.x - 0.5f, 158.0f);
    }

    // ---- lower panel: labels, screws, the Automation bracket, the wells, tracks and scales, the value boxes' wells
    g.setColour (kLabel);
    g.setFont (arial (12.0f, true));
    label (g, "Range", 63.0f, 200.0f);
    label (g, "Rider", 161.0f, 199.0f);
    label (g, "Output", 260.0f, 200.0f);
    g.setColour (theme::amber);
    g.setFont (arial (11.0f, true));
    label (g, "Automation", 159.5f, 515.0f);
    {
        Path bracket;
        bracket.startNewSubPath (127.0f, 516.5f);
        bracket.lineTo (98.5f, 516.5f);
        bracket.lineTo (98.5f, 520.0f);
        bracket.startNewSubPath (194.0f, 516.5f);
        bracket.lineTo (222.0f, 516.5f);
        bracket.lineTo (222.0f, 520.0f);
        g.setColour (kFrame);
        g.strokePath (bracket, PathStrokeType (1.2f));
    }
    for (float y : { 183.5f, 502.5f })
    {
        // a screw at its measured place: a dark ring on a raised face, a small cross
        const Point<float> c (160.5f, y);
        g.setColour (theme::raised);
        g.fillEllipse (Rectangle<float> (7.0f, 7.0f).withCentre (c));
        g.setColour (theme::well);
        g.drawEllipse (Rectangle<float> (8.4f, 8.4f).withCentre (c), 1.3f);
        g.setColour (theme::line.brighter (0.4f));
        g.fillRect (c.x - 1.5f, c.y - 0.5f, 3.0f, 1.0f);
        g.fillRect (c.x - 0.5f, c.y - 1.5f, 1.0f, 3.0f);
    }
    for (float x : { 51.0f, 149.5f, 247.0f })                       // the wells behind the tracks (measured: 21.5 px wide)
    {
        g.setColour (theme::well.withAlpha (0.45f));
        g.fillRoundedRectangle (x, 210.0f, 21.5f, 253.0f, 5.0f);
    }
    theme::drawWell (g, { 58.7f, 218.0f, 5.0f, 236.5f }, 2.5f);         // Range
    theme::drawWell (g, { 156.5f, 218.0f, 6.0f, 238.0f }, 3.0f);        // Rider
    theme::drawWell (g, { 254.7f, 223.0f, 5.0f, 227.0f }, 2.5f);        // Output
    // the scales: a mark every 3 dB (2 px at +-12 and 0, else 1 px), five fainter ticks between marks; the Rider's on
    // both sides (19 px marks, 8 px ticks), the Output's on its right (11 / 6 px marks, 4 px ticks); numbers every
    // 6 dB beside the Rider, at +-12 and 0 beside the Output
    g.setFont (arial (10.0f));
    for (int k = 0; k <= 8; ++k)
    {
        const float yk = 231.0f + 26.25f * (float) k;
        const bool thick = k % 4 == 0;
        const float top = thick ? yk - 1.0f : std::floor (yk), h = thick ? 2.0f : 1.0f;
        g.setColour (kTick);
        g.fillRect (129.0f, top, 19.0f, h);
        g.fillRect (172.0f, top, 19.0f, h);
        g.fillRect (270.0f, top, thick ? 11.0f : 6.0f, h);
        if (k % 2 == 0)
        {
            const int db = 12 - 3 * k;
            const String t = db > 0 ? "+" + String (db) : String (db);
            g.setColour (kLabel);
            label (g, t, 118.5f, yk + 0.5f, 30.0f);
            label (g, t, 202.5f, yk + 0.5f, 30.0f);
            if (thick) labelLeft (g, t, 284.0f, yk + 0.5f);
        }
        if (k == 8) break;
        g.setColour (kTick.withAlpha (0.5f));
        for (int j = 0; j < 5; ++j)
        {
            const float ty = (thick ? yk + 5.5f : top + 5.0f) + 4.0f * (float) j;
            g.fillRect (140.0f, ty, 8.0f, 1.0f);
            g.fillRect (172.0f, ty, 8.0f, 1.0f);
            g.fillRect (270.0f, ty, 4.0f, 1.0f);
        }
    }
}

// the panels in Flatwater's look: the background, a card for the top controls, one for the faders (the name plate is
// drawn with the static art)
static void paintPanels (Graphics& g)
{
    g.fillAll (theme::bg);
    theme::drawCard (g, { 6.0f, 37.0f, 308.0f, 132.0f });
    theme::drawCard (g, { 4.0f, 172.0f, 312.0f, 377.0f });
}

void FwVocalLevelerEditor::paintContent (Graphics& g)
{
    const bool live = vl::isLive (proc.modeIndex());
    // the static art, rendered once per physical scale and layout
    const float s = jlimit (0.25f, 8.0f, std::round (g.getInternalContext().getPhysicalPixelScaleFactor() * 100.0f) / 100.0f);
    if (background.isNull() || std::abs (s - backgroundScale) > 1.0e-3f || backgroundLive != (int) live)
    {
        backgroundScale = s;
        backgroundLive = (int) live;
        background = Image (Image::RGB, roundToInt (W * s), roundToInt (H * s), true);
        Graphics bg (background);
        bg.addTransform (AffineTransform::scale (s));
        paintPanels (bg);
        paintStatic (bg, live);
    }
    {
        Graphics::ScopedSaveState ss (g);
        g.addTransform (AffineTransform::scale (1.0f / s));
        g.drawImageAt (background, 0, 0);
    }
    const auto& lay = live ? kLive : kStudio;

    // the Range band: between Min and Max, lit, up to the Rider's scale; the Idle line across it
    const float lo = proc.get (vl::id::rangeMin), hi = proc.get (vl::id::rangeMax);
    g.setColour (theme::cyan.withAlpha (0.10f));
    g.fillRect (Rectangle<float>::leftTopRightBottom (65.0f, scaleY (hi), 147.5f, scaleY (lo)));
    g.setColour (kIdle.withAlpha (0.8f));
    g.fillRect (65.0f, scaleY (proc.idleDb()) - 0.5f, 82.5f, 1.0f);
    if (idleColumn.dragging || marks.hover == &idleColumn || marks.showsValue (&idleColumn)) hoverBox (g, { 3.0f, scaleY (proc.idleDb()) - 6.5f, 27.0f, 13.0f }, String (proc.idleDb(), 1));

    // the LEDs: Ride (cyan) above Idle (amber) in their 17 x 36 column, the clip LED, the Music LED (brighter as the
    // music is louder)
    const float lx = (float) lay.leds.getX() + 8.5f, ly = (float) lay.leds.getY();
    drawLed (g, { lx, ly + 9.5f }, 4.5f, riding ? 1.0f : 0.0f, theme::cyan);
    drawLed (g, { lx, ly + 26.5f }, 4.5f, riding ? 0.0f : 1.0f, theme::amber);
    drawLed (g, { 287.5f, 199.5f }, 4.5f, clip ? 1.0f : 0.0f, theme::red);
    drawLed (g, { (float) lay.musicLed.getX() + 16.5f, (float) lay.musicLed.getY() + 17.5f }, 10.0f, music01, theme::cyan);

    // value boxes
    theme::drawValueBox (g, { 44.5f, 476.0f, 35.0f, 15.0f }, String (hi - lo, 1));
    theme::drawValueBox (g, { 142.5f, 476.0f, 35.0f, 15.0f }, String (rider.getValue(), 1));
    theme::drawValueBox (g, { 239.5f, 476.0f, 35.0f, 15.0f }, String (output.getValue(), 1));

    // the values shown under the mouse
    if (target.isMouseButtonDown() || marks.showsValue (&target)) hoverBox (g, { 272.0f, 45.0f, 32.0f, 13.0f }, String (target.getValue(), 1));
    for (auto* f : { &rangeMax, &rangeMin })
        if (f->isMouseButtonDown() || marks.showsValue (f))
        {
            const auto cap = f->capCore().translated ((float) f->getX(), (float) f->getY());
            hoverBox (g, { 88.0f, cap.getCentreY() - 7.0f, 31.0f, 14.0f }, String (f->getValue(), 1));
        }
    const std::pair<Knob*, Rectangle<int>> knobs[] = { { &vocal, lay.vocalValue }, { &music, lay.musicValue }, { &spill, lay.spillValue } };
    for (auto [k, r] : knobs)
        if (k->isVisible() && k->showing() && ! r.isEmpty())
            hoverBox (g, r.toFloat(), k == &music && vl::musicOff ((float) k->getValue()) ? String ("Off") : String (k->getValue(), 1));
}
