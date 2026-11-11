#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>
#include <cstring>
#include "dsp/Model.h"

// Flatwater Pitch Bend's settings that are not host parameters, in the processor's state tree (RESEARCH.md, "The saved
// state"): the modulation slots, Fine, the two shape tables (base64 of their floats) and the editor's own state (the
// panel open or closed, its tab), each part a child created on first use; the user's own shapes as files. The values
// they hold are dsp/Model.h's.
namespace pb
{
inline const juce::Identifier modsTag { "Mods" }, shapesTag { "Shapes" }, fineId { "fine" }, expandedId { "expanded" },
                              tabId { "tab" };

inline int targetIndex (const juce::String& key)
{
    for (int i = 0; i < dsp::numTargets; ++i) if (key == dsp::targets[i].key) return i;
    return -1;
}

// the shapes' names in the editor's list (dsp::Shape order)
inline const juce::StringArray& shapeNames()
{
    static const juce::StringArray n { "Sine", "Triangle", "Square", "Saw Up", "Saw Down", "Ramp", "S-Curve", "Half Sine",
                                       "Pulse 25", "Pulse 75", "Four Steps", "Random Steps", "Flat" };
    return n;
}

// the state tree's parts (created on first use)
inline juce::ValueTree child (juce::ValueTree& state, const juce::Identifier& tag)
{
    auto c = state.getChildWithName (tag);
    if (! c.isValid()) { c = juce::ValueTree (tag); state.appendChild (c, nullptr); }
    return c;
}

inline juce::Identifier slotKey (int target, int slot, bool depth)
{
    return juce::String (dsp::targets[target].key) + "_" + juce::String (slot) + (depth ? "_depth" : "_src");
}

inline dsp::Slot getSlot (juce::ValueTree& state, int target, int slot)
{
    auto m = child (state, modsTag);
    return { (int) m.getProperty (slotKey (target, slot, false), 0), (float) m.getProperty (slotKey (target, slot, true), 0.0f) };
}

inline void setSlot (juce::ValueTree& state, int target, int slot, dsp::Slot s, juce::UndoManager* um)
{
    auto m = child (state, modsTag);
    m.setProperty (slotKey (target, slot, false), s.source, um);
    m.setProperty (slotKey (target, slot, true), juce::jlimit (-100.0f, 100.0f, std::round (s.depth)), um);
}

// a source already in one of a control's slots (the same source is never in two of them)
inline bool holds (juce::ValueTree& state, int target, int source, int exceptSlot = -1)
{
    for (int s = 0; s < dsp::numSlots; ++s)
        if (s != exceptSlot && getSlot (state, target, s).source == source) return true;
    return false;
}

// a source dropped on a control takes its first free slot, at depth 0; false when all four are taken or the control
// already has that source
inline bool assign (juce::ValueTree& state, int target, int source, juce::UndoManager* um)
{
    if (holds (state, target, source)) return false;
    for (int s = 0; s < dsp::numSlots; ++s)
        if (getSlot (state, target, s).source == dsp::None) { setSlot (state, target, s, { source, 0.0f }, um); return true; }
    return false;
}

inline juce::String encode (const dsp::Table& t)
{
    juce::MemoryBlock mb (t.data(), sizeof (float) * t.size());
    return mb.toBase64Encoding();
}

inline dsp::Table decode (const juce::var& v, int modulator)
{
    juce::MemoryBlock mb;
    if (v.isString() && mb.fromBase64Encoding (v.toString()) && mb.getSize() == sizeof (float) * (size_t) dsp::tableSize)
    {
        dsp::Table t;
        std::memcpy (t.data(), mb.getData(), mb.getSize());
        return t;
    }
    return dsp::defaultTable (modulator);
}

inline dsp::Table getTable (juce::ValueTree& state, int modulator)
{
    return decode (child (state, shapesTag).getProperty (modulator == 0 ? "m1" : "m2"), modulator);
}

inline void setTable (juce::ValueTree& state, int modulator, const dsp::Table& t, juce::UndoManager* um)
{
    child (state, shapesTag).setProperty (modulator == 0 ? "m1" : "m2", encode (t), um);
}

// the user's own shapes: one file each, the LFO cycle or the steps as text
inline juce::File userShapeDir()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
        .getChildFile ("Flatwater").getChildFile ("User Shapes").getChildFile ("Flatwater Pitch Bend");
}
}
