#pragma once
// Shared UI for the Flatwater plugins built on the older single-panel layout (Flatwater Vocal Comp, Flatwater Bass Lift,
// ...): colours, fonts, faders (big capsule, small round, horizontal), the IN switch, and an editor base class that does
// the design-grid scaling, the toolbar, resizing and the common drawing (panel, housing, meter window, capsules), all in
// Flatwater's own look (FwTheme.h). The layouts keep the measured positions and sizes.
#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>
#include "FwToolbar.h"
#include "FwTheme.h"
#include "FwWheel.h"

namespace fw::ren
{
// the kit's colours, from the theme: frames and grid lines in the line colour, labels light grey, scales faint,
// values light, meters and the reduction window cyan, the faders' rings raised with amber index lines
inline const juce::Colour kFrameLine (fw::theme::line), kPanel (fw::theme::panel), kOuterFill (fw::theme::raised),
    kOuterLine (fw::theme::line), kInnerLine (fw::theme::line.brighter (0.4f)), kInnerFill (fw::theme::well), kWindow (fw::theme::well),
    kGrid (fw::theme::line), kLabel (fw::theme::textDim), kScale (fw::theme::textFaint), kScaleInner (fw::theme::textFaint),
    kMeterBg (fw::theme::well), kMeterFg (fw::theme::cyan), kGrRed (0x5940d6f0), kGrRedLine (fw::theme::cyan),
    kValue (fw::theme::text), kReadout (fw::theme::text), kRing (fw::theme::amber), kRingActive (fw::theme::amberLight),
    kTrack (fw::theme::line);

juce::Font arial (float h, bool bold = false);
juce::String dbText (float v, bool infAtMinus80 = false);

// Big: the centre capsule fader (102 px wide). Small: the round fader on a 33 px column.
// Horizontal: the round knob on a horizontal track.
class Fader : public juce::Slider
{
public:
    enum Kind { Big, Small, Horizontal };
    Fader (Kind k, int thumbOffsetX = 4);

    // text shown / typed; negateTyped: a typed positive number is read as negative (dB controls)
    std::function<juce::String (double)> format = [] (double v) { return dbText ((float) v); };
    bool negateTyped = true;
    void applyFormat();

    void valueChanged() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override { wavesWheel (*this, e, w, wheelAcc, false, wheelScale); }
    bool hitTest (int x, int y) override;
    juce::Rectangle<int> valueTextBounds() const;
    double wheelAcc = 0.0, wheelScale = 1.0;
    // the drag (measured on RVox / RBass): the thumb follows the mouse (times plainScale); with Ctrl it moves ctrlFast of
    // that on mouse steps of 3 px and more, ctrlSlow on smaller ones
    float plainScale = 1.0f, ctrlFast = 0.19f, ctrlSlow = 0.037f;
    juce::Point<float> virt, last;
    // the cursor is hidden while dragging and comes back where it was grabbed, moved as far as the thumb (measured)
    juce::Point<float> downAt;
    double thumbAtDown = 0.0;

    const Kind kind;
    const int offsetX;
    juce::Rectangle<int> textArea { 0, 0, 42, 16 };   // Small / Horizontal: where the value text sits (local)
    int trackTop = 16;                                 // Small: local y of the 33 x 195 control rectangle
};

class Look : public juce::LookAndFeel_V4
{
public:
    int getSliderThumbRadius (juce::Slider&) override;
    juce::Slider::SliderLayout getSliderLayout (juce::Slider&) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int, int, int, int, float pos, float, float, juce::Slider::SliderStyle, juce::Slider&) override;
    void drawCornerResizer (juce::Graphics&, int w, int h, bool isMouseOver, bool isMouseDragging) override;
};

// the "IN" pill switch (33 x 38 control, pill at the top)
class InSwitch : public juce::Button
{
public:
    InSwitch() : juce::Button ("IN") { setClickingTogglesState (true); }
    void paintButton (juce::Graphics&, bool hot, bool down) override;
};

class Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    Editor (juce::AudioProcessor&, juce::AudioProcessorValueTreeState&, juce::UndoManager&,
            juce::RangedAudioParameter* modeParam, const juce::String& product, const juce::String& version,
            int designW, int designH, fw::Toolbar::Style toolbarStyle = fw::Toolbar::Style::Renaissance,
            const juce::StringArray& modes = { "Stereo", "Mono" });
    ~Editor() override;
    void resized() override;
    void parentHierarchyChanged() override;

protected:
    // subclasses draw and react in design coordinates
    virtual void paintContent (juce::Graphics&) = 0;
    virtual void contentMouseDown (const juce::MouseEvent&) {}
    virtual void contentMouseDrag (const juce::MouseEvent&) {}
    virtual void contentMouseUp (const juce::MouseEvent&) {}
    virtual void tick() {}                                         // 60 Hz: read meters, repaint

    // controls are owned here, so they outlive their parameter attachments during teardown
    Fader& makeFader (Fader::Kind, int thumbOffsetX = 4);
    InSwitch& makeSwitch();
    void attach (Fader&, const char* paramID, juce::Rectangle<int> bounds, double resetValue);
    void attach (juce::Button&, const char* paramID, juce::Rectangle<int> bounds);
    juce::Component& content();
    bool isMono() const;
    void setFactoryPresets (const std::vector<fw::FactoryPreset>& list) { toolbar->setFactoryPresets (&list); }
    void setKeepOnPresetLoad (std::vector<juce::AudioProcessorParameter*> list) { toolbar->setKeepOnPresetLoad (std::move (list)); }
    void onDefaultPreset (std::function<void()> f) { toolbar->onLoadDefault = std::move (f); }   // state that is not a parameter

    // for editors with their own widgets (e.g. the flat kit, FwFlatUI.h): the editor owns them and their look,
    // and keeps the parameter attachments; everything is torn down before the look
    template <class T, class... Args> T& make (Args&&... args)
    {
        owned.push_back (std::make_unique<T> (std::forward<Args> (args)...));
        return static_cast<T&> (*owned.back());
    }
    void setAuxLook (std::unique_ptr<juce::LookAndFeel> l) { auxLook = std::move (l); }
    juce::LookAndFeel* getAuxLook() const { return auxLook.get(); }
    void attachSlider (juce::Slider&, const char* paramID, juce::Rectangle<int> bounds, double resetValue);
    void attachCombo (juce::ComboBox&, const char* paramID, juce::Rectangle<int> bounds);
    void addToContent (juce::Component&, juce::Rectangle<int> bounds);
    void keepAttachment (std::unique_ptr<juce::ParameterAttachment> a) { paramAtts.push_back (std::move (a)); }

    // common drawing
    void paintPanel (juce::Graphics&) const;                      // black background, panel, frame
    static void paintHousing (juce::Graphics&);                    // big rounded housing + inner frame
    static void paintWindow (juce::Graphics&, float fillFromTop, juce::Colour fill, juce::Colour fillLine,
                             int lines, float firstLineY, float lineStep, bool fromBottom = false);
    static void paintCapsule (juce::Graphics&, juce::Rectangle<float>, float frac);
    static void paintTitle (juce::Graphics&, const juce::String& name, juce::Colour nameColour);   // the Flatwater mark + name, centred
    static void paintClipLight (juce::Graphics&, juce::Rectangle<float>, bool lit);

    // an editor whose design height changes (a panel that opens and closes): the window keeps its scale, the
    // resize limits follow the new aspect
    void setDesignHeight (int h);

    juce::AudioProcessorValueTreeState& apvts;
    juce::UndoManager& undo;
    const int W;
    int H;

private:
    class Content;
    void timerCallback() override { toolbar->refresh(); tick(); }

    Look look;
    std::unique_ptr<juce::LookAndFeel> auxLook;           // destroyed after the widgets that use it (declared first)
    std::unique_ptr<Content> contentComp;
    std::unique_ptr<fw::Toolbar> toolbar;
    juce::RangedAudioParameter* modeParam;
    juce::ComponentBoundsConstrainer constrainer;
    std::vector<std::unique_ptr<juce::SliderParameterAttachment>> sliderAtts;
    std::vector<std::unique_ptr<juce::ButtonParameterAttachment>> buttonAtts;
    std::vector<std::unique_ptr<juce::ComboBoxParameterAttachment>> comboAtts;
    std::vector<std::unique_ptr<juce::ParameterAttachment>> paramAtts;
    std::vector<Fader*> faders;
    std::vector<std::unique_ptr<juce::Component>> owned;
};
}
