// Renders Flatwater Comp's VU dial alone (Source/common/FwHybridUI VuMeter, at its place in the Flatwater Comp editor) to a PNG, for the
// pixel comparison with the reference's filmed frames:   VuRender out.png gr|io <needle degrees> [scale]
#include <juce_gui_basics/juce_gui_basics.h>
#include "shared/ui/HybridUI.h"

int main (int argc, char** argv)
{
    if (argc < 4) { std::fprintf (stderr, "usage: VuRender out.png gr|io degrees [scale]\n"); return 2; }
    juce::ScopedJuceInitialiser_GUI init;
    fw::hybrid::VuMeter vu;
    vu.setBounds (197, 73, 251, 170);                         // FwCompEditor's kDial
    vu.pivot = juce::Point<float> (320.7f, 220.9f) - vu.getPosition().toFloat();    // kVuPivot
    vu.setFace (juce::String (argv[2]) == "io" ? fw::hybrid::VuMeter::Face::io : fw::hybrid::VuMeter::Face::gr);
    vu.setNeedle ((float) std::atof (argv[3]));
    const float scale = argc > 4 ? (float) std::atof (argv[4]) : 1.0f;
    auto img = vu.createComponentSnapshot (vu.getLocalBounds(), true, scale);
    juce::File f (juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]));
    f.deleteFile();
    juce::FileOutputStream os (f);
    juce::PNGImageFormat().writeImageToStream (img, os);
    return 0;
}
