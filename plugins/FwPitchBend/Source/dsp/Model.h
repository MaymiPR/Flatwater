#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <cmath>
#include "Math.h"

// Flatwater Pitch Bend's settings that are not host parameters (as the reference's: RESEARCH.md, "The saved state"):
// the modulation slots (four per modulatable control, a source and a depth), Fine, the two modulators' shape tables
// (256 values for an LFO cycle, then 16 sequencer steps as value / 24), and the editor's own state (the panel open or
// closed, its tab). They live in the processor's state tree, so they are saved, compared (A / B) and undone with it.
namespace pb
{
// what a slot can hold
enum Source { None = 0, M1, M2, AM, PT, numSources };
inline const char* sourceName (int s) { static const char* n[] { "", "M1", "M2", "AM", "PT" }; return n[juce::jlimit (0, 4, s)]; }

// the controls a modulator can be put on (the reference's: every control but M1 / M2 Smooth, PT Offset, Link, Fine,
// Flatten and Steps), with the parameter each one moves
struct Target { const char* key; const char* param; };
inline constexpr Target targets[] {
    { "pitch", "pitch" }, { "formant", "formant" }, { "mix", "mix" },
    { "m1Rate", "m1Rate" }, { "m1Warp", "m1Warp" }, { "m1Phase", "m1Phase" }, { "m1Level", "m1Level" },
    { "m2Rate", "m2Rate" }, { "m2Warp", "m2Warp" }, { "m2Phase", "m2Phase" }, { "m2Level", "m2Level" },
    { "amAttack", "amAttack" }, { "amRelease", "amRelease" }, { "amLevel", "amLevel" },
    { "ptSmooth", "ptSmooth" }, { "ptLevel", "ptLevel" } };
inline constexpr int numTargets = (int) (sizeof (targets) / sizeof (targets[0])), numSlots = 4;
inline int targetIndex (const juce::String& key)
{
    for (int i = 0; i < numTargets; ++i) if (key == targets[i].key) return i;
    return -1;
}

inline constexpr int tableSize = 512, lfoPoints = 256, seqFirst = 256, maxSteps = 16;

inline const juce::Identifier modsTag { "Mods" }, shapesTag { "Shapes" }, fineId { "fine" }, expandedId { "expanded" },
                              tabId { "tab" };

struct Slot { int source = None; float depth = 0.0f; };

// the state tree's parts (created on first use)
inline juce::ValueTree child (juce::ValueTree& state, const juce::Identifier& tag)
{
    auto c = state.getChildWithName (tag);
    if (! c.isValid()) { c = juce::ValueTree (tag); state.appendChild (c, nullptr); }
    return c;
}

inline juce::Identifier slotKey (int target, int slot, bool depth)
{
    return juce::String (targets[target].key) + "_" + juce::String (slot) + (depth ? "_depth" : "_src");
}

inline Slot getSlot (juce::ValueTree& state, int target, int slot)
{
    auto m = child (state, modsTag);
    return { (int) m.getProperty (slotKey (target, slot, false), 0), (float) m.getProperty (slotKey (target, slot, true), 0.0f) };
}

inline void setSlot (juce::ValueTree& state, int target, int slot, Slot s, juce::UndoManager* um)
{
    auto m = child (state, modsTag);
    m.setProperty (slotKey (target, slot, false), s.source, um);
    m.setProperty (slotKey (target, slot, true), juce::jlimit (-100.0f, 100.0f, std::round (s.depth)), um);
}

// a source already in one of a control's slots (the same source is never in two of them)
inline bool holds (juce::ValueTree& state, int target, int source, int exceptSlot = -1)
{
    for (int s = 0; s < numSlots; ++s)
        if (s != exceptSlot && getSlot (state, target, s).source == source) return true;
    return false;
}

// a source dropped on a control takes its first free slot, at depth 0; false when all four are taken or the control
// already has that source
inline bool assign (juce::ValueTree& state, int target, int source, juce::UndoManager* um)
{
    if (holds (state, target, source)) return false;
    for (int s = 0; s < numSlots; ++s)
        if (getSlot (state, target, s).source == None) { setSlot (state, target, s, { source, 0.0f }, um); return true; }
    return false;
}

// ---- shapes ------------------------------------------------------------------------------------------------------
using Table = std::array<float, tableSize>;

// our own shapes, by their math (the original's drawn library is not shipped): one LFO cycle, -1 .. 1
enum class Shape { Sine, Triangle, Square, SawUp, SawDown, Ramp, SCurve, HalfSine, Pulse25, Pulse75, Steps4, Random, Flat };
inline const juce::StringArray& shapeNames()
{
    static const juce::StringArray n { "Sine", "Triangle", "Square", "Saw Up", "Saw Down", "Ramp", "S-Curve", "Half Sine",
                                       "Pulse 25", "Pulse 75", "Four Steps", "Random Steps", "Flat" };
    return n;
}

inline float shapeValue (Shape s, double ph)                  // ph 0 .. 1
{
    const double pi = juce::MathConstants<double>::pi;
    switch (s)
    {
        case Shape::Sine:     return (float) pbmath::sin (2.0 * pi * ph);
        case Shape::Triangle: return (float) (ph < 0.25 ? 4.0 * ph : ph < 0.75 ? 2.0 - 4.0 * ph : 4.0 * ph - 4.0);
        case Shape::Square:   return ph < 0.5 ? 1.0f : -1.0f;
        case Shape::SawUp:    return (float) (2.0 * ph - 1.0);
        case Shape::SawDown:  return (float) (1.0 - 2.0 * ph);
        case Shape::Ramp:     return (float) (2.0 * (pbmath::exp (3.0 * ph) - 1.0) / (pbmath::exp (3.0) - 1.0) - 1.0);
        case Shape::SCurve:                                    // a smooth step up over the first half, down over the second
        {
            const double x = ph < 0.5 ? 2.0 * ph : 2.0 - 2.0 * ph;
            return (float) (2.0 * x * x * (3.0 - 2.0 * x) - 1.0);
        }
        case Shape::HalfSine: return (float) (2.0 * std::abs (pbmath::sin (pi * ph)) - 1.0);
        case Shape::Pulse25:  return ph < 0.25 ? 1.0f : -1.0f;
        case Shape::Pulse75:  return ph < 0.75 ? 1.0f : -1.0f;
        case Shape::Steps4:   return (float) (1.0 - 2.0 * std::floor (ph * 4.0) / 3.0);
        case Shape::Random:
        {
            static const float r[8] { 0.62f, -0.48f, 0.15f, -0.91f, 0.83f, -0.12f, 0.37f, -0.70f };   // our own fixed draw
            return r[juce::jlimit (0, 7, (int) (ph * 8.0))];
        }
        case Shape::Flat:     return 0.0f;
    }
    return 0.0f;
}

inline void setLfo (Table& t, Shape s)
{
    for (int i = 0; i < lfoPoints; ++i) t[(size_t) i] = shapeValue (s, (double) i / lfoPoints);
}

// our own step patterns (whole steps -24 .. +24, 16 steps; fewer steps use the first ones)
struct Pattern { const char* name; int steps[maxSteps]; };
inline constexpr Pattern patterns[] {
    { "Octaves", { 0, 12, 0, 12, 0, 12, 0, 12, 0, 12, 0, 12, 0, 12, 0, 12 } },
    { "Octave Down", { 0, -12, 0, -12, 0, -12, 0, -12, 0, -12, 0, -12, 0, -12, 0, -12 } },
    { "Fifths", { 0, 7, 12, 7, 0, 7, 12, 7, 0, 7, 12, 7, 0, 7, 12, 7 } },
    { "Major Arp", { 0, 4, 7, 12, 7, 4, 0, 4, 7, 12, 16, 12, 7, 4, 0, -5 } },
    { "Minor Arp", { 0, 3, 7, 12, 7, 3, 0, 3, 7, 12, 15, 12, 7, 3, 0, -5 } },
    { "Climb", { 0, 2, 4, 5, 7, 9, 11, 12, 14, 16, 17, 19, 21, 23, 24, 12 } },
    { "Fall", { 0, -1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, -14, -15 } },
    { "Wide Jumps", { 0, 24, -12, 12, 0, -24, 7, -7, 0, 19, -5, 12, 0, -12, 5, 0 } },
    { "Zero", { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } } };
inline constexpr int numPatterns = (int) (sizeof (patterns) / sizeof (patterns[0]));

inline void setSteps (Table& t, const int* steps)
{
    for (int i = 0; i < maxSteps; ++i) t[(size_t) (seqFirst + i)] = (float) juce::jlimit (-24, 24, steps[i]) / 24.0f;
}

inline int stepValue (const Table& t, int i) { return juce::roundToInt (t[(size_t) (seqFirst + i)] * 24.0f); }

inline Table defaultTable (int modulator)                      // M1 a sine, M2 a triangle (the reference's defaults)
{
    Table t {};
    setLfo (t, modulator == 0 ? Shape::Sine : Shape::Triangle);
    return t;
}

inline juce::String encode (const Table& t)
{
    juce::MemoryBlock mb (t.data(), sizeof (float) * t.size());
    return mb.toBase64Encoding();
}

inline Table decode (const juce::var& v, int modulator)
{
    juce::MemoryBlock mb;
    if (v.isString() && mb.fromBase64Encoding (v.toString()) && mb.getSize() == sizeof (float) * (size_t) tableSize)
    {
        Table t;
        std::memcpy (t.data(), mb.getData(), mb.getSize());
        return t;
    }
    return defaultTable (modulator);
}

inline Table getTable (juce::ValueTree& state, int modulator)
{
    return decode (child (state, shapesTag).getProperty (modulator == 0 ? "m1" : "m2"), modulator);
}

inline void setTable (juce::ValueTree& state, int modulator, const Table& t, juce::UndoManager* um)
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
