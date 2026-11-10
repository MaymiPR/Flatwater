#pragma once
#include "PluginProcessor.h"
#include "shared/ui/RenUI.h"
#include "shared/ui/FlatUI.h"

// Flatwater Vocal Leveler editor: the reference's measured layout on a 320 x 598 px design grid (the narrow toolbar
// included), drawn by Flatwater in the suite's own look (shared/ui/Theme.h). The static art is rendered once per window scale
// and layout (Studio / Live); the controls draw themselves.
namespace vl::ui
{
// the reference's scale on the lower panel: +12 dB at y 231, 0 dB at 336, -12 dB at 441 (8.75 px per dB, measured)
inline float scaleY (float db) { return 336.0f - 8.75f * db; }
inline float dbAtY (float y) { return (336.0f - y) / 8.75f; }

enum class Mark { None, Selected, Member };                    // amber, cyan
void drawMark (juce::Graphics&, juce::Rectangle<float>, Mark, bool round = false);

// the selection, as Flatwater Bass Leveler's (measured on its reference, whose editor this one's shares): hovering shows
// nothing; a click marks the control in amber (the Rider and the Idle arrow in cyan) until another one is clicked; a lasso on
// the background selects the controls it touches; the selected control's value shows while the mouse is on it
struct Marks
{
    juce::Component* hover = nullptr;
    juce::Component* selected = nullptr;
    std::vector<juce::Component*> group;
    std::vector<const juce::Component*> blueAlone;
    bool inGroup (const juce::Component* c) const { return std::find (group.begin(), group.end(), c) != group.end(); }
    Mark of (const juce::Component* c) const
    {
        if (c == nullptr) return Mark::None;
        if (group.size() > 1) return c == selected ? Mark::Selected : inGroup (c) ? Mark::Member : Mark::None;
        if (c != selected) return Mark::None;
        return std::find (blueAlone.begin(), blueAlone.end(), c) != blueAlone.end() ? Mark::Member : Mark::Selected;
    }
    bool showsValue (const juce::Component* c) const
    {
        return hover != nullptr && hover == selected && (c == selected || (group.size() > 1 && inGroup (c)));
    }
    bool click (juce::Component* c)
    {
        const bool member = group.size() > 1 && inGroup (c);
        if (! member) group.clear();
        selected = c;
        if (changed) changed();
        return ! member;
    }
    std::function<void()> changed;
    std::function<void (juce::Slider*)> enter;                  // a double-click: type a value (the editor's box)
};

// the faders' and the Target slider's drag (as Flatwater Bass Leveler's): the thumb follows the mouse, Ctrl a fine mode, Alt-click
// resets, the cursor hidden while dragging and back where the thumb went
struct LinearDrag
{
    bool down (juce::Slider&, const juce::MouseEvent&);
    void drag (juce::Slider&, const juce::MouseEvent&, float pxPerUnit, bool vertical);
    void up (juce::Slider&, const juce::MouseEvent&);
    bool active = false, vertical = true;
    double start = 0.0;
    float pxPerUnit = 1.0f;
    juce::Point<float> origin, last, virt;
};

// a knob: Vocal and Music (54 x 60) or Spill (36 x 36), +-134.5 degrees from Lo to Hi; Music shows dimmed while no
// side-chain feeds it
class Knob : public juce::Slider
{
public:
    explicit Knob (bool small);
    void paint (juce::Graphics&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override { if (! disabledLook && marks != nullptr && marks->enter) marks->enter (this); }
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override { if (! disabledLook) flatwater::fineWheel (*this, e, w, wheelAcc, false); }
    flatwater::flat::FineDrag feel { flatwater::flat::FineDrag::Rotary };
    const bool small;
    bool blocked = false, disabledLook = false;
    Marks* marks = nullptr;
    double wheelAcc = 0.0;
    bool showing() const { return isMouseButtonDown() || (marks != nullptr && marks->showsValue (this)); }
};

// a vertical fader with its cap (Output, Range Max / Min, the Rider)
class Fader : public juce::Slider
{
public:
    enum Kind { Output, RangeMax, RangeMin, Rider };
    explicit Fader (Kind);
    void paint (juce::Graphics&) override;
    bool hitTest (int x, int y) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override { if (kind != Rider && marks != nullptr && marks->enter) marks->enter (this); }
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override { if (kind != Rider) flatwater::fineWheel (*this, e, w, wheelAcc, false); }
    juce::Rectangle<float> capBounds() const;                   // local: the cap's box (it sets the travel)
    juce::Rectangle<float> capCore() const;                     // local: its visible part
    const Kind kind;
    LinearDrag feel;
    bool blocked = false;
    Marks* marks = nullptr;
    double wheelAcc = 0.0;
    std::function<void (bool)> onGrab;                          // the Rider: grabbed / released
};

// the Target slider: a glass window with an amber centre line over the vocal level meter
class TargetSlider : public juce::Slider
{
public:
    TargetSlider();
    void paint (juce::Graphics&) override;
    bool hitTest (int x, int y) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override { if (marks != nullptr && marks->enter) marks->enter (this); }
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override { flatwater::fineWheel (*this, e, w, wheelAcc, false); }
    LinearDrag feel;
    bool blocked = false;
    Marks* marks = nullptr;
    double wheelAcc = 0.0;
    float levelDb = -100.0f;                                    // the meter under it
    float thumbX (double v) const;                              // local x of the glass's left edge
};

// the Idle arrow's column (the reference's Idle control, 33 .. 45 x 230 .. 442): the arrow at the Idle value, dragged
// within the range; while the mouse is on the column its value shows
class IdleColumn : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    std::function<float()> idleDb;
    std::function<void (float db)> setIdleDb;
    std::function<void (bool begin)> gesture;
    std::function<void()> reset;
    bool blocked = false;
    Marks* marks = nullptr;
    float grab = 0.0f;
    bool dragging = false;
};

// the Attack switch: Fast (up) / Slow (down)
class Switch : public juce::Button
{
public:
    Switch() : juce::Button ("Attack") { setClickingTogglesState (true); }
    void paintButton (juce::Graphics&, bool hot, bool down) override;
    void mouseEnter (const juce::MouseEvent& e) override { if (marks != nullptr) { marks->hover = this; if (marks->changed) marks->changed(); } Button::mouseEnter (e); }
    void mouseExit (const juce::MouseEvent& e) override { if (marks != nullptr && marks->hover == this) { marks->hover = nullptr; if (marks->changed) marks->changed(); } Button::mouseExit (e); }
    void mouseDown (const juce::MouseEvent& e) override { blocked = marks != nullptr && ! marks->click (this); if (! blocked) Button::mouseDown (e); }
    void mouseUp (const juce::MouseEvent& e) override { if (! blocked) Button::mouseUp (e); blocked = false; }
    bool blocked = false;
    Marks* marks = nullptr;
};

// the Automation buttons: Off / Write / Read, the chosen one lit
class AutomationButtons : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    std::function<int()> mode;
    std::function<void (int)> setMode;
    Marks* marks = nullptr;
};
}

class FwVocalLevelerEditor : public flatwater::ren::Editor
{
public:
    explicit FwVocalLevelerEditor (FwVocalLevelerProcessor&);
    ~FwVocalLevelerEditor() override;

    static constexpr int W = 320, H = 598;

private:
    void paintContent (juce::Graphics&) override;
    void tick() override;
    void contentMouseDown (const juce::MouseEvent&) override;
    void contentMouseDrag (const juce::MouseEvent&) override;
    void contentMouseUp (const juce::MouseEvent&) override;
    void paintStatic (juce::Graphics&, bool live) const;       // the static art (cached per scale and layout)
    void layoutControls();                                      // Studio / Live
    // typing a value (as Flatwater Bass Leveler's: a double-click on a knob, Target, Output or a Range thumb; Enter applies a value
    // inside the range; Spill reads a typed positive number as negative; the Rider and the Idle arrow take none)
    void openEntry (juce::Slider*);
    juce::Rectangle<int> entryArea (juce::Slider*) const;
    void closeEntry (bool apply);
    std::unique_ptr<juce::TextEditor> entry;
    juce::Slider* entryFor = nullptr;
    flatwater::flat::LassoOverlay& lasso;
    juce::Point<float> lassoStart;
    bool lassoing = false;

    FwVocalLevelerProcessor& proc;
    vl::ui::Marks marks;
    vl::ui::Knob& vocal;
    vl::ui::Knob& music;
    vl::ui::Knob& spill;
    vl::ui::Switch& attackSwitch;
    vl::ui::TargetSlider& target;
    vl::ui::Fader& rangeMax;
    vl::ui::Fader& rangeMin;
    vl::ui::Fader& output;
    vl::ui::Fader& rider;
    vl::ui::IdleColumn& idleColumn;
    vl::ui::AutomationButtons& automation;

    juce::Image background;
    float backgroundScale = 0.0f;
    int backgroundLive = -1;
    double lastTick = 0.0;
    float level = -100.0f, ride = 0.0f, music01 = 0.0f;
    bool clip = false, riding = false, sideChain = false;
    int laidOut = -1;
};
