#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <algorithm>
#include <atomic>
#include <functional>
#include <vector>

namespace flatwater
{
// A factory preset: real-world values for some parameters; every other parameter (except the Mode switch) goes back
// to its default, so a preset always gives the same sound.
struct FactoryPreset
{
    struct Value { const char* id; float value; };
    const char* group;
    const char* name;
    std::vector<Value> values;
    std::vector<const char*> leave {};   // parameters this preset leaves where they are (Flatwater Bass Leveler's Target, "*" in the reference)
};

inline const juce::Identifier presetExactTag { "PresetExact" };

// keep / alsoKeep: parameters a preset never changes (the Mode switch; Flatwater BassX Lift's Input, which the reference marks
// IgnoreOnPresetLoad). With an undo manager the preset's stored values enter the state as undoable changes.
void applyPreset (juce::AudioProcessorValueTreeState& apvts, const FactoryPreset& preset, juce::RangedAudioParameter* keep,
                  const std::vector<juce::AudioProcessorParameter*>& alsoKeep = {}, juce::UndoManager* undo = nullptr);

// The preset a plug-in is on and its two setups (A and B), kept by the processor so that they outlive the editor and
// travel with the saved state. All of it is in the processor's state tree, as root properties an older build ignores:
//   preset         the loaded preset's name (absent: the default preset)
//   presetIndex    its place in the factory list (absent: the default preset or a user preset)
//   compare        the setup in use, 0 for A and 1 for B (absent: A)
//   compareOther   the other setup, a state tree as XML text (absent: not used yet, the same as the one in use)
// Loading a preset and switching the setup set the parameters and the tree through the undo manager inside one
// transaction, so each is one undo step and the history stays. The host's program is the factory preset's index plus
// one, 0 for anything else, and comes back from a saved state without being applied again.
//
// The actions belong to the message thread. A tree change is taken on the thread that makes it (a restore may come
// from another thread), and what the toolbar reads is a copy behind a lock.
class PresetState : private juce::ValueTree::Listener, private juce::Timer
{
public:
    PresetState (juce::AudioProcessorValueTreeState&, juce::UndoManager&, const std::vector<FactoryPreset>& factoryList,
                 juce::RangedAudioParameter* modeParam, std::vector<juce::AudioProcessorParameter*> keepOnLoad = {});
    ~PresetState() override;

    // what a factory preset or the default sets beyond the parameters (the preset's stored values, the undo manager):
    // called inside the load's transaction
    std::function<void (const juce::ValueTree& exact, juce::UndoManager*)> applyExtras;

    static constexpr const char* defaultName = "Default Preset";
    const std::vector<FactoryPreset>& factory() const { return list; }
    juce::String name() const;
    int factoryIndex() const;                         // -1: the default preset or a user preset
    int activeSetup() const;                          // 0: A, 1: B
    bool modified();                                  // a parameter moved since the preset was loaded or saved

    void loadDefault();
    void loadFactory (int index);
    bool loadTree (const juce::ValueTree& preset, const juce::String& presetName);   // a user preset; false: another plug-in's
    juce::ValueTree presetTree();                     // the state as a user preset holds it (no window size, no setups)
    void savedAs (const juce::String& presetName);    // the current settings are now this user preset
    void switchSetup();
    void copyToOther();

    // the host's programs
    int numPrograms() const { return 1 + (int) list.size(); }
    int program() const { return factoryIndex() + 1; }
    void setProgram (int index);                      // from another thread than the message thread: applied by the timer
    // A program set within this time after a restore is not applied: a host that replays the program it remembers
    // would load a preset over the restored settings.
    static constexpr int restoreGuardMs = 500;
    void endRestoreGuard() { restoredAt.store (0); }
    void applyPendingProgram();                       // (the timer's work, also for a test)

private:
    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override;
    void valueTreeRedirected (juce::ValueTree&) override;
    void timerCallback() override { applyPendingProgram(); }
    void readTree();
    void begin();
    void end();
    void load (const FactoryPreset&, const juce::String& presetName, int index);
    void copyInto (const juce::ValueTree& source, bool withPreset);
    juce::ValueTree setupTree();
    std::vector<float> currentValues() const;

    juce::AudioProcessorValueTreeState& apvts;
    juce::UndoManager& undo;
    const std::vector<FactoryPreset>& list;
    juce::RangedAudioParameter* mode;
    std::vector<juce::AudioProcessorParameter*> keep;
    const FactoryPreset defaults { "", "Default", {} };   // every parameter at its default

    juce::CriticalSection lock;                       // the copies below
    juce::String shownName { defaultName };
    int shownIndex = -1, shownSetup = 0;
    std::vector<float> loadedValues;
    bool loadedStale = true;
    std::atomic<int> pendingProgram { -1 };
    std::atomic<juce::uint32> restoredAt { 0 };       // (0: no restore to guard)
};

// The editor's size in the state, two root properties side by side:
//   uiScale   the window's width over the editor's design width (1 is 100 %)
//   uiWidth   the window's width in pixels, the only form older builds read and wrote
// A state is read by `uiScale` first and by `uiWidth` when it has none, so a project an older build saved opens at
// its size; both are written, so an older build opens a newer project at its size too. An older build that resizes
// the window moves `uiWidth` alone: a `uiWidth` more than a pixel off its `uiScale` is the later one and is read.
// The window menu's entries and the resize corner share the one range below, and a size outside it opens at the
// nearer end.
inline constexpr float minUiScale = 0.75f, maxUiScale = 2.0f;
float readUiScale (const juce::ValueTree& state, int designWidth, float fallback = 1.0f);
void writeUiScale (juce::ValueTree& state, int designWidth, int width);

// a processor with presets: the toolbar finds them through it
struct HasPresets
{
    virtual ~HasPresets() = default;
    virtual PresetState& presetState() = 0;
};
}
