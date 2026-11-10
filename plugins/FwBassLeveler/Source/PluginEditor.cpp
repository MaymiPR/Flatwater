#include "PluginEditor.h"
#include "shared/ui/Theme.h"
#include "shared/ui/Widgets.h"

using namespace juce;
using namespace bl::ui;
using flatwater::ren::arial;
namespace theme = flatwater::theme;

namespace
{
// the editor in Flatwater's own look (shared/ui/Theme.h): the layout keeps the reference's measured positions and sizes
const Colour kLabelLight (theme::textDim), kLabelDim (theme::textFaint), kFrame (theme::line), kLabelDark (theme::textDim), kTick (theme::textFaint),
    kIdleLine (theme::amber), kIdleArrow (theme::amber), kMarkSelected (theme::amber), kMarkMember (theme::cyan);

// a value in a well (Range, Rider, Output)
void valueBox (Graphics& g, Rectangle<float> r, const String& text) { theme::drawValueBox (g, r, text); }

// a selected control's value: a raised box in an amber frame
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
}

//==============================================================================
void bl::ui::drawMark (Graphics& g, Rectangle<float> r, Mark m, bool round)
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
    e.source.enableUnboundedMouseMovement (true, false);         // hidden cursor, no screen edge
    s.Slider::mouseDown (e);                                       // the host's gesture begins
    return true;
}

void LinearDrag::drag (Slider& s, const MouseEvent& e, float unitPx, bool vert)
{
    if (! active) return;
    const auto d = e.position - last;
    last = e.position;
    float k = 1.0f;
    if (flatwater::ctrlHeldNow()) k = std::abs (vert ? d.y : d.x) <= 2.0f ? 0.037f : 0.19f;
    virt += d * k;
    vertical = vert;
    pxPerUnit = unitPx;
    const float moved = vertical ? origin.y - virt.y : virt.x - origin.x;
    s.setValue (jlimit (s.getMinimum(), s.getMaximum(), start + (double) (moved / pxPerUnit)), sendNotificationSync);
}

void LinearDrag::up (Slider& s, const MouseEvent& e)
{
    if (! active) return;
    active = false;
    s.Slider::mouseUp (e);
    e.source.enableUnboundedMouseMovement (false);
    const float moved = (float) (s.getValue() - start) * pxPerUnit;   // the cursor comes back where the thumb went
    const auto at = vertical ? juce::Point<float> (origin.x, origin.y - moved) : juce::Point<float> (origin.x + moved, origin.y);
    juce::Desktop::setMousePosition (s.localPointToGlobal (at).roundToInt());
}

//==============================================================================
Knob::Knob()
{
    setSliderStyle (RotaryVerticalDrag);
    setTextBoxStyle (NoTextBox, true, 0, 0);
    setRotaryParameters (degreesToRadians (228.0f), degreesToRadians (492.0f), true);   // +-132 degrees (measured on the strip)
    setMouseDragSensitivity (500);
    setVelocityModeParameters (1.0, 1, 0.0, false);
}

void Knob::mouseEnter (const MouseEvent&) { setMarks (marks, this, true); }
void Knob::mouseExit (const MouseEvent&) { setMarks (marks, this, false); }
void Knob::mouseDown (const MouseEvent& e)
{
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

// the knob in Flatwater's look: radius 19.6, centred 0.5 px above the control's centre; the selection a ring at the rim
void Knob::paint (Graphics& g)
{
    const Point<float> c (22.0f, 21.5f);
    const auto& rp = getRotaryParameters();
    const float a = rp.startAngleRadians + (float) valueToProportionOfLength (getValue()) * (rp.endAngleRadians - rp.startAngleRadians);
    theme::drawKnob (g, c, 15.5f, a, rp.startAngleRadians, rp.endAngleRadians, rp.startAngleRadians);
    drawMark (g, Rectangle<float> (39.0f, 39.0f).withCentre (c), marks != nullptr ? marks->of (this) : Mark::None, true);
}

//==============================================================================
Fader::Fader (Kind k) : kind (k)
{
    setSliderStyle (LinearVertical);
    setTextBoxStyle (NoTextBox, true, 0, 0);
    setSliderSnapsToMousePosition (false);
    setVelocityModeParameters (1.0, 1, 0.0, false);
}

// the cap's reference point: Output and the Rider centred on the value; Range Max's cap ends at its value, Range Min's
// starts there (measured: the band between them is the range)
float Fader::yOf (double v) const { return scaleY ((float) v) - (float) getY(); }

// each cap's place (measured): its left edge in the editor, its top from the value, its size
struct CapPlace { int x0, dy, w, h; };
static CapPlace capPlace (Fader::Kind k)
{
    return k == Fader::Rider ? CapPlace { 133, -33, 54, 69 } : k == Fader::Output ? CapPlace { 238, -8, 39, 20 }
         : k == Fader::RangeMax ? CapPlace { 41, -22, 39, 20 } : CapPlace { 41, -1, 39, 20 };
}

Rectangle<float> Fader::capBounds() const
{
    const auto c = capPlace (kind);
    // on whole pixels (a cap between pixels would let the scale show through it)
    return { (float) (c.x0 - getX()), std::round (yOf (getValue())) + (float) c.dy, (float) c.w, (float) c.h };
}

bool Fader::hitTest (int x, int y) { return capBounds().expanded (2.0f).contains ((float) x, (float) y); }
void Fader::mouseEnter (const MouseEvent&) { setMarks (marks, this, true); }
void Fader::mouseExit (const MouseEvent&) { setMarks (marks, this, false); }

void Fader::mouseDown (const MouseEvent& e)
{
    blocked = marks != nullptr && ! marks->click (this);
    if (blocked) return;
    if (kind == Rider && onGrab) onGrab (true);
    feel.down (*this, e);
}
void Fader::mouseDrag (const MouseEvent& e) { if (! blocked) feel.drag (*this, e, 8.75f, true); }
void Fader::mouseUp (const MouseEvent& e)
{
    if (blocked) { blocked = false; return; }
    feel.up (*this, e);
    if (kind == Rider && onGrab) onGrab (false);
}

void Fader::paint (Graphics& g)
{
    const auto cap = capBounds();
    theme::drawCap (g, cap.reduced (kind == Rider ? 2.0f : 1.0f, kind == Rider ? 4.0f : 1.0f), kind == Rider ? 5 : 1);
    drawMark (g, cap, marks != nullptr ? marks->of (this) : Mark::None);
}

//==============================================================================
TargetSlider::TargetSlider()
{
    setSliderStyle (LinearHorizontal);
    setTextBoxStyle (NoTextBox, true, 0, 0);
    setSliderSnapsToMousePosition (false);
    setVelocityModeParameters (1.0, 1, 0.0, false);
}

// the control is 210 px wide (59 .. 269); the thumb 29 px: its left edge from 0 (-42 dB) to 181 (0 dB)
float TargetSlider::thumbX (double v) const { return (float) ((v + 42.0) / 42.0 * 181.0); }
bool TargetSlider::hitTest (int x, int y) { return Rectangle<float> (thumbX (getValue()), 0.0f, 29.0f, 22.0f).expanded (2.0f).contains ((float) x, (float) y); }
void TargetSlider::mouseEnter (const MouseEvent&) { setMarks (marks, this, true); }
void TargetSlider::mouseExit (const MouseEvent&) { setMarks (marks, this, false); }
void TargetSlider::mouseDown (const MouseEvent& e)
{
    blocked = marks != nullptr && ! marks->click (this);
    if (! blocked) feel.down (*this, e);
}
void TargetSlider::mouseDrag (const MouseEvent& e) { if (! blocked) feel.drag (*this, e, 181.0f / 42.0f, false); }
void TargetSlider::mouseUp (const MouseEvent& e) { if (! blocked) feel.up (*this, e); blocked = false; }

void TargetSlider::paint (Graphics& g)
{
    // the Energy meter under the thumb: 184 x 15 at 12, 3 (local); -42 .. 0 dBFS from left to right
    theme::drawMeter (g, { 12.0f, 3.0f, 184.0f, 15.0f }, (energyDb + 42.0f) / 42.0f, false);
    // the thumb on whole pixels: a clear glass in an amber frame, the amber line down its centre
    const Rectangle<float> t (std::round (thumbX (getValue())), 0.0f, 29.0f, 22.0f);
    const auto glass = t.reduced (2.0f, 1.0f).withTrimmedBottom (2.0f);
    g.setColour (theme::text.withAlpha (0.12f));
    g.fillRoundedRectangle (glass, 2.0f);
    g.setColour (theme::amber);
    g.drawRoundedRectangle (glass.reduced (0.5f), 2.0f, 1.0f);
    g.fillRect (glass.getCentreX() - 0.5f, glass.getY() + 1.0f, 1.0f, glass.getHeight() - 2.0f);
    drawMark (g, t, marks != nullptr ? marks->of (this) : Mark::None);
}

//==============================================================================
// the switch (37 x 17): a dark slot, the ridged chrome block on the left (Slow) or the right (Fast)
void Switch::paintButton (Graphics& g, bool, bool)
{
    // a well with the raised block on the left (Slow) or the right (Fast), a cyan line on its outer edge
    const bool fast = getToggleState();
    theme::drawWell (g, getLocalBounds().toFloat(), 3.0f);
    const Rectangle<float> block (fast ? 16.0f : 1.0f, 1.0f, 20.0f, 15.0f);
    g.setGradientFill (ColourGradient (theme::raisedTop, 0.0f, block.getY(), theme::raised, 0.0f, block.getBottom(), false));
    g.fillRoundedRectangle (block, 2.0f);
    g.setColour (theme::line);
    g.drawRoundedRectangle (block.reduced (0.5f), 2.0f, 1.0f);
    g.setColour (theme::cyan);
    g.fillRect (fast ? block.getRight() - 3.0f : block.getX() + 2.0f, block.getY() + 3.0f, 1.0f, block.getHeight() - 6.0f);
    if (marks != nullptr && marks->of (this) != Mark::None)
    {
        g.setColour (kMarkSelected);
        g.drawRect (getLocalBounds(), 1);
    }
}

//==============================================================================
void IdleColumn::paint (Graphics& g)
{
    const float y = scaleY (idleDb ? idleDb() : 0.0f) - (float) getY();
    Path tri;
    tri.addTriangle (4.0f, y - 4.5f, 4.0f, y + 4.5f, 11.0f, y);
    g.setColour (kIdleArrow);
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
// the cursor over the knobs and Output (the reference's DisplayCursor controls, filmed with MeterCapture): an arrow
// with a small up / down arrow at its lower right, the hotspot at the arrow's tip (6, 7)
static MouseCursor dragCursor()
{
    Image img (Image::ARGB, 32, 32, true);
    {
        Graphics g (img);
        Path arrow;
        arrow.startNewSubPath (6.5f, 7.5f);
        arrow.lineTo (6.5f, 21.5f); arrow.lineTo (9.5f, 18.5f); arrow.lineTo (12.0f, 24.0f); arrow.lineTo (14.0f, 23.0f);
        arrow.lineTo (11.5f, 17.5f); arrow.lineTo (15.5f, 17.5f); arrow.closeSubPath();
        g.setColour (Colours::white); g.fillPath (arrow);
        g.setColour (Colours::black); g.strokePath (arrow, PathStrokeType (1.0f));
        Path updown;
        updown.startNewSubPath (22.5f, 17.5f); updown.lineTo (25.5f, 20.5f); updown.lineTo (23.5f, 20.5f); updown.lineTo (23.5f, 23.5f);
        updown.lineTo (25.5f, 23.5f); updown.lineTo (22.5f, 26.5f); updown.lineTo (19.5f, 23.5f); updown.lineTo (21.5f, 23.5f);
        updown.lineTo (21.5f, 20.5f); updown.lineTo (19.5f, 20.5f); updown.closeSubPath();
        g.setColour (Colours::white); g.fillPath (updown);
        g.setColour (Colours::black); g.strokePath (updown, PathStrokeType (1.0f));
    }
    return MouseCursor (img, 6, 7);
}

//==============================================================================
FwBassLevelerEditor::FwBassLevelerEditor (FwBassLevelerProcessor& p)
    : Editor (p, p.apvts, p.undo, p.apvts.getParameter (bl::mode), "Flatwater Bass Leveler", JucePlugin_VersionString, W, H,
              flatwater::Toolbar::Style::TwoRow, bl::modeNames()),
      lasso (make<flatwater::flat::LassoOverlay>()),
      proc (p),
      sensitivity (make<Knob>()), spill (make<Knob>()), artifacts (make<Knob>()), responseSwitch (make<Switch>()),
      target (make<TargetSlider>()), rangeMax (make<Fader> (Fader::RangeMax)), rangeMin (make<Fader> (Fader::RangeMin)),
      output (make<Fader> (Fader::Output)), rider (make<Fader> (Fader::Rider)), idleColumn (make<IdleColumn>())
{
    setFactoryPresets (bl::factoryPresets());             // Target stays where it is, as in the reference, unless a preset sets it
    marks.blueAlone = { &rider, &idleColumn };
    marks.changed = [this] { content().repaint(); };
    const auto dragging = dragCursor();
    for (Component* c : { (Component*) &sensitivity, (Component*) &spill, (Component*) &artifacts, (Component*) &output })
        c->setMouseCursor (dragging);
    marks.enter = [this] (Slider* sl) { openEntry (sl); };
    for (auto* k : { &sensitivity, &spill, &artifacts }) k->marks = &marks;
    for (auto* f : { &rangeMax, &rangeMin, &output, &rider }) f->marks = &marks;
    target.marks = &marks;
    responseSwitch.marks = &marks;

    // control rectangles from the reference's layout
    attachSlider (sensitivity, bl::sensitivity, { 24, 90, 44, 44 }, 50.0);
    attachSlider (spill, bl::spill, { 178, 90, 44, 44 }, -40.0);
    attachSlider (artifacts, bl::artifacts, { 253, 90, 44, 44 }, 50.0);
    attach (responseSwitch, bl::response, { 104, 106, 37, 17 });
    attachSlider (target, bl::target, { 59, 45, 210, 22 }, -14.0);
    attachSlider (rangeMax, bl::rangeMax, { 36, 190, 48, 270 }, 12.0);
    attachSlider (rangeMin, bl::rangeMin, { 36, 190, 48, 270 }, -12.0);
    attachSlider (output, bl::output, { 235, 190, 48, 270 }, 0.0);
    // the Rider: not a parameter; it shows the processor's gain and can be held by hand
    rider.setRange (-12.0, 12.0, 0.01);
    addToContent (rider, { 128, 190, 64, 270 });
    rider.onGrab = [this] (bool held)
    {
        proc.userRideDb.store ((float) rider.getValue());
        proc.userRide.store (held);
        if (! held)                                                 // riding resumes at once (measured)
        {
            ride = proc.idleDb();
            rider.setValue (ride, dontSendNotification);
            content().repaint();
        }
    };
    rider.onValueChange = [this] { if (proc.userRide.load()) proc.userRideDb.store ((float) rider.getValue()); };
    // Range Max stays at least 1 dB above Min (the Effective Range is 1 .. 24 dB)
    rangeMax.onValueChange = [this] { if (rangeMin.getValue() > rangeMax.getValue() - 1.0) rangeMin.setValue (rangeMax.getValue() - 1.0, sendNotificationSync); content().repaint(); };
    rangeMin.onValueChange = [this] { if (rangeMax.getValue() < rangeMin.getValue() + 1.0) rangeMax.setValue (rangeMin.getValue() + 1.0, sendNotificationSync); content().repaint(); };
    output.onValueChange = [this] { content().repaint (230, 460, 60, 30); };
    // the knobs' and the Target's value boxes are painted by the panel, so they follow a drag only if it repaints them
    for (auto* k : { &sensitivity, &spill, &artifacts })
        k->onValueChange = [this, k] { content().repaint (k->getX(), 150, k->getWidth() + 20, 28); };
    target.onValueChange = [this] { content().repaint (266, 42, 44, 26); };
    // the Idle arrow
    auto* idleParam = p.apvts.getParameter (bl::idle);
    idleColumn.marks = &marks;
    idleColumn.idleDb = [this] { return proc.idleDb(); };
    idleColumn.setIdleDb = [this, idleParam] (float db)
    {
        const float lo = proc.get (bl::rangeMin), hi = proc.get (bl::rangeMax);
        idleParam->setValueNotifyingHost (idleParam->convertTo0to1 (jlimit (0.02f, 0.98f, (jlimit (lo, hi, db) - lo) / jmax (1.0f, hi - lo))));
        content().repaint();
    };
    idleColumn.gesture = [idleParam] (bool begin) { if (begin) idleParam->beginChangeGesture(); else idleParam->endChangeGesture(); };
    idleColumn.reset = [this, idleParam] { idleParam->setValueNotifyingHost (idleParam->getDefaultValue()); content().repaint(); };
    addToContent (idleColumn, { 33, 214, 16, 226 });
    addToContent (lasso, { 0, 0, W, H });                          // over everything, takes no clicks
    lasso.setVisible (false);
}

FwBassLevelerEditor::~FwBassLevelerEditor()
{
    entry.reset();
    for (auto* k : { &sensitivity, &spill, &artifacts }) k->marks = nullptr;
}

Rectangle<int> FwBassLevelerEditor::entryArea (Slider* sl) const
{
    if (sl == &sensitivity) return { 27, 157, 37, 13 };
    if (sl == &spill) return { 181, 157, 37, 13 };
    if (sl == &artifacts) return { 256, 157, 37, 13 };
    if (sl == &target) return { 272, 48, 32, 13 };
    if (sl == &output) return { 241, 469, 34, 14 };
    if (sl == &rangeMax || sl == &rangeMin)
    {
        auto* f = static_cast<Fader*> (sl);
        const auto cap = f->capBounds().translated ((float) f->getX(), (float) f->getY());
        return { 88, roundToInt (cap.getCentreY()) - 7, 31, 14 };
    }
    return {};
}

void FwBassLevelerEditor::openEntry (Slider* sl)
{
    const auto area = entryArea (sl);
    if (area.isEmpty()) return;
    closeEntry (false);
    entryFor = sl;
    entry = std::make_unique<TextEditor>();
    entry->setFont (arial (11.0f));
    entry->setJustification (Justification::centredLeft);
    entry->setIndents (2, 0);
    entry->setColour (TextEditor::backgroundColourId, theme::well);
    entry->setColour (TextEditor::textColourId, theme::text);
    entry->setColour (TextEditor::outlineColourId, theme::line);
    entry->setColour (TextEditor::focusedOutlineColourId, theme::cyan);
    entry->setText (sl->getTextFromValue (sl->getValue()).upToFirstOccurrenceOf (" ", false, false), false);
    entry->onReturnKey = [this] { closeEntry (true); };
    entry->onEscapeKey = [this] { closeEntry (false); };
    entry->onFocusLost = [this] { closeEntry (false); };
    content().addAndMakeVisible (*entry);
    entry->setBounds (area);
    entry->selectAll();
    entry->grabKeyboardFocus();                                    // only while typing: the host keeps it otherwise
}

void FwBassLevelerEditor::closeEntry (bool apply)
{
    if (entry == nullptr) return;
    auto e = std::move (entry);                                    // (losing the focus calls back here)
    auto* sl = entryFor;
    entryFor = nullptr;
    if (apply && sl != nullptr)
    {
        const auto t = e->getText().trim().upToFirstOccurrenceOf (" ", false, false);
        if (t.containsAnyOf ("0123456789"))
        {
            double v = t.getDoubleValue();
            if (sl == &spill) v = -std::abs (v);                   // the reference's PositiveTextEntryToNegative
            if (v >= sl->getMinimum() - 1.0e-9 && v <= sl->getMaximum() + 1.0e-9)
            {
                Slider::ScopedDragNotification drag (*sl);
                sl->setValue (v, sendNotificationSync);
            }
        }
    }
    e->setVisible (false);
    MessageManager::callAsync ([owned = std::shared_ptr<TextEditor> (std::move (e))] {});   // destroyed after its callback returns
    content().repaint();
}

// the lasso (measured on the reference: a 1 px white rectangle while dragging on the background; the controls it
// touches, in the reference's control order: Spill, Rider, Range Max, Range Min, Output, Sensitivity, Target, Idle,
// Response, Artifacts)
void FwBassLevelerEditor::contentMouseDown (const MouseEvent& e)
{
    if (Rectangle<float> (279.0f, 183.0f, 17.0f, 15.0f).contains (e.position))   // the clip LED: a click clears it
    {
        clip = false;
        content().repaint (276, 182, 24, 18);
        return;
    }
    lassoing = true;
    lassoStart = e.position;
    marks.group.clear();
    marks.selected = nullptr;
    if (marks.changed) marks.changed();
}

void FwBassLevelerEditor::contentMouseDrag (const MouseEvent& e)
{
    if (! lassoing) return;
    const auto r = Rectangle<float> (lassoStart, e.position);
    lasso.rect = r.toNearestInt();
    lasso.setVisible (true);
    lasso.repaint();
    auto at = [] (Component& c, Rectangle<float> local) { return local.translated ((float) c.getX(), (float) c.getY()); };
    const std::pair<Component*, Rectangle<float>> items[] = {
        { &spill, spill.getBounds().toFloat() }, { &rider, at (rider, rider.capBounds()) },
        { &rangeMax, at (rangeMax, rangeMax.capBounds()) }, { &rangeMin, at (rangeMin, rangeMin.capBounds()) },
        { &output, at (output, output.capBounds()) }, { &sensitivity, sensitivity.getBounds().toFloat() },
        { &target, at (target, { target.thumbX (target.getValue()), 0.0f, 29.0f, 22.0f }) },
        { &idleColumn, Rectangle<float> (35.0f, scaleY (proc.idleDb()) - 5.5f, 12.0f, 11.0f) },
        { &responseSwitch, responseSwitch.getBounds().toFloat() }, { &artifacts, artifacts.getBounds().toFloat() } };
    marks.group.clear();
    for (auto& [c, b] : items)
        if (b.intersects (r)) marks.group.push_back (c);
    marks.selected = marks.group.empty() ? nullptr : marks.group.front();
    if (marks.changed) marks.changed();
}

void FwBassLevelerEditor::contentMouseUp (const MouseEvent&)
{
    lassoing = false;
    lasso.setVisible (false);
    if (marks.group.size() < 2) marks.group.clear();
    if (marks.changed) marks.changed();
}

void FwBassLevelerEditor::tick()
{
    const double now = Time::getMillisecondCounterHiRes() * 0.001, dt = lastTick > 0.0 ? now - lastTick : 0.0;
    lastTick = now;
    // the Energy meter's display (measured on the reference's films): it rises at once and falls 37.3 dB per second at
    // most (slower when the energy itself falls slower)
    const float e = proc.energyDb.take();
    energy = e >= energy ? e : jmax (e, energy - 37.3f * (float) jmin (dt, 0.25));
    target.energyDb = energy;
    target.repaint();
    if (proc.outDb.take() >= 0.0f && ! clip) { clip = true; content().repaint (276, 182, 24, 18); }   // latched
    const float r = proc.rideDb.load();
    if (! rider.isMouseButtonDown() && std::abs (r - ride) > 1.0e-4f)
    {
        ride = r;
        rider.setValue (r, dontSendNotification);
    }
    content().repaint (120, 170, 190, 320);
    idleColumn.repaint();
}



//==============================================================================
// the static art, in design coordinates
void FwBassLevelerEditor::paintStatic (Graphics& g) const
{
    // the name plate: a dark strip with fine scratches (our own seeded pattern), the Flatwater mark and the plug-in's name
    {
        g.setColour (theme::panel);
        g.fillRect (0.0f, 511.0f, 320.0f, 49.0f);
        g.setColour (theme::line);
        g.fillRect (0.0f, 511.0f, 320.0f, 1.0f);
        Random rnd (20261101);
        for (int i = 0; i < 220; ++i)
        {
            const float x = rnd.nextFloat() * 320.0f, y = 513.0f + rnd.nextFloat() * 46.0f;
            const float ang = degreesToRadians ((rnd.nextBool() ? 62.0f : 118.0f) + (rnd.nextFloat() - 0.5f) * 30.0f);
            const float len = 3.0f + rnd.nextFloat() * 16.0f;
            g.setColour (theme::textFaint.withAlpha (0.05f + 0.08f * rnd.nextFloat()));
            g.drawLine (x, y, x + len * std::cos (ang), jlimit (513.0f, 559.0f, y + len * std::sin (ang)), 0.6f);
        }
    }
    flatwater::drawMark (g, { 14.0f, 520.0f, 30.0f, 30.0f }, false, false);
    g.setColour (theme::text);
    g.setFont (theme::font (17.0f, true));
    g.drawText ("Flatwater Bass Leveler", Rectangle<float> (50.0f, 518.0f, 262.0f, 34.0f), Justification::centredLeft);

    // ---- top panel: the group frames (2 px of grey 132 at 0.69, measured on x 7-8 / 311-312, y 75-77 and 154-155 over
    // three panel greys), stroked as one path so the shared stems are not drawn twice
    const PathStrokeType frameStroke (2.0f);
    // each group (traced on the skin's 2x background): its title gap in the top line, the outer corners quadratic
    // fillets 10 px each way; at the junction (x 162) the top and bottom lines bend down / up in the same 10 px fillets
    // into short stems ending at y 87 and 144.6, the side open
    Path frames;
    auto group = [&frames] (float xOuter, float gapOuter, float gapJoin)    // the title gap's ends
    {
        const float top = 76.5f, bottom = 155.0f, r = 10.0f, xJoin = 162.0f, dir = xJoin > xOuter ? 1.0f : -1.0f;
        frames.startNewSubPath (gapOuter, top);
        frames.lineTo (xOuter + dir * r, top);
        frames.quadraticTo (xOuter, top, xOuter, top + r);
        frames.lineTo (xOuter, bottom - r);
        frames.quadraticTo (xOuter, bottom, xOuter + dir * r, bottom);
        frames.lineTo (xJoin - dir * r, bottom);
        frames.quadraticTo (xJoin, bottom, xJoin, bottom - r);
        frames.lineTo (xJoin, 144.6f);
        frames.startNewSubPath (gapJoin, top);
        frames.lineTo (xJoin - dir * r, top);
        frames.quadraticTo (xJoin, top, xJoin, top + r);
        frames.lineTo (xJoin, 87.0f);
    };
    group (8.0f, 61.0f, 101.5f);
    group (312.0f, 258.5f, 218.0f);
    g.setColour (kFrame);
    g.strokePath (frames, frameStroke);

    g.setColour (kLabelLight);
    g.setFont (arial (12.0f, true));
    g.drawText ("Target", Rectangle<float> (13.0f, 48.0f, 50.0f, 14.0f), Justification::centredLeft);
    g.setColour (kLabelLight.darker (0.08f));
    g.setFont (arial (11.4f, true));
    g.drawText ("Sensitivity", Rectangle<float> (14.0f, 137.0f, 64.0f, 14.0f), Justification::centred);
    g.drawText ("Response", Rectangle<float> (92.0f, 137.0f, 62.0f, 14.0f), Justification::centred);
    g.drawText ("Spill", Rectangle<float> (178.0f, 137.0f, 44.0f, 14.0f), Justification::centred);
    g.drawText ("Artifacts", Rectangle<float> (245.0f, 137.0f, 60.0f, 14.0f), Justification::centred);
    g.setColour (theme::amber);
    g.setFont (arial (13.5f, true));
    g.drawText ("Detect", Rectangle<float> (58.0f, 68.0f, 47.0f, 16.0f), Justification::centred);
    g.drawText ("Ignore", Rectangle<float> (214.0f, 68.0f, 48.0f, 16.0f), Justification::centred);
    g.setColour (kLabelDim);
    g.setFont (arial (10.0f, true));
    for (float cx : { 46.0f, 200.0f, 275.0f })
    {
        g.drawText ("Lo", Rectangle<float> (cx - 34.0f, 126.0f, 14.0f, 12.0f), Justification::centred);
        g.drawText ("Hi", Rectangle<float> (cx + 20.0f, 126.0f, 14.0f, 12.0f), Justification::centred);
    }
    g.drawText ("Slow", Rectangle<float> (76.0f, 108.0f, 24.0f, 12.0f), Justification::centred);
    g.drawText ("Fast", Rectangle<float> (141.5f, 108.0f, 24.0f, 12.0f), Justification::centred);

    // the knobs' scales at their measured places round the knobs (x 46 / 200 / 275, y 111.5): Sensitivity's 12 dashes on
    // a ring of their own (centred 0.4 px left of and 1.4 px below the knob's, radius 21.1 to 25.25, 16.5 degrees long),
    // Spill's and Artifacts' 13 ticks of their own lengths; a metal ring round those two
    {
        const Rectangle<float> ring = Rectangle<float> (50.5f, 50.5f).withCentre ({ 45.6f, 112.9f });
        static constexpr float dashes[] = { -122.3f, -100.0f, -76.7f, -54.0f, -32.0f, -10.0f, 11.4f, 33.2f, 54.6f, 76.6f, 98.8f, 120.2f };
        for (float mid : dashes)
        {
            g.setColour (theme::textFaint.withAlpha (std::abs (mid) > 110.0f ? 0.75f : 0.9f));
            Path seg;
            seg.addPieSegment (ring, degreesToRadians (mid - 8.25f), degreesToRadians (mid + 8.25f), 21.1f / 25.25f);
            g.fillPath (seg);
        }
        struct Tick { float angle, r0, r1, weight; };
        static constexpr Tick ticks[] = { { 0.0f, 20.5f, 25.8f, 1.0f }, { 23.0f, 20.3f, 24.0f, 0.95f }, { 46.0f, 20.3f, 25.6f, 0.95f },
                                          { 69.0f, 20.5f, 24.3f, 0.9f }, { 91.0f, 21.0f, 26.8f, 1.0f }, { 115.0f, 21.3f, 25.3f, 0.8f },
                                          { 135.0f, 21.3f, 27.3f, 0.7f } };
        for (float cx : { 200.0f, 275.0f })
        {
            const Point<float> c (cx, 111.5f);
            for (auto& t : ticks)
                for (float side : { -1.0f, 1.0f })
                {
                    if (juce::exactlyEqual (t.angle, 0.0f) && side > 0.0f) continue;
                    const float a = degreesToRadians (side * t.angle);
                    const Point<float> d (std::sin (a), -std::cos (a));
                    g.setColour (theme::textFaint.withAlpha (t.weight));
                    g.drawLine (Line<float> (c + d * t.r0, c + d * t.r1), 2.0f);
                }
            g.setGradientFill (ColourGradient (theme::line.brighter (0.6f), 0.0f, c.y - 20.0f, theme::line.darker (0.4f), 0.0f, c.y + 20.0f, false));
            g.drawEllipse (Rectangle<float> (39.6f, 39.6f).withCentre (c), 1.2f);
        }
    }

    // ---- lower panel: labels, screws, the clip LED's socket, the wells, tracks, scales and value boxes
    g.setColour (kLabelDark);
    g.setFont (arial (12.0f, true));
    g.drawText ("Range", Rectangle<float> (36.0f, 185.0f, 52.0f, 14.0f), Justification::centred);
    g.drawText ("Rider", Rectangle<float> (134.0f, 185.0f, 52.0f, 14.0f), Justification::centred);
    g.drawText ("Output", Rectangle<float> (231.0f, 185.0f, 52.0f, 14.0f), Justification::centred);
    for (float y : { 177.5f, 495.0f })
    {
        // a screw at its measured place (centred at 159.5): a dark ring on a raised face, a small cross
        const Point<float> c (159.5f, y);
        g.setColour (theme::raised);
        g.fillEllipse (Rectangle<float> (7.0f, 7.0f).withCentre (c));
        g.setColour (theme::well);
        g.drawEllipse (Rectangle<float> (8.4f, 8.4f).withCentre (c), 1.3f);
        g.setColour (theme::line.brighter (0.4f));
        g.fillRect (c.x - 1.5f, c.y - 0.5f, 3.0f, 1.0f);
        g.fillRect (c.x - 0.5f, c.y - 1.5f, 1.0f, 3.0f);
    }
    for (float x : { 52.0f, 150.0f, 248.0f })                       // the wells behind the tracks
    {
        g.setColour (theme::well.withAlpha (0.45f));
        g.fillRoundedRectangle (x, 200.0f, 20.0f, 256.0f, 5.0f);
    }
    theme::drawWell (g, { 59.0f, 210.0f, 5.0f, 238.0f }, 2.5f);           // Range
    theme::drawWell (g, { 156.5f, 210.0f, 6.0f, 238.0f }, 3.0f);          // Rider
    theme::drawWell (g, { 255.0f, 215.0f, 5.0f, 227.0f }, 2.5f);          // Output
    // the scales (measured on the reference's 2x skin): a mark every 3 dB (2 px at +-12 and 0, else 1 px), five faint
    // half-pixel ticks between marks at 5, 9, 13, 17 and 21 px below each; numbers every 6 dB centred at x 117.5 and
    // 202 (the Rider) and at +-12 / 0 centred at 291 (the Output)
    g.setFont (arial (10.0f));
    for (int k = -4; k <= 4; ++k)
    {
        const float db = 3.0f * (float) k, y = scaleY (db);
        const bool thick = k == 0 || std::abs (k) == 4, numbered = k % 2 == 0;
        const float h = thick ? 2.0f : 1.0f;
        g.setColour (kTick);
        g.fillRect (129.0f, y - h * 0.5f, 19.0f, h);
        g.fillRect (172.0f, y - h * 0.5f, 19.0f, h);
        g.fillRect (270.0f, y - h * 0.5f, thick ? 11.0f : 6.0f, h);
        if (k < 4)
        {
            g.setColour (kTick.withAlpha (0.5f));
            for (int t = 0; t < 5; ++t)
            {
                const float ty = y + 5.0f + 4.0f * (float) t;
                g.fillRect (139.0f, ty - 0.25f, 9.0f, 0.5f);
                g.fillRect (172.0f, ty - 0.25f, 8.0f, 0.5f);
                g.fillRect (270.0f, ty - 0.25f, 4.0f, 0.5f);
            }
        }
        if (numbered)
        {
            const String t = db > 0.0f ? "+" + String ((int) db) : String ((int) db);
            g.setColour (kLabelDark);
            g.drawText (t, Rectangle<float> (97.5f, y - 7.5f, 40.0f, 14.0f), Justification::centred);
            g.drawText (t, Rectangle<float> (182.0f, y - 7.5f, 40.0f, 14.0f), Justification::centred);
            if (thick) g.drawText (t, Rectangle<float> (271.0f, y - 7.5f, 40.0f, 14.0f), Justification::centred);
        }
    }
}

// the panels in Flatwater's look: the background, a card for the top controls, one for the faders, the name plate
static void paintPanels (Graphics& g)
{
    g.fillAll (theme::bg);
    theme::drawCard (g, { 6.0f, 38.0f, 308.0f, 122.0f });
    theme::drawCard (g, { 4.0f, 166.0f, 312.0f, 340.0f });
}

void FwBassLevelerEditor::paintContent (Graphics& g)
{
    // the static art, rendered once per physical scale (the window's size times the display's)
    const float s = jlimit (0.25f, 8.0f, std::round (g.getInternalContext().getPhysicalPixelScaleFactor() * 100.0f) / 100.0f);
    if (background.isNull() || std::abs (s - backgroundScale) > 1.0e-3f)
    {
        backgroundScale = s;
        background = Image (Image::RGB, roundToInt (W * s), roundToInt (H * s), true);
        Graphics bg (background);
        bg.addTransform (AffineTransform::scale (s));
        paintPanels (bg);
        paintStatic (bg);
    }
    {
        Graphics::ScopedSaveState ss (g);
        g.addTransform (AffineTransform::scale (1.0f / s));
        g.drawImageAt (background, 0, 0);
    }

    // the Range band: the space between Min and Max, lit, up to the Rider's scale; the Idle line across it
    const float lo = proc.get (bl::rangeMin), hi = proc.get (bl::rangeMax);
    g.setColour (theme::cyan.withAlpha (0.10f));
    g.fillRect (Rectangle<float>::leftTopRightBottom (72.0f, scaleY (hi), 149.0f, scaleY (lo)));
    g.setColour (kIdleLine.withAlpha (0.8f));
    g.fillRect (72.0f, scaleY (proc.idleDb()) - 0.5f, 77.0f, 1.0f);
    if (idleColumn.dragging || marks.hover == &idleColumn || marks.showsValue (&idleColumn)) hoverBox (g, { 3.0f, 323.0f, 27.0f, 13.0f }, String (proc.idleDb(), 1));

    // the clip LED at its measured place: lit red until it is clicked
    g.setColour (theme::well);
    g.fillRect (280.0f, 186.0f, 15.0f, 10.0f);
    theme::drawLamp (g, { 281.0f, 187.0f, 13.0f, 8.0f }, clip);

    // value boxes
    valueBox (g, { 44.0f, 468.5f, 34.0f, 13.5f }, String (hi - lo, 1));
    valueBox (g, { 143.0f, 468.5f, 34.0f, 13.5f }, String (rider.getValue(), 2));
    valueBox (g, { 241.0f, 468.5f, 34.0f, 13.5f }, String (output.getValue(), 1));

    // the values shown under the mouse (the reference's hover texts)
    if (target.isMouseButtonDown() || marks.showsValue (&target)) hoverBox (g, { 272.0f, 48.0f, 32.0f, 13.0f }, String (target.getValue(), 1));
    // Range Max / Min: the value beside the thumb (measured: a box right of the cap, at its centre)
    for (auto* f : { &rangeMax, &rangeMin })
        if (f->isMouseButtonDown() || marks.showsValue (f))
        {
            const auto cap = f->capBounds().translated ((float) f->getX(), (float) f->getY());
            hoverBox (g, { 88.0f, cap.getCentreY() - 7.0f, 31.0f, 14.0f }, String (f->getValue(), 1));
        }
    const std::pair<Knob*, float> knobs[] = { { &sensitivity, 27.0f }, { &spill, 181.0f }, { &artifacts, 256.0f } };
    for (auto [k, x] : knobs)
        if (k->showing()) hoverBox (g, { x, 157.0f, 37.0f, 13.0f }, String (k->getValue(), 1));

    // the Live components' badge
    if (bl::isLive (proc.modeIndex()))
    {
        const Rectangle<float> b (246.0f, 520.0f, 65.0f, 30.0f);
        g.setColour (theme::raised);
        g.fillRoundedRectangle (b, 3.0f);
        g.setColour (theme::line);
        g.drawRoundedRectangle (b.reduced (0.5f), 3.0f, 1.0f);
        g.setColour (theme::amber);
        g.setFont (arial (13.0f, true));
        g.drawText ("LIVE", b.withHeight (16.0f).translated (0.0f, 1.0f), Justification::centred);
        g.setColour (theme::text);
        g.setFont (arial (10.0f, true));
        g.drawText ("Low Latency", b.withTrimmedTop (15.0f), Justification::centred);
    }
}
