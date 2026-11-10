#include "shared/ui/RenUI.h"
#include "shared/ui/Widgets.h"

using namespace juce;

namespace flatwater::ren
{
namespace theme = flatwater::theme;

Font arial (float h, bool bold) { return Font (FontOptions ("Arial", h, bold ? Font::bold : Font::plain)); }

String dbText (float v, bool infAtMinus80)
{
    if (infAtMinus80 && v <= -79.95f) return "-Inf";
    return String (v, 1);
}

//==============================================================================
Fader::Fader (Kind k, int thumbOffsetX) : kind (k), offsetX (thumbOffsetX)
{
    setSliderStyle (k == Horizontal ? LinearHorizontal : LinearVertical);
    setTextBoxStyle (TextBoxBelow, false, 40, 16);
    setTextBoxIsEditable (true);
    for (auto* c : getChildren())
        if (auto* l = dynamic_cast<Label*> (c)) l->setEditable (false, true);
    setSliderSnapsToMousePosition (false);
    setNumDecimalPlacesToDisplay (1);
    setScrollWheelEnabled (true);
    setMouseDragSensitivity (150);
    setVelocityModeParameters (1.0, 1, 0.0, false);    // Ctrl must not switch to JUCE's velocity mode: it fine-tunes
    applyFormat();
}

void Fader::mouseDown (const MouseEvent& e)
{
    virt = last = downAt = e.position;
    thumbAtDown = getPositionOfValue (getValue());
    e.source.enableUnboundedMouseMovement (true, false);
    Slider::mouseDown (e);
}

void Fader::mouseDrag (const MouseEvent& e)
{
    const auto d = e.position - last;
    last = e.position;
    const float step = std::abs (kind == Horizontal ? d.x : d.y);
    const float k = ctrlHeldNow() ? (step <= 2.0f ? ctrlSlow : ctrlFast) : plainScale;
    virt += d * k;
    Slider::mouseDrag (e.withNewPosition (virt));
}

void Fader::applyFormat()
{
    auto f = format;
    const bool neg = negateTyped;
    textFromValueFunction = [f] (double v) { return f (v); };
    valueFromTextFunction = [neg] (const String& s) { const double v = s.trim().getDoubleValue(); return neg ? -std::abs (v) : v; };
    updateText();
}

void Fader::valueChanged() { Slider::valueChanged(); if (kind == Big) resized(); }

// like the reference: click the value (without dragging) or double-click to type; Alt-click resets
void Fader::mouseUp (const MouseEvent& e)
{
    Slider::mouseUp (e);
    e.source.enableUnboundedMouseMovement (false);
    if (e.mouseWasDraggedSinceMouseDown())
    {
        const auto moved = (float) (getPositionOfValue (getValue()) - thumbAtDown);
        const auto at = downAt + (kind == Horizontal ? juce::Point<float> (moved, 0.0f) : juce::Point<float> (0.0f, moved));
        Desktop::setMousePosition (localPointToGlobal (at).roundToInt());
    }
    if (e.mouseWasClicked() && ! e.mods.isAltDown() && valueTextBounds().contains (e.getPosition()))
        showTextBox();
}

void Fader::mouseDoubleClick (const MouseEvent&) { showTextBox(); }

Rectangle<int> Fader::valueTextBounds() const
{
    for (auto* c : getChildren())
        if (dynamic_cast<Label*> (c) != nullptr) return c->getBounds();
    return {};
}

// only the thumb (and its value text) takes the mouse; clicks on the rest of the track fall through
bool Fader::hitTest (int x, int y)
{
    const juce::Point<int> p (x, y);
    if (valueTextBounds().contains (p)) return true;
    const float pos = (float) getPositionOfValue (getValue());
    if (kind == Big)
        return Rectangle<float> (21.0f, pos - 22.0f, 60.0f, 35.0f).contains (p.toFloat());
    if (kind == Horizontal)
        return p.toFloat().getDistanceFrom ({ pos, 17.5f }) <= 14.0f;
    return p.toFloat().getDistanceFrom ({ 16.5f + (float) offsetX, pos - 3.0f }) <= 14.0f;
}

//==============================================================================
int Look::getSliderThumbRadius (Slider& s)
{
    auto& f = static_cast<Fader&> (s);
    return f.kind == Fader::Big ? 32 : f.kind == Fader::Horizontal ? 16 : 20;
}

Slider::SliderLayout Look::getSliderLayout (Slider& s)
{
    auto& f = static_cast<Fader&> (s);
    Slider::SliderLayout l;
    if (f.kind == Fader::Big)
    {
        l.sliderBounds = { 0, 32, 102, 143 };                      // range of the thumb centre
        const float pos = (float) s.getPositionOfValue (s.getValue());
        l.textBoxBounds = { 26, (int) std::round (pos - 32.0f) + 17, 50, 21 };
    }
    else if (f.kind == Fader::Horizontal)
    {
        l.sliderBounds = { 16, 0, 100, 41 };                       // thumb centre runs along the 100 px track
        l.textBoxBounds = f.textArea;
    }
    else
    {
        l.sliderBounds = { f.offsetX, f.trackTop + 20, 33, 155 };   // range of the thumb centre
        l.textBoxBounds = f.textArea;
    }
    return l;
}

Label* Look::createSliderTextBox (Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    const bool big = static_cast<Fader&> (s).kind == Fader::Big;
    l->setFont (arial (big ? 15.5f : 16.0f));
    l->setColour (Label::textColourId, big ? kValue : kReadout);
    l->setColour (Label::textWhenEditingColourId, theme::text);
    l->setColour (Label::backgroundColourId, Colours::transparentBlack);
    l->setColour (Label::outlineColourId, Colours::transparentBlack);
    l->setColour (Label::backgroundWhenEditingColourId, theme::well);
    l->setColour (Label::outlineWhenEditingColourId, theme::cyan.withAlpha (0.8f));
    l->setColour (TextEditor::backgroundColourId, theme::well);
    l->setColour (TextEditor::textColourId, theme::text);
    l->setColour (TextEditor::highlightColourId, theme::cyan.withAlpha (0.35f));
    l->setColour (TextEditor::highlightedTextColourId, theme::text);
    l->setColour (CaretComponent::caretColourId, theme::cyan);
    l->setBorderSize ({ 0, 0, 0, 0 });
    l->setJustificationType (Justification::centred);
    l->setInterceptsMouseClicks (false, false);
    return l;
}

// a thumb's ring: raised material round a clear middle (the meters behind it show through), a dark edge on both sides;
// amber outside it while the thumb is held
static void ringThumb (Graphics& g, const Path& shape, Rectangle<float> bounds, bool down)
{
    g.setColour (Colours::black.withAlpha (0.4f));
    g.strokePath (shape, PathStrokeType (4.5f), AffineTransform::translation (0.0f, 1.5f));
    g.setColour (theme::line.darker (0.3f));
    g.strokePath (shape, PathStrokeType (4.5f));
    g.setGradientFill (ColourGradient (theme::raisedTop.brighter (0.35f), 0.0f, bounds.getY(), theme::raised, 0.0f, bounds.getBottom(), false));
    g.strokePath (shape, PathStrokeType (2.5f));
    if (down)
    {
        g.setColour (kRingActive);
        g.strokePath (shape, PathStrokeType (1.0f), AffineTransform::scale ((bounds.getWidth() + 5.0f) / bounds.getWidth(),
                                                                           (bounds.getHeight() + 5.0f) / bounds.getHeight(),
                                                                           bounds.getCentreX(), bounds.getCentreY()));
    }
}

void Look::drawLinearSlider (Graphics& g, int, int, int, int, float pos, float, float, Slider::SliderStyle, Slider& s)
{
    auto& f = static_cast<Fader&> (s);
    const bool down = s.isMouseButtonDown();
    if (f.kind == Fader::Big)
    {
        // the capsule round the value, the amber index lines out to the control's edges
        const float top = pos - 32.0f, cy = top + 27.0f;
        const Rectangle<float> pill (22.0f, top + 11.0f, 58.0f, 33.0f), ring = pill.reduced (1.5f);
        g.setColour (kRing);
        g.fillRect (0.0f, cy - 1.0f, pill.getX() + 1.0f, 2.0f);
        g.fillRect (pill.getRight() - 1.0f, cy - 1.0f, 102.0f - pill.getRight() + 1.0f, 2.0f);
        Path p;
        p.addRoundedRectangle (ring, ring.getHeight() * 0.5f);
        ringThumb (g, p, ring, down);
    }
    else
    {
        // a round thumb: the ring and an amber index line across it (along the track's normal)
        const bool horiz = f.kind == Fader::Horizontal;
        const Point<float> c = horiz ? Point<float> (pos, 17.5f) : Point<float> (16.5f + (float) f.offsetX, pos - 3.0f);
        const auto ring = Rectangle<float> (23.0f, 23.0f).withCentre (c);
        Path p;
        p.addEllipse (ring);
        ringThumb (g, p, ring, down);
        g.setColour (kRing);
        if (horiz) g.fillRect (c.x - 1.0f, c.y - 8.5f, 2.0f, 17.0f);
        else       g.fillRect (c.x - 8.5f, c.y - 1.0f, 17.0f, 2.0f);
    }
}

// the window's resize grip: three faint diagonal lines, lighter under the mouse
void Look::drawCornerResizer (Graphics& g, int w, int h, bool isMouseOver, bool isMouseDragging)
{
    const float size = (float) jmin (w, h);
    g.setColour ((isMouseOver || isMouseDragging) ? theme::textDim : theme::textFaint.withAlpha (0.7f));
    for (float k : { 0.25f, 0.5f, 0.75f })
        g.drawLine ((float) w - size * k, (float) h - 1.0f, (float) w - 1.0f, (float) h - size * k, 1.0f);
}

//==============================================================================
void InSwitch::paintButton (Graphics& g, bool hot, bool down)
{
    // the pill (27 x 21 at 3, 1): in, a raised button with its name lit in cyan; out, a dark well with a faint name
    const Rectangle<float> pill (3.0f, 1.0f, 27.0f, 21.0f);
    const bool on = getToggleState();
    g.setColour (Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (pill.translated (0.0f, 1.5f), 9.0f);
    if (on)
    {
        g.setColour (theme::cyan.withAlpha (0.18f));
        g.fillRoundedRectangle (pill.expanded (1.5f), 10.5f);
        g.setGradientFill (ColourGradient (theme::raisedTop, 0.0f, pill.getY(), theme::raised, 0.0f, pill.getBottom(), false));
    }
    else
        g.setColour (theme::well);
    g.fillRoundedRectangle (pill, 9.0f);
    g.setColour (down ? theme::amber : hot ? theme::cyan.withAlpha (0.8f) : on ? theme::cyan.withAlpha (0.55f) : theme::line);
    g.drawRoundedRectangle (pill.reduced (0.5f), 9.0f, 1.0f);
    g.setColour (on ? theme::cyan : theme::textFaint);
    g.setFont (arial (12.0f, true));
    g.drawText ("IN", pill, Justification::centred);
}

//==============================================================================
class Editor::Content : public Component
{
public:
    explicit Content (Editor& e) : ed (e) {}
    void paint (Graphics& g) override { ed.paintContent (g); }
    void mouseDown (const MouseEvent& e) override { ed.contentMouseDown (e); }
    void mouseDrag (const MouseEvent& e) override { ed.contentMouseDrag (e); }
    void mouseUp (const MouseEvent& e) override { ed.contentMouseUp (e); }
    // the controls take clicks on their thumbs only, the wheel over their whole area (as the references)
    void mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w) override { flatwater::forwardWheel (*this, e, w); }
    Editor& ed;
};

Editor::Editor (AudioProcessor& p, AudioProcessorValueTreeState& state, UndoManager& u, RangedAudioParameter* mode,
                const String& product, const String& version, int designW, int designH, flatwater::Toolbar::Style tbStyle,
                const StringArray& modes)
    : AudioProcessorEditor (&p), apvts (state), undo (u), W (designW), H (designH),
      contentComp (std::make_unique<Content> (*this)),
      toolbar (std::make_unique<flatwater::Toolbar> (u, state, modes, mode, product, version, tbStyle)),
      modeParam (mode)
{
    // never take the keyboard focus: the host keeps its shortcuts (space = play / pause) while the window is open
    setWantsKeyboardFocus (false);
    setMouseClickGrabsKeyboardFocus (false);
    contentComp->setWantsKeyboardFocus (false);
    contentComp->setMouseClickGrabsKeyboardFocus (false);
    addAndMakeVisible (*contentComp);
    contentComp->setBounds (0, 0, W, H);
    toolbar->setWantsKeyboardFocus (false);
    toolbar->setMouseClickGrabsKeyboardFocus (false);
    contentComp->addAndMakeVisible (*toolbar);
    toolbar->setTopLeftPosition (0, 0);
    if (tbStyle == flatwater::Toolbar::Style::Hybrid || tbStyle == flatwater::Toolbar::Style::Classic) toolbar->setSize (W, flatwater::Toolbar::hybridHeight);   // the editor's width
    else if (tbStyle == flatwater::Toolbar::Style::TwoRow && W < flatwater::Toolbar::width) toolbar->setSize (W, flatwater::Toolbar::height);   // narrow
    toolbar->getWindowScale = [this] { return (float) getWidth() / (float) W; };
    toolbar->setWindowScale = [this] (float s) { setSize (roundToInt (W * s), roundToInt (H * s)); };

    // resizable, fixed aspect ratio; the size is remembered with the plugin state
    constrainer.setMinimumSize (W * 3 / 4, H * 3 / 4);
    constrainer.setMaximumSize (W * 3, H * 3);
    constrainer.setFixedAspectRatio ((double) W / H);
    setConstrainer (&constrainer);
    setResizable (true, true);
    for (auto* c : getChildren())                                     // the resize grip in the kit's look
        if (dynamic_cast<ResizableCornerComponent*> (c) != nullptr) c->setLookAndFeel (&look);
    // the user's default window size applies in a host; the tests and offline tools (no wrapper) open at design size
    const bool hosted = state.processor.wrapperType != AudioProcessor::wrapperType_Undefined;
    const int savedW = (int) state.state.getProperty ("uiWidth", roundToInt (W * (hosted ? flatwater::defaultWindowScale() : 1.0f)));
    setSize (jlimit (W * 3 / 4, W * 3, savedW), jlimit (H * 3 / 4, H * 3, savedW * H / W));
    startTimerHz (60);   // meter frames; the reference's bars fall smoothly
}

Editor::~Editor()
{
    stopTimer();
    sliderAtts.clear(); buttonAtts.clear(); comboAtts.clear(); paramAtts.clear();
    for (auto* f : faders) f->setLookAndFeel (nullptr);
    for (auto& c : owned) c->setLookAndFeel (nullptr);
    for (auto* c : getChildren()) if (dynamic_cast<ResizableCornerComponent*> (c) != nullptr) c->setLookAndFeel (nullptr);
    owned.clear();
}

Fader& Editor::makeFader (Fader::Kind k, int thumbOffsetX)
{
    owned.push_back (std::make_unique<Fader> (k, thumbOffsetX));
    return static_cast<Fader&> (*owned.back());
}

InSwitch& Editor::makeSwitch()
{
    owned.push_back (std::make_unique<InSwitch>());
    return static_cast<InSwitch&> (*owned.back());
}

void Editor::resized()
{
    contentComp->setTransform (AffineTransform::scale ((float) getWidth() / (float) W));
    apvts.state.setProperty ("uiWidth", getWidth(), nullptr);
}

Component& Editor::content() { return *contentComp; }

void Editor::setDesignHeight (int h)
{
    if (h == H) return;
    const float scale = (float) getWidth() / (float) W;
    H = h;
    contentComp->setBounds (0, 0, W, H);
    constrainer.setMinimumSize (W * 3 / 4, H * 3 / 4);
    constrainer.setMaximumSize (W * 3, H * 3);
    constrainer.setFixedAspectRatio ((double) W / H);
    setSize (getWidth(), roundToInt ((float) H * scale));
}

// As Chan Sampler does (its space bar works in FL Studio): no component of the editor ever takes the keyboard focus,
// recursively, including each fader's value label; keys nothing uses then go on to the host (space = play / pause).
// Typing a value still works: the text box JUCE opens asks for the focus itself.
void Editor::parentHierarchyChanged()
{
    AudioProcessorEditor::parentHierarchyChanged();
    std::function<void (Component&)> noFocus = [&noFocus] (Component& c)
    {
        c.setWantsKeyboardFocus (false);
        c.setMouseClickGrabsKeyboardFocus (false);
        for (auto* child : c.getChildren())
            noFocus (*child);
    };
    noFocus (*this);
}

bool Editor::isMono() const { return modeParam != nullptr && modeParam->getValue() > 0.5f; }

void Editor::attach (Fader& f, const char* paramID, Rectangle<int> bounds, double resetValue)
{
    f.setLookAndFeel (&look);
    f.setWantsKeyboardFocus (false);
    f.setMouseClickGrabsKeyboardFocus (false);
    contentComp->addAndMakeVisible (f);
    f.setBounds (bounds);
    sliderAtts.push_back (std::make_unique<SliderParameterAttachment> (*apvts.getParameter (paramID), f, &undo));
    f.applyFormat();                                      // the attachment installs its own text functions
    f.setDoubleClickReturnValue (true, resetValue);
    faders.push_back (&f);
}

void Editor::addToContent (Component& c, Rectangle<int> bounds)
{
    c.setWantsKeyboardFocus (false);
    c.setMouseClickGrabsKeyboardFocus (false);
    if (auxLook != nullptr) c.setLookAndFeel (auxLook.get());
    contentComp->addAndMakeVisible (c);
    c.setBounds (bounds);
}

void Editor::attachSlider (Slider& s, const char* paramID, Rectangle<int> bounds, double resetValue)
{
    addToContent (s, bounds);
    sliderAtts.push_back (std::make_unique<SliderParameterAttachment> (*apvts.getParameter (paramID), s, &undo));
    s.setDoubleClickReturnValue (true, resetValue);
}

void Editor::attachCombo (ComboBox& c, const char* paramID, Rectangle<int> bounds)
{
    addToContent (c, bounds);
    comboAtts.push_back (std::make_unique<ComboBoxParameterAttachment> (*apvts.getParameter (paramID), c, &undo));
}

void Editor::attach (Button& b, const char* paramID, Rectangle<int> bounds)
{
    b.setWantsKeyboardFocus (false);
    b.setMouseClickGrabsKeyboardFocus (false);
    contentComp->addAndMakeVisible (b);
    b.setBounds (bounds);
    buttonAtts.push_back (std::make_unique<ButtonParameterAttachment> (*apvts.getParameter (paramID), b, &undo));
}

// the background and the panel under the toolbar, a card in the theme
void Editor::paintPanel (Graphics& g) const
{
    g.fillAll (theme::bg);
    const Rectangle<float> panel (1.0f, 36.0f, (float) W - 2.0f, (float) H - 37.0f);
    theme::drawCard (g, panel, 6.0f);
    // our own shading: a little light from the top of the panel
    ColourGradient lift (theme::raisedTop.withAlpha (0.18f), 0.0f, panel.getY(), theme::raisedTop.withAlpha (0.0f), 0.0f, panel.getY() + 140.0f, false);
    g.setGradientFill (lift);
    g.fillRoundedRectangle (panel.reduced (1.0f), 5.0f);
}

// the housing round the centre fader: a raised outer frame, the dark inner frame
void Editor::paintHousing (Graphics& g)
{
    const Rectangle<float> outer (105.0f, 80.0f, 162.0f, 235.0f), inner (133.0f, 108.0f, 106.0f, 181.0f);
    g.setColour (Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (outer.translated (0.0f, 2.0f), 46.0f);
    g.setGradientFill (ColourGradient (theme::panel, 0.0f, outer.getY(), kOuterFill, 0.0f, outer.getCentreY(), false));   // lit at the top, dark below
    g.fillRoundedRectangle (outer, 46.0f);
    g.setColour (kOuterLine);
    g.drawRoundedRectangle (outer.reduced (1.0f), 45.0f, 2.0f);
    g.setColour (theme::raisedTop.withAlpha (0.5f));                   // a lit rim along the top
    g.drawRoundedRectangle (outer.reduced (2.5f), 43.5f, 0.8f);
    g.setColour (kInnerFill);
    g.fillRoundedRectangle (inner, 25.0f);
    g.setColour (kInnerLine);
    g.drawRoundedRectangle (inner, 25.0f, 2.0f);
}

void Editor::paintWindow (Graphics& g, float frac, Colour fill, Colour fillLine, int lines, float firstLineY, float lineStep, bool fromBottom)
{
    const Rectangle<float> win (145.0f, 121.0f, 82.0f, 155.0f);
    Path winPath; winPath.addRoundedRectangle (win, 10.0f);
    g.setColour (kWindow);
    g.fillPath (winPath);
    Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (winPath);
    const float h = jlimit (0.0f, 1.0f, frac) * win.getHeight();
    const auto filled = fromBottom ? win.withTop (win.getBottom() - h) : win.withHeight (h);
    g.setColour (fill);
    g.fillRect (filled);
    if (h > 0.5f)                                                   // the moving edge, bright
    {
        g.setColour (fillLine);
        g.fillRect (win.getX(), fromBottom ? filled.getY() : filled.getBottom() - 1.5f, win.getWidth(), 1.5f);
    }
    for (int i = 0; i < lines; ++i)
    {
        const float y = firstLineY + lineStep * (float) i;
        g.setColour (filled.contains (150.0f, y) ? fillLine.withMultipliedAlpha (0.55f) : kGrid);
        g.fillRect (145.0f, std::round (y), 82.0f, 1.0f);
    }
}

// a capsule meter: a well rounded at both ends, filled from the bottom in the theme's cyan-to-amber
void Editor::paintCapsule (Graphics& g, Rectangle<float> r, float frac)
{
    Path p; p.addRoundedRectangle (r, r.getWidth() * 0.5f);
    g.setColour (kMeterBg);
    g.fillPath (p);
    g.setColour (theme::line);
    g.strokePath (p, PathStrokeType (1.0f));
    const float h = jlimit (0.0f, 1.0f, frac) * r.getHeight();
    if (h <= 0.0f) return;
    Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (p);
    g.setGradientFill (theme::meterGradient (r, true));
    g.fillRect (r.withTop (r.getBottom() - h).reduced (r.getWidth() > 8.0f ? 1.5f : 1.0f, 0.0f));
}

// the plug-in's name plate: the Flatwater mark and the name, centred on the panel above the housing
void Editor::paintTitle (Graphics& g, const String& name, Colour nameColour)
{
    const auto nameFont = theme::font (20.0f, true);
    const float nameW = GlyphArrangement::getStringWidth (nameFont, name);
    const float markW = 30.0f, gap = 7.0f, total = markW + gap + nameW, x0 = 185.5f - total * 0.5f;
    flatwater::drawMark (g, { x0, 47.0f, markW, 26.0f }, false, false);
    g.setColour (nameColour);
    g.setFont (nameFont);
    g.drawText (name, Rectangle<float> (x0 + markW + gap, 46.0f, nameW + 4.0f, 28.0f), Justification::centredLeft);
}

// the clip light (23 x 13 control, a 19 x 11 pill): a dark well, lit red with a soft glow
void Editor::paintClipLight (Graphics& g, Rectangle<float> r, bool lit)
{
    const auto pill = Rectangle<float> (r.getX() + 2.0f, r.getY() + 1.0f, 19.0f, 11.0f);
    if (lit)
    {
        g.setColour (theme::red.withAlpha (0.25f));
        g.fillRoundedRectangle (pill.expanded (2.0f), 7.5f);
    }
    g.setColour (lit ? theme::red : theme::well);
    g.fillRoundedRectangle (pill, 5.5f);
    g.setColour (lit ? theme::red.brighter (0.4f) : theme::line);
    g.drawRoundedRectangle (pill.reduced (0.5f), 5.0f, 1.0f);
}
}
