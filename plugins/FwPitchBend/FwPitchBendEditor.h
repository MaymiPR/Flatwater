#pragma once
#include "FwPitchBendProcessor.h"
#include "FwPitchBendWidgets.h"
#include "FwRenUI.h"
#include "FwMenuLook.h"

// Flatwater Pitch Bend editor: the reference's measured layout (564 px wide; the shifter plate, the modulator tabs and,
// below them, the modulators' panel; RESEARCH.md, "The editor") drawn in Flatwater's own look (FwTheme.h). A
// modulator's tab dragged onto a control puts it in that control's first free slot; the slots' depths are dragged;
// Link moves Pitch and Formant together.
class FwPitchBendEditor : public fw::ren::Editor
{
public:
    explicit FwPitchBendEditor (FwPitchBendProcessor&);
    ~FwPitchBendEditor() override;

    static constexpr int W = 564, HOpen = 465, HClosed = 289;     // with the modulators' panel open / closed

private:
    using ModKnob = fwpitchbend::ui::ModKnob;
    void paintContent (juce::Graphics&) override;
    void contentMouseDown (const juce::MouseEvent&) override;
    void contentMouseDrag (const juce::MouseEvent&) override;
    void contentMouseUp (const juce::MouseEvent&) override;
    void tick() override;

    ModKnob& knob (ModKnob::Kind, int target, const char* param, juce::String label, juce::Rectangle<int>, bool slots = true);
    void showTab (int tab);
    void setExpanded (bool);
    bool expanded() const;
    int tab() const;
    ModKnob* targetAt (juce::Point<float>) const;
    void openEntry (ModKnob&);
    void closeEntry (bool apply);
    juce::RangedAudioParameter& param (const char* id) { return *proc.apvts.getParameter (id); }

    FwPitchBendProcessor& proc;
    fwpitchbend::ui::Ui ui;
    fw::MenuLook menuLook;                                       // the lists (the Flatten note's), as the toolbar's
    std::vector<ModKnob*> knobs;
    std::vector<juce::Component*> panel[4];                      // what each tab's panel shows (M1, M2, AM, PT)
    std::vector<juce::Component*> modTools;                      // the M1 / M2 screen's tools
    ModKnob* pitch = nullptr;
    ModKnob* formant = nullptr;
    ModKnob* mix = nullptr;
    ModKnob* rateHz[2] {};
    ModKnob* rateSynced[2] {};
    fwpitchbend::ui::Screen* screen = nullptr;
    fwpitchbend::ui::Library* library = nullptr;
    fwpitchbend::ui::Toggle* drawTool = nullptr;
    fwpitchbend::ui::Toggle* browseTool = nullptr;
    fwpitchbend::ui::Toggle* saveTool = nullptr;
    fwpitchbend::ui::Toggle* deleteTool = nullptr;
    class Overlay;
    Overlay* overlay = nullptr;

    // a drag that starts on the plate: a tab (an assignment), Link, the Flatten note, Steps
    enum class Press { None, Tab, Link, Note, Steps } press = Press::None;
    int pressTab = -1, pressStart = 0;
    juce::Point<float> pressAt;
    float lastY = 0.0f, linkAccum = 0.0f, linkMoved = 0.0f;
    bool linkShown = false;
    juce::String readoutText;
    juce::Rectangle<int> readoutArea;
    std::unique_ptr<juce::TextEditor> entry;
    ModKnob* entryFor = nullptr;
    int frame = 0;
    bool noteHot = false;
};
