// Offline renderer of Flatwater Pitch Bend's shifter alone (the scorecard of MEASUREMENTS.md runs on it while the
// processor is built around it; no JUCE needed):
//   PbShifterRender in.f32 out.f32 channels sampleRate [pitch=<st>] [formant=<st>] [flatten=<Hz>] [block=<n>]
//                            [track=1] [repeat=<k>] [lfo=<st>] [lforate=<Hz>] [pitchctl=<st>] [formantctl=<st>]
// track=1 also writes out.f32.trk: per block, three float32 (detected Hz, aperiodicity, last grain kind).
// repeat=<k> renders k times (a timing loop) and prints the seconds per render.
// lfo=<st>: Pitch moves by a sine of that depth (semitones) at lforate Hz (default 1), set once per block.
// step=<st>: Pitch moves by that much and back every steplen samples (default 12000), set once per block.
// pitchctl= / formantctl=: the controls' own values before modulation (default: the same as the totals).
// Files are raw interleaved 32-bit floats; the output is the same length as the input (the shifter reports no latency).
// An unknown key, an argument that is no key=value, a channel count other than 1 or 2 or a block under 1 ends the
// render with exit code 2 and one line on stderr; an output that could not be written in full ends it with 1.
//   tune:key=value: a constant of Source/dsp/Tuning.h for this render; tuning=<file>: key=value lines of them
#include <cstdio>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <string>
#include <vector>
#include "dsp/Shifter.h"

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
    if (argc < 5) { std::fprintf (stderr, "usage: PbShifterRender in.f32 out.f32 channels fs [pitch=] [formant=] [flatten=] [block=]\n"); return 2; }
    const int ch = std::atoi (argv[3]);
    if (ch < 1 || ch > 2) return fail ("channels must be 1 or 2", argv[3]);
    const double fs = std::atof (argv[4]);
    float pitch = 0.0f, formant = 0.0f, flatten = 0.0f, lfo = 0.0f, lfoRate = 1.0f, step = 0.0f;
    long long stepLen = 12000;
    float pitchCtl = std::numeric_limits<float>::quiet_NaN(), formantCtl = std::numeric_limits<float>::quiet_NaN();
    int block = 512, track = 0, repeat = 1;
    for (int i = 5; i < argc; ++i)
    {
        const std::string a = argv[i];
        const auto eq = a.find ('=');
        if (a.rfind ("tune:", 0) == 0 || a.rfind ("tuning=", 0) == 0) continue;   // (read below)
        if (eq == std::string::npos) return fail ("not a key=value argument", a.c_str());
        const std::string k = a.substr (0, eq);
        const double v = std::atof (a.c_str() + eq + 1);
        if (k == "pitch") pitch = (float) v;
        else if (k == "formant") formant = (float) v;
        else if (k == "flatten") flatten = (float) v;
        else if (k == "block") { block = (int) v; if (block < 1) return fail ("block must be 1 or more", a.c_str()); }
        else if (k == "track") track = (int) v;
        else if (k == "lfo") lfo = (float) v;
        else if (k == "lforate") lfoRate = (float) v;
        else if (k == "step") step = (float) v;
        else if (k == "steplen") stepLen = std::max (1LL, (long long) v);
        else if (k == "repeat") repeat = std::max (1, (int) v);
        else if (k == "pitchctl") pitchCtl = (float) v;
        else if (k == "formantctl") formantCtl = (float) v;
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
    // tune:key=value and tuning=file first: the Tuning constants (Source/dsp/Tuning.h) the DSP copies when it is made
    for (int i = 5; i < argc; ++i)
    {
        const std::string a = argv[i];
        if (a.rfind ("tuning=", 0) == 0 && pb::dsp::tuning().load (a.c_str() + 7) != 0) return 1;
        if (a.rfind ("tune:", 0) == 0)
        {
            const auto eq = a.find ('=');
            if (eq == std::string::npos || ! pb::dsp::tuning().set (a.substr (5, eq - 5), std::atof (a.c_str() + eq + 1)))
            {
                std::fprintf (stderr, "unknown tuning key in %s\n", a.c_str());
                return 2;
            }
        }
    }
    pb::dsp::Shifter s;
    s.prepare (fs, block, ch);
    std::vector<std::vector<float>> buf ((size_t) ch, std::vector<float> ((size_t) block));
    std::vector<float*> ptr ((size_t) ch);
    std::vector<float> out ((size_t) (n * ch)), trk;
    const auto t0 = std::chrono::steady_clock::now();
    for (int rep = 0; rep < repeat; ++rep)
    {
        s.reset();
        trk.clear();
        for (long long p = 0; p < n; p += block)
        {
            const int m = (int) std::min<long long> (block, n - p);
            for (int c = 0; c < ch; ++c)
            {
                for (int i = 0; i < m; ++i) buf[(size_t) c][(size_t) i] = in[(size_t) ((p + i) * ch + c)];
                ptr[(size_t) c] = buf[(size_t) c].data();
            }
            const float pb = pitch + lfo * (float) std::sin (2.0 * 3.14159265358979 * lfoRate * (double) p / fs) + ((p / stepLen) % 2 == 1 ? step : 0.0f);
            s.process (ptr.data(), ch, m, pb, formant, flatten, pitchCtl, formantCtl);
            for (int c = 0; c < ch; ++c)
                for (int i = 0; i < m; ++i) out[(size_t) ((p + i) * ch + c)] = buf[(size_t) c][(size_t) i];
            if (track) { trk.push_back (s.detectedHz()); trk.push_back (s.aperiodicity()); trk.push_back ((float) s.lastGrainKind()); }
        }
    }
    if (repeat > 1)
        std::printf ("%.6f s per render\n", std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count() / repeat);
    if (const std::string trkPath = std::string (argv[2]) + ".trk"; track && ! writeAll (trkPath.c_str(), trk))
    {
        std::fprintf (stderr, "cannot write %s\n", trkPath.c_str());
        return 1;
    }
    if (! writeAll (argv[2], out)) { std::fprintf (stderr, "cannot write %s\n", argv[2]); return 1; }
    return 0;
}
