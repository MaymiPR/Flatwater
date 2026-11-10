#pragma once
// Every constant of the DSP that a fit or a render may override (tools/ renderers set the fields by name).

namespace vc
{
struct Tuning
{
    // fast path, in samples at 44.1 / 48 kHz (scaled by latency / 64 above)
    int hold = 65, box1 = 39, box2 = 26;
    // slow path (ms, dB): stage 1 rise / fall, dead band, stage 2 rise / fall
    int slowMode = 2;               // 0: two dB stages with dead band; 1: amplitude charge stage + dB/s peak decay;
                                    // 2: amplitude stage (ta1 / tr1), then the gain ratio through a one-pole (ta2 / tr2)
    float decayDbS = 20.0f;         // slowMode 1: fall rate of the held level (dB per second)
    float slowWinMs = 2.1754f;       // peak window in front of the slow path
    float ta1 = 1.5704f, tr1 = 0.67043f, band = 0.048f, ta2 = 249.5f, tr2 = 498.49f;
    // gate level release (ms): the reference's hidden GateRel parameter, 10 ... 250 ms, default 130
    float gateRelMs = 130.0f;
    float fixedGain = 0.99f;        // the reference's constant -0.0873 dB
    bool learned = true;            // the learned correction of the slow path (SlowNet); off = the analytical path only
};
}
