#pragma once
#include "PluginProcessor.h"

// Flatwater Pitch Bend's controls, drawn in Flatwater's own look (shared/ui/Theme.h) at the reference's measured sizes and
// with its measured feel (RESEARCH.md, "The editor"): knobs that carry their modulation slots' arcs and a dot at the
// modulated value, the slots themselves, the modulators' screen (an LFO cycle, a sequence, AM's envelope, PT's pitch)
// and the shape library. They share one Ui: the processor, the selected control, an assignment being dragged.
namespace fwpitchbend::ui
{
class ModKnob;

// a modulator's colour: M1 and M2 cyan, AM and PT a light amber (the theme's tokens; amber itself marks selection)
juce::Colour sourceColour (int source);

struct Ui
{
    FwPitchBendProcessor& proc;
    juce::Component* selected = nullptr;                          // the control clicked last (marked in amber)
    int dragging = None;                                         // a source being dragged onto a control
    ModKnob* dropTarget = nullptr;                               // the control under it
    std::function<void()> changed {};                            // repaint the editor's overlays
    std::function<void (ModKnob&)> enter {};                     // a double-click: type a value
    std::function<void (const juce::String&, juce::Rectangle<int>)> readout {};   // a slot's "M1 | 50" while dragged
    juce::LookAndFeel* menuLook = nullptr;                       // the lists (the slots' sources), as the toolbar's

    juce::ValueTree& state() { return proc.apvts.state; }
    juce::UndoManager* um() { return &proc.undo; }
    void select (juce::Component* c) { selected = c; if (changed) changed(); }
};

// a knob: Pitch and Formant (Big: 170 px, a fluted cap, ±125.8°), Mix (Medium, 96 px), or a panel knob (Small, 68 px;
// Medium and Small ±131.6°). Vertical drag at the
// measured speeds (slow mouse steps finer than fast ones), Ctrl a quarter, Alt-click its default, the wheel by its
// step, a double-click to type; the cursor hides while dragging and comes back where it was pressed
class ModKnob : public juce::Slider
{
public:
    enum Kind { Big, Medium, Small };
    ModKnob (Ui&, Kind, int target, juce::String label);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override { if (ui.enter) ui.enter (*this); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    juce::Point<float> centre() const { return getLocalBounds().toFloat().getCentre(); }
    float angleOf (double proportion) const;                      // radians from straight up
    juce::String valueText() const;                               // as the value box shows it
    // the wheel's step per notch, a part of the range, measured on the reference's knob (0: 1.35 %); Pitch and
    // Formant a semitone with Fine off
    double defaultValue = 0.0, wheelStep = 0.0;
    bool stepped = false;                                         // a choice (Rate synced): a step per notch
    std::function<juce::String (double)> format;
    const Kind kind;
    const int target;                                             // its place in targets[], or -1 (not modulatable)
    const juce::String label;
    bool framed = false;                                          // a possible target while an assignment is dragged

private:
    Ui& ui;
    double startValue = 0.0, accum = 0.0;
    float lastY = 0.0f;
};

// one of a control's four slots: its source's name in its colour; vertical drag sets the depth (0.2 per px), Alt-click
// 0, right-click the source menu. Empty slots show only while an assignment is dragged
class SlotPad : public juce::Component
{
public:
    SlotPad (Ui&, ModKnob& owner, int slot, bool big);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    ModKnob& owner;
    const int slot;

private:
    Ui& ui;
    const bool big;
    float startDepth = 0.0f, lastY = 0.0f;
    bool draggingDepth = false;
};

// the modulators' screen: M1 / M2 (an LFO's cycle with a play-head, or a sequence's steps), AM's envelope, PT's pitch;
// draw mode draws an LFO cycle through the mouse's path, a vertical drag on a step sets it
class Screen : public juce::Component, private juce::Timer
{
public:
    explicit Screen (Ui&);
    void show (int tab);                                          // 0 M1, 1 M2, 2 AM, 3 PT
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    bool drawMode = false;

private:
    void timerCallback() override;
    bool isSeq() const;
    int steps() const;
    juce::Rectangle<float> plot() const;
    Ui& ui;
    int tab = 0, editStep = -1, lastIndex = -1;
    float lastValue = 0.0f, startY = 0.0f;
    int startStep = 0;
    std::vector<float> history;                                   // AM / PT: the last seconds of their value
};

// the shape library over the screen: our shapes or step patterns, then the user's; a click loads one and closes it,
// Save adds the current one, Delete (a mode) removes a user's
class Library : public juce::Component
{
public:
    explicit Library (Ui&);
    void open (int modulator, bool seq);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void saveCurrent();
    bool deleteMode = false;
    std::function<void()> closed;

private:
    struct Tile { juce::String name; Table table; juce::File file; };
    void rebuild();
    juce::Rectangle<float> tileRect (int i) const;
    Ui& ui;
    int modulator = 0;
    bool seq = false;
    float scroll = 0.0f;
    std::vector<Tile> tiles;
};

// a lamp with its label (Fine, Flatten), the LFO / SEQ pair, the tabs, the panel's open / close, the screen's tools
class Toggle : public juce::Component
{
public:
    Toggle (Ui&, juce::String label, std::function<bool()> get, std::function<void()> click, bool lampLeft = false);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    enum class Look { Lamp, Pair, Icon } look = Look::Lamp;
    juce::String icon;                                            // Icon: "draw", "erase", "browse", "save", "delete", "sync", "open"

private:
    Ui& ui;
    juce::String label;
    std::function<bool()> get;
    std::function<void()> click;
    bool lampLeft;
};
}
