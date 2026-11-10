#pragma once
#include "PluginProcessor.h"
#include "shared/ui/RenUI.h"
#include "shared/ui/HybridUI.h"
#include "shared/ui/Widgets.h"

// Flatwater Delay editor: 749 x 292 px design grid including the toolbar, resizable at that aspect ratio. The layout keeps
// the measured positions and sizes (RESEARCH.md "Flatwater Delay"); everything is drawn in Flatwater's own look (shared/ui/Theme.h).
class FwDelayEditor : public flatwater::ren::Editor
{
public:
    explicit FwDelayEditor (FwDelayProcessor&);

    static constexpr int W = 749, H = 292;

    // the time display: the note value and tempo (Host / BPM), the time in ms, Ping Pong's start side, Dual's
    // two sides. Clicks choose the side the Delay knob sets (Dual) or flip the Ping Pong start; drags and the wheel set
    // the value under the mouse (a time, a note, the tempo).
    class Display : public juce::Component
    {
    public:
        explicit Display (FwDelayEditor& e) : ed (e) {}
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
        void selectBox (bool s) { if (s != boxSelected) { boxSelected = s; repaint(); } }
    private:
        struct Hit
        {
            juce::RangedAudioParameter* param = nullptr;
            int side = -1;                            // Dual: 0 the left side, 1 the right
            bool note = false, box = false;           // a note value (stepped); Ping Pong's start box
        };
        Hit hit (juce::Point<float> designPos) const;
        FwDelayEditor& ed;
        juce::RangedAudioParameter* dragParam = nullptr;
        bool noteDrag = false, boxSelected = false;
        float lastY = 0.0f, stepAcc = 0.0f;
    };

    // the rubber tap pad: the time between the last two taps sets the delay (Host: the nearest note; BPM: the tempo)
    class TapPad : public juce::Component
    {
    public:
        explicit TapPad (FwDelayEditor& e) : ed (e) {}
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override { pressed = false; repaint(); }
        void deselect() { if (selected) { selected = false; repaint(); } }
    private:
        FwDelayEditor& ed;
        bool pressed = false, selected = false;
        double lastTap = 0.0;
    };

    // the knurled Link wheel between the filters: turning it moves HiPass and LoPass by the same amount
    class LinkWheel : public juce::Component
    {
    public:
        explicit LinkWheel (FwDelayEditor& e) : ed (e) {}
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
        juce::Point<float> centre;                    // local
    private:
        void move (float normDelta);
        FwDelayEditor& ed;
        float lastY = 0.0f, turn = 0.0f;
    };

    // Dual's right side on the Delay knob: the small box after DELAY
    class RMark : public juce::Component
    {
    public:
        RMark() { setInterceptsMouseClicks (false, false); }
        void paint (juce::Graphics&) override;
    };

    // the output meter: an amber pointer per channel moving along the printed scale
    class Meter : public juce::Component
    {
    public:
        Meter() { setInterceptsMouseClicks (false, false); }
        void paint (juce::Graphics&) override;
        void set (float lDb, float rDb, bool stereo);
    private:
        float l = -100.0f, r = -100.0f;
        bool two = true;
    };

private:
    void paintContent (juce::Graphics&) override;
    void tick() override;
    float value (const char* id) const;
    void setValue (const char* id, float realValue);
    double tempo() const;                                 // the tempo the note values follow (Host or BPM)
    void updateDelayKnobs();
    void contentMouseDown (const juce::MouseEvent&) override { selectOnly (nullptr); }
    int modeIndex() const { return juce::roundToInt (value (de::mode)); }
    bool mono() const { return modeIndex() == 1; }

    FwDelayProcessor& proc;
    flatwater::hybrid::Knob& delay;
    flatwater::hybrid::Knob& delayNote;
    flatwater::hybrid::Knob& delayR;
    flatwater::hybrid::Knob& delayNoteR;
    flatwater::hybrid::Knob& feedback;
    flatwater::hybrid::Knob& depth;
    flatwater::hybrid::Knob& rate;
    flatwater::hybrid::Knob& hipass;
    flatwater::hybrid::Knob& lopass;
    flatwater::hybrid::Knob& mix;
    flatwater::hybrid::Knob& output;
    flatwater::hybrid::Knob& analog;
    Display& display;
    TapPad& tap;
    LinkWheel& link;
    Meter& meter;
    RMark& rMark;
    flatwater::hybrid::ToggleRow* stereoRow = nullptr;
    flatwater::hybrid::ToggleRow* monoRow = nullptr;
    flatwater::hybrid::ButtonGroup* syncGroup = nullptr;
    flatwater::hybrid::ButtonGroup* lofiButton = nullptr;
    juce::Component* lastPrimary = nullptr;
    void selectOnly (juce::Component*);
    flatwater::flat::Selection selection;
    // Dual: the Delay knob sets the right side (kept while Ping Pong or Dual is off, as the reference)
    bool rightSide = false;
    int lastSync = -1, lastMode = -1, lastDual = -1, lastPing = -1, lastDualOn = -1;
    bool lastRight = false;
    double lastTick = 0.0, clipLevel = 0.0, clipAge = 1.0e9;
    flatwater::MeterBallistics meterL, meterR;
    friend class Display;
    friend class TapPad;
    friend class LinkWheel;
};
