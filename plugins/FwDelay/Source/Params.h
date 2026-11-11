#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include <cstring>
#include <limits>
#include "dsp/Engine.h"
#include "plugins/FwComp/Source/dsp/Analog.h"
#include "dsp/AnalogTables.h"

// Parameters follow the reference's parameter list (its order, ranges, tapers and defaults; see RESEARCH.md,
// Flatwater Delay). The DSP (dsp/Engine.h) and the Analog stage as identified on the reference (MEASUREMENTS.md).
namespace de
{
namespace id
{
inline constexpr const char* delay = "delay";          // 1 .. 3500 ms (the reference's taper), default 375
inline constexpr const char* delayNote = "delayNote";  // 20 note values (1/64T .. 2 bars), default 1/8D
inline constexpr const char* delayR = "delayR";        // the right side's time and note (Dual)
inline constexpr const char* delayNoteR = "delayNoteR";
inline constexpr const char* sync = "sync";            // BPM / Host / ms, default Host
inline constexpr const char* bpm = "bpm";              // 30 .. 300, default 120 (used in BPM sync)
inline constexpr const char* feedback = "feedback";    // 0 .. 200 %, default 60
inline constexpr const char* mix = "mix";              // 0 (dry) .. 100 (wet), default 100
inline constexpr const char* output = "output";        // -18 .. +18 dB, default 0
inline constexpr const char* analog = "analog";        // Off / 1 .. 4, default 2
inline constexpr const char* lofi = "lofi";            // Off / On
inline constexpr const char* rate = "rate";            // 0.1 .. 6000 Hz, logarithmic, default 0.1
inline constexpr const char* depth = "depth";          // 0 .. 100 %, default 0
inline constexpr const char* hipass = "hipass";        // 19 (Off) .. 20 139 Hz, logarithmic
inline constexpr const char* lopass = "lopass";        // 19 .. 20 139 Hz (Off from 20 100), logarithmic
inline constexpr const char* phaseL = "phaseL";        // the left (mono: the only) delay line inverted
inline constexpr const char* phaseR = "phaseR";
inline constexpr const char* pingPong = "pingPong";
inline constexpr const char* pingPongStart = "pingPongStart";   // Left / Right
inline constexpr const char* dual = "dual";            // separate left and right times
inline constexpr const char* mode = "mode";            // 0 Stereo, 1 Mono, 2 mono in, stereo out
}

constexpr float kHiPassOff = 19.5f, kLoPassOff = 20100.0f;
inline const juce::StringArray& noteNames()
{
    static const juce::StringArray n { "1/64T", "1/64", "1/64D", "1/32T", "1/32", "1/32D", "1/16T", "1/16", "1/16D", "1/8T",
                                       "1/8", "1/8D", "1/4T", "1/4", "1/4D", "1/2T", "1/2", "1/2D", "1B", "2B" };
    return n;
}
// a note value's length in beats (quarter notes)
double noteBeats (int note);
// the note value closest to a time at a tempo (used when the display changes from ms to note values)
int nearestNote (double ms, double tempo);
// the reference's Delay taper: 1 ms per 1/1000 of the range up to 501 ms at the middle, then the measured curve to 3500
float delayFromNorm (float n);
float delayToNorm (float ms);
juce::String freqText (float hz, bool hiPass);
// a parameter's value as the reference uses it (rounded to its resolution)
float atResolution (const char* pid, float value);
juce::String rateText (float hz);

inline constexpr int numParams = 21;
// every parameter in the layout's order (createLayout below, the order hosts list them in)
inline constexpr const char* const allIds[numParams] {
    id::sync, id::bpm, id::delay, id::delayNote, id::mix, id::feedback, id::phaseL, id::phaseR, id::pingPong,
    id::output, id::lopass, id::hipass, id::depth, id::rate, id::lofi, id::analog, id::pingPongStart, id::delayR,
    id::delayNoteR, id::dual, id::mode };
// what a factory preset sets: every parameter but Mode, which a preset leaves as it is (flatwater::applyPreset)
inline constexpr const char* const presetIds[numParams - 1] {
    id::sync, id::bpm, id::delay, id::delayNote, id::mix, id::feedback, id::phaseL, id::phaseR, id::pingPong,
    id::output, id::lopass, id::hipass, id::depth, id::rate, id::lofi, id::analog, id::pingPongStart, id::delayR,
    id::delayNoteR, id::dual };

// the host parameters: the reference's names, ranges, tapers, defaults and display texts, in its order
inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto addF = [&] (const char* pid, const char* name, NormalisableRange<float> r, float def, std::function<String (float)> text)
    {
        auto a = AudioParameterFloatAttributes().withStringFromValueFunction ([text] (float v, int) { return text (v); });
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { pid, 1 }, name, r, def, a));
    };
    auto addC = [&] (const char* pid, const char* name, StringArray items, int def)
    {
        l.add (std::make_unique<AudioParameterChoice> (ParameterID { pid, 1 }, name, items, def));
    };
    auto logRange = [] (float lo, float hi)
    {
        return NormalisableRange<float> (lo, hi,
            [] (float s, float e, float n) { return s * std::pow (e / s, n); },
            [] (float s, float e, float v) { return std::log (v / s) / std::log (e / s); });
    };
    const NormalisableRange<float> delayRange (1.0f, 3500.0f,
        [] (float, float, float n) { return de::delayFromNorm (n); },
        [] (float, float, float v) { return de::delayToNorm (v); });
    auto ms = [] (float v) { return String (roundToInt (v)) + " ms"; };
    const auto& notes = de::noteNames();
    // continuous values, as the reference keeps them (its presets store fractions; a slow Ctrl drag moves less than the
    // display's last digit per mouse step); only the texts are rounded

    addC (de::id::sync, "Sync", { "BPM", "Host", "ms" }, 1);
    addF (de::id::bpm, "BPM", { 30.0f, 300.0f }, 120.0f, [] (float v) { return String (roundToInt (v)) + " BPM"; });
    addF (de::id::delay, "Delay", delayRange, 375.0f, ms);
    addC (de::id::delayNote, "Delay Note", notes, 11);
    addF (de::id::mix, "Mix", { 0.0f, 100.0f }, 100.0f, [] (float v) { return String (roundToInt (v)); });
    addF (de::id::feedback, "Feedback", { 0.0f, 200.0f }, 60.0f, [] (float v) { return String (roundToInt (v)); });
    addC (de::id::phaseL, "Phase L", { "+", "-" }, 0);
    addC (de::id::phaseR, "Phase R", { "+", "-" }, 0);
    addC (de::id::pingPong, "Ping Pong", { "Stereo", "PingPong" }, 0);
    addF (de::id::output, "Output", { -18.0f, 18.0f }, 0.0f, [] (float v) { return String (v, 1) + " dB"; });
    addF (de::id::lopass, "LoPass", logRange (19.0f, 20138.77f), 20138.77f, [] (float v) { return de::freqText (v, false); });
    addF (de::id::hipass, "HiPass", logRange (19.0f, 20138.77f), 19.0f, [] (float v) { return de::freqText (v, true); });
    addF (de::id::depth, "Depth", { 0.0f, 100.0f }, 0.0f, [] (float v) { return String (roundToInt (v)) + " %"; });
    addF (de::id::rate, "Rate", logRange (0.1f, 6000.0f), 0.1f, [] (float v) { return de::rateText (v); });
    addC (de::id::lofi, "LoFi", { "HiFi", "LoFi" }, 0);
    addC (de::id::analog, "Analog", { "Off", "1", "2", "3", "4" }, 2);
    addC (de::id::pingPongStart, "Ping Pong Start", { "Left", "Right" }, 0);
    addF (de::id::delayR, "Delay R", delayRange, 375.0f, ms);
    addC (de::id::delayNoteR, "Delay Note R", notes, 11);
    addC (de::id::dual, "Dual", { "Off", "On" }, 0);
    addC (de::id::mode, "Mode", { "Stereo", "Mono", "M>S" }, 0);
    return l;
}
}
