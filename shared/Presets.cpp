#include "shared/Presets.h"
#include <cmath>

namespace flatwater
{
namespace
{
const juce::Identifier presetId { "preset" }, presetIndexId { "presetIndex" }, compareId { "compare" },
                       compareOtherId { "compareOther" }, uiScaleId { "uiScale" }, uiWidthId { "uiWidth" }, paramTag { "PARAM" },
                       idId { "id" }, valueId { "value" };

juce::String asText (const juce::ValueTree& t) { return t.toXmlString (juce::XmlElement::TextFormat().singleLine()); }
}

void applyPreset (juce::AudioProcessorValueTreeState& apvts, const FactoryPreset& preset, juce::RangedAudioParameter* keep,
                  const std::vector<juce::AudioProcessorParameter*>& alsoKeep, juce::UndoManager* undo)
{
    for (auto* p : apvts.processor.getParameters())
    {
        auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p);
        if (rp == nullptr || rp == keep || std::find (alsoKeep.begin(), alsoKeep.end(), p) != alsoKeep.end()) continue;
        if (std::any_of (preset.leave.begin(), preset.leave.end(), [rp] (const char* id) { return rp->getParameterID() == id; })) continue;
        float target = rp->getDefaultValue();
        for (const auto& v : preset.values)
            if (rp->getParameterID() == v.id) target = rp->convertTo0to1 (v.value);
        rp->beginChangeGesture();
        rp->setValueNotifyingHost (target);
        rp->endChangeGesture();
    }
    // the preset's values as stored, before the parameters' ranges and snapping (a plug-in may use them while its
    // parameters stay where the preset put them: Flatwater Delay, as the reference takes a preset's values exactly).
    // The child stays in its place and takes the new values as property changes, which an undo manager can take back.
    auto exact = apvts.state.getChildWithName (presetExactTag);
    if (! exact.isValid())
    {
        exact = juce::ValueTree (presetExactTag);
        apvts.state.appendChild (exact, nullptr);
    }
    juce::ValueTree stored (presetExactTag);
    for (const auto& v : preset.values) stored.setProperty (v.id, v.value, nullptr);
    exact.copyPropertiesFrom (stored, undo);
}

//==============================================================================
PresetState::PresetState (juce::AudioProcessorValueTreeState& s, juce::UndoManager& u, const std::vector<FactoryPreset>& factoryList,
                          juce::RangedAudioParameter* modeParam, std::vector<juce::AudioProcessorParameter*> keepOnLoad)
    : apvts (s), undo (u), list (factoryList), mode (modeParam), keep (std::move (keepOnLoad))
{
    apvts.state.addListener (this);
    readTree();
    startTimerHz (10);
}

PresetState::~PresetState()
{
    stopTimer();
    apvts.state.removeListener (this);
}

// the tree's preset and setup, copied for the toolbar
void PresetState::readTree()
{
    const auto& st = apvts.state;
    const auto text = st.getProperty (presetId).toString();
    const int index = (int) st.getProperty (presetIndexId, -1);
    const juce::ScopedLock sl (lock);
    shownName = text.isNotEmpty() ? text : juce::String (defaultName);
    shownIndex = text.isNotEmpty() && index >= 0 && index < (int) list.size() ? index : -1;
    shownSetup = (int) st.getProperty (compareId, 0) == 1 ? 1 : 0;
}

void PresetState::valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property)
{
    if (tree != apvts.state || (property != presetId && property != presetIndexId && property != compareId)) return;
    readTree();
    if (undo.isPerformingUndoRedo())            // an undo across a preset load: the star is counted from where it lands
    {
        const juce::ScopedLock sl (lock);
        loadedStale = true;
    }
}

// a restore (the only replacement of the tree): its preset and setup, without loading anything
void PresetState::valueTreeRedirected (juce::ValueTree&)
{
    readTree();
    {
        const juce::ScopedLock sl (lock);
        loadedStale = true;                     // (the parameters take the new tree's values after this call)
    }
    restoredAt.store (juce::jmax ((juce::uint32) 1, juce::Time::getMillisecondCounter()));
}

juce::String PresetState::name() const { const juce::ScopedLock sl (lock); return shownName; }
int PresetState::factoryIndex() const  { const juce::ScopedLock sl (lock); return shownIndex; }
int PresetState::activeSetup() const   { const juce::ScopedLock sl (lock); return shownSetup; }

std::vector<float> PresetState::currentValues() const
{
    std::vector<float> v;
    for (auto* p : apvts.processor.getParameters()) v.push_back (p->getValue());
    return v;
}

bool PresetState::modified()
{
    const auto now = currentValues();
    const juce::ScopedLock sl (lock);
    if (loadedStale)
    {
        loadedValues = now;
        loadedStale = false;
        return false;
    }
    for (size_t i = 0; i < now.size() && i < loadedValues.size(); ++i)
        if (std::abs (now[i] - loadedValues[i]) > 1.0e-5f) return true;
    return false;
}

//==============================================================================
// One action, one undo step: what the parameters hold reaches the tree first (it belongs to the step before), then
// everything the action changes goes through the undo manager inside a transaction of its own.
void PresetState::begin()
{
    apvts.copyState();
    undo.beginNewTransaction();
}

void PresetState::end()
{
    apvts.copyState();                          // the parameters' new values into the tree, inside the transaction
    undo.beginNewTransaction();
}

void PresetState::load (const FactoryPreset& preset, const juce::String& presetName, int index)
{
    begin();
    applyPreset (apvts, preset, mode, keep, &undo);
    apvts.copyState();
    if (applyExtras) applyExtras (apvts.state.getChildWithName (presetExactTag), &undo);
    if (presetName.isEmpty())
    {
        apvts.state.removeProperty (presetId, &undo);
        apvts.state.removeProperty (presetIndexId, &undo);
    }
    else
    {
        apvts.state.setProperty (presetId, presetName, &undo);
        apvts.state.setProperty (presetIndexId, index, &undo);
    }
    end();
    const auto now = currentValues();
    const juce::ScopedLock sl (lock);
    loadedValues = now;
    loadedStale = false;
}

void PresetState::loadDefault() { load (defaults, {}, -1); }

void PresetState::loadFactory (int index)
{
    if (index < 0 || index >= (int) list.size()) return;
    load (list[(size_t) index], list[(size_t) index].name, index);
}

// The tree takes another one's content as undoable changes: the parameters' values, the root's properties and the
// other children's (the window's size and the setups stay; the preset's name too unless `withPreset`). A parameter
// the source does not hold stays where it is.
void PresetState::copyInto (const juce::ValueTree& source, bool withPreset)
{
    auto& live = apvts.state;
    const auto stays = [withPreset] (const juce::Identifier& n)
    {
        return n == uiScaleId || n == uiWidthId || n == compareId || n == compareOtherId || (! withPreset && (n == presetId || n == presetIndexId));
    };
    for (int i = live.getNumProperties(); --i >= 0;)
    {
        const auto n = live.getPropertyName (i);
        if (! stays (n) && ! source.hasProperty (n)) live.removeProperty (n, &undo);
    }
    for (int i = 0; i < source.getNumProperties(); ++i)
    {
        const auto n = source.getPropertyName (i);
        if (! stays (n)) live.setProperty (n, source.getProperty (n), &undo);
    }
    std::vector<juce::ValueTree> mine;
    for (const auto& c : live) mine.push_back (c);
    for (auto& c : mine)
        if (! c.hasType (paramTag) && ! source.getChildWithName (c.getType()).isValid()) c.removeAllProperties (&undo);
    for (const auto& c : source)
    {
        if (c.hasType (paramTag))
        {
            auto p = live.getChildWithProperty (idId, c.getProperty (idId));
            if (p.isValid() && c.hasProperty (valueId)) p.setProperty (valueId, (float) c.getProperty (valueId), &undo);
            continue;
        }
        auto part = live.getChildWithName (c.getType());
        if (! part.isValid())
        {
            part = juce::ValueTree (c.getType());
            live.appendChild (part, nullptr);
        }
        part.copyPropertiesFrom (c, &undo);
    }
}

bool PresetState::loadTree (const juce::ValueTree& preset, const juce::String& presetName)
{
    if (! preset.hasType (apvts.state.getType())) return false;      // not a preset of this plug-in: the state stays
    begin();
    copyInto (preset, false);
    apvts.state.setProperty (presetId, presetName, &undo);
    apvts.state.removeProperty (presetIndexId, &undo);
    end();
    const auto now = currentValues();
    const juce::ScopedLock sl (lock);
    loadedValues = now;
    loadedStale = false;
    return true;
}

// the state as a setup holds it: everything but the window's size and the setups themselves
juce::ValueTree PresetState::setupTree()
{
    auto t = apvts.copyState();
    for (const auto& n : { uiScaleId, uiWidthId, compareId, compareOtherId }) t.removeProperty (n, nullptr);
    return t;
}

juce::ValueTree PresetState::presetTree()
{
    auto t = setupTree();
    for (const auto& n : { presetId, presetIndexId }) t.removeProperty (n, nullptr);
    return t;
}

void PresetState::savedAs (const juce::String& presetName)
{
    apvts.state.setProperty (presetId, presetName, nullptr);
    apvts.state.removeProperty (presetIndexId, nullptr);
    const auto now = currentValues();
    const juce::ScopedLock sl (lock);
    loadedValues = now;
    loadedStale = false;
}

// Setup A/B, as the reference: one button toggles between two setups. The setup that is left is kept as text; the
// other one, once it exists, is loaded with its own preset name.
void PresetState::switchSetup()
{
    begin();
    const auto leaving = asText (setupTree());
    const auto otherText = apvts.state.getProperty (compareOtherId).toString();
    if (otherText.isNotEmpty())
        if (auto xml = juce::parseXML (otherText))
        {
            const auto other = juce::ValueTree::fromXml (*xml);
            if (other.hasType (apvts.state.getType())) copyInto (other, true);
        }
    apvts.state.setProperty (compareOtherId, leaving, &undo);
    apvts.state.setProperty (compareId, 1 - activeSetup(), &undo);
    end();
}

void PresetState::copyToOther()
{
    apvts.state.setProperty (compareOtherId, asText (setupTree()), nullptr);
}

//==============================================================================
void PresetState::setProgram (int index)
{
    if (index < 0 || index >= numPrograms()) return;
    // a tree is edited on the message thread only: a call from elsewhere waits for the timer
    if (auto* mm = juce::MessageManager::getInstanceWithoutCreating())
        if (! mm->isThisTheMessageThread())
        {
            pendingProgram.store (index);
            return;
        }
    pendingProgram.store (index);
    applyPendingProgram();
}

void PresetState::applyPendingProgram()
{
    const int index = pendingProgram.exchange (-1);
    if (index < 0 || index == program()) return;
    const auto at = restoredAt.load();
    if (at != 0 && juce::Time::getMillisecondCounter() - at < (juce::uint32) restoreGuardMs) return;
    if (index == 0) loadDefault(); else loadFactory (index - 1);
}

//==============================================================================
float readUiScale (const juce::ValueTree& state, int designWidth, float fallback)
{
    float scale = fallback;
    const float saved = (float) (double) state.getProperty (uiScaleId, 0.0);
    const bool hasScale = std::isfinite (saved) && saved > 0.0f;
    const bool hasWidth = state.hasProperty (uiWidthId) && designWidth > 0;
    const float width = hasWidth ? (float) (int) state.getProperty (uiWidthId) : 0.0f;
    // the two are written together, so a width more than a pixel off its scale was set later, by an older build
    if (hasScale && ! (hasWidth && std::abs (saved * (float) designWidth - width) > 1.0f)) scale = saved;
    else if (hasWidth) scale = width / (float) designWidth;
    return std::isfinite (scale) ? juce::jlimit (minUiScale, maxUiScale, scale) : 1.0f;
}

void writeUiScale (juce::ValueTree& state, int designWidth, int width)
{
    if (designWidth > 0) state.setProperty (uiScaleId, (double) width / (double) designWidth, nullptr);
    state.setProperty (uiWidthId, width, nullptr);
}
}
