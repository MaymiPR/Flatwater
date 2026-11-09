#pragma once
#include "PluginProcessor.h"
#include "shared/ui/RenUI.h"

// Flatwater Bass Lift editor: 371 x 392 px design grid (the measured layout, in Flatwater's look), resizable at that
// aspect ratio.
class FwBassLiftEditor : public fw::ren::Editor
{
public:
    explicit FwBassLiftEditor (FwBassLiftProcessor&);

    static constexpr int W = 371, H = 392;

private:
    void paintContent (juce::Graphics&) override;
    void contentMouseDown (const juce::MouseEvent&) override;
    void tick() override;
    juce::Rectangle<int> vuRect (bool right) const;
    juce::Rectangle<int> vuTextRect (bool right) const;

    FwBassLiftProcessor& proc;
    fw::ren::Fader& intensity;
    fw::ren::Fader& gain;
    fw::ren::Fader& freq;
    fw::ren::InSwitch& origBass;

    fw::MeterBallistics origBar, harmBar, outLBar, outRBar;
    double lastTick = 0.0;
    float orig = -80.0f, harm = -80.0f, outL = -80.0f, outR = -80.0f;
    float origHold = -80.0f, harmHold = -80.0f, outLHold = -80.0f, outRHold = -80.0f;
    bool clip = false;
};
