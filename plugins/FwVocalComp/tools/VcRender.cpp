// Offline renderer for the Flatwater Vocal Comp DSP (used by the null test and the tuning scripts).
//   VcRender in.f32 out.f32 channels sampleRate comp gate gain [key=value ...]
// Files are raw interleaved 32-bit floats. The output is shifted back by the latency, so it lines up with the input
// (the reference host does the same).  Keys: hold box1 box2 ta1 tr1 ta2 tr2 gateRelMs band slowWinMs slowMode decayDbS
// fixedGain learned, compensate (0: the latency stays in the output), dump (1: also <out>.trace).
// An unknown key, an argument that is no key=value or a channel count other than 1 or 2 ends the render with exit
// code 2 and one line on stderr; an output that could not be written in full ends it with 1.
//   tune:key=value: a constant of Source/dsp/Tuning.h for this render; tuning=<file>: key=value lines of them
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include "dsp/Engine.h"

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
    if (argc < 8) { std::fprintf (stderr, "usage: VcRender in.f32 out.f32 channels fs comp gate gain [key=value ...]\n"); return 2; }
    const int ch = std::atoi (argv[3]);
    if (ch < 1 || ch > 2) return fail ("channels must be 1 or 2", argv[3]);
    const double fs = std::atof (argv[4]);
    vc::dsp::Processor::Params p;
    p.compDb = (float) std::atof (argv[5]);
    p.gateDb = (float) std::atof (argv[6]);
    p.gainDb = (float) std::atof (argv[7]);
    // tune:key=value and tuning=file first: the Tuning constants (Source/dsp/Tuning.h) the DSP copies when it is made
    for (int i = 8; i < argc; ++i)
    {
        const std::string a = argv[i];
        if (a.rfind ("tuning=", 0) == 0 && vc::dsp::tuning().load (a.c_str() + 7) != 0) return 1;
        if (a.rfind ("tune:", 0) == 0)
        {
            const auto eq = a.find ('=');
            if (eq == std::string::npos || ! vc::dsp::tuning().set (a.substr (5, eq - 5), std::atof (a.c_str() + eq + 1)))
            {
                std::fprintf (stderr, "unknown tuning key in %s\n", a.c_str());
                return 2;
            }
        }
    }
    vc::dsp::Tuning t = vc::dsp::tuning();
    bool compensate = true, dump = false;
    for (int i = 8; i < argc; ++i)
    {
        const std::string a = argv[i];
        const auto eq = a.find ('=');
        if (a.rfind ("tune:", 0) == 0 || a.rfind ("tuning=", 0) == 0) continue;   // (read above)
        if (eq == std::string::npos) return fail ("not a key=value argument", a.c_str());
        const std::string k = a.substr (0, eq); const float v = (float) std::atof (a.c_str() + eq + 1);
        if (k == "hold") t.hold = (int) v; else if (k == "box1") t.box1 = (int) v; else if (k == "box2") t.box2 = (int) v;
        else if (k == "ta1") t.ta1 = v; else if (k == "tr1") t.tr1 = v; else if (k == "ta2") t.ta2 = v; else if (k == "tr2") t.tr2 = v;
        else if (k == "gateRelMs") t.gateRelMs = v; else if (k == "band") t.band = v; else if (k == "slowWinMs") t.slowWinMs = v; else if (k == "slowMode") t.slowMode = (int) v; else if (k == "decayDbS") t.decayDbS = v;
        else if (k == "fixedGain") t.fixedGain = v;
        else if (k == "compensate") compensate = v != 0.0f;
        else if (k == "dump") dump = v != 0.0f;
        else if (k == "learned") t.learned = v != 0.0f;
        else return fail ("unknown key", a.c_str());
    }
    FILE* f = std::fopen (argv[1], "rb");
    if (! f) { std::fprintf (stderr, "cannot read %s\n", argv[1]); return 1; }
    std::fseek (f, 0, SEEK_END); const long bytes = std::ftell (f); std::fseek (f, 0, SEEK_SET);
    const int frames = (int) (bytes / (long) (sizeof (float) * (size_t) ch));
    std::vector<float> inter ((size_t) frames * (size_t) ch);
    if (std::fread (inter.data(), sizeof (float), inter.size(), f) != inter.size()) return 1;
    std::fclose (f);

    vc::dsp::Processor proc;
    proc.prepare (fs, ch, t);
    proc.setParams (p);
    std::vector<float> trace;
    if (dump) { trace.reserve ((size_t) frames * 4 + 4096); proc.trace = &trace; }
    const int lat = proc.getLatency();
    const int total = frames + lat;                       // feed zeros to flush the look-ahead
    std::vector<std::vector<float>> in ((size_t) ch, std::vector<float> ((size_t) total, 0.0f)), out = in;
    for (int i = 0; i < frames; ++i) for (int c = 0; c < ch; ++c) in[(size_t) c][(size_t) i] = inter[(size_t) i * (size_t) ch + (size_t) c];
    const float* ip[2] = { in[0].data(), ch > 1 ? in[1].data() : nullptr };
    float* op[2] = { out[0].data(), ch > 1 ? out[1].data() : nullptr };
    const int block = 512;
    for (int s = 0; s < total; s += block)
    {
        const int nn = std::min (block, total - s);
        const float* ib[2] = { ip[0] + s, ch > 1 ? ip[1] + s : nullptr };
        float* ob[2] = { op[0] + s, ch > 1 ? op[1] + s : nullptr };
        proc.process (ib, ob, nn);
    }
    const int shift = compensate ? lat : 0;
    for (int i = 0; i < frames; ++i) for (int c = 0; c < ch; ++c) inter[(size_t) i * (size_t) ch + (size_t) c] = out[(size_t) c][(size_t) (i + shift)];
    if (! writeAll (argv[2], inter)) { std::fprintf (stderr, "cannot write %s\n", argv[2]); return 1; }
    // <out>.trace: per processed sample v, fast ratio, slow ratio, gate gain (index i + latency acts on input i)
    if (const std::string tracePath = std::string (argv[2]) + ".trace"; dump && ! writeAll (tracePath.c_str(), trace))
    {
        std::fprintf (stderr, "cannot write %s\n", tracePath.c_str());
        return 1;
    }
    std::printf ("latency %d\n", lat);
    return 0;
}
