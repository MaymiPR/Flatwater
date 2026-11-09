#include "shared/ui/FlatUI.h"
using namespace juce;

namespace fw::flat
{
namespace th = fw::theme;

Font font (float h, bool bold) { return Font (FontOptions ("Arial", h, bold ? Font::bold : Font::plain)); }

float gainDbFromNorm (float v)
{
    if (v <= 0.0f) return -100.0f;
    if (v <= 2.0f / 3.0f) return std::max (-100.0f, 40.0f * std::log10 (1.5f * v));
    const float u = 3.0f * v - 2.0f;
    return 8.0f * u + 4.0f * u * u;
}

float gainNormFromDb (float db)
{
    if (db <= -99.95f) return 0.0f;
    if (db <= 0.0f) return std::pow (10.0f, db / 40.0f) / 1.5f;
    const float u = -1.0f + std::sqrt (1.0f + db / 4.0f);
    return (u + 2.0f) / 3.0f;
}

NormalisableRange<float> gainRange (float maxDb)
{
    const float top = gainNormFromDb (maxDb);           // 2/3 for a 0 dB fader, 1 for +12 dB
    return NormalisableRange<float> (-100.0f, maxDb,
        [top] (float, float, float n) { return gainDbFromNorm (n * top); },
        [top] (float, float, float db) { return jlimit (0.0f, 1.0f, gainNormFromDb (db) / top); },
        [] (float, float, float db) { return db <= -99.95f ? -100.0f : std::round (db * 10.0f) / 10.0f; });
}

String gainText (float db) { return db <= -99.95f ? String ("-inf") : String (db, 1); }

void applyFormat (Slider& s, std::function<String (double)> format, bool negateTyped)
{
    s.textFromValueFunction = format;
    s.valueFromTextFunction = [negateTyped] (const String& t)
    {
        const auto x = t.trim();
        if (x.containsIgnoreCase ("inf")) return -100.0;
        const double v = x.getDoubleValue();
        return negateTyped ? -std::abs (v) : v;
    };
    s.updateText();
}

//==============================================================================
// Value entry as in the reference (measured with tools/MeterCapture click / dbl scripts): a double-click on a knob,
// fader or the Freq slider opens an entry box over the value with the whole value selected; Enter applies it.
// A single click on the value does nothing special, Alt-click resets to the default.
static Rectangle<int> editAreaOf (Component* c)
{
    if (auto* k = dynamic_cast<Knob*> (c)) return k->editArea;
    if (auto* f = dynamic_cast<VFader*> (c)) return f->editArea;
    if (auto* h = dynamic_cast<HSlider*> (c)) return h->editArea;
    return {};
}

struct ValueEditStyle : public Label::Listener
{
    void labelTextChanged (Label*) override {}
    void editorShown (Label* l, TextEditor& ed) override
    {
        auto* parent = l->getParentComponent();
        const auto r = editAreaOf (parent);
        if (! r.isEmpty()) l->setBounds (r);
        if (dynamic_cast<HSlider*> (parent) != nullptr)                // the reference shows the bare number ("80")
            ed.setText (ed.getText().upToFirstOccurrenceOf (" ", false, false), false);
        ed.setJustification (Justification::centredLeft);
        ed.applyFontToAllText (font (dynamic_cast<VFader*> (parent) != nullptr ? 15.0f : 13.0f));   // measured per control
        ed.setIndents (1, 0);
        ed.selectAll();
    }
    void editorHidden (Label* l, TextEditor&) override
    {
        if (auto* s = dynamic_cast<Slider*> (l->getParentComponent())) s->resized();   // back to the value layout
    }
};

static ValueEditStyle& valueEditStyle() { static ValueEditStyle s; return s; }

static bool resetOnAlt (Slider& s, const MouseEvent& e)
{
    if (! e.mods.isAltDown() || ! s.isDoubleClickReturnEnabled()) return false;
    s.setValue (s.getDoubleClickReturnValue(), sendNotificationSync);
    return true;
}

//==============================================================================
void Selection::repaintAll()
{
    for (auto* m : members) m->repaint();
}

void Selection::select (Component* c)
{
    if (contains (c)) { if (primary != c) { primary = c; repaintAll(); } return; }
    repaintAll();
    members = { c };
    primary = c;
    repaintAll();
}

void Selection::setGroup (std::vector<Component*> list)
{
    if (list == members) return;
    repaintAll();
    members = std::move (list);
    primary = members.empty() ? nullptr : members.front();
    repaintAll();
}

void Selection::clear() { setGroup ({}); }

bool Selection::contains (const Component* c) const
{
    return c != nullptr && std::find (members.begin(), members.end(), c) != members.end();
}

void LassoOverlay::paint (Graphics& g)
{
    if (rect.isEmpty()) return;
    g.setColour (th::text.withAlpha (0.9f));                        // 1 px, light
    g.drawRect (rect.translated (-getX(), -getY()), 1);
}

void drawSelectRing (Graphics& g, Point<float> c, float radius, Colour colour)
{
    constexpr int n = 96;
    for (int i = 0; i < n; ++i)
    {
        const float a0 = MathConstants<float>::twoPi * (float) i / (float) n, a1 = MathConstants<float>::twoPi * (float) (i + 1) / (float) n;
        const float w = std::pow (std::abs (std::sin ((a0 + a1) * 0.5f)), 4.0f);   // angles from 12 o'clock; 0.2 at 45 degrees
        if (w < 0.01f) continue;
        Path seg;
        seg.addCentredArc (c.x, c.y, radius, radius, 0.0f, a0, a1 + 0.01f, true);
        g.setColour (colour.withAlpha (w));
        g.strokePath (seg, PathStrokeType (1.3f));
    }
}

//==============================================================================
bool WavesDrag::down (Slider& s, const MouseEvent& e, Rectangle<int>, Selection* sel)
{
    if (e.mods.isPopupMenu()) return false;
    if (resetOnAlt (s, e)) return true;
    active = true;
    virt = last = e.position;
    startValue = s.getValue(); stepAcc = 0.0;
    e.source.enableUnboundedMouseMovement (true, false);         // hide the cursor, no screen edge
    s.Slider::mouseDown (e);
    ganged = sel != nullptr && sel->grouped() && sel->contains (&s);
    if (ganged && sel->gangBegin) sel->gangBegin (&s);
    return true;
}

void WavesDrag::drag (Slider& s, const MouseEvent& e, Selection* sel)
{
    const double before = s.valueToProportionOfLength (s.getValue());
    const auto d = e.position - last;
    last = e.position;
    float k = 1.0f;
    if (kind == Rotary)
    {
        const float step = std::abs (std::abs (d.y) > std::abs (d.x) ? d.y : d.x);
        const bool slow = step <= slowMaxStep, ctrl = ctrlHeldNow();
        if (stepPx[0] > 0.0f && s.getInterval() > 0.0)      // stepped: whole steps from where the drag began
        {
            stepAcc += -d.y / stepPx[(ctrl ? 2 : 0) + (slow ? 0 : 1)];
            const double whole = std::trunc (stepAcc + (stepAcc >= 0.0 ? 1.0e-6 : -1.0e-6));   // 100 x 1/50 is 1.99999...
            const double v = jlimit (s.getMinimum(), s.getMaximum(), startValue + whole * s.getInterval());
            if (v != s.getValue()) s.setValue (v, sendNotificationSync);
            const double moved = s.valueToProportionOfLength (s.getValue()) - before;
            if (ganged && sel != nullptr && sel->gangMove && std::abs (moved) > 1.0e-9)
                sel->gangMove (&s, (float) moved * referenceScale);
            return;
        }
        k = ctrl ? (ctrlScalesSpeed ? ctrlFactor * (slow ? 0.5f : 1.0f) : ctrlFactor) : slow ? 0.5f : 1.0f;
    }
    else if (ctrlHeldNow())
        k = std::abs (std::abs (d.y) > std::abs (d.x) ? d.y : d.x) <= 2.0f ? linCtrlSlow : linCtrlFast;
    virt += d * k;
    s.Slider::mouseDrag (e.withNewPosition (virt));
    const double step = s.valueToProportionOfLength (s.getValue()) - before;
    if (ganged && sel != nullptr && sel->gangMove && std::abs (step) > 1.0e-9)
        sel->gangMove (&s, (float) step * referenceScale);
}

bool WavesDrag::up (Slider& s, const MouseEvent& e, Point<float> restoreAt, Selection* sel)
{
    if (! active) return false;
    active = false;
    s.Slider::mouseUp (e.withNewPosition (virt));
    if (ganged && sel != nullptr && sel->gangEnd) sel->gangEnd (&s);
    ganged = false;
    e.source.enableUnboundedMouseMovement (false);
    Desktop::setMousePosition (s.localPointToGlobal (restoreAt).roundToInt());   // the cursor comes back on the control
    return true;
}

static void setUpFeel (Slider& s, int pixelsForFullRange)
{
    s.setMouseDragSensitivity (pixelsForFullRange);
    s.setVelocityModeParameters (1.0, 1, 0.0, false);              // Ctrl / Alt must not switch to JUCE's velocity mode
}

//==============================================================================
VFader::VFader (Colour line) : lineColour (line)
{
    setSliderStyle (LinearVertical);
    setTextBoxStyle (TextBoxBelow, false, 31, 18);
    setSliderSnapsToMousePosition (false);
    setUpFeel (*this, 165);
    setScrollWheelEnabled (true);
}

bool VFader::hitTest (int x, int y)
{
    const float pos = (float) getPositionOfValue (getValue());
    return textArea.contains (x, y) || Rectangle<float> (0.0f, pos - 16.0f, (float) getWidth(), 32.0f).contains ((float) x, (float) y);
}

void VFader::mouseDown (const MouseEvent& e)
{
    if (selection != nullptr) selection->select (this);
    if (! feel.down (*this, e, textArea, selection)) Slider::mouseDown (e);
}
void VFader::mouseDoubleClick (const MouseEvent&) { showTextBox(); }
void VFader::mouseDrag (const MouseEvent& e) { if (feel.active) feel.drag (*this, e, selection); else Slider::mouseDrag (e); }
void VFader::mouseUp (const MouseEvent& e)
{
    if (feel.up (*this, e, { (float) getWidth() * 0.5f, (float) getPositionOfValue (getValue()) }, selection)) return;
    Slider::mouseUp (e);
}

// the thumb: a raised cap with an index line in the fader's colour across its middle, grip lines above and below it
static void drawThumb (Graphics& g, Rectangle<float> r, Colour index)
{
    g.setColour (Colours::black.withAlpha (0.4f));
    g.fillRoundedRectangle (r.translated (0.0f, 2.0f), 2.5f);
    g.setGradientFill (ColourGradient (th::raisedTop, 0.0f, r.getY(), th::raised, 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r, 2.5f);
    g.setColour (th::line);
    g.drawRoundedRectangle (r.reduced (0.5f), 2.5f, 1.0f);
    const float cy = r.getCentreY();
    g.setColour (th::line.brighter (0.3f));
    for (float dy : { -9.0f, -6.0f, 6.0f, 9.0f })
        g.fillRect (r.getX() + 4.0f, std::round (cy + dy) - 0.5f, r.getWidth() - 8.0f, 1.0f);
    g.setColour (index);
    g.fillRoundedRectangle (r.getCentreX() - 6.0f, cy - 1.0f, 12.0f, 2.0f, 1.0f);
}

void VFader::paint (Graphics& g)
{
    const float cx = (float) getWidth() * 0.5f;
    th::drawWell (g, { cx - 2.0f, (float) travelTop - 2.0f, 4.0f, (float) (travelBottom - travelTop) + 4.0f }, 2.0f);
    const float pos = (float) getPositionOfValue (getValue());
    const Rectangle<float> thumb (cx - 8.5f, pos - 14.0f, 17.0f, 28.0f);
    const bool marked = selection != nullptr && selection->contains (this);
    if (marked)
    {
        // a selected thumb: its index colour lights the cap, a short bar under it in amber (the primary) or cyan (a member)
        g.setColour (lineColour.withAlpha (0.18f));
        g.fillRoundedRectangle (thumb.expanded (2.0f), 4.0f);
    }
    drawThumb (g, thumb, lineColour);
    if (marked)
    {
        const Colour bc = selection->is (this) ? kSelect : kMember;
        ColourGradient bar (bc.withAlpha (0.0f), cx - 8.0f, 0.0f, bc.withAlpha (0.0f), cx + 8.0f, 0.0f, false);
        bar.addColour (0.5, bc);
        g.setGradientFill (bar);
        g.fillRect (cx - 8.0f, pos + 16.0f, 16.0f, 2.0f);
    }
}

//==============================================================================
Knob::Knob (String l) : label (std::move (l))
{
    setSliderStyle (RotaryVerticalDrag);
    setTextBoxStyle (TextBoxBelow, false, 54, 14);
    setUpFeel (*this, 500);
    setScrollWheelEnabled (true);
    setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    // Ctrl: a quarter of the rate, keeping the slow / fast difference (measured on MaxxBass, as H-Comp's)
    feel.ctrlFactor = 0.25f;
    feel.ctrlScalesSpeed = true;
}

void Knob::mouseDown (const MouseEvent& e)
{
    if (selection != nullptr) selection->select (this);
    if (! feel.down (*this, e, textArea, selection)) Slider::mouseDown (e);
}
void Knob::mouseDoubleClick (const MouseEvent&) { showTextBox(); }
void Knob::mouseDrag (const MouseEvent& e) { if (feel.active) feel.drag (*this, e, selection); else Slider::mouseDrag (e); }
void Knob::mouseUp (const MouseEvent& e)
{
    if (feel.up (*this, e, centre, selection)) return;
    Slider::mouseUp (e);
}

void Knob::paint (Graphics& g)
{
    const auto c = centre;
    g.setColour (th::textFaint);
    g.fillEllipse (c.x - 1.3f, 9.5f, 2.6f, 2.6f);                                 // centre-position dot
    const auto rp = getRotaryParameters();
    const float frac = (float) valueToProportionOfLength (getValue());
    const float a0 = rp.startAngleRadians, a = a0 + frac * (rp.endAngleRadians - rp.startAngleRadians);
    th::drawKnob (g, c, radius - 3.5f, a, a0, rp.endAngleRadians, a0);           // its arc on the ring (radius)
    if (selection != nullptr && selection->contains (this)) drawSelectRing (g, c, radius + 4.0f, selection->is (this) ? kSelect : kMember);

    g.setColour (kLabel);
    g.setFont (font (12.0f));
    g.drawText (label, Rectangle<int> (0, 51, getWidth(), 16), Justification::centred);
    g.setColour (th::line);
    g.fillRect (2.0f, 67.0f, (float) getWidth() - 4.0f, 1.0f);
}

//==============================================================================
HSlider::HSlider()
{
    setSliderStyle (LinearHorizontal);
    setTextBoxStyle (TextBoxRight, false, 50, 15);      // only shown while typing a value (the value is painted on hover)
    setSliderSnapsToMousePosition (false);
    setUpFeel (*this, 264);
    setScrollWheelEnabled (true);
    feel.linCtrlFast = 0.234f; feel.linCtrlSlow = 0.067f;   // measured on MaxxBass's Freq (finer than its faders)
}

void HSlider::mouseDown (const MouseEvent& e)
{
    if (selection != nullptr) selection->select (this);
    if (! feel.down (*this, e, {}, selection)) Slider::mouseDown (e);
}
void HSlider::mouseDoubleClick (const MouseEvent&) { showTextBox(); }
void HSlider::mouseDrag (const MouseEvent& e) { if (feel.active) feel.drag (*this, e, selection); else Slider::mouseDrag (e); }
void HSlider::mouseUp (const MouseEvent& e)
{
    if (! feel.up (*this, e, { (float) getPositionOfValue (getValue()), y }, selection)) Slider::mouseUp (e);
}

void HSlider::paint (Graphics& g)
{
    g.setColour (kTick);
    for (float t : tickPositions)
        g.fillRect (std::round (x0 + t * (x1 - x0)), y - 13.0f, 1.0f, 8.0f);
    const float pos = (float) getPositionOfValue (getValue());
    th::drawWell (g, { x0, y - 2.5f, x1 - x0, 5.0f }, 2.5f);
    g.setColour (th::cyan);
    g.fillRoundedRectangle (x0 + 1.0f, y - 1.0f, jmax (0.0f, pos - x0 - 1.0f), 2.0f, 1.0f);
    const auto knob = Rectangle<float> (10.0f, 10.0f).withCentre ({ pos, y });
    g.setColour (Colours::black.withAlpha (0.4f));
    g.fillEllipse (knob.translated (0.0f, 1.0f));
    g.setGradientFill (ColourGradient (th::raisedTop, 0.0f, knob.getY(), th::raised, 0.0f, knob.getBottom(), false));
    g.fillEllipse (knob);
    g.setColour (th::amber);
    g.drawEllipse (knob.reduced (0.7f), 1.5f);
    if (selection != nullptr && selection->is (this)) drawSelectRing (g, { pos, y }, 6.2f);
    if ((isMouseOverOrDragging() || isMouseButtonDown()) && format && ! textArea.isEmpty())
    {
        g.setColour (kValue);
        g.setFont (font (12.0f));
        g.drawText (format (getValue()), textArea, Justification::centredLeft);
    }
}

//==============================================================================
void drawSlopeIcon (Graphics& g, Rectangle<float> r, int slope)
{
    // three pictures: a straight 6 dB/oct ramp, a curved 12 dB/oct knee, a steep 24 dB/oct edge, each running into a
    // flat top
    Path p;
    const float x = r.getX(), y = r.getY(), w = r.getWidth(), h = r.getHeight();
    const float top = y + 0.5f, bottom = y + h - 0.5f;
    if (slope == 0)      { p.startNewSubPath (x + 0.5f, bottom); p.lineTo (x + w * 0.44f, top); }
    else if (slope == 1) { p.startNewSubPath (x + 0.5f, bottom); p.quadraticTo (x + w * 0.2f, top + h * 0.12f, x + w * 0.44f, top); }
    else                 { p.startNewSubPath (x + w * 0.12f, bottom); p.lineTo (x + w * 0.28f, top); }
    p.lineTo (x + w - 0.5f, top);
    g.setColour (th::cyan.withAlpha (0.18f));                                     // a soft glow
    g.strokePath (p, PathStrokeType (4.0f, PathStrokeType::mitered, PathStrokeType::rounded));
    g.setColour (th::cyan);
    g.strokePath (p, PathStrokeType (1.7f, PathStrokeType::mitered, PathStrokeType::butt));
}

void SlopeBox::mouseDown (const MouseEvent& e)
{
    if (selection != nullptr) selection->select (this);
    stepping = (float) e.x < box.getRight() - (float) arrowWidth && getNumItems() > 0;
    if (stepping)
        setSelectedItemIndex ((getSelectedItemIndex() + 1) % getNumItems(), sendNotificationSync);
    else
        ComboBox::mouseDown (e);
}

void SlopeBox::mouseUp (const MouseEvent& e)
{
    if (! stepping) ComboBox::mouseUp (e);
    stepping = false;
}

void SlopeBox::paint (Graphics& g)
{
    const auto r = box;
    if (selection != nullptr && selection->contains (this))
    {
        // the selection: a 1.5 px outline 2.5 px outside the box, amber on the primary, cyan on a group member
        const auto o = r.expanded (2.5f);
        g.setColour (selection->is (this) ? kSelect : kMember);
        g.drawRoundedRectangle (o, o.getHeight() * 0.5f, 1.5f);
    }
    g.setGradientFill (ColourGradient (th::raisedTop, 0.0f, r.getY(), th::raised, 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (th::line.brighter (0.2f));
    g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);
    drawSlopeIcon (g, { 11.0f, 10.0f, 16.0f, 9.0f }, jmax (0, getSelectedItemIndex()));
    g.setColour (th::line);
    g.fillRect (r.getRight() - 20.0f, r.getY() + 4.0f, 1.0f, r.getHeight() - 8.0f);
    Path tri;
    const float ax = r.getRight() - 10.5f, ay = r.getCentreY();
    tri.addTriangle (ax - 3.0f, ay - 2.0f, ax + 3.0f, ay - 2.0f, ax, ay + 2.5f);
    g.setColour (th::textDim);
    g.fillPath (tri);
}

//==============================================================================
RadioColumn::RadioColumn (RangedAudioParameter& p, UndoManager* um, StringArray l, std::vector<Colour> a)
    : labels (std::move (l)), accents (std::move (a))
{
    attachment = std::make_unique<ParameterAttachment> (p, [this] (float v) { selected = roundToInt (v); repaint(); }, um);
    attachment->sendInitialUpdate();
}

void RadioColumn::paint (Graphics& g)
{
    for (int i = 0; i < labels.size(); ++i)
    {
        const Rectangle<float> r (0.5f, (float) (i * rowHeight) + 0.5f, (float) getWidth() - 1.0f, (float) buttonHeight - 1.0f);
        const bool on = i == selected;
        const auto acc = accents[(size_t) i];
        if (on)
        {
            g.setColour (acc.withAlpha (0.18f));                                    // a soft glow
            g.fillRoundedRectangle (r.expanded (1.5f), 5.5f);
            g.setGradientFill (ColourGradient (th::raisedTop, 0.0f, r.getY(), th::raised, 0.0f, r.getBottom(), false));
            g.fillRoundedRectangle (r, 4.0f);
            g.setColour (acc.withAlpha (0.85f));
            g.drawRoundedRectangle (r, 4.0f, 1.2f);
        }
        else
        {
            g.setColour (th::well);
            g.fillRoundedRectangle (r, 4.0f);
            g.setColour (th::line);
            g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.0f);
        }
        g.setColour (on ? acc : th::textFaint);
        g.setFont (font (11.0f));
        g.drawText (labels[i], r, Justification::centred);
    }
    if (selection != nullptr && selection->contains (this))
    {
        // a 1 px line under the buttons, amber (cyan in a group), fading as a cosine to both ends
        const Colour y = selection->is (this) ? kSelect : kMember;
        const float c = (float) getWidth() * 0.5f, half = c - 2.0f;
        for (int x = 0; x < getWidth(); ++x)
        {
            const float d = std::abs ((float) x + 0.5f - c) / half;
            if (d >= 1.0f) continue;
            g.setColour (y.withAlpha (std::cos (d * MathConstants<float>::halfPi)));
            g.fillRect ((float) x, (float) selectLineY, 1.0f, 1.0f);
        }
    }
}

void RadioColumn::mouseDown (const MouseEvent& e)
{
    if (selection != nullptr) selection->select (this);
    const int i = e.y / rowHeight;
    if (i >= 0 && i < labels.size() && e.y - i * rowHeight < buttonHeight)
        attachment->setValueAsCompleteGesture ((float) i);
}

//==============================================================================
Slider::SliderLayout FlatLook::getSliderLayout (Slider& s)
{
    Slider::SliderLayout l;
    if (auto* f = dynamic_cast<VFader*> (&s))
    {
        l.sliderBounds = { 0, f->travelTop, s.getWidth(), f->travelBottom - f->travelTop };
        l.textBoxBounds = f->textArea;
    }
    else if (auto* k = dynamic_cast<Knob*> (&s))
    {
        l.sliderBounds = s.getLocalBounds();
        l.textBoxBounds = k->textArea;
    }
    else if (auto* h = dynamic_cast<HSlider*> (&s))
    {
        l.sliderBounds = { (int) h->x0, 0, (int) (h->x1 - h->x0), s.getHeight() };
        l.textBoxBounds = h->editArea;
    }
    else
        l = LookAndFeel_V4::getSliderLayout (s);
    return l;
}

Label* FlatLook::createSliderTextBox (Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (font (11.5f));
    l->setColour (Label::textColourId, kValue);
    l->setColour (Label::backgroundColourId, Colours::transparentBlack);
    l->setColour (Label::outlineColourId, Colours::transparentBlack);
    l->setBorderSize ({ 0, 0, 0, 0 });
    l->setJustificationType (Justification::centred);
    l->setInterceptsMouseClicks (false, true);                    // the entry box itself takes clicks while typing
    // the entry box: a well, light text, a 1 px frame in the line colour, the selection in cyan
    l->setColour (Label::backgroundWhenEditingColourId, th::well);
    l->setColour (Label::textWhenEditingColourId, th::text);
    l->setColour (Label::outlineWhenEditingColourId, Colours::transparentBlack);
    l->setColour (TextEditor::backgroundColourId, th::well);
    l->setColour (TextEditor::textColourId, th::text);
    l->setColour (TextEditor::highlightColourId, th::cyan.withAlpha (0.4f));
    l->setColour (TextEditor::highlightedTextColourId, th::text);
    l->setColour (TextEditor::outlineColourId, th::cyan.withAlpha (0.7f));
    l->setColour (TextEditor::focusedOutlineColourId, th::cyan.withAlpha (0.7f));
    l->setColour (CaretComponent::caretColourId, th::cyan);
    if (dynamic_cast<HSlider*> (&s) != nullptr) l->setColour (Label::textColourId, Colours::transparentBlack);   // hidden until typing
    l->addListener (&valueEditStyle());
    return l;
}

PopupMenu::Options FlatLook::getOptionsForComboBoxPopupMenu (ComboBox& box, Label& label)
{
    return LookAndFeel_V4::getOptionsForComboBoxPopupMenu (box, label).withStandardItemHeight (20).withMinimumWidth (130);
}

void FlatLook::positionComboBoxText (ComboBox& box, Label& label)
{
    if (dynamic_cast<SlopeBox*> (&box) != nullptr) { label.setBounds ({}); return; }   // the slope box shows a picture, no text
    LookAndFeel_V4::positionComboBoxText (box, label);
}

void FlatLook::fillTextEditorBackground (Graphics& g, int w, int h, TextEditor& ed)
{
    g.setColour (ed.findColour (TextEditor::backgroundColourId));
    g.fillRect (0, 0, w, h);
}

void FlatLook::drawTextEditorOutline (Graphics& g, int w, int h, TextEditor& ed)
{
    g.setColour (ed.findColour (TextEditor::outlineColourId));
    g.fillRect (0, 0, 1, h);
    g.fillRect (w - 1, 0, 1, h);
}

void FlatLook::drawPopupMenuBackground (Graphics& g, int w, int h)
{
    g.fillAll (th::panel);
    g.setColour (th::line);
    g.drawRect (0, 0, w, h, 1);
}

void FlatLook::drawPopupMenuItem (Graphics& g, const Rectangle<int>& area, bool isSeparator, bool isActive, bool isHighlighted,
                                  bool isTicked, bool, const String& text, const String&, const Drawable*, const Colour*)
{
    if (isSeparator) { g.setColour (th::line); g.fillRect (area.reduced (6, 0).withHeight (1).withY (area.getCentreY())); return; }
    if (isHighlighted && isActive) { g.setColour (th::raised); g.fillRect (area); }
    if (isTicked)
    {
        Path tick;
        const float cx = (float) area.getX() + 12.0f, cy = (float) area.getCentreY();
        tick.startNewSubPath (cx - 4.0f, cy); tick.lineTo (cx - 1.0f, cy + 3.0f); tick.lineTo (cx + 4.0f, cy - 3.5f);
        g.setColour (th::cyan);
        g.strokePath (tick, PathStrokeType (1.6f, PathStrokeType::curved, PathStrokeType::rounded));
    }
    auto textArea = area.withTrimmedLeft (24);
    if (text.endsWith ("dB/oct"))                  // slope entries show their picture
    {
        const int v = text.getIntValue();
        drawSlopeIcon (g, Rectangle<float> ((float) textArea.getX(), (float) area.getCentreY() - 4.5f, 13.0f, 9.0f),
                       v <= 6 ? 0 : v <= 12 ? 1 : 2);
        textArea = textArea.withTrimmedLeft (20);
    }
    g.setColour (! isActive ? th::textFaint : isHighlighted ? th::text : th::textDim);
    g.setFont (font (13.0f));
    g.drawText (text, textArea, Justification::centredLeft);
}

//==============================================================================
void drawMeterBar (Graphics& g, Rectangle<float> t, float frac)
{
    drawMeterBarTo (g, t, frac <= 0.0f ? t.getBottom() : t.getBottom() - t.getHeight() * jlimit (0.0f, 1.0f, frac));
}

void drawMeterBarTo (Graphics& g, Rectangle<float> t, float topY)
{
    g.setColour (kTrack);
    g.fillRoundedRectangle (t, t.getWidth() * 0.5f);
    g.setColour (th::line);
    g.drawRoundedRectangle (t.expanded (0.5f), t.getWidth() * 0.5f + 0.5f, 1.0f);
    topY = jlimit (t.getY(), t.getBottom(), topY);
    if (topY >= t.getBottom()) return;
    const auto bar = t.withTop (topY);
    g.setColour (th::cyan.withAlpha (0.12f));                                    // a soft glow
    g.fillRoundedRectangle (bar.expanded (1.5f), (t.getWidth() + 3.0f) * 0.5f);
    g.setGradientFill (th::meterGradient (t, true));                            // cyan at the foot, amber at full scale
    g.fillRoundedRectangle (bar, t.getWidth() * 0.5f);
}

void drawBand (Graphics& g, const std::vector<Point<float>>& pts, float bottomY, Colour c)
{
    if (pts.size() < 2) return;
    Path edge, fill;
    edge.startNewSubPath (pts.front());
    for (size_t i = 1; i < pts.size(); ++i) edge.lineTo (pts[i]);
    fill = edge;
    fill.lineTo (pts.back().x, bottomY);
    fill.lineTo (pts.front().x, bottomY);
    fill.closeSubPath();
    float top = bottomY;
    for (auto& p : pts) top = std::min (top, p.y);
    ColourGradient grad (c.withAlpha (0.64f), 0.0f, top, c.withAlpha (0.14f), 0.0f, bottomY, false);   // fades toward the bottom
    g.setGradientFill (grad);
    g.fillPath (fill);
    g.setColour (c.withAlpha (0.3f));
    g.strokePath (edge, PathStrokeType (3.0f));
    g.setColour (c.brighter (0.15f));
    g.strokePath (edge, PathStrokeType (1.4f));
}
}
