#pragma once
#include "FwVocalChainProcessor.h"
#include "FwRenUI.h"
#include "FwFlatUI.h"

// Flatwater Vocal Chain editor in Flatwater's own look (FwTheme.h): the original's layout on a 738 x 474 px design grid
// (the toolbar's strip included), its measured positions and sizes kept, everything drawn in code. The static art
// (cards, side cheeks, the grille, the stages' window, slots, scales, screws, labels, the name plate and the mark) is
// rendered once per window scale and component; the controls draw themselves.
namespace fwvocalchain::ui
{
// the selection as filmed on the reference (MeterCapture gl=1 click / hover / drag scripts, RESEARCH.md): hovering
// shows nothing; a click on a control (a fader's cap, a colour's push button, the Balance knob) marks it (yellow) until
// a click elsewhere: on another control, the background, a meter, a flavour's name or a fader's track off its cap;
// Shift-click on the marked control unmarks it; there is no lasso. The marked control's value shows below it while the
// mouse is on it (and while it is dragged).
struct Marks
{
    juce::Component* selected = nullptr;
    juce::Component* hover = nullptr;
    void select (juce::Component* c) { if (c != selected) { selected = c; if (changed) changed(); } }
    void clear() { select (nullptr); }
    bool showsValue (const juce::Component* c) const { return c != nullptr && c == selected && c == hover; }
    std::function<void()> changed;
    std::function<void (juce::Slider*)> enter;                    // a double-click: type a value
};

inline const juce::Colour activeColour { 0xffffaa1e };             // the marked control's frame (the theme's amber)

// a channel fader: the cap (34 x 58) on its track, its shadow under it; a click on the cap marks it, Alt-click resets,
// a drag moves the cap with the mouse (Ctrl fine) with the cursor hidden and put back on the cap, a double-click
// types a value; a click on the track off the cap does nothing but unmark
class Fader : public juce::Slider
{
public:
    Fader (bool sensitivityCap, Marks&);
    void paint (juce::Graphics&) override;
    void mouseEnter (const juce::MouseEvent&) override { marks.hover = this; if (marks.changed) marks.changed(); }
    void mouseExit (const juce::MouseEvent&) override { if (marks.hover == this) marks.hover = nullptr; if (marks.changed) marks.changed(); }
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    float capTop() const;                                            // the cap's top in local px (the control is 43 x 215)
    bool onCap (juce::Point<float> p) const { return p.x >= 0.0f && p.x < 34.0f && p.y >= capTop() && p.y < capTop() + 58.0f; }
    // a value the user set (Alt-click, typed or with the wheel) as one gesture of its own
    void userSet (double v);
    bool dragging = false;
    // the user's own moves (not the host's, a preset's or undo's): the value moves from -> to (from: where the
    // gesture began); the editor's In/Out link follows them
    std::function<void (double from, double to)> onUserMove;

private:
    const bool sens;
    Marks& marks;
    double start = 0.0, wheelAcc = 0.0;
    juce::Point<float> origin, last, virt;
};

// a stage's colour: the names of its three flavours in the window (the selected one lit in cyan; the editor draws
// the push button under them, lit amber while a colour is on); the push button steps through the flavours and marks
// the control; a click on a flavour's name selects it, on the selected one turns the stage's colour off (the
// reference's clear position)
class ColourSelector : public juce::Component
{
public:
    ColourSelector (juce::RangedAudioParameter& param, const Stage& stage, Marks&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void setState (int s);
    int state() const;

private:
    juce::RangedAudioParameter& p;
    const Stage& stage;
    Marks& marks;
};

// the input or output meter (stereo bars, or one bar on the Mono/Stereo component's input) with its clip lights; a
// click clears the clip lights (and unmarks the marked control, as a click on the background)
class Meter : public juce::Component
{
public:
    explicit Meter (Marks& m) : marks (m) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { clip[0] = clip[1] = false; marks.clear(); repaint(); }
    void set (float l, float r, double dt);
    bool mono = false;
    float shown[2] { -80.0f, -80.0f };
    bool clip[2] { false, false };
    fw::MeterBallistics ballistics[2];

private:
    Marks& marks;
};

// the Sensitivity LED over the input meter: off, then cyan, amber and red as the level into the chain rises
class SensLed : public juce::Component
{
public:
    explicit SensLed (Marks& m) : marks (m) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { marks.clear(); }
    void set (float levelDb);
    int frame = 0;

private:
    Marks& marks;
};

// Flatwater's own In/Out link (not in the reference): a push button in a pocket at the top of the Output fader's track,
// the mirror of the Sensitivity LED's on the input side, lit amber while Sensitivity and Output are linked; a click
// toggles it (and unmarks the marked control)
class LinkButton : public juce::Button
{
public:
    explicit LinkButton (Marks& m) : juce::Button ("Link In/Out"), marks (m) { setClickingTogglesState (true); }
    void paintButton (juce::Graphics&, bool, bool) override;
    void clicked() override { marks.clear(); }
    using juce::Button::clicked;

private:
    Marks& marks;
};

// the Balance knob (Stereo component only): a click marks it (an amber ring round its cap), a vertical drag turns it
// (the reference's knob rate: 1/1000 of the range per px on small mouse steps) with the cursor hidden and put back
// where the drag began, Alt-click resets, a double-click types a value
class BalanceKnob : public juce::Slider
{
public:
    explicit BalanceKnob (Marks&);
    void paint (juce::Graphics&) override;
    void mouseEnter (const juce::MouseEvent&) override { marks.hover = this; if (marks.changed) marks.changed(); }
    void mouseExit (const juce::MouseEvent&) override { if (marks.hover == this) marks.hover = nullptr; if (marks.changed) marks.changed(); }
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent& e) override { feel.drag (*this, e, nullptr); }
    void mouseUp (const juce::MouseEvent& e) override { feel.up (*this, e, e.mouseDownPosition, nullptr); }
    void mouseDoubleClick (const juce::MouseEvent&) override { if (marks.enter) marks.enter (this); }
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override { fw::wavesWheel (*this, e, w, wheelAcc, false); }
    bool dragging() const { return feel.active; }

private:
    Marks& marks;
    fw::flat::WavesDrag feel { fw::flat::WavesDrag::Rotary };
    double wheelAcc = 0.0;
};
}

class FwVocalChainEditor : public fw::ren::Editor
{
public:
    explicit FwVocalChainEditor (FwVocalChainProcessor&);
    ~FwVocalChainEditor() override;
    static constexpr int W = 738, H = 474;

    // the value box under a control (a raised box in an amber frame), shown while the marked control is under the
    // mouse or being dragged
    juce::Rectangle<int> valueBox (const juce::Slider*) const;
    fwvocalchain::ui::Marks marks;

protected:
    void paintContent (juce::Graphics&) override;
    void contentMouseDown (const juce::MouseEvent&) override { marks.clear(); }
    void tick() override;

private:
    void showEntry (juce::Slider*);
    void layoutForMode();
    void linkFaders (fwvocalchain::ui::Fader& mine, fwvocalchain::ui::Fader& other);

    FwVocalChainProcessor& proc;
    std::vector<fwvocalchain::ui::Fader*> faders;                      // Sensitivity, the six stages, Output
    std::vector<fwvocalchain::ui::ColourSelector*> colours;
    fwvocalchain::ui::Meter& inMeter;
    fwvocalchain::ui::Meter& outMeter;
    fwvocalchain::ui::SensLed& led;
    fwvocalchain::ui::BalanceKnob& balance;
    fwvocalchain::ui::LinkButton& linkButton;
    std::unique_ptr<juce::TextEditor> entry;
    juce::Slider* entryFor = nullptr;
    juce::Image background;
    float backgroundScale = 0.0f;
    int backgroundMode = -1, laidOut = -1;
    double lastTick = 0.0;
};
