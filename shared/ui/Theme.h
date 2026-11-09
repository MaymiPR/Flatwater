#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Flatwater's own look, the same on every editor: true black behind, neutral dark-grey panels with visible edges,
// controls lit from above, near-white lettering, the mark's cyan and amber as the accents (docs/branding/colours.json,
// docs/branding/flatwater-icon.svg). The layouts keep each original's measured positions and sizes; everything drawn
// on them comes from here.
namespace fw::theme
{
inline const juce::Colour bg { 0xff000000 },           // behind everything
                          panel { 0xff15171a },        // a card
                          raised { 0xff0b0c0e },       // a control's body, at its foot
                          raisedTop { 0xff4a4e55 },    // its lit top
                          rim { 0xff8e939a },          // the light along a control's upper rim
                          well { 0xff000000 },         // a slot, a meter's trough, a value box
                          line { 0xff353a40 },         // frames and borders
                          text { 0xfff4f4f4 },         // names and values
                          textDim { 0xffd2d6da },      // labels
                          textFaint { 0xff9a9fa5 },    // scales and ticks
                          cyan { 0xff40d6f0 },         // the value: arcs, fills, a member of a selection
                          amber { 0xffffaa1e },        // the hand: pointers, cap lines, the selected control
                          amberLight { 0xffffbf56 },
                          red { 0xffff4d4d };          // clipping

inline juce::Font font (float size, bool bold = false)
{
    return juce::Font (juce::FontOptions ("Segoe UI", size, bold ? juce::Font::bold : juce::Font::plain));
}

// a card: the panel colour, a 1 px border
inline void drawCard (juce::Graphics& g, juce::Rectangle<float> r, float radius = 6.0f)
{
    g.setColour (panel);
    g.fillRoundedRectangle (r, radius);
    g.setColour (line);
    g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);
}

// a well: a slot, a trough, a value box
inline void drawWell (juce::Graphics& g, juce::Rectangle<float> r, float radius = 2.0f)
{
    g.setColour (well);
    g.fillRoundedRectangle (r, radius);
    g.setColour (line.withAlpha (0.8f));
    g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);
}

// the cyan-to-amber fill of a meter, along its length
inline juce::ColourGradient meterGradient (juce::Rectangle<float> r, bool vertical)
{
    return vertical ? juce::ColourGradient (cyan, 0.0f, r.getBottom(), amber, 0.0f, r.getY(), false)
                    : juce::ColourGradient (cyan, r.getX(), 0.0f, amber, r.getRight(), 0.0f, false);
}

// a meter: its well, filled to `fraction` from the bottom (vertical) or the left
inline void drawMeter (juce::Graphics& g, juce::Rectangle<float> r, float fraction, bool vertical)
{
    drawWell (g, r, juce::jmin (2.0f, juce::jmin (r.getWidth(), r.getHeight()) * 0.5f));
    fraction = juce::jlimit (0.0f, 1.0f, fraction);
    if (fraction <= 0.0f) return;
    const auto inner = r.reduced (1.5f);
    const auto lit = vertical ? inner.withTop (inner.getBottom() - inner.getHeight() * fraction) : inner.withWidth (inner.getWidth() * fraction);
    juce::Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (lit.getSmallestIntegerContainer());
    g.setGradientFill (meterGradient (inner, vertical));
    g.fillRoundedRectangle (inner, 1.0f);
}

// a knob: a faint track and the value's cyan arc round a raised disc, its pointer in amber. Angles in radians,
// clockwise from straight up; `from` is where the arc starts (the start angle, or the centre of a bipolar knob)
inline void drawKnob (juce::Graphics& g, juce::Point<float> c, float radius, float angle, float start, float end, float from)
{
    const float arcR = radius + 3.5f;
    juce::Path track;
    track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, start, end, true);
    g.setColour (line);
    g.strokePath (track, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    if (std::abs (angle - from) > 1.0e-3f)
    {
        juce::Path value;
        value.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, juce::jmin (from, angle), juce::jmax (from, angle), true);
        g.setColour (cyan);
        g.strokePath (value, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    const auto disc = juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (c);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillEllipse (disc.translated (0.0f, 1.5f));
    g.setGradientFill (juce::ColourGradient (raisedTop, 0.0f, disc.getY(), raised, 0.0f, disc.getBottom(), false));
    g.fillEllipse (disc);
    g.setColour (juce::Colours::black);
    g.drawEllipse (disc.reduced (0.5f), 1.0f);
    g.setGradientFill (juce::ColourGradient (rim, 0.0f, disc.getY(), rim.withAlpha (0.0f), 0.0f, disc.getCentreY(), false));
    g.drawEllipse (disc.reduced (1.5f), 1.0f);
    const juce::Point<float> d (std::sin (angle), -std::cos (angle));
    g.setColour (amber);
    g.drawLine (juce::Line<float> (c + d * (radius * 0.25f), c + d * (radius - 2.5f)), 2.5f);
}

// a fader cap: a raised body, a border, grip lines in the line colour and an amber index across its middle
inline void drawCap (juce::Graphics& g, juce::Rectangle<float> r, int grips = 0)
{
    const float radius = juce::jmin (3.0f, r.getHeight() * 0.2f);
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRoundedRectangle (r.translated (0.0f, 2.0f), radius);
    g.setGradientFill (juce::ColourGradient (raisedTop, 0.0f, r.getY(), raised, 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r, radius);
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);
    g.setColour (rim.withAlpha (0.7f));
    g.fillRect (r.getX() + radius, r.getY() + 1.0f, r.getWidth() - 2.0f * radius, 1.0f);
    const float cy = r.getCentreY();
    g.setColour (line.brighter (0.3f));
    for (int i = 1; i <= grips; ++i)
        for (float side : { -1.0f, 1.0f })
            g.fillRect (r.getX() + 4.0f, std::round (cy + side * (float) i * 5.0f) - 0.5f, r.getWidth() - 8.0f, 1.0f);
    g.setColour (amber);
    g.fillRect (r.getX() + 3.0f, std::round (cy) - 1.0f, r.getWidth() - 6.0f, 2.0f);
}

// a lamp: dark when off, lit in its colour with a soft glow
inline void drawLamp (juce::Graphics& g, juce::Rectangle<float> r, bool on, juce::Colour colour = red)
{
    if (on)
    {
        g.setColour (colour.withAlpha (0.25f));
        g.fillRoundedRectangle (r.expanded (2.0f), 3.0f);
    }
    g.setColour (on ? colour : well);
    g.fillRoundedRectangle (r, 1.5f);
    g.setColour (on ? colour.brighter (0.4f) : line);
    g.drawRoundedRectangle (r.reduced (0.5f), 1.5f, 1.0f);
}

// a value in a well, centred
inline void drawValueBox (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& value, float size = 11.0f)
{
    drawWell (g, r);
    g.setColour (text);
    g.setFont (font (size));
    g.drawText (value, r, juce::Justification::centred);
}
}
