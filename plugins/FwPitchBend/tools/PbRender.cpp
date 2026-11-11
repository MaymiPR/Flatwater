// Offline renderer for Flatwater Pitch Bend (used by the null tests): the plug-in's own processor, not a copy of its DSP.
//   PbRender in.f32 out.f32 channels sampleRate [key=value ...]
// Files are raw interleaved 32-bit floats. Keys are the parameter IDs in real units (pitch=-5, mix=50, m1Rate=2,
// flattenNote=12 for C3 as the index of a choice, ...), and:
//   block=<n>                  the host block (default 512)
//   bpm=<tempo>                a playing host with that tempo; its song position is 0 where the input starts
//   preroll=<seconds>          that much silence is processed first and not written (as tools/RefRender's)
//   fine=1                     Pitch and Formant in cents
//   slot=<target>:<s>:<source>:<depth>   a modulation slot (target a key of targets[], s 0 .. 3, source 0 .. 4)
//   table=<m>:<file>           modulator m's (0 M1, 1 M2) shape table, 512 raw float32 values
//   preset=<k>                 factory preset k (1 .. 40, as the host's program list; given first, later keys change it)
//   tune:key=value: a constant of Source/dsp/Tuning.h for this render; tuning=<file>: key=value lines of them
// There is no latency; the output is the same length as the input.
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

struct Head : public juce::AudioPlayHead
{
    double bpm = 0.0, fs = 48000.0;
    juce::int64 pos = 0;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo p;
        if (bpm > 0.0) { p.setBpm (bpm); p.setTimeSignature (TimeSignature { 4, 4 }); p.setPpqPosition ((double) pos / fs * bpm / 60.0); }
        p.setTimeInSamples (pos);
        p.setTimeInSeconds ((double) pos / fs);
        p.setIsPlaying (true);
        return p;
    }
};

int main (int argc, char** argv)
{
    if (argc < 5) { std::fprintf (stderr, "usage: PbRender in.f32 out.f32 channels fs [key=value ...]\n"); return 2; }
    juce::ScopedJuceInitialiser_GUI init;
    const int ch = std::max (1, std::min (2, std::atoi (argv[3])));
    const double fs = std::atof (argv[4]);
    int block = 512;
    double preroll = 0.0;
    Head head;
    head.fs = fs;
    // tune:key=value and tuning=file first: the Tuning constants (Source/dsp/Tuning.h) the DSP copies when it is made
    for (int i = 5; i < argc; ++i)
    {
        const std::string a = argv[i];
        if (a.rfind ("tuning=", 0) == 0 && pb::tuning().load (a.c_str() + 7) != 0) return 1;
        if (a.rfind ("tune:", 0) == 0)
        {
            const auto eq = a.find ('=');
            if (eq == std::string::npos || ! pb::tuning().set (a.substr (5, eq - 5), std::atof (a.c_str() + eq + 1)))
            {
                std::fprintf (stderr, "unknown tuning key in %s\n", a.c_str());
                return 2;
            }
        }
    }
    FwPitchBendProcessor p;
    p.setPlayConfigDetails (ch, ch, fs, block);
    auto& st = p.apvts.state;
    for (int i = 5; i < argc; ++i)
    {
        const std::string a = argv[i];
        const auto eq = a.find ('=');
        if (eq == std::string::npos || a.rfind ("tune:", 0) == 0 || a.rfind ("tuning=", 0) == 0) continue;
        const std::string k = a.substr (0, eq), v = a.substr (eq + 1);
        const juce::StringArray parts = juce::StringArray::fromTokens (juce::String (v), ":", "");
        if (k == "preset") p.setCurrentProgram (std::atoi (v.c_str()));
        else if (k == "block") block = std::max (1, std::atoi (v.c_str()));
        else if (k == "bpm") head.bpm = std::atof (v.c_str());
        else if (k == "preroll") preroll = std::atof (v.c_str());
        else if (k == "fine") st.setProperty (pb::fineId, std::atoi (v.c_str()) != 0, nullptr);
        else if (k == "slot" && parts.size() == 4)
        {
            const int t = pb::targetIndex (parts[0]);
            if (t < 0) { std::fprintf (stderr, "unknown target %s\n", parts[0].toRawUTF8()); return 2; }
            pb::setSlot (st, t, parts[1].getIntValue(), { parts[2].getIntValue(), parts[3].getFloatValue() }, nullptr);
        }
        else if (k == "table" && parts.size() >= 2)
        {
            juce::MemoryBlock mb;
            juce::File (v.substr (v.find (':') + 1)).loadFileAsData (mb);
            if (mb.getSize() != sizeof (float) * (size_t) pb::tableSize) { std::fprintf (stderr, "a table is 512 float32\n"); return 2; }
            pb::Table t;
            std::memcpy (t.data(), mb.getData(), mb.getSize());
            pb::setTable (st, parts[0].getIntValue(), t, nullptr);
        }
        else if (auto* q = p.apvts.getParameter (k)) q->setValueNotifyingHost (q->convertTo0to1 ((float) std::atof (v.c_str())));
        else { std::fprintf (stderr, "unknown key %s\n", k.c_str()); return 2; }
    }
    FILE* f = std::fopen (argv[1], "rb");
    if (! f) { std::fprintf (stderr, "cannot read %s\n", argv[1]); return 1; }
    std::vector<float> in;
    float tmp[4096];
    size_t got;
    while ((got = std::fread (tmp, sizeof (float), 4096, f)) > 0) in.insert (in.end(), tmp, tmp + got);
    std::fclose (f);
    const long long n = (long long) in.size() / ch;
    const long long pre = preroll > 0.0 ? std::max<long long> (2 * block, std::llround (preroll * fs)) : 0;
    p.setPlayConfigDetails (ch, ch, fs, block);
    p.setPlayHead (&head);
    p.prepareToPlay (fs, block);
    juce::AudioBuffer<float> b (ch, block);
    juce::MidiBuffer midi;
    std::vector<float> out ((size_t) (n * ch), 0.0f);
    for (long long s = -pre; s < n; s += block)
    {
        const int m = (int) std::min<long long> (block, n - s);
        b.setSize (ch, m, false, false, true);
        for (int c = 0; c < ch; ++c)
            for (int i = 0; i < m; ++i) b.setSample (c, i, s + i >= 0 && s + i < n ? in[(size_t) ((s + i) * ch + c)] : 0.0f);
        head.pos = s;
        p.processBlock (b, midi);
        for (int c = 0; c < ch; ++c)
            for (int i = 0; i < m; ++i)
                if (s + i >= 0) out[(size_t) ((s + i) * ch + c)] = b.getSample (c, i);
    }
    FILE* o = std::fopen (argv[2], "wb");
    if (! o) { std::fprintf (stderr, "cannot write %s\n", argv[2]); return 1; }
    std::fwrite (out.data(), sizeof (float), out.size(), o);
    std::fclose (o);
    return 0;
}
