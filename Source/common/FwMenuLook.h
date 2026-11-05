#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "FwTheme.h"

namespace fw
{
// The toolbar's popup menus and dialogs in Flatwater's look (FwTheme.h): a dark-grey card with a 1 px border, light
// lettering, the highlighted row raised, ticks in cyan, section headings in amber.
class MenuLook : public juce::LookAndFeel_V4
{
public:
    MenuLook()
    {
        namespace th = fw::theme;
        setColour (juce::AlertWindow::backgroundColourId, th::panel);
        setColour (juce::AlertWindow::textColourId, th::text);
        setColour (juce::AlertWindow::outlineColourId, th::line);
        setColour (juce::TextEditor::backgroundColourId, th::well);
        setColour (juce::TextEditor::textColourId, th::text);
        setColour (juce::TextEditor::highlightColourId, th::cyan.withAlpha (0.35f));
        setColour (juce::TextEditor::highlightedTextColourId, th::text);
        setColour (juce::TextEditor::outlineColourId, th::line);
        setColour (juce::TextEditor::focusedOutlineColourId, th::cyan);
        setColour (juce::CaretComponent::caretColourId, th::cyan);
        setColour (juce::TextButton::buttonColourId, th::raised);
        setColour (juce::TextButton::buttonOnColourId, th::raisedTop);
        setColour (juce::TextButton::textColourOffId, th::text);
        setColour (juce::TextButton::textColourOnId, th::text);
        setColour (juce::ComboBox::outlineColourId, th::line);
        setColour (juce::PopupMenu::backgroundColourId, th::panel);
        setColour (juce::PopupMenu::textColourId, th::textDim);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, th::raised);
        setColour (juce::PopupMenu::highlightedTextColourId, th::text);
    }
    juce::Font getPopupMenuFont() override { return fw::theme::font (12.0f); }
    int getPopupMenuBorderSize() override { return 1; }

    void drawPopupMenuBackground (juce::Graphics& g, int w, int h) override
    {
        g.fillAll (fw::theme::panel);
        g.setColour (fw::theme::line);
        g.drawRect (0, 0, w, h, 1);
    }

    void getIdealPopupMenuItemSize (const juce::String& text, bool isSeparator, int, int& w, int& h) override
    {
        if (isSeparator) { w = 50; h = 5; return; }
        w = juce::GlyphArrangement::getStringWidthInt (getPopupMenuFont(), text) + 34;
        h = 18;
    }

    void drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& a, bool sep, bool active, bool hot, bool ticked,
                            bool sub, const juce::String& text, const juce::String&, const juce::Drawable*, const juce::Colour*) override
    {
        namespace th = fw::theme;
        if (sep)
        {
            g.setColour (th::line);
            g.fillRect (a.getX() + 4, a.getCentreY(), a.getWidth() - 8, 1);
            return;
        }
        if (hot && active)
        {
            g.setColour (th::raised);
            g.fillRect (a.reduced (1, 0));
            g.setColour (th::cyan);
            g.fillRect (a.getX() + 1, a.getY() + 2, 2, a.getHeight() - 4);
        }
        g.setColour ((hot && active ? th::text : th::textDim).withMultipliedAlpha (active ? 1.0f : 0.45f));
        g.setFont (getPopupMenuFont());
        g.drawText (text, a.withTrimmedLeft (18).withTrimmedRight (8), juce::Justification::centredLeft, true);
        if (ticked)
        {
            juce::Path p;
            const float x = (float) a.getX() + 5.0f, y = (float) a.getCentreY();
            p.startNewSubPath (x, y); p.lineTo (x + 2.5f, y + 3.0f); p.lineTo (x + 7.0f, y - 3.5f);
            g.setColour (th::cyan);
            g.strokePath (p, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        if (sub)
        {
            juce::Path p;
            const float x = (float) a.getRight() - 9.0f, y = (float) a.getCentreY();
            p.addTriangle (x, y - 3.0f, x, y + 3.0f, x + 4.0f, y);
            g.setColour (hot && active ? th::text : th::textDim);
            g.fillPath (p);
        }
    }

    void drawPopupMenuSectionHeader (juce::Graphics& g, const juce::Rectangle<int>& a, const juce::String& s) override
    {
        g.setColour (fw::theme::amber);
        g.setFont (fw::theme::font (11.5f, true));
        g.drawText (s, a.withTrimmedLeft (8), juce::Justification::centredLeft, true);
        g.setColour (fw::theme::line);
        g.fillRect (a.getX() + 4, a.getBottom() - 1, a.getWidth() - 8, 1);
    }
};
}
