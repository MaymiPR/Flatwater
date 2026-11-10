#pragma once
// Widgets and drawing for the Flatwater editors on the Hybrid layout (Flatwater Comp, Flatwater Delay), in Flatwater's own
// dark look (shared/ui/Theme.h): raised grey knobs lit from above (the fluted caps at the original's size) with the value's cyan arc and an amber pointer, scales printed in the faint
// colour, push buttons in a well, a dark VU face with an amber needle and segment displays in cyan. Everything is drawn
// here in code (no bitmaps, no tables); the layouts keep the measured positions and sizes (RESEARCH.md). Knobs follow the
// same mouse feel and selection model as the flat kit.
#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>
#include "shared/ui/FlatUI.h"

namespace flatwater::hybrid
{
juce::Font font (float h, bool bold = true);

// the background under the toolbar (area in design pixels)
void drawPanel (juce::Graphics&, juce::Rectangle<int> area);

// a scale printed around a knob: ticks from angle a0 to a1 (radians, JUCE convention: clockwise from 12 o'clock)
struct Scale
{
    int ticks = 21;
    float rIn = 36.0f, rOut = 47.0f, thick = 1.8f;
    struct Label { int tick; juce::String text; float radius; float dx = 0.0f, dy = 0.0f; };
    std::vector<Label> labels;
    float labelSize = 11.0f;
    juce::Colour colour;                              // the theme's faint colour unless set
};
void drawScale (juce::Graphics&, juce::Point<float> centre, float a0, float a1, const Scale&);

// text centred on its ink at `centre` (the measured places): em is the font size in pixels, hs its horizontal scale,
// bold for the labels, angle a rotation in radians. Drawn in the current colour.
void printText (juce::Graphics&, const juce::String&, juce::Point<float> centre, float em, float hs, bool bold, float angle = 0.0f);

// A rotary knob. Kind Small: radius 32.4; Mini: 23.65, a plain cap; Knurled: 32.4 with a knurled rim. Large / Huge /
// Grand: a fluted cap in a wider skirt (radius 53.9 / 73 / 63.6). Drawn in the theme: a raised disc, the value's cyan
// arc just inside the knob's radius, an amber pointer; the flutes and knurls turn with the value.
class Knob : public juce::Slider
{
public:
    enum Kind { Small, Large, Huge, Grand, Mini, Knurled };
    Knob (Kind k, float startDeg, float endDeg);     // the measured angles (0 = down, clockwise)
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseEnter (const juce::MouseEvent&) override { if (onHover) onHover (this, true); }
    void mouseExit (const juce::MouseEvent&) override { if (onHover) onHover (this, false); }
    bool hitTest (int x, int y) override;

    const Kind kind;
    juce::Point<float> centre;                        // local
    int steps = 0;                                    // > 0: a stepped knob (Analog)
    flatwater::flat::FineDrag feel { flatwater::flat::FineDrag::Rotary };
    flatwater::flat::Selection* selection = nullptr;
    std::function<juce::String (double)> format;
    std::function<void (Knob*, bool)> onHover;
    double wheelAcc = 0.0;                            // notches not yet turned into steps (stepped knobs)
    float ringRadius() const { return kind == Small ? 21.0f : kind == Large ? 40.0f : kind == Huge ? 53.0f : kind == Grand ? 46.0f : kind == Knurled ? 23.0f : 15.0f; }
    juce::Rectangle<int> valueArea;                   // the value slot (local): the value box while selected, the entry box
    float radius() const { return kind == Small ? 32.4f : kind == Large ? 53.9f : kind == Huge ? 73.0f : kind == Grand ? 63.6f : kind == Knurled ? 32.4f : 23.65f; }
    float pointerAngle() const;
    float paintExtent() const;                        // how far the drawing (shadow, arc) reaches from the centre
};

// the buttons' layout: Comp's (a 1 px frame, buttons 1 px apart) or Delay's (a 2 px frame, 2 px dividers, the component
// 9 px taller below the frame)
enum class ButtonLook { comp, delay };

// push buttons in a well (a radio group of a choice parameter, or a single toggle). The component's bounds are the
// frame expanded by 4 px, and by 9 px at the bottom with the Delay layout.
class ButtonGroup : public juce::Component
{
public:
    ButtonGroup (juce::RangedAudioParameter&, juce::UndoManager*, juce::StringArray labels, bool vertical, bool toggle = false,
                 ButtonLook = ButtonLook::comp);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    std::unique_ptr<juce::ParameterAttachment> attachment;
    std::function<void (juce::Component*)> onClick;   // the editor's selection (the outline)
    void setOutlined (bool o) { if (o != outlined) { outlined = o; repaint(); } }
private:
    juce::Rectangle<float> buttonRect (int i) const;
    juce::Rectangle<float> frameRect() const;
    juce::StringArray labels;
    bool vertical, toggle;
    ButtonLook look;
    int selected = 0;
    bool outlined = false;
};

// several toggles side by side in one frame, each on its own parameter (Delay's Phase L / Ping Pong / Dual / Phase R
// row): `widths` are the buttons' widths in px (the measured control rectangles)
class ToggleRow : public juce::Component
{
public:
    struct Item { juce::RangedAudioParameter* param; juce::String label; float width; };
    ToggleRow (std::vector<Item>, juce::UndoManager*, ButtonLook = ButtonLook::comp);   // delay: widths are the faces'
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void setVisibleItems (int n) { if (n != shown) { shown = n; repaint(); } }   // the mono component shows the first only
    std::function<void (juce::Component*)> onClick;
    void setOutlinedItem (int i) { if (i != outlinedItem) { outlinedItem = i; repaint(); } }
private:
    juce::Rectangle<float> buttonRect (int i) const;
    juce::Rectangle<float> frameRect() const;
    std::vector<Item> items;
    std::vector<std::unique_ptr<juce::ParameterAttachment>> attachments;
    std::vector<bool> on;
    int shown;
    ButtonLook look;
    int outlinedItem = -1;
};

// one button face, lit or not, its label centred
void drawButtonFace (juce::Graphics&, juce::Rectangle<float>, const juce::String& label, bool on);

// the VU meter's face and needle, inside the housing's window (the housing is part of the panel drawing)
class VuMeter : public juce::Component
{
public:
    VuMeter();
    void paint (juce::Graphics&) override;
    enum class Face { gr, io };                       // the printed scale: gain reduction, or the In / Out level
    void setNeedle (float angleDeg) { if (! juce::exactlyEqual (angleDeg, needle)) { needle = angleDeg; repaint(); } }
    void setFace (Face f) { if (f != face) { face = f; repaint(); } }
    juce::Point<float> pivot;                         // local: the scale's centre (the needle turns 2.72, -0.04 px from it)
    float needle = 59.0f;                             // degrees from vertical about the needle's pivot, + to the right (0 dB GR)
    Face face = Face::gr;
};
void drawVuHousing (juce::Graphics&, juce::Rectangle<float> housing, juce::Point<float> archCentre, float archRadius);

// segment text in a dark window (the BPM display); with digits: a small caption and large 7-segment digits
void drawLedDisplay (juce::Graphics&, juce::Rectangle<float> box, const juce::String& text, const juce::String& digits = {});
// text in the display's segment font: cells of cellW x cellH from `origin` (the first cell's top left) at `pitch`,
// strokes t px thick, leaning right by `slant` px per px of height
void drawSegText (juce::Graphics&, juce::Point<float> origin, const juce::String& text, float cellW, float cellH, float pitch,
                  float t, float slant);

// a segment font (measured sizes): cells w x h, strokes t wide, the horizontal strokes from hx to w - hx, the vertical
// ones vy short of the cell's ends and middle, leaning right by `slant` px per px of height, characters `pitch` apart
struct SegFont { float w, h, t, hx, vy, slant, pitch; bool solidOne = false; };   // solidOne: "1" as one stroke
// text from the first cell's bottom-left corner's x and its top y: 0-9, B D A R T L P M and -
void drawSegmentText (juce::Graphics&, const SegFont&, float x, float y, const juce::String& text);

// the name plate: the Flatwater mark and the plug-in's name, left-aligned in `area` (two lines when `stacked`: "Flatwater"
// small over the plug-in's own word)
void drawNamePlate (juce::Graphics&, juce::Rectangle<float> area, const juce::String& name, bool stacked = false);

// a screw at its measured place: a dark recess, a raised head with a cross (or a slot)
void drawScrew (juce::Graphics&, juce::Point<float> centre, float radius, bool slot = false);

// the value entry (double-click) sits on each knob's value slot; the text box is invisible otherwise
class HybridLook : public flatwater::flat::FlatLook
{
public:
    juce::Slider::SliderLayout getSliderLayout (juce::Slider&) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
};
}
