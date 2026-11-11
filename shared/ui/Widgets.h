#pragma once
// The suite's general controls, shared by every Flatwater editor: the settings file, the mark, the meter
// ballistics, the mouse wheel, the menu look and the toolbar. The editor styles several plug-ins choose from keep
// headers of their own (FlatUI, HybridUI, RenUI); the colours and type are in Theme.h.
#include <juce_audio_processors/juce_audio_processors.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <functional>
#include "shared/ui/Theme.h"
#include "shared/Presets.h"

namespace flatwater
{
// Settings shared by all Flatwater plugins ("Apply as Default to All" window size), kept in
// <user app data>/Flatwater/settings.xml.
inline juce::File settingsFile()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Flatwater/settings.xml");
}

inline float defaultWindowScale()
{
    if (auto xml = juce::XmlDocument::parse (settingsFile()))
        return (float) xml->getDoubleAttribute ("windowScale", 1.0);
    return 1.0f;
}

inline void setDefaultWindowScale (float s)
{
    auto f = settingsFile();
    f.getParentDirectory().createDirectory();
    juce::XmlElement xml ("FwSettings");
    xml.setAttribute ("windowScale", (double) s);
    xml.writeTo (f);
}
}

namespace flatwater
{
// What the snapshot tool (tools/Snapshot.cpp, one for every plug-in of the suite) may ask of an editor beyond its
// parameters: meters frozen at given levels, and the plug-in's own options (a view, a style), each a "--name" argument
// the tool hands over after the editor is open. An editor with meters or options derives from it; the tool finds it
// by dynamic_cast.
struct SnapshotHooks
{
    virtual ~SnapshotHooks() = default;
    virtual void setTestLevels (float inDb, float outDb) = 0;
    virtual bool snapshotOption (const juce::String&) { return false; }     // true when the editor took the option
};

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

// Meter ballistics identified on the reference meters (tools/MeterCapture + scripts/nulltest/fit_meter.py, see MEASUREMENTS.md
// "Meter ballistics"): each editor frame takes the highest peak since the last frame; the bar jumps up to it at once and
// otherwise moves toward it as a one-pole on linear amplitude with a 0.325 s time constant. In silence that is a
// straight 26.7 dB/s fall; after a level drop with the signal still playing the bar settles gently onto the new level.
// The audio thread only records peaks, so the bars move the same at any buffer size.
namespace flatwater
{
// audio thread -> editor: the highest peak (dB) since the last take()
struct PeakTap
{
    static constexpr float kSilent = -100.0f;   // nothing since the last take(): -inf, so peak holds read "-Inf"
    std::atomic<float> db { kSilent };

    void push (float v) noexcept
    {
        float old = db.load (std::memory_order_relaxed);
        while (v > old && ! db.compare_exchange_weak (old, v, std::memory_order_relaxed)) {}
    }
    float take() noexcept { return db.exchange (kSilent, std::memory_order_relaxed); }
};

// editor side: instant rise, one-pole release toward the current peak (linear amplitude); an optional second stage
// of the same kind after it (the vocal compressor reference's Energy meter: 0.341 s then 0.094 s, which makes its fall start gently)
struct MeterBallistics
{
    static constexpr double floorDb = -80.0;
    double tau = 0.325, tau2 = 0.0;
    double level = 0.0, level2 = 0.0;   // linear

    MeterBallistics() = default;
    MeterBallistics (double t, double t2 = 0.0) : tau (t), tau2 (t2) {}

    float update (float peakDb, double dtSeconds) noexcept
    {
        const double dt = std::min (dtSeconds, 0.25);
        const double peak = peakDb > floorDb ? std::pow (10.0, peakDb / 20.0) : 0.0;
        level = follow (level, peak, dt, tau);
        const double shown = tau2 > 0.0 ? (level2 = follow (level2, level, dt, tau2)) : level;
        return shown > 1.0e-4 ? (float) (20.0 * std::log10 (shown)) : (float) floorDb;
    }

private:
    static double follow (double state, double target, double dt, double t) noexcept
    {
        return target >= state ? target : state + (target - state) * (1.0 - std::exp (-dt / t));
    }
};
}

// The mouse wheel on the references' controls, as measured on four of them (MeterCapture wheel scripts,
// MEASUREMENTS.md): 1/72 of the control's range per notch, a stepped control one step per notch; Ctrl changes nothing.
// JUCE's own wheel moves 15 % of the range per unit, 3.5 % per notch: 2.5 times as far. JUCE gives a Windows notch as
// 60 / 256 of wheel delta.

namespace flatwater
{
// Ctrl held right now. A host that keeps the keyboard focus (FL) sends the plug-in no key messages, so JUCE's
// modifiers stay as they were at the mouse-down; the references read the key itself, so pressing or releasing Ctrl
// during a drag switches the fine adjustment on and off (measured: MeterCapture hostwin=1, midctrl / ctrlup)
inline bool ctrlHeldNow() { return juce::ModifierKeys::getCurrentModifiersRealtime().isCtrlDown(); }

// acc: notches not yet turned into steps (stepped controls); scale: a control whose range is not the reference's (measured)
inline void fineWheel (juce::Slider& s, const juce::MouseEvent& e, const juce::MouseWheelDetails& w, double& acc, bool stepped,
                        double scale = 1.0)
{
    if (e.mods.isAnyMouseButtonDown() || ! s.isEnabled() || s.getMaximum() <= s.getMinimum()) return;
    const double notches = (std::abs (w.deltaX) > std::abs (w.deltaY) ? -w.deltaX : w.deltaY) * (w.isReversed ? -1.0 : 1.0) / (60.0 / 256.0);
    double v = s.getValue();
    if (stepped && s.getInterval() > 0.0)
    {
        acc += notches;
        const double whole = std::trunc (acc);
        acc -= whole;
        v += whole * s.getInterval();
    }
    else v = s.proportionOfLengthToValue (juce::jlimit (0.0, 1.0, s.valueToProportionOfLength (v) + notches * scale / 72.0));
    v = juce::jlimit (s.getMinimum(), s.getMaximum(), v);
    if (juce::exactlyEqual (v, s.getValue())) return;
    juce::Slider::ScopedDragNotification drag (s);
    s.setValue (v, juce::sendNotificationSync);
}

// a wheel event that reached a parent because the control under the mouse only takes clicks on its thumb (hitTest):
// the reference's controls take the wheel over their whole area
inline bool forwardWheel (juce::Component& parent, const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    for (auto* c : parent.getChildren())
        if (auto* s = dynamic_cast<juce::Slider*> (c))
            if (s->isVisible() && s->getBounds().contains (e.getEventRelativeTo (&parent).getPosition()))
            {
                s->mouseWheelMove (e.getEventRelativeTo (s), w);
                return true;
            }
    return false;
}
}

namespace flatwater
{
// The toolbar's popup menus and dialogs in Flatwater's look (shared/ui/Theme.h): a dark-grey card with a 1 px border, light
// lettering, the highlighted row raised, ticks in cyan, section headings in amber.
class MenuLook : public juce::LookAndFeel_V4
{
public:
    MenuLook()
    {
        namespace theme = flatwater::theme;
        setColour (juce::AlertWindow::backgroundColourId, theme::panel);
        setColour (juce::AlertWindow::textColourId, theme::text);
        setColour (juce::AlertWindow::outlineColourId, theme::line);
        setColour (juce::TextEditor::backgroundColourId, theme::well);
        setColour (juce::TextEditor::textColourId, theme::text);
        setColour (juce::TextEditor::highlightColourId, theme::cyan.withAlpha (0.35f));
        setColour (juce::TextEditor::highlightedTextColourId, theme::text);
        setColour (juce::TextEditor::outlineColourId, theme::line);
        setColour (juce::TextEditor::focusedOutlineColourId, theme::cyan);
        setColour (juce::CaretComponent::caretColourId, theme::cyan);
        setColour (juce::TextButton::buttonColourId, theme::raised);
        setColour (juce::TextButton::buttonOnColourId, theme::raisedTop);
        setColour (juce::TextButton::textColourOffId, theme::text);
        setColour (juce::TextButton::textColourOnId, theme::text);
        setColour (juce::ComboBox::outlineColourId, theme::line);
        setColour (juce::PopupMenu::backgroundColourId, theme::panel);
        setColour (juce::PopupMenu::textColourId, theme::textDim);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, theme::raised);
        setColour (juce::PopupMenu::highlightedTextColourId, theme::text);
    }
    juce::Font getPopupMenuFont() override { return flatwater::theme::font (12.0f); }
    int getPopupMenuBorderSize() override { return 1; }

    void drawPopupMenuBackground (juce::Graphics& g, int w, int h) override
    {
        g.fillAll (flatwater::theme::panel);
        g.setColour (flatwater::theme::line);
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
        namespace theme = flatwater::theme;
        if (sep)
        {
            g.setColour (theme::line);
            g.fillRect (a.getX() + 4, a.getCentreY(), a.getWidth() - 8, 1);
            return;
        }
        if (hot && active)
        {
            g.setColour (theme::raised);
            g.fillRect (a.reduced (1, 0));
            g.setColour (theme::cyan);
            g.fillRect (a.getX() + 1, a.getY() + 2, 2, a.getHeight() - 4);
        }
        g.setColour ((hot && active ? theme::text : theme::textDim).withMultipliedAlpha (active ? 1.0f : 0.45f));
        g.setFont (getPopupMenuFont());
        g.drawText (text, a.withTrimmedLeft (18).withTrimmedRight (8), juce::Justification::centredLeft, true);
        if (ticked)
        {
            juce::Path p;
            const float x = (float) a.getX() + 5.0f, y = (float) a.getCentreY();
            p.startNewSubPath (x, y); p.lineTo (x + 2.5f, y + 3.0f); p.lineTo (x + 7.0f, y - 3.5f);
            g.setColour (theme::cyan);
            g.strokePath (p, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        if (sub)
        {
            juce::Path p;
            const float x = (float) a.getRight() - 9.0f, y = (float) a.getCentreY();
            p.addTriangle (x, y - 3.0f, x, y + 3.0f, x + 4.0f, y);
            g.setColour (hot && active ? theme::text : theme::textDim);
            g.fillPath (p);
        }
    }

    void drawPopupMenuSectionHeader (juce::Graphics& g, const juce::Rectangle<int>& a, const juce::String& s) override
    {
        g.setColour (flatwater::theme::amber);
        g.setFont (flatwater::theme::font (11.5f, true));
        g.drawText (s, a.withTrimmedLeft (8), juce::Justification::centredLeft, true);
        g.setColour (flatwater::theme::line);
        g.fillRect (a.getX() + 4, a.getBottom() - 1, a.getWidth() - 8, 1);
    }
};
}

namespace flatwater
{
// The two-row toolbar of every Flatwater plugin, on the reference's grid (371 x 36 px):
//   row 1: mark | Skins | Mode | Undo | Redo | Prev | Next | A>B copy | Setup A/B | Save | menu
//   row 2: the preset bar across the full width (name, "*" when modified).
// Setup A/B follows the reference: one Setup button toggles between two settings, the copy
// button copies the current setup to the other one.
// Style::Flat is the newer single-row bar (800 x 36 px, e.g. Flatwater BassX Lift's reference): mark | Mode | undo | redo |
// preset box | prev | next | A>B copy | Setup A/B | save icon | menu; same behaviour.
// Style::Hybrid is the Hybrid-line bar (643 x 19 px, e.g. Flatwater Comp's reference): a logo block (the mark and the suite name),
// then Mode | undo | redo | preset bar | prev | next | A>B copy | Setup A/B | Save | menu.
// Style::Classic is the older single-row bar (e.g. the signature line's, 738 x 19 px): the Hybrid bar's layout.
// Every style keeps its layout and is drawn in Flatwater's look (shared/ui/Theme.h): raised buttons, the preset name in a well,
// the Flatwater mark at the left (on the Hybrid and Classic bars with the suite's name beside it).
class Toolbar : public juce::Component
{
public:
    enum class Style { TwoRow, Flat, Hybrid, Classic };
    bool hybridLayout() const { return style == Style::Hybrid || style == Style::Classic; }
    Toolbar (juce::UndoManager& undo, juce::AudioProcessorValueTreeState& state, juce::StringArray modeNames,
             juce::RangedAudioParameter* modeParam, juce::String productName, juce::String version,
             Style style = Style::TwoRow);

    std::function<float()> getWindowScale;            // current window magnification (1 = 100 %)
    std::function<void (float)> setWindowScale;       // choose a magnification
    std::function<void()> onLoadDefault;              // after Default Preset: the plug-in resets what is not a parameter

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void refresh();
    void setFactoryPresets (const std::vector<FactoryPreset>* list) { factory = list; }
    // parameters the Default and factory presets leave alone (besides Mode)
    void setKeepOnPresetLoad (std::vector<juce::AudioProcessorParameter*> list) { keepOnLoad = std::move (list); }

    static constexpr int width = 371, height = 36, flatWidth = 800, flatHeight = 36, hybridWidth = 643, hybridHeight = 19;
    const Style style;

private:
    void paintFlat (juce::Graphics&);
    enum Item { About, Skins, Mode, Undo, Redo, Prev, Next, Copy, Setup, Save, Menu, Preset, numItems };
    juce::Rectangle<int> rect (Item) const;
    int itemAt (juce::Point<int>) const;

    void switchSetup();
    void copyToOther();

    // presets: factory "Default" and the plugin's factory list, then user presets (.flatwaterpreset files in
    // Documents/Flatwater/User Presets/<plugin>)
    juce::File userDir() const;
    juce::Array<juce::File> userPresets() const;
    void loadDefault();
    void loadFactory (int index);
    void loadUser (const juce::File&);
    void step (int delta);
    void saveAs();
    void saveTo (const juce::File&);
    void showPresetMenu();
    void showToolbarMenu();
    void showAboutMenu();
    std::vector<float> currentValues() const;
    bool isModified() const;

    juce::UndoManager& undo;
    juce::AudioProcessorValueTreeState& apvts;
    juce::StringArray modes;
    juce::RangedAudioParameter* modeParam;
    juce::String product, version;
    juce::ValueTree slot[2];
    int active = 0;
    int hot = -1;

    juce::String presetName { "Default Preset" };
    juce::File presetFile;                 // empty for the factory presets
    const std::vector<FactoryPreset>* factory = nullptr;
    std::vector<juce::AudioProcessorParameter*> keepOnLoad;
    int factoryIndex = -1;                 // -1: Default Preset or a user preset
    std::vector<float> loadedValues;
    MenuLook menuLook;
};
}
