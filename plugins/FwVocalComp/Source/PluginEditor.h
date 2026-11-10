#pragma once
#include "PluginProcessor.h"
#include "shared/ui/RenUI.h"

// Flatwater Vocal Comp editor. Designed on a 371 x 357 px grid (Flatwater's look); the window can be resized
// at that aspect ratio and everything scales with it (see flatwater::ren::Editor).
class FwVocalCompEditor : public flatwater::ren::Editor
{
public:
    explicit FwVocalCompEditor (FwVocalCompProcessor&);

    static constexpr int W = 371, H = 357;

private:
    void paintContent (juce::Graphics&) override;
    void contentMouseDown (const juce::MouseEvent&) override;
    void tick() override;
    juce::Rectangle<int> vuRect (bool right) const;
    juce::Rectangle<int> vuTextRect (bool right) const;

    FwVocalCompProcessor& proc;
    flatwater::ren::Fader& comp;
    flatwater::ren::Fader& gate;
    flatwater::ren::Fader& gain;

    flatwater::MeterBallistics energyBar { 0.341, 0.094 }, outLBar, outRBar;   // Energy: two stages, as measured
    double lastTick = 0.0;
    float energy = -80.0f, attn = 0.0f, outL = -80.0f, outR = -80.0f;
    float energyHold = -80.0f, attnHold = 0.0f, outLHold = -80.0f, outRHold = -80.0f;
};
