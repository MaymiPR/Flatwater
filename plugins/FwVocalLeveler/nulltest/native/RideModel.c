/* Flatwater Vocal Leveler's riding model (Phase 4), driven by model_lib.py; plugins/FwVocalLeveler/Source/dsp/Engine.h is the same model.
   Per input sample (mono, or the stereo mid) it returns the gain in dB the reference applies (no latency).
   Stages (MEASUREMENTS.md "Flatwater Vocal Leveler: Phase 3 / Phase 4"):
     weighting  two shelves, each two identical first-order sections with half the gain (low 100 Hz -18 dB, high
                6.5 kHz -6 dB)
     follower   Q, the weighted power plus the floor (bias): up at most by the attack's rate, down at most by the
                release's (the hold's on samples the detector does not pass), both multiplicative per sample (a slew
                in dB; the coefficients raised to fpow); it starts at the floor
     detector   per sample, a short-term power (VADAtt / VADRel, one-pole) over -(Vocal Sensitivity) - vk dB times Q,
                plus the Spill level (Live; off at its minimum)
     ride       v (the gain less half the Min gain) toward c Target / sqrt (Q), only on the samples the detector
                passes: down by the ride attack, up by the ride release (multiplicative), clamped so that
                1/2 gMin + v stays in the range; v starts at 1
     park       once the detector has passed nothing for the hold (it runs from the start too), v toward the Idle
                value by the park rate (both ways, multiplicative)
     music      ride_music: the side-chain's mid through the same weighting into a follower of its own, M (the
                weighted power plus its floor; up by its attack's rate, down by its release's on samples within vk dB
                of M and by its hold's on the others); the ride aims at sqrt (Target^2 + k M), k = Music Sensitivity
                plus the offset, in dB */
#include <math.h>

enum { P_C, P_BIAS, P_FATT_S, P_FATT_F, P_FREL, P_FHOLD, P_FPOW, P_VATT, P_VREL, P_VK, P_RATT_S, P_RATT_F, P_RREL,
       P_RPOW, P_PARK, P_PHOLD_S, P_PHOLD_F, P_SPILL, P_V0, P_N };

typedef struct { double b0, b1, a1, z; } Sec;

static void shelf (Sec* s, double fs, double fc, double gdb, int low)
{
    const double g = sqrt (pow (10.0, gdb / 20.0)), K = tan (M_PI * fc / fs);
    double b0, b1, a0, a1;
    if (low) { b0 = 1.0 / K + g; b1 = g - 1.0 / K; a0 = 1.0 / K + 1.0 / g; a1 = 1.0 / g - 1.0 / K; }
    else     { b0 = g / K + 1.0; b1 = 1.0 - g / K; a0 = 1.0 / (g * K) + 1.0; a1 = 1.0 - 1.0 / (g * K); }
    s->b0 = b0 / a0; s->b1 = b1 / a0; s->a1 = a1 / a0; s->z = 0.0;
}

static double run (Sec* s, double x)
{
    const double y = s->b0 * x + s->z;
    s->z = s->b1 * x - s->a1 * y;
    return y;
}

/* settings: target, rmin, rmax, idle (dB), vocal sensitivity, spill, fast (0/1), live (0/1)
   m: the side-chain's mid, or 0 for none; mp: Music Sensitivity (dB), then the music follower's bias (dB), attack,
   release, hold (ms), vk (dB) and the offset (dB) */
static void run_ride (const float* x, const float* m, int n, double fs, const double* st, const double* p, const double* mp, float* out)
{
    const double target = st[0], rmin = st[1], rmax = st[2], idle = st[3], vs = st[4], spill = st[5];
    const int fast = st[6] > 0.5, live = st[7] > 0.5;
    Sec w[4];
    shelf (&w[0], fs, 100.0, -18.0 / 2.0, 1); shelf (&w[1], fs, 100.0, -18.0 / 2.0, 1);
    shelf (&w[2], fs, 6500.0, -6.0 / 2.0, 0); shelf (&w[3], fs, 6500.0, -6.0 / 2.0, 0);
    const double gmin = pow (10.0, rmin / 20.0), gmax = pow (10.0, rmax / 20.0);
    const double vlo = 0.5 * gmin, vhi = gmax - 0.5 * gmin;
    double vidle = pow (10.0, idle / 20.0) - 0.5 * gmin;
    if (vidle < vlo) vidle = vlo; if (vidle > vhi) vidle = vhi;
    const double T = pow (10.0, target / 20.0), c = pow (10.0, p[P_C] / 20.0), b2 = pow (10.0, p[P_BIAS] / 10.0);
    #define COEF(ms) exp (-1.0 / (fs * (ms) * 0.001))
    const double fp = p[P_FPOW];
    const double fup = pow (COEF (fast ? p[P_FATT_F] : p[P_FATT_S]), -fp), fdn = pow (COEF (p[P_FREL]), fp),
                 fhold = pow (COEF (p[P_FHOLD]), fp);
    const double va = COEF (p[P_VATT]), vr = COEF (p[P_VREL]);
    const double thr = pow (10.0, (-vs - p[P_VK]) / 10.0);
    const double sp2 = (live && spill > -47.95) ? pow (10.0, (spill + p[P_SPILL]) / 10.0) : 0.0;
    const double rp = p[P_RPOW];
    const double rdn = pow (COEF (fast ? p[P_RATT_F] : p[P_RATT_S]), rp), rup = pow (COEF (p[P_RREL]), rp);
    const double kpark = COEF (p[P_PARK]);
    const long phold = (long) floor ((fast ? p[P_PHOLD_F] : p[P_PHOLD_S]) * 0.001 * fs + 0.5);
    Sec wm[4];
    for (int k = 0; k < 4; ++k) wm[k] = w[k];
    const double mb2 = m ? pow (10.0, mp[1] / 10.0) : 0.0;
    const double mup = m ? pow (COEF (mp[2]), -fp) : 1.0, mdn = m ? pow (COEF (mp[3]), fp) : 1.0, mhold = m ? pow (COEF (mp[4]), fp) : 1.0;
    const double mthr = m ? pow (10.0, -mp[5] / 10.0) : 0.0, mgain = m ? c * c * pow (10.0, (mp[0] + mp[6]) / 10.0) : 0.0;
    double M = mb2;
    double Q = b2, s = 0.0, v = p[P_V0];
    if (v < vlo) v = vlo; if (v > vhi) v = vhi;
    long quiet = 0;
    int voice = 0;
    for (int i = 0; i < n; ++i)
    {
        double aim = c * T;
        if (m)
        {
            double ym = m[i];
            for (int k = 0; k < 4; ++k) ym = run (&wm[k], ym);
            const double pm = ym * ym, qm = pm + mb2;
            if (qm > M) { const double t = M * mup; M = qm < t ? qm : t; }
            else { const double t = M * (pm > mthr * M ? mdn : mhold); M = qm > t ? qm : t; }
            aim = sqrt (c * T * c * T + mgain * M);
        }
        double y = x[i];
        for (int k = 0; k < 4; ++k) y = run (&w[k], y);
        const double pw = y * y, q = pw + b2;
        /* the follower */
        if (q > Q) { const double t = Q * fup; Q = q < t ? q : t; }
        else { const double t = Q * (voice ? fdn : fhold); Q = q > t ? q : t; }
        /* the detector */
        s = pw > s ? pw + va * (s - pw) : pw + vr * (s - pw);
        voice = s > thr * Q + sp2;
        quiet = voice ? 0 : quiet + 1;
        if (voice)
        {
            double vd = aim / sqrt (Q);
            if (vd < vlo) vd = vlo; if (vd > vhi) vd = vhi;
            if (vd < v) { const double t = v * rdn; v = vd > t ? vd : t; }
            else { const double t = v / rup; v = vd < t ? vd : t; }
        }
        else if (quiet > phold)
        {
            if (v > vidle) { const double t = v * kpark; v = vidle > t ? vidle : t; }
            else { const double t = v / kpark; v = vidle < t ? vidle : t; }
        }
        out[i] = (float) (20.0 * log10 (0.5 * gmin + v));
    }
}

__declspec(dllexport) void ride (const float* x, int n, double fs, const double* st, const double* p, float* out)
{
    run_ride (x, 0, n, fs, st, p, 0, out);
}

__declspec(dllexport) void ride_music (const float* x, const float* m, int n, double fs, const double* st, const double* p, const double* mp, float* out)
{
    run_ride (x, m, n, fs, st, p, mp, out);
}
