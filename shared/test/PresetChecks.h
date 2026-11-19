#pragma once
// What every Flatwater plug-in's unit tests check of its presets, programs and saved state (shared/Presets.h): the
// preset's name and index and the two setups survive a save and a reload, a state without them (as 1.0.0 saved it)
// loads at the default preset, a preset load and a setup switch are undo steps, and a host that sets a program just
// after a restore changes nothing. A test program calls presetChecks<Processor> with its own check function.
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <cmath>
#include <thread>
#include <vector>
#include "shared/Presets.h"

namespace flatwater::test
{
inline std::vector<float> values (juce::AudioProcessor& p)
{
    std::vector<float> v;
    for (auto* prm : p.getParameters()) v.push_back (prm->getValue());
    return v;
}

inline bool same (const std::vector<float>& a, const std::vector<float>& b)
{
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (std::abs (a[i] - b[i]) > 1.0e-5f) return false;
    return true;
}

// the first automatable parameter that is not `mode`, moved to the other end of its range
inline void moveOne (juce::AudioProcessorValueTreeState& apvts, const char* modeId)
{
    for (auto* prm : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm))
            if (rp->isAutomatable() && rp->getParameterID() != modeId && ! rp->isBoolean())
            {
                rp->setValueNotifyingHost (rp->getValue() > 0.5f ? 0.0f : 1.0f);
                return;
            }
}

inline juce::MemoryBlock saved (juce::AudioProcessor& p)
{
    juce::MemoryBlock mb;
    p.getStateInformation (mb);
    return mb;
}

// Through the host's calls alone (programs, the state, the undo history). check (passed, what).
template <class Processor, class Check>
void programChecks (const char* modeId, Check&& check)
{
    {   // the program is part of the state
        Processor p;
        const int k = juce::jmin (2, p.getNumPrograms() - 1);
        check (k > 0, "the plug-in has factory programs");
        p.setCurrentProgram (k);
        const auto mb = saved (p);
        Processor q;
        q.setStateInformation (mb.getData(), (int) mb.getSize());
        check (q.getCurrentProgram() == k, "a reload keeps the program's index");
        check (same (values (p), values (q)), "a reload keeps the program's values");
    }
    {   // a host that sets program 0 just after a restore: the restored settings stay
        Processor p;
        const int k = juce::jmin (2, p.getNumPrograms() - 1);
        p.setCurrentProgram (k);
        moveOne (p.apvts, modeId);
        const auto mb = saved (p);
        Processor q;
        q.setCurrentProgram (1);
        q.setStateInformation (mb.getData(), (int) mb.getSize());
        const auto restored = values (q);
        check (same (restored, values (p)), "the state restores its values over another program");
        q.setCurrentProgram (0);
        check (same (restored, values (q)), "program 0 set just after a restore leaves the restored values");
        q.setCurrentProgram (k);
        check (same (restored, values (q)), "the restored program set again leaves the restored values");
    }
    {   // a program change is one undo step
        Processor p;
        const int k = juce::jmin (2, p.getNumPrograms() - 1);
        moveOne (p.apvts, modeId);
        p.apvts.copyState();                               // (the parameter's move reaches the tree, as the tree's timer does)
        const auto before = values (p);
        p.setCurrentProgram (k);
        const auto loaded = values (p);
        check (! same (before, loaded), "the program changes a value");
        check (p.undo.undo(), "a program change can be undone");
        check (same (before, values (p)), "undo after a program change gives the values before it");
        check (p.undo.redo(), "the program change can be redone");
        check (same (loaded, values (p)), "redo gives the program's values");
    }
}

// The processor's flatwater::PresetState, which the toolbar drives (the toolbar's own part is the files and the menus).
template <class Processor, class Check>
void presetChecks (const char* modeId, Check&& check)
{
    const juce::String defaultName (PresetState::defaultName);
    const auto rootProperties = [] (const juce::ValueTree& t)
    {
        juce::StringArray names;
        for (int i = 0; i < t.getNumProperties(); ++i) names.add (t.getPropertyName (i).toString());
        return names;
    };
    {   // the preset's name and index and both setups survive a save and a reload in a new instance
        Processor p;
        auto& ps = p.presetState();
        const auto fresh = rootProperties (p.apvts.copyState());
        const int k = juce::jmin (1, (int) ps.factory().size() - 1);
        const juce::String factoryName (ps.factory()[(size_t) k].name);
        check (ps.name() == defaultName && ps.factoryIndex() == -1 && ps.activeSetup() == 0, "a new instance is on the default preset, setup A");
        ps.loadFactory (k);
        check (ps.name() == factoryName && ps.factoryIndex() == k && p.getCurrentProgram() == k + 1, "a factory preset gives its name, index and program");
        check (! ps.modified(), "a loaded preset is not marked as changed");
        moveOne (p.apvts, modeId);
        check (ps.modified(), "a moved parameter marks the preset as changed");
        const auto setupA = values (p);
        ps.switchSetup();
        check (ps.activeSetup() == 1 && same (setupA, values (p)), "the first switch to setup B keeps the values");
        ps.loadDefault();
        check (ps.name() == defaultName && ps.factoryIndex() == -1 && p.getCurrentProgram() == 0, "the default preset has the default name, program 0");
        const auto setupB = values (p);
        check (! same (setupA, setupB), "the two setups differ");

        const auto mb = saved (p);
        Processor q;
        auto& qs = q.presetState();
        q.setStateInformation (mb.getData(), (int) mb.getSize());
        check (qs.activeSetup() == 1 && qs.name() == defaultName && same (setupB, values (q)), "a reload comes back on setup B with its preset and values");
        check (! qs.modified(), "a reloaded state is not marked as changed");
        qs.switchSetup();
        check (qs.activeSetup() == 0 && same (setupA, values (q)), "the reloaded setup A has its values");
        check (qs.name() == factoryName && qs.factoryIndex() == k && q.getCurrentProgram() == k + 1, "the reloaded setup A has its preset's name and index");
        qs.switchSetup();
        check (qs.activeSetup() == 1 && qs.name() == defaultName && same (setupB, values (q)), "and setup B is still there after it");

        // what the new state adds for an older build to skip: root properties only, under these four names
        juce::StringArray added;
        for (const auto& n : rootProperties (q.apvts.copyState())) if (! fresh.contains (n)) added.add (n);
        added.removeString ("preset"); added.removeString ("presetIndex"); added.removeString ("compare"); added.removeString ("compareOther");
        check (added.isEmpty(), "the state adds no root property besides preset, presetIndex, compare and compareOther");
        check (q.apvts.state.getType() == Processor().apvts.state.getType(), "the state keeps its tag");
    }
    {   // a state without any of it, as 1.0.0 saved it: the default preset, setup A, nothing to switch to
        Processor old;
        moveOne (old.apvts, modeId);
        auto tree = old.apvts.copyState();
        check (! tree.hasProperty ("preset") && ! tree.hasProperty ("presetIndex") && ! tree.hasProperty ("compare") && ! tree.hasProperty ("compareOther"),
               "a state that never loaded a preset holds no preset or setup");
        juce::MemoryBlock mb;
        if (auto xml = tree.createXml()) juce::AudioProcessor::copyXmlToBinary (*xml, mb);
        Processor q;
        auto& qs = q.presetState();
        qs.loadFactory (0);
        qs.switchSetup();
        q.setStateInformation (mb.getData(), (int) mb.getSize());
        check (same (values (old), values (q)), "the old state's values load");
        check (qs.name() == defaultName && qs.factoryIndex() == -1 && q.getCurrentProgram() == 0 && qs.activeSetup() == 0, "the old state loads on the default preset, setup A");
        q.setCurrentProgram (0);
        check (same (values (old), values (q)), "program 0 after the old state leaves its values");
        qs.switchSetup();
        check (qs.activeSetup() == 1 && same (values (old), values (q)), "the old state has no second setup: the switch keeps the values");
    }
    {   // loading a preset and switching the setup are undo steps, and the history stays
        Processor p;
        auto& ps = p.presetState();
        const int k = juce::jmin (1, (int) ps.factory().size() - 1);
        moveOne (p.apvts, modeId);
        p.apvts.copyState();
        check (p.undo.canUndo(), "a moved parameter is in the undo history");
        const auto before = values (p);
        ps.loadFactory (k);
        const auto loaded = values (p);
        check (p.undo.undo() && same (before, values (p)), "undo after a factory preset gives the values before it");
        check (ps.name() == defaultName && ps.factoryIndex() == -1, "and the preset's name before it");
        check (p.undo.canUndo(), "the history before the preset is still there");
        check (p.undo.redo() && same (loaded, values (p)) && ps.factoryIndex() == k, "redo loads the preset again");

        const auto user = ps.presetTree();                 // as the toolbar writes a user preset file
        check (! user.hasProperty ("preset") && ! user.hasProperty ("compareOther") && ! user.hasProperty ("uiWidth"), "a user preset holds no preset name, setup or window size");
        ps.loadDefault();
        const auto atDefault = values (p);
        check (ps.loadTree (user, "Mine") && same (loaded, values (p)), "a user preset loads its values");
        check (ps.name() == "Mine" && ps.factoryIndex() == -1 && p.getCurrentProgram() == 0, "a user preset has its name, program 0");
        check (p.undo.undo() && same (atDefault, values (p)) && ps.name() == defaultName, "undo after a user preset gives the values and the name before it");
        check (! ps.loadTree (juce::ValueTree ("AnotherPlugIn"), "Other") && same (atDefault, values (p)) && ps.name() == defaultName,
               "a preset of another plug-in changes nothing");

        ps.switchSetup();                                  // B starts as A
        ps.loadFactory (k);
        ps.switchSetup();                                  // back on A: the default
        check (ps.activeSetup() == 0 && same (atDefault, values (p)) && ps.name() == defaultName, "setup A comes back with its values and name");
        check (p.undo.undo() && ps.activeSetup() == 1 && same (loaded, values (p)) && ps.factoryIndex() == k, "undo after a setup switch goes back to the other setup");
        ps.copyToOther();
        ps.switchSetup();
        check (ps.activeSetup() == 0 && same (loaded, values (p)), "a copied setup is the same on the other side");
    }
    {   // a program from the host: not applied just after a restore, applied once the restore has settled, and never
        // on another thread than the message thread
        Processor p;
        const int k = juce::jmin (2, p.getNumPrograms() - 1);
        p.setCurrentProgram (k);
        moveOne (p.apvts, modeId);
        const auto mb = saved (p);
        Processor q;
        auto& qs = q.presetState();
        q.setStateInformation (mb.getData(), (int) mb.getSize());
        const auto restored = values (q);
        q.setCurrentProgram (1);
        check (same (restored, values (q)) && q.getCurrentProgram() == k, "another program just after a restore is not applied");
        qs.endRestoreGuard();
        q.setCurrentProgram (1);
        check (q.getCurrentProgram() == 1 && ! same (restored, values (q)), "a program set later is applied");
        const auto one = values (q);
        std::thread ([&q] { q.setCurrentProgram (0); }).join();
        check (q.getCurrentProgram() == 1 && same (one, values (q)), "a program set from another thread waits");
        qs.applyPendingProgram();
        check (q.getCurrentProgram() == 0, "and is applied on the message thread");
    }
}
}
