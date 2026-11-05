#pragma once
#include "FwBassXLiftProcessor.h"
#include "FwRenUI.h"
#include "FwFlatUI.h"
#include <map>

// Flatwater BassX Lift editor: 800 x 395 px design grid (the measured flat layout, drawn in Flatwater's look), resizable at
// that aspect ratio.
class FwBassXLiftEditor : public fw::ren::Editor
{
public:
    explicit FwBassXLiftEditor (FwBassXLiftProcessor&);

    static constexpr int W = 800, H = 395;

private:
    void paintContent (juce::Graphics&) override;
    void contentMouseDown (const juce::MouseEvent&) override;
    void contentMouseDrag (const juce::MouseEvent&) override;
    void contentMouseUp (const juce::MouseEvent&) override;
    void setUpGroups();
    void tick() override;
    void paintGraph (juce::Graphics&);
    void paintMeters (juce::Graphics&);
    float value (const char* id) const;

    FwBassXLiftProcessor& proc;
    fw::flat::VFader& input;
    fw::flat::VFader& origBass;
    fw::flat::VFader& harmonics;
    fw::flat::Knob& ratio;
    fw::flat::Knob& response;
    fw::flat::Knob& decay;
    fw::flat::HSlider& freq;
    fw::flat::SlopeBox& slope;
    fw::flat::RadioColumn* monitor = nullptr;
    fw::flat::LassoOverlay& lasso;
    juce::Point<int> lassoStart;
    bool lassoing = false;
    // the controls a lasso can take, in the reference's control order, with their parameters
    struct Member { juce::Component* c; const char* id; float referenceScale; };
    std::vector<Member> groupable;
    std::map<juce::Component*, float> gangStart;          // each member's value when the group drag began
    float gangTotal = 0.0f;

    fw::flat::Selection selection;
    fw::MeterBallistics barL { 0.155 }, barR { 0.155 };     // MaxxBass's meters release faster than RBass's (measured)
    int lastBlocks = -1;
    double lastBlockMs = -1.0e9;
    double lastTick = 0.0;
    float outL = -100.0f, outR = -100.0f, holdL = -100.0f, holdR = -100.0f;
    bool clip = false;
};
