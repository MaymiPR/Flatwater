#pragma once
#include "PluginProcessor.h"
#include "ui/Controls.h"
#include "shared/ui/RenUI.h"
#include "shared/ui/HybridUI.h"

// Flatwater Comp editor: 643 x 454 px design grid (the measured layout), resizable at that aspect ratio, in Flatwater's
// own look (shared/ui/Theme.h).
class FwCompEditor : public flatwater::ren::Editor
{
public:
    explicit FwCompEditor (FwCompProcessor&);

    static constexpr int W = 643, H = 454;

private:
    void paintContent (juce::Graphics&) override;
    void tick() override;
    float value (const char* id) const;
    void updateReleaseKnob();
    void showMeter();

    FwCompProcessor& proc;
    flatwater::hybrid::Knob& analog;
    flatwater::hybrid::Knob& output;
    flatwater::hybrid::Knob& ratio;
    flatwater::hybrid::Knob& mix;
    flatwater::hybrid::Knob& threshold;
    flatwater::hybrid::Knob& punch;
    flatwater::hybrid::Knob& attack;
    flatwater::hybrid::Knob& release;
    flatwater::hybrid::Knob& releaseNote;
    flatwater::hybrid::VuMeter& vu;
    juce::Slider& bpmField = make<juce::Slider>();
    void contentMouseDown (const juce::MouseEvent&) override { selection.clear(); }
    flatwater::flat::Selection selection;
    int lastSync = -1;
    double lastTick = 0.0, pendingDt = 0.0, lampLevel = 0.0;
    int lastBlocks = -1;
    co::needle::Gr grNeedle;                           // the three meters run all the time; the needle shows one
    co::needle::Io inNeedle, outNeedle;
    co::needle::Lamp lamp;
};
