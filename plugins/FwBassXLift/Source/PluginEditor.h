#pragma once
#include "PluginProcessor.h"
#include "shared/ui/RenUI.h"
#include "shared/ui/FlatUI.h"
#include <map>

// Flatwater BassX Lift editor: 800 x 395 px design grid (the measured flat layout, drawn in Flatwater's look), resizable at
// that aspect ratio.
class FwBassXLiftEditor : public flatwater::ren::Editor
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
    flatwater::flat::VFader& input;
    flatwater::flat::VFader& origBass;
    flatwater::flat::VFader& harmonics;
    flatwater::flat::Knob& ratio;
    flatwater::flat::Knob& response;
    flatwater::flat::Knob& decay;
    flatwater::flat::HSlider& freq;
    flatwater::flat::SlopeBox& slope;
    flatwater::flat::RadioColumn* monitor = nullptr;
    flatwater::flat::LassoOverlay& lasso;
    juce::Point<int> lassoStart;
    bool lassoing = false;
    // the controls a lasso can take, in the reference's control order, with their parameters
    struct Member { juce::Component* c; const char* id; float referenceScale; };
    std::vector<Member> groupable;
    std::map<juce::Component*, float> gangStart;          // each member's value when the group drag began
    float gangTotal = 0.0f;

    flatwater::flat::Selection selection;
    flatwater::MeterBallistics barL { 0.155 }, barR { 0.155 };     // the reference's meters release faster than Bass Lift's (measured)
    int lastBlocks = -1;
    double lastBlockMs = -1.0e9;
    double lastTick = 0.0;
    float outL = -100.0f, outR = -100.0f, holdL = -100.0f, holdR = -100.0f;
    bool clip = false;
};
