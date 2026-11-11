#include "PluginEditor.h"
#include "shared/ui/Theme.h"
#include "shared/ui/Widgets.h"

using namespace juce;
using namespace pb;
using namespace pb::dsp;
using namespace pb::ui;
namespace theme = flatwater::theme;

namespace
{
// the main plate: the reference's arrangement, its controls enlarged to fill the plate; the panel
// keeps the reference's measured boxes (design px; RESEARCH.md, "The editor")
const Rectangle<int> kPitch { 85, 65, 170, 170 }, kFormant { 302, 65, 170, 170 }, kLink { 258, 130, 41, 41 },
    kMix { 467, 93, 96, 96 }, kFine { 256, 186, 45, 46 }, kFlattenLamp { 10, 149, 20, 20 }, kNote { 32, 146, 60, 24 },
    kExpand { 513, 259, 34, 25 }, kType { 55, 302, 101, 18 }, kScreenM { 56, 325, 219, 104 }, kScreenOrganic { 38, 325, 237, 105 },
    kScale { 277, 321, 15, 111 }, kSteps { 56, 434, 76, 16 }, kSync { 359, 306, 20, 20 };
// Pitch's and Formant's names, and in their place their values while one is selected or Link is dragged
const Rectangle<int> kPill[2] { Rectangle<int> (116, 24).withCentre ({ 170, 46 }), Rectangle<int> (116, 24).withCentre ({ 387, 46 }) };
// the slots: a row of four under each knob, evenly spaced (Pitch and Formant 32 x 22 with 6 px between, Mix 20 x 17
// with 3, the small knobs 14 x 12 with 4), clear of Link and Fine
Rectangle<int> slotRect (int target, Rectangle<int> knob, int s)
{
    if (target <= 1) return { knob.getCentreX() - 73 + 38 * s, 216, 32, 22 };
    if (target == 2) return { knob.getCentreX() - 44 + 23 * s, 190, 20, 17 };
    return { knob.getX() + 18 * s, knob.getY() + 57, 14, 12 };
}
constexpr int kTabX[4] { 161, 223, 285, 347 }, kTabY = 259, kTabW = 56, kTabH = 26, kPlateBottom = 289;
const char* const kTabNames[4] { "M1", "M2", "AM", "PT" };

Rectangle<int> tabRect (int t) { return { kTabX[t], kTabY, kTabW, kTabH }; }
// a knob's value while it is selected: over a panel knob's name, or over Mix's name above it
Rectangle<int> valueBox (const Component& k)
{
    return k.getWidth() > 80 ? Rectangle<int> (64, 20).withCentre ({ k.getBounds().getCentreX(), k.getY() - 7 })
                             : Rectangle<int> { k.getX() + 6, k.getY() - 5, 57, 17 };
}

String fixed (double v, int digits) { return String (v, digits); }
}

// a plain area with its own mouse handling and drawing (Link, the Flatten note, Steps)
struct Hotspot : public Component
{
    std::function<void (Graphics&)> draw;
    std::function<void (const MouseEvent&)> down, drag, up;
    std::function<void (int)> wheel;
    std::function<void (bool)> hover;
    Hotspot() { setWantsKeyboardFocus (false); setMouseClickGrabsKeyboardFocus (false); }
    void paint (Graphics& g) override { if (draw) draw (g); }
    void mouseDown (const MouseEvent& e) override { if (down) down (e); }
    void mouseDrag (const MouseEvent& e) override { if (drag) drag (e); }
    void mouseUp (const MouseEvent& e) override { if (up) up (e); }
    void mouseEnter (const MouseEvent&) override { if (hover) hover (true); }
    void mouseExit (const MouseEvent&) override { if (hover) hover (false); }
    void mouseWheelMove (const MouseEvent&, const MouseWheelDetails& w) override { if (wheel && ! exactlyEqual (w.deltaY, 0.0f)) wheel (w.deltaY > 0.0f ? 1 : -1); }
};

// on top of everything, taking no clicks: a dragged modulator's badge, a slot's readout, the selected small knob's value
class FwPitchBendEditor::Overlay : public Component
{
public:
    explicit Overlay (FwPitchBendEditor& e) : ed (e) { setInterceptsMouseClicks (false, false); }
    void paint (Graphics& g) override
    {
        if (auto* k = dynamic_cast<ModKnob*> (ed.ui.selected); k != nullptr && k->kind != ModKnob::Big && k->isVisible())
            theme::drawValueBox (g, valueBox (*k).toFloat(), k->valueText(), k->kind == ModKnob::Medium ? 13.0f : 11.0f);
        if (ed.readoutText.isNotEmpty())
            theme::drawValueBox (g, Rectangle<float> (84.0f, 18.0f).withCentre (ed.readoutArea.getCentre().toFloat())
                                     .withY ((float) ed.readoutArea.getY() - 20.0f), ed.readoutText, 11.0f);
        if (ed.ui.dragging != None)
        {
            const auto r = Rectangle<float> (40.0f, 20.0f).withCentre (badgeAt);
            g.setColour (theme::raised);
            g.fillRoundedRectangle (r, 10.0f);
            g.setColour (sourceColour (ed.ui.dragging));
            g.drawRoundedRectangle (r.reduced (0.5f), 10.0f, 1.5f);
            g.setFont (theme::font (12.0f, true));
            g.drawText (sourceName (ed.ui.dragging), r, Justification::centred);
        }
    }
    Point<float> badgeAt;
    FwPitchBendEditor& ed;
};

//==============================================================================
FwPitchBendEditor::FwPitchBendEditor (FwPitchBendProcessor& p)
    : Editor (p, p.apvts, p.undo, p.apvts.getParameter (pb::id::mode), "Flatwater Pitch Bend", JucePlugin_VersionString, W,
              (bool) p.apvts.state.getProperty (pb::dsp::expandedId, false) ? HOpen : HClosed,
              flatwater::Toolbar::Style::Hybrid, pb::modeNames()),
      proc (p), ui { p }
{
    ui.changed = [this] { content().repaint(); };
    ui.menuLook = &menuLook;
    setFactoryPresets (pb::factoryPresets());
    onDefaultPreset ([this] { proc.resetExtras(); });
    ui.enter = [this] (ModKnob& k) { openEntry (k); };
    ui.readout = [this] (const String& t, Rectangle<int> r) { readoutText = t; readoutArea = r; if (overlay) overlay->repaint(); };

    auto semis = [this] (double v)
    {
        return proc.fine() ? String (roundToInt (v * 100.0)) + " cents" : String (roundToInt (v)) + " semitones";
    };
    pitch = &knob (ModKnob::Big, 0, pb::id::pitch, "PITCH", kPitch);
    formant = &knob (ModKnob::Big, 1, pb::id::formant, "FORMANT", kFormant);
    pitch->format = formant->format = semis;
    mix = &knob (ModKnob::Medium, 2, pb::id::mix, "MIX", kMix);
    mix->format = [] (double v) { return fixed (v, 1); };
    mix->wheelStep = 0.013;

    // Link: moves Pitch and Formant by the same amount, stopping where either reaches its end; it keeps no value
    auto& link = make<Hotspot>();
    addToContent (link, kLink);
    link.draw = [] (Graphics& g)
    {
        const auto c = Point<float> (20.5f, 20.5f);
        const auto disc = Rectangle<float> (36.0f, 36.0f).withCentre (c);
        g.setGradientFill (ColourGradient (theme::raisedTop, 0.0f, disc.getY(), theme::raised, 0.0f, disc.getBottom(), false));
        g.fillEllipse (disc);
        g.setColour (Colours::black);
        g.drawEllipse (disc.reduced (0.5f), 1.0f);
        g.setColour (theme::textDim);                                   // a chain: two links
        g.drawRoundedRectangle (Rectangle<float> (14.0f, 8.0f).withCentre (c.translated (-4.0f, 0.0f)), 4.0f, 1.8f);
        g.drawRoundedRectangle (Rectangle<float> (14.0f, 8.0f).withCentre (c.translated (4.0f, 0.0f)), 4.0f, 1.8f);
    };
    auto moveBoth = [this] (float delta)
    {
        const float p0 = (float) pitch->getValue(), f0 = (float) formant->getValue();
        const float d = jlimit (-12.0f - jmin (p0, f0), 12.0f - jmax (p0, f0), delta);   // the offset is kept
        if (exactlyEqual (d, 0.0f)) return;
        pitch->setValue (p0 + d, sendNotificationSync);
        formant->setValue (f0 + d, sendNotificationSync);
    };
    link.down = [this] (const MouseEvent& e)
    {
        ui.select (nullptr);
        linkShown = true;
        if (e.mods.isAltDown()) { pitch->setValue (0.0, sendNotificationSync); formant->setValue (0.0, sendNotificationSync); return; }
        lastY = e.position.y;
        linkAccum = 0.0f;
        linkMoved = 0.0f;
        if (pitch->onDragStart) pitch->onDragStart();
        if (formant->onDragStart) formant->onDragStart();
        if (isShowing()) e.source.enableUnboundedMouseMovement (true);
    };
    link.drag = [this, moveBoth] (const MouseEvent& e)
    {
        const float dy = lastY - e.position.y;
        lastY = e.position.y;
        const bool fine = proc.fine();
        const float slow = fine ? 1.0f / 2400.0f : 0.53e-3f, fast = fine ? 2.0e-3f : 1.6e-3f, step = std::abs (dy);
        const float perPx = (step <= 2.0f ? slow : step >= 4.0f ? fast : slow + (fast - slow) * (step - 2.0f) / 2.0f) * 24.0f;
        linkAccum += dy * perPx * (e.mods.isCtrlDown() ? 0.25f : 1.0f);
        // Fine off: both stick on whole semitones, as Pitch and Formant do
        const float want = proc.fine() ? linkAccum : std::round (linkAccum);
        const float p0 = (float) pitch->getValue();
        moveBoth (want - linkMoved);
        linkMoved += (float) pitch->getValue() - p0;
        content().repaint (kPill[0].getUnion (kPill[1]));
    };
    link.up = [this] (const MouseEvent&)
    {
        if (pitch->onDragEnd) pitch->onDragEnd();
        if (formant->onDragEnd) formant->onDragEnd();
        linkShown = false;
        content().repaint();
    };
    link.wheel = [this, moveBoth] (int dir)
    {
        pitch->setValue (std::round (pitch->getValue()), sendNotificationSync);
        formant->setValue (std::round (formant->getValue()), sendNotificationSync);
        moveBoth ((float) dir);
    };

    // Fine, Flatten
    auto& fine = make<Toggle> (ui, "FINE", [this] { return proc.fine(); }, [this]
    {
        proc.undo.beginNewTransaction();
        proc.apvts.state.setProperty (fineId, ! proc.fine(), &proc.undo);
        content().repaint();
    });
    addToContent (fine, kFine);
    auto& flattenLamp = make<Toggle> (ui, "", [this] { return proc.get (pb::id::flatten) > 0.5f; }, [this]
    {
        auto& pr = param (pb::id::flatten);
        pr.beginChangeGesture();
        pr.setValueNotifyingHost (pr.getValue() > 0.5f ? 0.0f : 1.0f);
        pr.endChangeGesture();
    }, true);
    addToContent (flattenLamp, kFlattenLamp);

    // the Flatten note: a box as the toolbar's preset box. A click steps it a semitone up (as the reference's, measured),
    // Alt-click its default (C3), the arrow at its right end the list (the toolbar's menu look, the current note ticked);
    // a vertical drag steps it (~10 px per semitone), the wheel a semitone
    auto& note = make<Hotspot>();
    addToContent (note, kNote);
    auto setNote = [this] (int i)
    {
        auto& pr = param (pb::id::flattenNote);
        pr.beginChangeGesture();
        pr.setValueNotifyingHost (pr.convertTo0to1 ((float) jlimit (0, 23, i)));
        pr.endChangeGesture();
    };
    note.draw = [this, &note] (Graphics& g)
    {
        const auto r = note.getLocalBounds().toFloat().reduced (0.5f);
        const bool hot = noteHot || press == Press::Note;
        g.setColour (theme::well);
        g.fillRoundedRectangle (r, 2.0f);
        g.setColour (hot ? theme::cyan.withAlpha (0.8f) : theme::line);
        g.drawRoundedRectangle (r, 2.0f, 1.0f);
        g.setColour (proc.get (pb::id::flatten) > 0.5f ? theme::text : theme::textDim);
        g.setFont (theme::font (14.0f, true));
        g.drawText (noteNames()[roundToInt (proc.get (pb::id::flattenNote))], r.withTrimmedRight (16.0f), Justification::centred);
        g.setColour (theme::line);
        g.fillRect (r.getRight() - 16.0f, r.getY() + 3.0f, 1.0f, r.getHeight() - 6.0f);
        Path tri;
        const Point<float> c (r.getRight() - 8.0f, r.getCentreY());
        tri.addTriangle (c.x - 3.0f, c.y - 1.5f, c.x + 3.0f, c.y - 1.5f, c.x, c.y + 2.0f);
        g.setColour (hot ? theme::cyan : theme::textDim);
        g.fillPath (tri);
    };
    note.down = [this, setNote] (const MouseEvent& e)
    {
        if (e.mods.isAltDown())
        {
            setNote (roundToInt (param (pb::id::flattenNote).convertFrom0to1 (param (pb::id::flattenNote).getDefaultValue())));
            press = Press::None;
            return;
        }
        press = Press::Note;
        pressStart = roundToInt (proc.get (pb::id::flattenNote));
        pressAt = e.position;
    };
    note.drag = [this, setNote] (const MouseEvent& e)
    {
        if (press == Press::Note && e.position.getDistanceFrom (pressAt) > 2.0f) setNote (pressStart + roundToInt ((pressAt.y - e.position.y) / 10.0f));
    };
    note.up = [this, setNote, &note] (const MouseEvent& e)
    {
        if (press != Press::Note) return;
        press = Press::None;
        note.repaint();
        if (e.position.getDistanceFrom (pressAt) > 2.0f) return;
        if (pressAt.x < (float) note.getWidth() - 17.0f) { setNote (pressStart + 1); return; }   // the box: the next key
        PopupMenu m;
        m.setLookAndFeel (&menuLook);
        m.addSectionHeader ("Flatten to");
        const int cur = roundToInt (proc.get (pb::id::flattenNote));
        for (int i = 0; i < 24; ++i) m.addItem (i + 1, noteNames()[i], true, i == cur);
        m.showMenuAsync (PopupMenu::Options().withTargetComponent (&note).withMinimumWidth (note.getWidth()).withStandardItemHeight (18),
                         [setNote] (int r) { if (r > 0) setNote (r - 1); });
    };
    note.hover = [this, &note] (bool in) { noteHot = in; note.repaint(); };
    note.wheel = [this, setNote] (int dir) { setNote (roundToInt (proc.get (pb::id::flattenNote)) + dir); };

    // the modulators' panel: M1 and M2 (each its own knobs), AM, PT
    auto& scr = make<Screen> (ui);
    screen = &scr;
    addToContent (scr, kScreenM);
    auto& lib = make<Library> (ui);
    library = &lib;
    addToContent (lib, kScreenM);
    lib.setVisible (false);
    lib.closed = [this] { content().repaint(); };

    for (int m = 0; m < 2; ++m)
    {
        const auto& d = pb::mod[m];
        const int t0 = 3 + 4 * m;                                    // m?Rate, Warp, Phase, Level in targets[]
        auto& type = make<Toggle> (ui, "LFO|SEQ", [this, m] { return proc.get (pb::mod[m].type) > 0.5f; }, [this, m]
        {
            auto& pr = param (pb::mod[m].type);
            pr.beginChangeGesture();
            pr.setValueNotifyingHost (pr.getValue() > 0.5f ? 0.0f : 1.0f);
            pr.endChangeGesture();
            library->setVisible (false);
            screen->repaint();
        });
        type.look = Toggle::Look::Pair;
        addToContent (type, kType);
        auto& hz = knob (ModKnob::Small, t0, d.rate, "RATE", { 301, 310, 68, 68 });
        hz.format = [] (double v) { return fixed (v, 3); };
        hz.wheelStep = 0.0137;                                       // (the wheels measured per notch)
        auto& synced = knob (ModKnob::Small, t0, d.rateSynced, "RATE", { 301, 310, 68, 68 }, false);
        synced.stepped = true;
        synced.format = [] (double v) { return syncedRates()[roundToInt (v)]; };
        rateHz[m] = &hz;
        rateSynced[m] = &synced;
        auto& sync = make<Toggle> (ui, "", [this, m] { return proc.get (pb::mod[m].sync) < 0.5f; }, [this, m]
        {
            auto& pr = param (pb::mod[m].sync);
            pr.beginChangeGesture();
            pr.setValueNotifyingHost (pr.getValue() > 0.5f ? 0.0f : 1.0f);
            pr.endChangeGesture();
        });
        sync.look = Toggle::Look::Icon;
        sync.icon = "sync";
        addToContent (sync, kSync);
        auto& warp = knob (ModKnob::Small, t0 + 1, d.warp, "WARP", { 378, 310, 68, 68 });
        warp.format = [] (double v) { return fixed (v, 2); };
        warp.wheelStep = 0.0088;
        auto& ph = knob (ModKnob::Small, t0 + 2, d.phase, "PHASE", { 301, 384, 68, 68 });
        ph.format = [] (double v) { return fixed (v, 3); };
        ph.wheelStep = 0.0118;
        auto& smooth = knob (ModKnob::Small, -1, d.smooth, "SMOOTH", { 378, 384, 68, 68 });
        smooth.format = [] (double v) { return fixed (v, 1); };
        auto& level = knob (ModKnob::Small, t0 + 3, d.level, "LEVEL", { 472, 349, 68, 68 });
        level.format = [] (double v) { return fixed (v, 2); };
        level.wheelStep = 0.0161;
        // (the synced Rate shares the Hz knob's slots: they are drawn once, under whichever shows)
        for (auto* c : std::initializer_list<Component*> { &type, &hz, &synced, &sync, &warp, &ph, &smooth, &level }) panel[m].push_back (c);
        for (auto* c : content().getChildren())
            if (auto* s = dynamic_cast<SlotPad*> (c); s != nullptr && (&s->owner == &hz || &s->owner == &warp || &s->owner == &ph || &s->owner == &level))
                panel[m].push_back (s);
    }
    auto tool = [this] (const char* icon, Rectangle<int> r, std::function<bool()> get, std::function<void()> click) -> Toggle&
    {
        auto& t = make<Toggle> (ui, "", std::move (get), std::move (click));
        t.look = Toggle::Look::Icon;
        t.icon = icon;
        addToContent (t, r);
        modTools.push_back (&t);
        return t;
    };
    drawTool = &tool ("draw", { 20, 331, 28, 26 }, [this] { return screen->drawMode; }, [this] { screen->drawMode = ! screen->drawMode; screen->repaint(); });
    tool ("erase", { 19, 363, 27, 30 }, [] { return false; }, [this]
    {
        const int m = tab();
        auto t = getTable (proc.apvts.state, m);
        if (proc.get (pb::mod[m].type) > 0.5f) for (int i = 0; i < maxSteps; ++i) t[(size_t) (seqFirst + i)] = 0.0f;
        else for (int i = 0; i < lfoPoints; ++i) t[(size_t) i] = 0.0f;
        proc.undo.beginNewTransaction();
        setTable (proc.apvts.state, m, t, &proc.undo);
        screen->repaint();
    });
    browseTool = &tool ("browse", { 17, 393, 31, 31 }, [this] { return library->isVisible(); }, [this]
    {
        if (library->isVisible()) library->setVisible (false);
        else library->open (tab(), proc.get (pb::mod[tab()].type) > 0.5f);
        content().repaint();
    });
    deleteTool = &tool ("delete", { 9, 429, 25, 28 }, [this] { return library->deleteMode; }, [this] { library->deleteMode = ! library->deleteMode; library->repaint(); });
    saveTool = &tool ("save", { 32, 433, 19, 19 }, [] { return false; }, [this] { library->saveCurrent(); });

    // AM and PT
    const char* organic[2][3] { { pb::id::amAttack, pb::id::amRelease, pb::id::amLevel },
                                { pb::id::ptSmooth, pb::id::ptOffset, pb::id::ptLevel } };
    const char* organicLabels[2][3] { { "ATTACK", "RELEASE", "LEVEL" }, { "SMOOTH", "OFFSET", "LEVEL" } };
    const int organicTargets[2][3] { { 11, 12, 13 }, { 14, -1, 15 } };
    for (int o = 0; o < 2; ++o)
        for (int k = 0; k < 3; ++k)
        {
            auto& kn = knob (ModKnob::Small, organicTargets[o][k], organic[o][k], organicLabels[o][k], { 308 + 86 * k - (k == 2 ? 8 : 0), 350, 68, 68 });
            kn.format = o == 0 && k < 2 ? std::function<String (double)> ([] (double v) { return fixed (v, 1) + " ms"; })
                                        : std::function<String (double)> ([o, k] (double v) { return fixed (v, o == 1 && k == 0 ? 1 : 2); });
            kn.wheelStep = o == 0 ? (k == 2 ? 0.0148 : 0.0135) : (k == 0 ? 0.0135 : 0.0149);
            panel[2 + o].push_back (&kn);
            for (auto* c : content().getChildren())
                if (auto* s = dynamic_cast<SlotPad*> (c); s != nullptr && &s->owner == &kn) panel[2 + o].push_back (s);
        }

    // Steps (the sequencer only): a text knob, ~40 px per step, the wheel a step, Alt-click 16
    auto& steps = make<Hotspot>();
    addToContent (steps, kSteps);
    steps.draw = [this] (Graphics& g)
    {
        const int m = jmin (1, tab());
        g.setColour (theme::textDim);
        g.setFont (theme::font (8.5f, true));
        g.drawText ("STEPS", Rectangle<int> (0, 0, 32, 16), Justification::centredLeft);
        theme::drawValueBox (g, Rectangle<float> (32.0f, 0.5f, 42.0f, 15.0f), String (roundToInt (proc.get (pb::mod[m].steps))), 11.0f);
    };
    auto setSteps = [this] (int n)
    {
        auto& pr = param (pb::mod[jmin (1, tab())].steps);
        pr.beginChangeGesture();
        pr.setValueNotifyingHost (pr.convertTo0to1 ((float) jlimit (2, 16, n)));
        pr.endChangeGesture();
        screen->repaint();
    };
    steps.down = [this, setSteps] (const MouseEvent& e)
    {
        if (e.mods.isAltDown()) { setSteps (16); return; }
        press = Press::Steps;
        pressStart = roundToInt (proc.get (pb::mod[jmin (1, tab())].steps));
        pressAt = e.position;
    };
    steps.drag = [this, setSteps] (const MouseEvent& e) { if (press == Press::Steps) setSteps (pressStart + roundToInt ((pressAt.y - e.position.y) / 40.0f)); };
    steps.up = [this] (const MouseEvent&) { press = Press::None; };
    steps.wheel = [this, setSteps] (int dir) { setSteps (roundToInt (proc.get (pb::mod[jmin (1, tab())].steps)) + dir); };
    for (int m = 0; m < 2; ++m) panel[m].push_back (&steps);

    overlay = &make<Overlay> (*this);
    addToContent (*overlay, { 0, 0, W, HOpen });
    setExpanded (expanded());
}

FwPitchBendEditor::~FwPitchBendEditor()
{
    closeEntry (false);
}

// a knob bound to its parameter (gestures while dragging, a complete change otherwise) and, for a modulatable
// control, its four slots at the reference's places
ModKnob& FwPitchBendEditor::knob (ModKnob::Kind kind, int target, const char* id, String label, Rectangle<int> r, bool slots)
{
    auto& k = make<ModKnob> (ui, kind, target, std::move (label));
    auto& rp = param (id);
    const auto fr = rp.getNormalisableRange();
    NormalisableRange<double> dr ((double) fr.start, (double) fr.end,
                                  [fr] (double, double, double v) { return (double) fr.convertFrom0to1 ((float) v); },
                                  [fr] (double, double, double v) { return (double) fr.convertTo0to1 ((float) v); },
                                  [fr] (double, double, double v) { return (double) fr.snapToLegalValue ((float) v); });
    k.setNormalisableRange (dr);
    k.defaultValue = (double) rp.convertFrom0to1 (rp.getDefaultValue());
    auto gesture = std::make_shared<bool> (false);
    auto att = std::make_unique<ParameterAttachment> (rp, [&k] (float v) { k.setValue (v, dontSendNotification); k.repaint(); }, &proc.undo);
    auto* a = att.get();
    k.onDragStart = [a, gesture] { *gesture = true; a->beginGesture(); };
    k.onDragEnd = [a, gesture] { *gesture = false; a->endGesture(); };
    k.onValueChange = [this, a, gesture, &k]
    {
        if (*gesture) a->setValueAsPartOfGesture ((float) k.getValue());
        else a->setValueAsCompleteGesture ((float) k.getValue());
        if (&k == pitch || &k == formant) content().repaint (kPill[0].getUnion (kPill[1]));
        if (overlay != nullptr && ui.selected == &k) overlay->repaint (valueBox (k));
    };
    att->sendInitialUpdate();
    keepAttachment (std::move (att));
    addToContent (k, r);
    knobs.push_back (&k);
    if (target >= 0 && slots)
        for (int s = 0; s < numSlots; ++s)
        {
            addToContent (make<SlotPad> (ui, k, s, target <= 1), slotRect (target, r, s));
        }
    return k;
}

bool FwPitchBendEditor::expanded() const { return (bool) proc.apvts.state.getProperty (expandedId, false); }
int FwPitchBendEditor::tab() const { return jlimit (0, 3, (int) proc.apvts.state.getProperty (tabId, 3)); }

// the panel opens and closes with the window (564 x 465 / 289), at the same scale; the choice is saved with the state
void FwPitchBendEditor::setExpanded (bool e)
{
    proc.apvts.state.setProperty (expandedId, e, nullptr);
    setDesignHeight (e ? HOpen : HClosed);
    showTab (tab());
}

void FwPitchBendEditor::showTab (int t)
{
    proc.apvts.state.setProperty (tabId, t, nullptr);
    const bool open = expanded();
    for (auto& list : panel) for (auto* c : list) c->setVisible (false);
    for (auto* c : modTools) c->setVisible (false);
    library->setVisible (false);
    screen->drawMode = false;
    if (ui.selected != nullptr && ! ui.selected->isVisible()) ui.selected = nullptr;
    screen->setVisible (open);
    if (open)
    {
        for (auto* c : panel[t]) c->setVisible (true);
        screen->setBounds (t < 2 ? kScreenM : kScreenOrganic);
        screen->show (t);
        if (t < 2)
        {
            for (auto* c : modTools) c->setVisible (c != saveTool && c != deleteTool);
            const bool synced = proc.get (pb::mod[t].sync) < 0.5f;
            rateHz[t]->setVisible (! synced);
            rateSynced[t]->setVisible (synced);
            const bool seq = proc.get (pb::mod[t].type) > 0.5f;
            for (auto* c : content().getChildren())
                if (auto* h = dynamic_cast<Hotspot*> (c); h != nullptr && h->getBounds() == kSteps) h->setVisible (seq);
            drawTool->setVisible (! seq);
        }
    }
    content().repaint();
}

ModKnob* FwPitchBendEditor::targetAt (Point<float> p) const
{
    for (auto* k : knobs)
        if (k->framed && k->isVisible() && k->getBounds().toFloat().contains (p)) return k;
    return nullptr;
}

void FwPitchBendEditor::tick()
{
    if (++frame % 2 != 0) return;
    // the dots of modulated controls, the lamps and switches (30 Hz)
    for (auto* k : knobs)
        if (k->target >= 0 && k->isVisible() && std::abs (proc.modulated[(size_t) k->target].load() - (float) k->getValue()) > 1.0e-6f) k->repaint();
    if (frame % 6 == 0)
    {
        for (auto* c : content().getChildren())
            if (dynamic_cast<Toggle*> (c) != nullptr || dynamic_cast<Hotspot*> (c) != nullptr) c->repaint();
        const int t = tab();
        if (expanded() && t < 2)                                    // Sync or Type changed from the host
        {
            const bool synced = proc.get (pb::mod[t].sync) < 0.5f;
            const bool seq = proc.get (pb::mod[t].type) > 0.5f;
            bool stepsShown = false;
            for (auto* c : content().getChildren())
                if (auto* h = dynamic_cast<Hotspot*> (c); h != nullptr && h->getBounds() == kSteps) stepsShown = h->isVisible();
            if (rateHz[t]->isVisible() == synced || stepsShown != seq) showTab (t);
        }
    }
}

//==============================================================================
void FwPitchBendEditor::paintContent (Graphics& g)
{
    g.fillAll (theme::bg);
    theme::drawCard (g, Rectangle<float> (2.0f, 23.0f, (float) W - 4.0f, (float) kPlateBottom - 25.0f), 8.0f);

    // our mark and name where the original prints its own
    flatwater::drawMark (g, { 500.0f, 30.0f, 30.0f, 26.0f }, false, false);
    g.setColour (theme::text);
    g.setFont (theme::font (14.0f, true));
    g.drawText ("PITCH BEND", Rectangle<float> (473.0f, 56.0f, 84.0f, 18.0f), Justification::centred);

    // the names (a value takes its name's place while it shows)
    g.setColour (theme::textDim);
    g.setFont (theme::font (15.0f, true));
    for (int i = 0; i < 2; ++i)
        if (! (ui.selected == (i == 0 ? pitch : formant) || linkShown))
            g.drawText (i == 0 ? "PITCH" : "FORMANT", kPill[i], Justification::centred);
    g.setFont (theme::font (13.0f, true));
    if (ui.selected != mix) g.drawText ("MIX", Rectangle<int> (64, 18).withCentre ({ kMix.getCentreX(), kMix.getY() - 7 }), Justification::centred);
    g.setFont (theme::font (12.0f, true));
    g.drawText ("FLATTEN", Rectangle<int> (90, 16).withCentre ({ 51, 131 }), Justification::centred);

    // the scales' ends: -12 / 12, or -1200 / 1200 with Fine
    const bool fine = proc.fine();
    g.setColour (theme::textFaint);
    g.setFont (theme::font (11.0f));
    for (auto* k : { pitch, formant })
    {
        const auto c = k->getBounds().toFloat().getCentre();
        for (int side : { -1, 1 })
            g.drawText (fine ? (side < 0 ? "-1200" : "1200") : (side < 0 ? "-12" : "12"),
                        Rectangle<float> (40.0f, 13.0f).withCentre ({ c.x + (float) side * 66.0f, 206.0f }), Justification::centred);
    }

    // the values above Pitch and Formant while one is selected or Link is dragged
    for (int i = 0; i < 2; ++i)
    {
        auto* k = i == 0 ? pitch : formant;
        if (ui.selected == k || linkShown) theme::drawValueBox (g, kPill[i].toFloat(), k->valueText(), 14.0f);
    }

    // the tabs (each a modulator to drag onto a control) and the panel's open / close
    const int active = tab();
    for (int t = 0; t < 4; ++t)
    {
        const auto r = tabRect (t).toFloat();
        const bool on = expanded() && t == active;
        g.setColour (on ? theme::raisedTop : theme::raised);
        g.fillRoundedRectangle (r, 13.0f);
        g.setColour (on ? theme::amber : theme::line);
        g.drawRoundedRectangle (r.reduced (0.5f), 13.0f, 1.0f);
        g.setColour (on ? theme::amber : sourceColour (t + 1));
        g.setFont (theme::font (14.0f, true));
        g.drawText (kTabNames[t], r, Justification::centred);
    }
    {
        const auto r = kExpand.toFloat().reduced (9.0f, 7.0f);
        Path tri;
        if (expanded()) tri.addTriangle (r.getX(), r.getY(), r.getRight(), r.getY(), r.getCentreX(), r.getBottom());
        else tri.addTriangle (r.getX(), r.getBottom(), r.getRight(), r.getBottom(), r.getCentreX(), r.getY());
        g.setColour (theme::textDim);
        g.fillPath (tri);
    }

    if (! expanded()) return;
    theme::drawCard (g, Rectangle<float> (2.0f, 292.0f, (float) W - 4.0f, (float) HOpen - 294.0f), 8.0f);
    g.setColour (theme::textFaint);
    g.setFont (theme::font (8.0f));
    if (active < 2)
    {
        const bool seq = proc.get (pb::mod[active].type) > 0.5f;
        const char* labels[3] { seq ? "24" : "1", "0", seq ? "-24" : "-1" };
        for (int i = 0; i < 3; ++i)
            g.drawText (labels[i], Rectangle<int> (kScale.getX(), kScale.getY() + i * 50, kScale.getWidth() + 6, 12), Justification::centredLeft);
    }
    else
    {
        g.setColour (theme::textDim);
        g.setFont (theme::font (11.0f, true));
        g.drawText (active == 2 ? "AMPLITUDE" : "PITCH", Rectangle<int> (38, 304, 150, 16), Justification::centredLeft);
        g.setColour (theme::textFaint);
        g.setFont (theme::font (8.0f));
        const char* labels[3] { active == 2 ? "1" : "C5", active == 2 ? "" : "C3", active == 2 ? "0" : "C1" };
        for (int i = 0; i < 3; ++i)
            g.drawText (labels[i], Rectangle<int> (277, 321 + i * 50, 22, 12), Justification::centredLeft);
    }
}

void FwPitchBendEditor::contentMouseDown (const MouseEvent& e)
{
    press = Press::None;
    for (int t = 0; t < 4; ++t)
        if (tabRect (t).toFloat().contains (e.position)) { press = Press::Tab; pressTab = t; pressAt = e.position; return; }
    if (kExpand.toFloat().contains (e.position)) { setExpanded (! expanded()); return; }
    ui.select (nullptr);
}

void FwPitchBendEditor::contentMouseDrag (const MouseEvent& e)
{
    if (press != Press::Tab) return;
    if (ui.dragging == None && e.position.getDistanceFrom (pressAt) < 4.0f) return;
    if (ui.dragging == None)                                         // the assignment starts: frame what it can go on
    {
        ui.dragging = pressTab + 1;
        for (auto* k : knobs) k->framed = k->target >= 0 && k->isVisible();
        for (auto* c : content().getChildren()) if (dynamic_cast<SlotPad*> (c) != nullptr) c->repaint();
    }
    auto* t = targetAt (e.position);
    if (t != ui.dropTarget) { ui.dropTarget = t; content().repaint(); }
    overlay->badgeAt = e.position;
    overlay->repaint();
}

void FwPitchBendEditor::contentMouseUp (const MouseEvent&)
{
    if (press != Press::Tab) return;
    press = Press::None;
    if (ui.dragging == None)                                         // a click: that modulator's panel
    {
        proc.apvts.state.setProperty (tabId, pressTab, nullptr);
        if (! expanded()) setExpanded (true); else showTab (pressTab);
        return;
    }
    if (ui.dropTarget != nullptr)                                    // its first free slot, at depth 0 (none free: nothing)
    {
        proc.undo.beginNewTransaction();
        assign (proc.apvts.state, ui.dropTarget->target, ui.dragging, &proc.undo);
    }
    ui.dragging = None;
    ui.dropTarget = nullptr;
    for (auto* k : knobs) k->framed = false;
    content().repaint();
}

//==============================================================================
// typing a value (a double-click): in the value's place; Enter applies a value inside the range (Pitch and Formant
// whole semitones, or cents with Fine), Esc or a click elsewhere cancels
void FwPitchBendEditor::openEntry (ModKnob& k)
{
    closeEntry (false);
    entryFor = &k;
    entry = std::make_unique<TextEditor>();
    entry->setJustification (Justification::centred);
    entry->setFont (theme::font (12.0f));
    entry->setColour (TextEditor::backgroundColourId, theme::well);
    entry->setColour (TextEditor::textColourId, theme::text);
    entry->setColour (TextEditor::outlineColourId, theme::amber);
    entry->setColour (TextEditor::focusedOutlineColourId, theme::amber);
    const bool big = k.kind == ModKnob::Big;
    const auto r = big ? kPill[&k == pitch ? 0 : 1] : valueBox (k);
    String text = big ? String (roundToInt (k.getValue() * (proc.fine() ? 100.0 : 1.0))) : k.valueText().upToFirstOccurrenceOf (" ", false, false);
    entry->setText (text, false);
    entry->onReturnKey = [this] { closeEntry (true); };
    entry->onEscapeKey = [this] { closeEntry (false); };
    entry->onFocusLost = [this] { closeEntry (false); };
    content().addAndMakeVisible (*entry);
    entry->setBounds (r);
    entry->selectAll();
    entry->grabKeyboardFocus();
}

void FwPitchBendEditor::closeEntry (bool apply)
{
    if (entry == nullptr) return;
    auto e = std::move (entry);
    auto* k = std::exchange (entryFor, nullptr);
    if (apply && k != nullptr)
    {
        const auto t = e->getText().trim();
        double v = t.getDoubleValue();
        bool ok = t.containsAnyOf ("0123456789");
        if (k->stepped) { const int i = syncedRates().indexOf (t); ok = i >= 0; v = i; }
        else if (k->kind == ModKnob::Big)
        {
            if (proc.fine()) v /= 100.0;
            else ok = ok && std::abs (v - std::round (v)) < 1.0e-9;      // whole semitones only
        }
        if (ok && v >= k->getMinimum() && v <= k->getMaximum()) k->setValue (v, sendNotificationSync);
    }
    content().removeChildComponent (e.get());
}
