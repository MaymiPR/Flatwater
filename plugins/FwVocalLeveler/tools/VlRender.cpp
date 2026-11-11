// Offline renderer for Flatwater Vocal Leveler (used by the null tests): the plug-in's own processor, not a copy of its DSP.
//   VlRender in.f32 out.f32 channels sampleRate [key=value ...]
// Files are raw interleaved 32-bit floats. Keys (real units, the parameter IDs): target vocalSensitivity musicSensitivity
// rangeMax rangeMin output spill rider (dB), attack (0 Slow, 1 Fast), automation (0 Off, 1 Write, 2 Read), idle (dB, its
// place in the range as the editor's arrow), mode (0 Stereo, 1 Mono, 2 Live Stereo, 3 Live Mono), preset (a factory
// preset's index, applied where it appears among the keys), block (samples, default 512). There is no latency; the
// output is the same length as the input.
//   tune:key=value: a constant of Source/dsp/Tuning.h for this render; tuning=<file>: key=value lines of them
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

int main (int argc, char** argv)
{
    if (argc < 5) { std::fprintf (stderr, "usage: VlRender in.f32 out.f32 channels fs [key=value ...]\n"); return 2; }
    juce::ScopedJuceInitialiser_GUI init;
    const int ch = std::max (1, std::min (2, std::atoi (argv[3])));
    const double fs = std::atof (argv[4]);
    int block = 512;
    // tune:key=value and tuning=file first: the Tuning constants (Source/dsp/Tuning.h) the DSP copies when it is made
    for (int i = 5; i < argc; ++i)
    {
        const std::string a = argv[i];
        if (a.rfind ("tuning=", 0) == 0 && vl::dsp::tuning().load (a.c_str() + 7) != 0) return 1;
        if (a.rfind ("tune:", 0) == 0)
        {
            const auto eq = a.find ('=');
            if (eq == std::string::npos || ! vl::dsp::tuning().set (a.substr (5, eq - 5), std::atof (a.c_str() + eq + 1)))
            {
                std::fprintf (stderr, "unknown tuning key in %s\n", a.c_str());
                return 2;
            }
        }
    }
    FwVocalLevelerProcessor p;
    auto set = [&p] (const char* id, float v) { if (auto* q = p.apvts.getParameter (id)) q->setValueNotifyingHost (q->convertTo0to1 (v)); };
    float idleDbValue = 0.0f;
    bool idleGiven = false;
    for (int i = 5; i < argc; ++i)
    {
        const std::string a = argv[i];
        const auto eq = a.find ('=');
        if (eq == std::string::npos) continue;
        const std::string k = a.substr (0, eq);
        const float v = (float) std::atof (a.c_str() + eq + 1);
        if (k == "block") block = std::max (1, (int) v);
        else if (k == "preset")
        {
            const auto& list = vl::factoryPresets();
            if ((int) v >= 0 && (int) v < (int) list.size()) { flatwater::applyPreset (p.apvts, list[(size_t) v], p.apvts.getParameter (vl::id::mode)); p.readExactIdle(); }
        }
        else if (k == "idle") { idleDbValue = v; idleGiven = true; }
        else set (k.c_str(), v);
    }
    if (idleGiven)       // the arrow's place in the range for the dB asked
    {
        const float lo = p.get (vl::id::rangeMin), hi = p.get (vl::id::rangeMax);
        set (vl::id::idle, hi > lo ? (idleDbValue - lo) / (hi - lo) : 0.5f);
    }
    FILE* f = std::fopen (argv[1], "rb");
    if (! f) { std::fprintf (stderr, "cannot read %s\n", argv[1]); return 1; }
    std::vector<float> in;
    float tmp[4096];
    size_t got;
    while ((got = std::fread (tmp, sizeof (float), 4096, f)) > 0) in.insert (in.end(), tmp, tmp + got);
    std::fclose (f);
    const long long n = (long long) in.size() / ch;
    p.setPlayConfigDetails (ch, ch, fs, block);
    p.prepareToPlay (fs, block);
    const int lat = p.getLatencySamples();
    juce::AudioBuffer<float> b (ch, block);
    juce::MidiBuffer midi;
    std::vector<float> out ((size_t) (n * ch), 0.0f);
    for (long long s = 0; s < n + lat; s += block)
    {
        const int m = (int) std::min<long long> (block, n + lat - s);
        b.setSize (ch, m, false, false, true);
        for (int c = 0; c < ch; ++c)
            for (int i = 0; i < m; ++i) b.setSample (c, i, s + i < n ? in[(size_t) ((s + i) * ch + c)] : 0.0f);
        p.processBlock (b, midi);
        for (int c = 0; c < ch; ++c)
            for (int i = 0; i < m; ++i)
            {
                const long long t = s + i - lat;
                if (t >= 0 && t < n) out[(size_t) (t * ch + c)] = b.getSample (c, i);
            }
    }
    FILE* o = std::fopen (argv[2], "wb");
    if (! o) { std::fprintf (stderr, "cannot write %s\n", argv[2]); return 1; }
    std::fwrite (out.data(), sizeof (float), out.size(), o);
    std::fclose (o);
    return 0;
}
