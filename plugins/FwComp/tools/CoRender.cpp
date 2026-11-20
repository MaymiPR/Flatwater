// Offline renderer for the Flatwater Comp DSP (used by the null tests).
//   CoRender in.f32 out.f32 channels sampleRate [key=value ...]
// Files are raw interleaved 32-bit floats; the output is aligned with the input (the latency removed).
// Keys: threshold ratio attack release punch mix (0..100) output limiter (0/1) analog (0 Off, 1..4) block
//       learned=0: without the learned gain correction (as the training set is rendered)
//       sc=<file>: the side-chain signal (the key), raw interleaved floats of scch channels (scch=, default: channels)
//       trace=<file>: per processed sample (the latency first) six floats: held peak, target, R, S (dB), the gains g1, g
// An unknown key, an argument that is no key=value, a channel count other than 1 or 2 or a block under 1 ends the
// render with exit code 2 and one line on stderr; an output that could not be written in full ends it with 1.
//   tune:key=value: a constant of Source/dsp/Tuning.h for this render; tuning=<file>: key=value lines of them
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include "dsp/Engine.h"
#include "dsp/Analog.h"

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
    if (argc < 5) { std::fprintf (stderr, "usage: CoRender in.f32 out.f32 channels fs [key=value ...]\n"); return 2; }
    const int ch = std::atoi (argv[3]);
    if (ch < 1 || ch > 2) return fail ("channels must be 1 or 2", argv[3]);
    const double fs = std::atof (argv[4]);
    // tune:key=value and tuning=file first: the Tuning constants (Source/dsp/Tuning.h) the DSP copies when it is made
    for (int i = 5; i < argc; ++i)
    {
        const std::string a = argv[i];
        if (a.rfind ("tuning=", 0) == 0 && co::dsp::tuning().load (a.c_str() + 7) != 0) return 1;
        if (a.rfind ("tune:", 0) == 0)
        {
            const auto eq = a.find ('=');
            if (eq == std::string::npos || ! co::dsp::tuning().set (a.substr (5, eq - 5), std::atof (a.c_str() + eq + 1)))
            {
                std::fprintf (stderr, "unknown tuning key in %s\n", a.c_str());
                return 2;
            }
        }
    }
    co::dsp::Compressor comp;
    co::dsp::Compressor::Settings s;
    co::dsp::Analog analog;
    int block = 512, analogMode = 0;
    std::string tracePath, keyPath;
    int keyCh = 0;
    std::vector<float> trace;
    for (int i = 5; i < argc; ++i)
    {
        const std::string a = argv[i];
        const auto eq = a.find ('=');
        if (a.rfind ("tune:", 0) == 0 || a.rfind ("tuning=", 0) == 0) continue;   // (read above)
        if (eq == std::string::npos) return fail ("not a key=value argument", a.c_str());
        const std::string k = a.substr (0, eq); const float v = (float) std::atof (a.c_str() + eq + 1);
        if (k == "trace") { tracePath = a.substr (eq + 1); continue; }
        if (k == "sc") { keyPath = a.substr (eq + 1); continue; }
        if (k == "scch") { keyCh = (int) v; continue; }
        if (k == "threshold") s.thresholdDb = v; else if (k == "ratio") s.ratio = v; else if (k == "attack") s.attackMs = v;
        else if (k == "release") s.releaseMs = v; else if (k == "punch") s.punch = v; else if (k == "mix") s.mix = v / 100.0f;
        else if (k == "output") s.outputDb = v; else if (k == "limiter") s.limiter = v > 0.5f;
        else if (k == "block") { block = (int) v; if (block < 1) return fail ("block must be 1 or more", a.c_str()); }
        else if (k == "analog") analogMode = (int) v;
        else if (k == "limest") comp.t.limEstimate = (int) v;
        else if (k == "learned") comp.t.learned = v > 0.5f;
        else return fail ("unknown key", a.c_str());
    }
    FILE* f = std::fopen (argv[1], "rb");
    if (! f) { std::fprintf (stderr, "cannot read %s\n", argv[1]); return 1; }
    std::fseek (f, 0, SEEK_END); const long bytes = std::ftell (f); std::fseek (f, 0, SEEK_SET);
    const int frames = (int) (bytes / (long) (sizeof (float) * (size_t) ch));
    std::vector<float> inter ((size_t) frames * (size_t) ch);
    if (std::fread (inter.data(), sizeof (float), inter.size(), f) != inter.size()) return 1;
    std::fclose (f);
    comp.prepare (fs, ch);
    comp.setSettings (s);
    if (! tracePath.empty()) comp.trace = &trace;
    analog.prepare (fs, ch);
    analog.setMode (analogMode);
    const int lat = comp.latency();
    const int total = frames + lat;
    std::vector<std::vector<float>> buf ((size_t) ch, std::vector<float> ((size_t) total, 0.0f));
    for (int i = 0; i < frames; ++i) for (int c = 0; c < ch; ++c) buf[(size_t) c][(size_t) i] = inter[(size_t) i * (size_t) ch + (size_t) c];
    // the key: silent after its end and through the latency's tail
    std::vector<std::vector<float>> key;
    if (! keyPath.empty())
    {
        keyCh = std::max (1, std::min (2, keyCh > 0 ? keyCh : ch));
        key.assign ((size_t) keyCh, std::vector<float> ((size_t) total, 0.0f));
        FILE* kf = std::fopen (keyPath.c_str(), "rb");
        if (! kf) { std::fprintf (stderr, "cannot read %s\n", keyPath.c_str()); return 1; }
        std::vector<float> one ((size_t) keyCh);
        for (int i = 0; i < frames && std::fread (one.data(), sizeof (float), one.size(), kf) == one.size(); ++i)
            for (int c = 0; c < keyCh; ++c) key[(size_t) c][(size_t) i] = one[(size_t) c];
        std::fclose (kf);
    }
    for (int p = 0; p < total; p += block)
    {
        float* q[2] = { buf[0].data() + p, ch > 1 ? buf[1].data() + p : nullptr };
        const float* kq[2] = { key.empty() ? nullptr : key[0].data() + p, key.size() > 1 ? key[1].data() + p : nullptr };
        comp.process (q, std::min (block, total - p), key.empty() ? nullptr : kq, (int) key.size());
        analog.process (q, std::min (block, total - p));
    }
    for (int i = 0; i < frames; ++i) for (int c = 0; c < ch; ++c) inter[(size_t) i * (size_t) ch + (size_t) c] = buf[(size_t) c][(size_t) (i + lat)];
    if (! writeAll (argv[2], inter)) { std::fprintf (stderr, "cannot write %s\n", argv[2]); return 1; }
    if (! tracePath.empty() && ! writeAll (tracePath.c_str(), trace)) { std::fprintf (stderr, "cannot write %s\n", tracePath.c_str()); return 1; }
    return 0;
}
