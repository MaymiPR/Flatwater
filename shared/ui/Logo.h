#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>

namespace fw
{
inline const char* const repoUrl = "https://github.com/MaymiPR/Flatwater";

// The Flatwater mark (docs/branding/flatwater-icon.svg): a sun half set on a dead-calm horizon, cyan into amber,
// its reflection in three flat amber bars, each shorter and fainter than the last, on a black tile (or on whatever
// is behind it). With an opaque `mono` colour everything is drawn in that one colour (a print on a panel or a
// coloured badge).
inline void drawMark (juce::Graphics& g, juce::Rectangle<float> r, bool bright, bool tile = true,
                      juce::Colour mono = juce::Colours::transparentBlack, float strokeScale = 1.0f)
{
    if (tile)
    {
        g.setColour (juce::Colours::black);
        g.fillRect (r);
    }

    // design space: the sun's top to the last bar, the horizon's ends (200 x 200 icon)
    constexpr float x0 = 26.0f, x1 = 174.0f, y0 = 62.0f, y1 = 154.5f;
    const float s = juce::jmin ((r.getWidth() - 2.0f) / (x1 - x0), (r.getHeight() - 2.0f) / (y1 - y0));
    const auto t = juce::AffineTransform::translation (-(x0 + x1) * 0.5f, -(y0 + y1) * 0.5f)
                       .scaled (s).translated (r.getCentreX(), r.getCentreY());
    const auto line = [&] (float xa, float xb, float y)
    {
        juce::Path p;
        p.startNewSubPath (xa, y);
        p.lineTo (xb, y);
        p.applyTransform (t);
        return p;
    };
    const auto width = [&] (float w) { return juce::jmax (1.2f, w * 1.3f * s) * strokeScale; };   // bolder than the icon: holds at 16 px

    const auto cyan = bright ? juce::Colour (150, 235, 255) : juce::Colour (64, 214, 240);
    const auto amber = bright ? juce::Colour (255, 210, 110) : juce::Colour (255, 170, 30);
    const bool one = ! mono.isTransparent();

    juce::Path sun;   // the half disc, its flat side on the horizon
    sun.addCentredArc (100.0f, 104.0f, 42.0f, 42.0f, 0.0f, -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi, true);
    sun.closeSubPath();
    g.setColour (one ? mono : amber);
    g.fillPath (sun, t);

    if (one) g.setColour (mono);
    else g.setGradientFill (juce::ColourGradient (cyan, r.getX() + 1.0f, 0.0f, amber, r.getRight() - 1.0f, 0.0f, false));
    g.strokePath (line (30.0f, 170.0f, 110.0f), juce::PathStrokeType (width (8.0f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    struct Bar { float xa, xb, y, w, alpha; };
    for (auto b : { Bar { 66.0f, 134.0f, 126.0f, 7.0f, 0.6f }, Bar { 80.0f, 120.0f, 140.0f, 6.0f, 0.35f }, Bar { 92.0f, 108.0f, 152.0f, 5.0f, 0.2f } })
    {
        g.setColour ((one ? mono : amber).withMultipliedAlpha (one ? 0.45f + 0.55f * b.alpha : b.alpha));
        g.strokePath (line (b.xa, b.xb, b.y), juce::PathStrokeType (width (b.w), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}
}
