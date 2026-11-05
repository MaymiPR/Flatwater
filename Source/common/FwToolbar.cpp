#include "FwToolbar.h"
#include "FwLogo.h"
#include "FwSettings.h"
#include "FwTheme.h"

namespace fw
{
namespace
{
namespace th = fw::theme;
juce::Font tbFont (float h = 11.0f, bool bold = false) { return th::font (h, bold); }
const float kScales[] = { 0.75f, 1.0f, 1.25f, 1.5f, 2.0f };

// the bar's pieces in Flatwater's look (FwTheme.h): raised buttons with a 1 px border, the preset name in a well, a
// cyan border on the button under the mouse; lettering and icons light grey, white-ish when hot, faint when idle
juce::Colour ink (bool hot, bool enabled = true) { return ! enabled ? th::textFaint : hot ? th::text : th::textDim; }

void button (juce::Graphics& g, juce::Rectangle<float> r, bool hot, float radius)
{
    g.setGradientFill (juce::ColourGradient (hot ? th::raisedTop.brighter (0.15f) : th::raisedTop, 0.0f, r.getY(),
                                             hot ? th::raised.brighter (0.1f) : th::raised, 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r, radius);
    g.setColour (hot ? th::cyan.withAlpha (0.8f) : th::line);
    g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);
}

void well (juce::Graphics& g, juce::Rectangle<float> r, bool hot, float radius)
{
    g.setColour (th::well);
    g.fillRoundedRectangle (r, radius);
    g.setColour (hot ? th::cyan.withAlpha (0.8f) : th::line);
    g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);
}

// a small down arrow (a menu opens here)
void downArrow (juce::Graphics& g, juce::Point<float> c, juce::Colour col)
{
    juce::Path tri;
    tri.addTriangle (c.x - 3.0f, c.y - 1.5f, c.x + 3.0f, c.y - 1.5f, c.x, c.y + 2.0f);
    g.setColour (col);
    g.fillPath (tri);
}
}

Toolbar::Toolbar (juce::UndoManager& u, juce::AudioProcessorValueTreeState& s, juce::StringArray m,
                  juce::RangedAudioParameter* mp, juce::String name, juce::String ver, Style st)
    : style (st), undo (u), apvts (s), modes (std::move (m)), modeParam (mp), product (std::move (name)), version (std::move (ver))
{
    loadedValues = currentValues();
    if (style == Style::Flat) setSize (flatWidth, flatHeight);
    else if (hybridLayout()) setSize (hybridWidth, hybridHeight);
    else setSize (width, height);
}

// Control rectangles: the reference's toolbar rectangles, with the arrow / copy / setup
// widths narrowed so the Mode drop-down fits next to Skins.
juce::Rectangle<int> Toolbar::rect (Item i) const
{
    if (style == Style::Flat)      // the reference's toolbar (BassX Lift's original), Mode first, the preset box shortened for it
        switch (i)
        {
            case About:  return { 4, 10, 19, 19 };
            case Mode:   return { 32, 8, 66, 22 };
            case Undo:   return { 106, 10, 19, 19 };
            case Redo:   return { 130, 10, 19, 19 };
            case Preset: return { 158, 8, 356, 22 };
            case Prev:   return { 528, 10, 17, 19 };
            case Next:   return { 552, 10, 17, 19 };
            case Copy:   return { 581, 8, 57, 22 };
            case Setup:  return { 659, 8, 57, 22 };
            case Save:   return { 740, 10, 19, 19 };
            case Menu:   return { 776, 10, 24, 17 };
            default:     return {};
        }
    if (hybridLayout())            // measured on the reference (H-Comp): boxes on y 2 ... 16; a wider editor (H-Delay,
    {                              // 749) widens the preset box and moves the rest right
        const int x = getWidth() - hybridWidth;
        switch (i)
        {
            case About:  return { 2, 2, 98, 15 };       // the orange logo block (the reference's is 2 ... 156)
            case Mode:   return { 102, 2, 55, 15 };
            case Undo:   return { 159, 2, 32, 15 };
            case Redo:   return { 193, 2, 32, 15 };
            case Preset: return { 227, 2, 191 + x, 15 };
            case Prev:   return { 420 + x, 2, 32, 15 };
            case Next:   return { 454 + x, 2, 32, 15 };
            case Copy:   return { 488 + x, 2, 44, 15 };
            case Setup:  return { 534 + x, 2, 44, 15 };
            case Save:   return { 580 + x, 2, 44, 15 };
            case Menu:   return { 626 + x, 2, 15, 15 };
            default:     return {};
        }
    }
    if (getWidth() < width)        // a narrow editor (Bass Rider, 320): its toolbar has no Skins box; the Mode
        switch (i)                 // drop-down takes its place, the arrow / copy / setup / save boxes narrowed for it
        {
            case About:  return { 6, 2, 16, 15 };
            case Mode:   return { 24, 2, 62, 15 };
            case Undo:   return { 88, 2, 24, 15 };
            case Redo:   return { 114, 2, 24, 15 };
            case Prev:   return { 140, 2, 24, 15 };
            case Next:   return { 166, 2, 24, 15 };
            case Copy:   return { 192, 2, 38, 15 };
            case Setup:  return { 232, 2, 38, 15 };
            case Save:   return { 272, 2, 28, 15 };
            case Menu:   return { 302, 2, 16, 15 };
            case Preset: return { 2, 19, getWidth() - 4, 15 };
            default:     return {};
        }
    switch (i)
    {
        case About:  return { 4, 2, 16, 15 };
        case Skins:  return { 23, 2, 49, 15 };
        case Mode:   return { 74, 2, 42, 15 };
        case Undo:   return { 118, 2, 24, 15 };
        case Redo:   return { 144, 2, 24, 15 };
        case Prev:   return { 170, 2, 24, 15 };
        case Next:   return { 196, 2, 24, 15 };
        case Copy:   return { 222, 2, 42, 15 };
        case Setup:  return { 266, 2, 42, 15 };
        case Save:   return { 310, 2, 38, 15 };
        case Menu:   return { 352, 2, 17, 15 };
        case Preset: return { 2, 19, 367, 15 };
        default:     return {};
    }
}

int Toolbar::itemAt (juce::Point<int> p) const
{
    for (int i = 0; i < numItems; ++i)
        if (rect ((Item) i).contains (p)) return i;
    return -1;
}

void Toolbar::refresh() { repaint(); }

//==============================================================================
// Setup A/B
void Toolbar::switchSetup()
{
    slot[active] = apvts.copyState();
    const int other = 1 - active;
    if (! slot[other].isValid()) slot[other] = slot[active].createCopy();
    active = other;
    const auto w = apvts.state.getProperty ("uiWidth");
    apvts.replaceState (slot[active].createCopy());
    apvts.state.setProperty ("uiWidth", w, nullptr);
}

void Toolbar::copyToOther()
{
    slot[active] = apvts.copyState();
    slot[1 - active] = slot[active].createCopy();
}

//==============================================================================
// Presets
juce::File Toolbar::userDir() const
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
        .getChildFile ("Flatwater").getChildFile ("User Presets").getChildFile (product);
}

juce::Array<juce::File> Toolbar::userPresets() const
{
    auto files = userDir().findChildFiles (juce::File::findFiles, false, "*.flatwaterpreset");
    files.sort();
    return files;
}

std::vector<float> Toolbar::currentValues() const
{
    std::vector<float> v;
    for (auto* p : apvts.processor.getParameters())
        v.push_back (p->getValue());
    return v;
}

bool Toolbar::isModified() const
{
    const auto now = currentValues();
    for (size_t i = 0; i < now.size() && i < loadedValues.size(); ++i)
        if (std::abs (now[i] - loadedValues[i]) > 1.0e-5f) return true;
    return false;
}

void Toolbar::loadDefault()
{
    for (auto* p : apvts.processor.getParameters())
    {
        if (p == modeParam || std::find (keepOnLoad.begin(), keepOnLoad.end(), p) != keepOnLoad.end()) continue;
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->getDefaultValue());
        p->endChangeGesture();
    }
    if (onLoadDefault) onLoadDefault();
    presetName = "Default Preset";
    presetFile = juce::File();
    factoryIndex = -1;
    loadedValues = currentValues();
    repaint();
}

void Toolbar::loadFactory (int index)
{
    if (factory == nullptr || index < 0 || index >= (int) factory->size()) return;
    applyPreset (apvts, (*factory)[(size_t) index], modeParam, keepOnLoad);
    presetName = (*factory)[(size_t) index].name;
    presetFile = juce::File();
    factoryIndex = index;
    loadedValues = currentValues();
    repaint();
}

void Toolbar::loadUser (const juce::File& f)
{
    if (auto xml = juce::XmlDocument::parse (f))
    {
        const auto w = apvts.state.getProperty ("uiWidth");
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
        apvts.state.setProperty ("uiWidth", w, nullptr);
        presetName = f.getFileNameWithoutExtension();
        presetFile = f;
        factoryIndex = -1;
        loadedValues = currentValues();
        repaint();
    }
}

void Toolbar::step (int delta)
{
    // order: Default Preset, the factory list, then the user presets
    const int nf = factory != nullptr ? (int) factory->size() : 0;
    const auto users = userPresets();
    const int total = 1 + nf + users.size();
    int idx = 0;
    if (factoryIndex >= 0) idx = 1 + factoryIndex;
    else if (presetFile.exists() && users.contains (presetFile)) idx = 1 + nf + users.indexOf (presetFile);
    idx = (idx + delta + total) % total;
    if (idx == 0) loadDefault();
    else if (idx <= nf) loadFactory (idx - 1);
    else loadUser (users[idx - 1 - nf]);
}

void Toolbar::saveTo (const juce::File& f)
{
    f.getParentDirectory().createDirectory();
    auto state = apvts.copyState();
    state.removeProperty ("uiWidth", nullptr);
    if (auto xml = state.createXml()) xml->writeTo (f);
    presetName = f.getFileNameWithoutExtension();
    presetFile = f;
    factoryIndex = -1;
    loadedValues = currentValues();
    repaint();
}

void Toolbar::saveAs()
{
    auto* w = new juce::AlertWindow ("Save Preset", "Preset name:", juce::MessageBoxIconType::NoIcon, this);
    w->setLookAndFeel (&menuLook);
    w->addTextEditor ("name", presetFile.exists() ? presetName : juce::String());
    w->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<Toolbar> safe (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([safe, w] (int r)
    {
        if (r != 1 || safe == nullptr) return;
        const auto name = juce::File::createLegalFileName (w->getTextEditorContents ("name").trim());
        if (name.isNotEmpty())
            safe->saveTo (safe->userDir().getChildFile (name + ".flatwaterpreset"));
    }), true);
}

void Toolbar::showPresetMenu()
{
    juce::PopupMenu m;
    m.setLookAndFeel (&menuLook);
    m.addSectionHeader ("Factory Presets");
    m.addItem (1, "Default Preset", true, ! presetFile.exists() && factoryIndex < 0);
    if (factory != nullptr)
    {
        juce::StringArray groups;
        for (const auto& p : *factory) groups.addIfNotAlreadyThere (p.group);
        for (const auto& g : groups)
        {
            juce::PopupMenu sub;
            sub.setLookAndFeel (&menuLook);
            bool inside = false;
            for (int i = 0; i < (int) factory->size(); ++i)
                if (g == (*factory)[(size_t) i].group)
                {
                    sub.addItem (1000 + i, (*factory)[(size_t) i].name, true, i == factoryIndex);
                    inside = inside || i == factoryIndex;
                }
            m.addSubMenu (g, sub, true, nullptr, inside);
        }
    }
    const auto users = userPresets();
    if (! users.isEmpty())
    {
        m.addSeparator();
        m.addSectionHeader ("User Presets");
        for (int i = 0; i < users.size(); ++i)
            m.addItem (100 + i, users[i].getFileNameWithoutExtension(), true, users[i] == presetFile);
    }
    m.addSeparator();
    m.addItem (2, "Save As...");
    if (presetFile.exists()) m.addItem (3, "Save " + presetName);
    m.addItem (4, "Show User Presets Folder");
    juce::Component::SafePointer<Toolbar> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (rect (Preset))).withMinimumWidth (rect (Preset).getWidth()).withStandardItemHeight (18),
                     [safe, users] (int r)
    {
        if (safe == nullptr || r <= 0) return;
        if (r == 1) safe->loadDefault();
        else if (r == 2) safe->saveAs();
        else if (r == 3) safe->saveTo (safe->presetFile);
        else if (r == 4) { safe->userDir().createDirectory(); safe->userDir().revealToUser(); }
        else if (r >= 1000) safe->loadFactory (r - 1000);
        else if (r >= 100 && r - 100 < users.size()) safe->loadUser (users[r - 100]);
    });
}

void Toolbar::showAboutMenu()
{
    juce::PopupMenu m;
    m.setLookAndFeel (&menuLook);
    m.addSectionHeader (product + "  v" + version);
    m.addItem (1, "About " + product);
    m.addSeparator();
    m.addItem (2, "Flatwater on GitHub");
    m.addItem (3, "Report an Issue");
    m.addItem (4, "License (AGPL-3.0)");
    juce::Component::SafePointer<Toolbar> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (rect (About))).withMinimumWidth (150).withStandardItemHeight (18),
                     [safe] (int r)
    {
        if (safe == nullptr || r <= 0) return;
        if (r == 1)
        {
            auto* w = new juce::AlertWindow (safe->product, safe->product + "  v" + safe->version
                                             + "\n\nPart of Flatwater: open-source recreations of Waves mixing plug-ins,\n"
                                               "null-tested against the originals.\n"
                                               "Open source under the GNU Affero General Public License v3\n"
                                             + repoUrl + "\n\nNot affiliated with or endorsed by Waves Audio.",
                                             juce::MessageBoxIconType::NoIcon, safe.getComponent());
            w->setLookAndFeel (&safe->menuLook);
            w->addButton ("OK", 1, juce::KeyPress (juce::KeyPress::returnKey));
            w->enterModalState (true, nullptr, true);
        }
        else if (r == 2) juce::URL (repoUrl).launchInDefaultBrowser();
        else if (r == 3) juce::URL (juce::String (repoUrl) + "/issues").launchInDefaultBrowser();
        else if (r == 4) juce::URL (juce::String (repoUrl) + "/blob/main/LICENSE").launchInDefaultBrowser();
    });
}

void Toolbar::showToolbarMenu()
{
    juce::PopupMenu m;
    m.setLookAndFeel (&menuLook);
    const float cur = getWindowScale ? getWindowScale() : 1.0f;
    m.addSectionHeader ("Window Size");
    for (int i = 0; i < 5; ++i)
        m.addItem (10 + i, juce::String (juce::roundToInt (kScales[i] * 100.0f)) + "%", true, std::abs (cur - kScales[i]) < 0.02f);
    m.addSeparator();
    m.addItem (20, "Apply as Default to All");
    juce::Component::SafePointer<Toolbar> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (rect (Menu))).withStandardItemHeight (18),
                     [safe] (int r)
    {
        if (safe == nullptr || r <= 0) return;
        if (r >= 10 && r < 15) { if (safe->setWindowScale) safe->setWindowScale (kScales[r - 10]); }
        else if (r == 20 && safe->getWindowScale) setDefaultWindowScale (safe->getWindowScale());
    });
}

//==============================================================================
// the glyphs every bar shares
namespace
{
// a circular arrow: the arc runs over the top, the head sits at its open end (undo points left, redo right)
void curvedArrow (juce::Graphics& g, juce::Point<float> c, bool redo, juce::Colour col)
{
    const float rad = 4.2f, dir = redo ? -1.0f : 1.0f;
    const float a0 = juce::degreesToRadians (25.0f), a1 = juce::degreesToRadians (-215.0f);
    auto pt = [&] (float a) { return juce::Point<float> (c.x + dir * rad * std::cos (a), c.y + 0.5f + rad * std::sin (a)); };
    juce::Path arc;
    for (int i = 0; i <= 24; ++i)
    {
        const float a = a0 + (a1 - a0) * (float) i / 24.0f;
        if (i == 0) arc.startNewSubPath (pt (a)); else arc.lineTo (pt (a));
    }
    g.setColour (col);
    g.strokePath (arc, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    const auto tip = pt (a1);
    const juce::Point<float> tan (dir * std::sin (a1), -std::cos (a1));
    const juce::Point<float> nrm (-tan.y, tan.x);
    juce::Path head;
    head.addTriangle (tip + tan * 2.8f, tip - tan * 0.6f + nrm * 2.6f, tip - tan * 0.6f - nrm * 2.6f);
    g.fillPath (head);
}

// a straight arrow with a solid head (previous / next preset)
void straightArrow (juce::Graphics& g, juce::Point<float> c, bool right, juce::Colour col, float len = 5.5f)
{
    const float s = right ? 1.0f : -1.0f;
    g.setColour (col);
    g.drawLine (c.x - s * len, c.y, c.x + s * (len - 0.5f), c.y, 1.4f);
    juce::Path h;
    h.addTriangle (c.x + s * (len + 2.2f), c.y, c.x + s * (len - 1.2f), c.y - 3.0f, c.x + s * (len - 1.2f), c.y + 3.0f);
    g.fillPath (h);
}

// a floppy disk: a 9 x 9 body, the top-right corner cut, a slot at the top and a label below it
void diskIcon (juce::Graphics& g, juce::Point<float> c, juce::Colour col)
{
    const juce::Rectangle<float> d (c.x - 4.5f, c.y - 4.5f, 9.0f, 9.0f);
    juce::Path p;
    p.startNewSubPath (d.getX(), d.getY()); p.lineTo (d.getRight() - 1.8f, d.getY()); p.lineTo (d.getRight(), d.getY() + 1.8f);
    p.lineTo (d.getRight(), d.getBottom()); p.lineTo (d.getX(), d.getBottom()); p.closeSubPath();
    g.setColour (col);
    g.fillPath (p);
    g.setColour (th::bg);
    g.fillRect (d.getX() + 2.2f, d.getY(), 4.2f, 3.0f);
    g.fillRect (d.getX() + 1.8f, d.getY() + 5.2f, 5.4f, 2.6f);
}

// three bars (the bar's own menu)
void menuIcon (juce::Graphics& g, juce::Point<float> c, juce::Colour col, float w = 10.0f)
{
    g.setColour (col);
    for (int k = -1; k <= 1; ++k)
        g.fillRoundedRectangle (c.x - w * 0.5f, c.y + (float) k * 3.5f - 0.6f, w, 1.2f, 0.6f);
}
}

void Toolbar::paintFlat (juce::Graphics& g)
{
    // the single-row bar: the same pieces as the other bars, as pills on the editor's background
    g.fillAll (th::bg);
    g.setColour (th::line);
    g.fillRect (0, getHeight() - 1, getWidth(), 1);
    auto label = [&] (juce::Rectangle<int> r, const juce::String& t, bool hot, float size = 12.0f)
    {
        g.setColour (ink (hot));
        g.setFont (tbFont (size));
        g.drawText (t, r, juce::Justification::centred);
    };
    for (int i = 0; i < numItems; ++i)
    {
        const auto r = rect ((Item) i);
        if (r.isEmpty()) continue;
        const bool hot = (i == this->hot);
        const auto rf = r.toFloat();
        const auto c = rf.getCentre();
        switch ((Item) i)
        {
            case About: drawMark (g, rf.reduced (0.5f), hot, false); break;
            case Undo: case Redo:
            {
                const bool can = i == Undo ? undo.canUndo() : undo.canRedo();
                curvedArrow (g, c, i == Redo, ink (hot && can, can));
                break;
            }
            case Preset:
            {
                well (g, rf, hot, rf.getHeight() * 0.5f);
                label (r.withTrimmedLeft (8).withTrimmedRight (22), juce::String (active == 0 ? "A: " : "B: ") + presetName + (isModified() ? "*" : ""), true);
                g.setColour (th::line);
                g.fillRect (r.getRight() - 18, r.getY() + 4, 1, r.getHeight() - 8);
                downArrow (g, { (float) r.getRight() - 9.5f, c.y }, hot ? th::cyan : th::textDim);
                break;
            }
            case Mode:
            {
                button (g, rf, hot, rf.getHeight() * 0.5f);
                const auto name = modeParam != nullptr ? modes[(int) modeParam->convertFrom0to1 (modeParam->getValue())] : juce::String();
                label (r.withTrimmedRight (12), name, hot);
                downArrow (g, { (float) r.getRight() - 13.0f, c.y }, hot ? th::cyan : th::textDim);
                break;
            }
            case Prev: case Next:
            {
                const float s = i == Next ? 1.0f : -1.0f;
                juce::Path tri;
                tri.addTriangle (c.x + s * 3.0f, c.y, c.x - s * 3.0f, c.y - 4.5f, c.x - s * 3.0f, c.y + 4.5f);
                g.setColour (ink (hot));
                g.fillPath (tri);
                break;
            }
            case Copy:
                button (g, rf, hot, rf.getHeight() * 0.5f);
                label (r, juce::String::fromUTF8 (active == 0 ? "A \xe2\x86\x92 B" : "B \xe2\x86\x92 A"), hot);
                break;
            case Setup: button (g, rf, hot, rf.getHeight() * 0.5f); label (r, active == 0 ? "Setup A" : "Setup B", hot); break;
            case Save: diskIcon (g, c, ink (hot)); break;
            case Menu: menuIcon (g, c, ink (hot), 11.0f); break;
            default: break;
        }
    }
}

void Toolbar::paint (juce::Graphics& g)
{
    if (style == Style::Flat) { paintFlat (g); return; }
    const bool hybrid = hybridLayout();
    if (hybrid) g.fillAll (th::bg);
    auto label = [&] (juce::Rectangle<int> r, const juce::String& t, bool hot, float size = 11.0f)
    {
        g.setColour (ink (hot));
        g.setFont (tbFont (size));
        g.drawText (t, r, juce::Justification::centred);
    };

    for (int i = 0; i < numItems; ++i)
    {
        const auto r = rect ((Item) i);
        if (r.isEmpty()) continue;                                 // (no Skins box on a narrow bar)
        const bool hot = (i == this->hot);
        const auto rf = r.toFloat();
        const auto c = rf.getCentre();
        switch ((Item) i)
        {
            case About:
                if (hybrid)
                {
                    // the suite's badge: a raised block with the mark and the suite's name
                    button (g, rf, hot, 2.0f);
                    drawMark (g, juce::Rectangle<float> (rf.getX() + 4.0f, rf.getY() + 1.5f, 15.0f, 12.0f), hot, false);
                    g.setColour (hot ? th::text : th::textDim);
                    g.setFont (th::font (10.0f, true).withExtraKerningFactor (0.12f));
                    g.drawText ("FLATWATER", r.withTrimmedLeft (23).withTrimmedRight (2), juce::Justification::centredLeft);
                }
                else drawMark (g, rf, hot, false);
                break;
            case Skins:
                button (g, rf, hot, 2.0f);
                label (r.withTrimmedRight (10), "Skins", hot);
                downArrow (g, { rf.getRight() - 8.0f, c.y }, hot ? th::cyan : th::textDim);
                break;
            case Mode:
            {
                button (g, rf, hot, 2.0f);
                const auto name = modeParam != nullptr ? modes[(int) modeParam->convertFrom0to1 (modeParam->getValue())] : juce::String();
                label (r.withTrimmedRight (10), name, hot, 10.5f);
                downArrow (g, { rf.getRight() - 7.0f, c.y }, hot ? th::cyan : th::textDim);
                break;
            }
            case Undo: button (g, rf, hot && undo.canUndo(), 2.0f); curvedArrow (g, c, false, ink (hot, undo.canUndo())); break;
            case Redo: button (g, rf, hot && undo.canRedo(), 2.0f); curvedArrow (g, c, true, ink (hot, undo.canRedo())); break;
            case Prev: button (g, rf, hot, 2.0f); straightArrow (g, c, false, ink (hot)); break;
            case Next: button (g, rf, hot, 2.0f); straightArrow (g, c, true, ink (hot)); break;
            case Copy:
                button (g, rf, hot, 2.0f);
                label (r, juce::String::fromUTF8 (active == 0 ? "A \xe2\x86\x92 B" : "B \xe2\x86\x92 A"), hot);
                break;
            case Setup:
                button (g, rf, hot, 2.0f);
                label (r, active == 0 ? "Setup A" : "Setup B", hot);
                break;
            case Save: button (g, rf, hot, 2.0f); label (r, "Save", hot); break;
            case Menu:
                button (g, rf, hot, 2.0f);
                menuIcon (g, c, ink (hot), 9.0f);
                break;
            case Preset:
                well (g, rf, hot, 2.0f);
                g.setColour (th::text);
                g.setFont (tbFont (11.0f));
                g.drawText (juce::String (active == 0 ? "A: " : "B: ") + presetName + (isModified() ? "*" : ""),
                            r.withTrimmedRight (16), juce::Justification::centred);
                // the menu's arrow at the right end
                g.setColour (th::line);
                g.fillRect (r.getRight() - 16, r.getY() + 2, 1, r.getHeight() - 4);
                downArrow (g, { rf.getRight() - 8.0f, c.y }, hot ? th::cyan : th::textDim);
                break;
            default: break;
        }
    }
}

void Toolbar::mouseMove (const juce::MouseEvent& e)
{
    const int h = itemAt (e.getPosition());
    if (h != hot) { hot = h; repaint(); }
}

void Toolbar::mouseExit (const juce::MouseEvent&) { hot = -1; repaint(); }

void Toolbar::mouseDown (const juce::MouseEvent& e)
{
    const auto item = (Item) itemAt (e.getPosition());
    const auto target = juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (rect (item))).withStandardItemHeight (18);
    switch (item)
    {
        case Prev:   step (-1); break;
        case Next:   step (+1); break;
        case Undo:   undo.undo(); break;
        case Redo:   undo.redo(); break;
        case Copy:   copyToOther(); break;
        case Setup:  switchSetup(); break;
        case Save:   if (presetFile.exists()) saveTo (presetFile); else saveAs(); break;
        case Preset: showPresetMenu(); break;
        case About:  showAboutMenu(); break;
        case Menu:   showToolbarMenu(); break;
        case Skins:
        {
            juce::PopupMenu m; m.setLookAndFeel (&menuLook);
            m.addItem (1, "Dark", true, true);   // the other skins of the reference are not part of Flatwater
            m.showMenuAsync (target);
            break;
        }
        case Mode:
            if (modeParam != nullptr)
            {
                juce::PopupMenu m; m.setLookAndFeel (&menuLook);
                const int cur = (int) modeParam->convertFrom0to1 (modeParam->getValue());
                for (int i = 0; i < modes.size(); ++i) m.addItem (i + 1, modes[i], true, i == cur);
                m.showMenuAsync (target.withMinimumWidth (rect (Mode).getWidth()), [this] (int r)
                {
                    if (r <= 0) return;
                    modeParam->beginChangeGesture();
                    modeParam->setValueNotifyingHost (modeParam->convertTo0to1 ((float) (r - 1)));
                    modeParam->endChangeGesture();
                    repaint();
                });
            }
            break;
        default: break;
    }
    repaint();
}
}
