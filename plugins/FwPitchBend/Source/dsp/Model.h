#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include "shared/dsp/Math.h"

// Flatwater Pitch Bend's settings that are not host parameters (as the reference's: RESEARCH.md, "The saved state"):
// the modulation slots (four per modulatable control, a source and a depth), Fine, the two modulators' shape tables
// (256 values for an LFO cycle, then 16 sequencer steps as value / 24), and the editor's own state (the panel open or
// closed, its tab). They live in the processor's state tree (Source/ModulationState.h reads and writes them there),
// so they are saved, compared (A / B) and undone with it; here are their values and the controls they act on.
namespace pb::dsp
{
// what a slot can hold
enum Source { None = 0, M1, M2, AM, PT, numSources };
inline const char* sourceName (int s) { static const char* n[] { "", "M1", "M2", "AM", "PT" }; return n[std::clamp (s, 0, 4)]; }

// A control's range as its host parameter has it (Params.h builds the parameter from the same): linear from start to
// end, or exponential, x = start (end / start)^v, the value rounded to 1e-5 so that a value set comes back as itself
// (a free rate of 4 Hz a hair over 4 runs a tick short); computed the same on every platform. from0to1 and to0to1
// give what the host parameter's convertFrom0to1 and convertTo0to1 give (the value clipped to the range).
struct Range
{
    float start = 0.0f, end = 1.0f;
    bool exponential = false;

    static float expFrom0to1 (float a, float b, float v) { return (float) (std::round (a * flatwater::math::pow (b / a, v) * 1e5) / 1e5); }
    static float expTo0to1 (float a, float b, float x) { return (float) (flatwater::math::log (x / a) / flatwater::math::log (b / a)); }

    float from0to1 (float v) const
    {
        const float p = std::clamp (v, 0.0f, 1.0f);
        return snap (exponential ? expFrom0to1 (start, end, p) : start + (end - start) * p);
    }
    float to0to1 (float x) const
    {
        const float v = snap (x);
        return std::clamp (exponential ? expTo0to1 (start, end, v) : (v - start) / (end - start), 0.0f, 1.0f);
    }

private:
    float snap (float v) const { return (v <= start || end <= start) ? start : (v >= end ? end : v); }
};

// the controls a modulator can be put on (the reference's: every control but M1 / M2 Smooth, PT Offset, Link, Fine,
// Flatten and Steps), with the parameter each one moves and its range
namespace ranges
{
inline constexpr Range semitones { -12.0f, 12.0f }, mix { 0.0f, 100.0f }, unit { 0.0f, 1.0f }, twice { 0.0f, 2.0f },
                       offset { -1.0f, 1.0f }, rate { 0.06f, 30.0f, true }, warp { 0.01f, 100.0f, true },
                       smooth { 1.0f, 1000.0f, true }, amTime { 0.1f, 1000.0f, true };
}
struct Target { const char* key; const char* param; Range range; };
inline constexpr Target targets[] {
    { "pitch", "pitch", ranges::semitones }, { "formant", "formant", ranges::semitones }, { "mix", "mix", ranges::mix },
    { "m1Rate", "m1Rate", ranges::rate }, { "m1Warp", "m1Warp", ranges::warp }, { "m1Phase", "m1Phase", ranges::unit },
    { "m1Level", "m1Level", ranges::unit },
    { "m2Rate", "m2Rate", ranges::rate }, { "m2Warp", "m2Warp", ranges::warp }, { "m2Phase", "m2Phase", ranges::unit },
    { "m2Level", "m2Level", ranges::unit },
    { "amAttack", "amAttack", ranges::amTime }, { "amRelease", "amRelease", ranges::amTime }, { "amLevel", "amLevel", ranges::twice },
    { "ptSmooth", "ptSmooth", ranges::smooth }, { "ptLevel", "ptLevel", ranges::twice } };
inline constexpr int numTargets = (int) (sizeof (targets) / sizeof (targets[0])), numSlots = 4;

inline constexpr int tableSize = 512, lfoPoints = 256, seqFirst = 256, maxSteps = 16;

struct Slot { int source = None; float depth = 0.0f; };

// ---- shapes ------------------------------------------------------------------------------------------------------
using Table = std::array<float, tableSize>;

// our own shapes, by their math (the original's drawn library is not shipped): one LFO cycle, -1 .. 1
// (their names: Source/ModulationState.h shapeNames)
enum class Shape { Sine, Triangle, Square, SawUp, SawDown, Ramp, SCurve, HalfSine, Pulse25, Pulse75, Steps4, Random, Flat };

inline float shapeValue (Shape s, double ph)                  // ph 0 .. 1
{
    const double pi = flatwater::math::pi;
    switch (s)
    {
        case Shape::Sine:     return (float) flatwater::math::sin (2.0 * pi * ph);
        case Shape::Triangle: return (float) (ph < 0.25 ? 4.0 * ph : ph < 0.75 ? 2.0 - 4.0 * ph : 4.0 * ph - 4.0);
        case Shape::Square:   return ph < 0.5 ? 1.0f : -1.0f;
        case Shape::SawUp:    return (float) (2.0 * ph - 1.0);
        case Shape::SawDown:  return (float) (1.0 - 2.0 * ph);
        case Shape::Ramp:     return (float) (2.0 * (flatwater::math::exp (3.0 * ph) - 1.0) / (flatwater::math::exp (3.0) - 1.0) - 1.0);
        case Shape::SCurve:                                    // a smooth step up over the first half, down over the second
        {
            const double x = ph < 0.5 ? 2.0 * ph : 2.0 - 2.0 * ph;
            return (float) (2.0 * x * x * (3.0 - 2.0 * x) - 1.0);
        }
        case Shape::HalfSine: return (float) (2.0 * std::abs (flatwater::math::sin (pi * ph)) - 1.0);
        case Shape::Pulse25:  return ph < 0.25 ? 1.0f : -1.0f;
        case Shape::Pulse75:  return ph < 0.75 ? 1.0f : -1.0f;
        case Shape::Steps4:   return (float) (1.0 - 2.0 * std::floor (ph * 4.0) / 3.0);
        case Shape::Random:
        {
            static const float r[8] { 0.62f, -0.48f, 0.15f, -0.91f, 0.83f, -0.12f, 0.37f, -0.70f };   // our own fixed draw
            return r[std::clamp ((int) (ph * 8.0), 0, 7)];
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
    for (int i = 0; i < maxSteps; ++i) t[(size_t) (seqFirst + i)] = (float) std::clamp (steps[i], -24, 24) / 24.0f;
}

inline int stepValue (const Table& t, int i) { return flatwater::math::roundToInt (t[(size_t) (seqFirst + i)] * 24.0f); }

inline Table defaultTable (int modulator)                      // M1 a sine, M2 a triangle (the reference's defaults)
{
    Table t {};
    setLfo (t, modulator == 0 ? Shape::Sine : Shape::Triangle);
    return t;
}
}
