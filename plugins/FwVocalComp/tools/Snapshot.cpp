// Renders the Flatwater Vocal Comp editor to a PNG:  FwVocalCompSnapshot <out.png> [comp gate gain [mono]]
#include <juce_gui_basics/juce_gui_basics.h>
#include <iostream>
#include "PluginEditor.h"

int main (int argc, char** argv)
{
    if (argc < 2) { std::cerr << "usage: FwVocalCompSnapshot out.png [comp gate gain [mono]]\n"; return 2; }
    juce::ScopedJuceInitialiser_GUI init;
    FwVocalCompProcessor proc;
    if (argc >= 5)
    {
        auto set = [&] (const char* id, float v) { auto* p = proc.apvts.getParameter (id); p->setValueNotifyingHost (p->convertTo0to1 (v)); };
        set (vc::comp, (float) atof (argv[2]));
        set (vc::gate, (float) atof (argv[3]));
        set (vc::gain, (float) atof (argv[4]));
    }
    if (argc >= 6) { auto* p = proc.apvts.getParameter (vc::mode); p->setValueNotifyingHost (1.0f); }
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
    juce::File f (juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]));
    f.deleteFile();
    juce::FileOutputStream os (f);
    juce::PNGImageFormat().writeImageToStream (img, os);
    return 0;
}
