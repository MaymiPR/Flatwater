#pragma once
#include <juce_core/juce_core.h>

namespace fw
{
// Settings shared by all Flatwater plugins ("Apply as Default to All" window size), kept in
// <user app data>/Flatwater/settings.xml.
inline juce::File settingsFile()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Flatwater/settings.xml");
}

inline float defaultWindowScale()
{
    if (auto xml = juce::XmlDocument::parse (settingsFile()))
        return (float) xml->getDoubleAttribute ("windowScale", 1.0);
    return 1.0f;
}

inline void setDefaultWindowScale (float s)
{
    auto f = settingsFile();
    f.getParentDirectory().createDirectory();
    juce::XmlElement xml ("FwSettings");
    xml.setAttribute ("windowScale", (double) s);
    xml.writeTo (f);
}
}
