// Offline renderer for Flatwater Vocal Chain (used by the null tests): the plug-in's own processor, not a copy of its DSP.
//   ChRender in.f32 out.f32 channels sampleRate [key=value ...]
// Files are raw interleaved 32-bit floats (channels 1: the Mono/Stereo component, the output still stereo; 2: Stereo).
// Keys (real units, the parameter IDs): sensitivity output balance bass treble compress reverb delay pitch, and
// bassColor ... pitchColor (0 clear, 1 .. 3), noise (0 off, as the reference's state setting), block (samples, default 512),
// compressAuto (a raw float file: the Compress fader per block, set before each, as a host automates it), preset (a
// factory preset, 1 .. 43, applied where it stands among the keys: keys after it override it). The latency
// is removed; the output is stereo and the same length as the input.
// An unknown key, an argument that is no key=value, a channel count other than 1 or 2 or a block under 1 ends the
// render with exit code 2 and one line on stderr; an input or a compressAuto file that cannot be read, a bus layout
// the processor refuses and an output that could not be written in full end it with 1.
//   tune:key=value: a constant of Source/dsp/Tuning.h for this render; tuning=<file>: key=value lines of them
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// a wrong argument: one line on stderr, exit code 2
static int fail (const char* what, const char* arg) { std::fprintf (stderr, "%s: %s\n", what, arg); return 2; }

// the whole of v into the file, or false
static bool writeAll (const char* path, const std::vector<float>& v)
{
    FILE* o = std::fopen (path, "wb");
    if (! o) return false;
    const bool written = std::fwrite (v.data(), sizeof (float), v.size(), o) == v.size();
    return std::fclose (o) == 0 && written;
}

int main (int argc, char** argv)
{
    if (argc < 5) { std::fprintf (stderr, "usage: ChRender in.f32 out.f32 channels fs [key=value ...]\n"); return 2; }
    juce::ScopedJuceInitialiser_GUI init;
    const int ch = std::atoi (argv[3]);
    if (ch < 1 || ch > 2) return fail ("channels must be 1 or 2", argv[3]);
    const double fs = std::atof (argv[4]);
    int block = 512;
    std::vector<float> autoCompress;
    // tune:key=value and tuning=file first: the Tuning constants (Source/dsp/Tuning.h) the DSP copies when it is made
    for (int i = 5; i < argc; ++i)
    {
        const std::string a = argv[i];
        if (a.rfind ("tuning=", 0) == 0 && ch::dsp::tuning().load (a.c_str() + 7) != 0) return 1;
        if (a.rfind ("tune:", 0) == 0)
        {
            const auto eq = a.find ('=');
            if (eq == std::string::npos || ! ch::dsp::tuning().set (a.substr (5, eq - 5), std::atof (a.c_str() + eq + 1)))
            {
                std::fprintf (stderr, "unknown tuning key in %s\n", a.c_str());
                return 2;
            }
        }
    }
    FwVocalChainProcessor p;
    auto set = [&p] (const char* id, float v) { if (auto* q = p.apvts.getParameter (id)) q->setValueNotifyingHost (q->convertTo0to1 (v)); };
    set (ch::id::mode, ch == 1 ? 1.0f : 0.0f);
    for (int i = 5; i < argc; ++i)
    {
        const std::string a = argv[i];
        const auto eq = a.find ('=');
        if (a.rfind ("tune:", 0) == 0 || a.rfind ("tuning=", 0) == 0) continue;   // (read above)
        if (eq == std::string::npos) return fail ("not a key=value argument", a.c_str());
        const std::string k = a.substr (0, eq);
        const float v = (float) std::atof (a.c_str() + eq + 1);
        if (k == "block") { block = (int) v; if (block < 1) return fail ("block must be 1 or more", a.c_str()); }
        else if (k == "compressAuto")
        {
            FILE* af = std::fopen (a.c_str() + eq + 1, "rb");
            if (! af) { std::fprintf (stderr, "cannot read %s\n", a.c_str() + eq + 1); return 1; }
            float t[1024]; size_t m;
            while ((m = std::fread (t, sizeof (float), 1024, af)) > 0) autoCompress.insert (autoCompress.end(), t, t + m);
            std::fclose (af);
        }
        else if (k == "noise") p.setNoise (v != 0.0f);
        else if (k == "preset") p.setCurrentProgram ((int) v);       // a factory preset (1 ..), before the keys after it
        else if (p.apvts.getParameter (k.c_str()) == nullptr) return fail ("unknown key", a.c_str());
        else set (k.c_str(), v);
    }
    FILE* f = std::fopen (argv[1], "rb");
    if (! f) { std::fprintf (stderr, "cannot read %s\n", argv[1]); return 1; }
    std::vector<float> in;
    float tmp[4096];
    size_t got;
    while ((got = std::fread (tmp, sizeof (float), 4096, f)) > 0) in.insert (in.end(), tmp, tmp + got);
    std::fclose (f);
    const long long n = (long long) in.size() / ch;
    juce::AudioProcessor::BusesLayout layout;
    layout.inputBuses.add (ch == 1 ? juce::AudioChannelSet::mono() : juce::AudioChannelSet::stereo());
    layout.outputBuses.add (juce::AudioChannelSet::stereo());
    if (! p.setBusesLayout (layout)) { std::fprintf (stderr, "the bus layout was refused\n"); return 1; }
    p.setPlayConfigDetails (ch, 2, fs, block);
    p.prepareToPlay (fs, block);
    const int lat = p.getLatencySamples();
    juce::AudioBuffer<float> b (2, block);
    juce::MidiBuffer midi;
    std::vector<float> out ((size_t) (n * 2), 0.0f);
    for (long long s = 0; s < n + lat; s += block)
    {
        const int m = (int) std::min<long long> (block, n + lat - s);
        b.setSize (2, m, false, false, true);
        b.clear();
        for (int c = 0; c < ch; ++c)
            for (int i = 0; i < m; ++i) b.setSample (c, i, s + i < n ? in[(size_t) ((s + i) * ch + c)] : 0.0f);
        if (const auto k = (size_t) (s / block); k < autoCompress.size()) set ("compress", autoCompress[k]);
        p.processBlock (b, midi);
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < m; ++i)
            {
                const long long t = s + i - lat;
                if (t >= 0 && t < n) out[(size_t) (t * 2 + c)] = b.getSample (c, i);
            }
    }
    if (! writeAll (argv[2], out)) { std::fprintf (stderr, "cannot write %s\n", argv[2]); return 1; }
    return 0;
}
