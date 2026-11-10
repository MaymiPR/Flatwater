"""Flatwater Comp: the release law the plug-in uses (the two-state form, native/ReleaseLaw.c), fitted with CMA-ES, a
derivative-free search that copes with the rough error surface where least squares and gradient steps stall. The data:
the settled probes (a 1 s quiet lead-in: the reference releases slowly for the first 0.1 to 0.3 s after a reset, so
short lead-ins measure its start-up), the double-burst memory probes, the realistic envelopes of fit_release_real.py
and the null-test and preset material weighted x3, all on the alternating probe e[n] (-1)^n (the detector reads |x|,
the gain is y / x exactly). Writes measurements/FwComp/release_law.json and release_w0.json.

    python -I fit_release_law.py collectsettled      the settled probes (build/hc_rel4_cases.npz)
    python -I fit_release_law.py collectmem          the memory probes (build/hc_memory_cases.npz)
    python -I fit_release_law.py collect             the null-test material (build/hc_nullset_cases.npz)
    python -I fit_release_law.py collectpresets      the factory presets' material (build/hc_preset_cases.npz)
    python -I fit_release_law.py first [stage ...]   the law's first fit by least squares (stages slow fast joint report)
    python -I fit_release_law.py refit [nfev]        a least-squares refit on every probe set
    python -I fit_release_law.py report              the error per probe set
    python -I fit_release_law.py [generations] [sigma]   the CMA-ES fit (the previous tables kept as build/*.before_cma.json)

It replaces fit_release2.py (the first fit), fit_release4.py (the settled probes) and fit_release8.py (the probe sets
and the refit), whose other release forms the plug-in does not use (MEASUREMENTS.md, Flatwater Comp, declined changes).
"""
import sys, os, json, time, shutil
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'scripts', 'nulltest'))   # scripts/nulltest/
import numpy as np
from concurrent.futures import ThreadPoolExecutor
from scipy import optimize
from scipy.ndimage import maximum_filter1d
from model_lib import static_gain_db
from fit_release import load as load_bursts
import release_law_lib as LAW
import fit_release_time_factor as W
import fit_release_real as IR

FS = 48000
HERE = os.path.dirname(os.path.abspath(__file__))
BUILD = os.path.join(HERE, '..', '..', '..', 'build')
CACHE = os.path.join(BUILD, 'hc_rel4_cases.npz')


# ---- the settled probes -------------------------------------------------------------------------------------------
LEAD, QUIET = FS, 0.0003
SKIP = LEAD - 2400                      # the simulation starts 50 ms before the event (the state settled at rest)

DEPTHS = [(-30.0, 4.0, 0.054), (-30.0, 4.0, 0.08), (-30.0, 4.0, 0.2), (-30.0, 50.0, 0.5), (-30.0, 50.0, 1.0), (-40.0, 2.0, 1.0)]
DURS = [0.002, 0.005, 0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1.0]
RELEASES = [30.0, 100.0, 300.0, 1000.0]


def alt(e):
    return (e * (1 - 2 * (np.arange(len(e)) % 2))).astype(np.float32)


def cases_spec():
    rng = np.random.default_rng(7)
    spec = []
    for thr, rat, L in DEPTHS:
        for D in DURS:
            for rel in RELEASES:
                e = np.concatenate([np.full(int(D * FS), L), np.full(int((0.1 + 1.5 * rel / 1000) * FS), QUIET)])
                spec.append(('burst L %.3f %g ms' % (L, D * 1000), e, thr, rat, 0.5, rel))
    # the attack: the reduction not yet reached when the event ends
    for att in (7.0, 30.0):
        for D in (0.004, 0.02, 0.1, 0.5):
            for rel in (100.0, 300.0):
                e = np.concatenate([np.full(int(D * FS), 0.08), np.full(int(0.4 * FS), QUIET)])
                spec.append(('burst att %g %g ms' % (att, D * 1000), e, -30.0, 4.0, att, rel))
    # pulse trains (crest factors): width w every T, for 0.6 s
    for w, T in ((0.001, 0.007), (0.002, 0.007), (0.004, 0.01), (0.002, 0.02), (0.005, 0.05), (0.01, 0.03), (0.02, 0.04)):
        for L, thr, rat in ((0.2, -30.0, 4.0), (1.0, -30.0, 50.0)):
            for rel in (100.0, 300.0):
                per = np.full(int(T * FS), QUIET); per[:int(w * FS)] = L
                e = np.concatenate([np.tile(per, int(0.6 / T)), np.full(int(0.3 * FS), QUIET)])
                spec.append(('pulses %g/%g ms L %.1f' % (w * 1000, T * 1000, L), e, thr, rat, 0.5, rel))
    # steps down to a level still above the threshold, after short and long holds
    for D in (0.02, 0.2, 1.0):
        for L2 in (0.1, 0.05):
            for rel in (100.0, 300.0):
                e = np.concatenate([np.full(int(D * FS), 1.0), np.full(int(0.5 * FS), L2), np.full(int(0.2 * FS), QUIET)])
                spec.append(('step 1.0 %g ms -> %.2f' % (D * 1000, L2), e, -40.0, 4.0, 0.5, rel))
    # random envelopes with a settled start: syllables and hits (smooth and sharp), random levels
    for k in range(40):
        n = int(1.5 * FS); t = np.arange(n) / FS
        if k % 2 == 0:
            f = rng.uniform(2, 6); e = 10 ** (rng.uniform(-24, 0) / 20) * np.clip(np.sin(2 * np.pi * f * t + rng.uniform(0, 6)), 0, None) ** 2
            pitch = rng.uniform(80, 250); e = e * np.abs(sum(np.sin(2 * np.pi * pitch * j * t) / j for j in range(1, 10))) / 2.0
        else:
            e = np.zeros(n)
            for h in rng.uniform(0, 1.4, int(rng.integers(3, 12))):
                s0 = int(h * FS); m = min(n - s0, int(0.3 * FS)); tt = np.arange(m) / FS
                e[s0:s0 + m] += 10 ** (rng.uniform(-20, 0) / 20) * np.exp(-tt / rng.uniform(0.01, 0.15)) * np.abs(rng.standard_normal(m) * 0.5 + np.sin(2 * np.pi * 60 * tt))
        e = np.maximum(e, QUIET)
        spec.append(('random %d' % k, e, float(rng.choice([-12.0, -18.0, -24.0, -30.0, -36.0])), float(rng.choice([2.0, 3.0, 4.0, 10.0, 50.0])),
                     float(rng.choice([0.5, 3.0, 7.0, 20.0])), float(rng.choice([30.0, 100.0, 300.0, 1000.0]))))
    return spec


def collect_settled():
    import co_ref as CO
    out = []
    for i, (name, body, thr, rat, att, rel) in enumerate(cases_spec()):
        e = np.concatenate([np.full(LEAD, QUIET), body])
        x = alt(e)
        y = CO.render_ref(x, FS, threshold=thr, ratio=rat, attack=att, release=rel, punch=0.0, output=-18.0)[0]
        g = (y / x).astype(np.float64)
        g /= np.abs(g[LEAD - 4000:LEAD - 3000]).mean()          # the uncompressed gain (makeup and output)
        out.append(dict(a=e[SKIP:], g=np.abs(g[SKIP:]), thr=thr, rat=rat, att=att, rel=rel, name=name))
        if i % 25 == 0: print(i, name, flush=True)
    np.savez(CACHE, cases=np.array(out, dtype=object))
    print('saved', len(out))


def target(a, thr, rat):
    H, lead = 64, 1
    pad = np.concatenate([a, np.full(H + lead, a[-1])])
    h = maximum_filter1d(pad, size=H, origin=-(H // 2))[lead:lead + len(a)]
    return np.ascontiguousarray(-static_gain_db(h, thr, rat), np.float64)


def load_settled():
    """the settled probes with their target reductions"""
    cs = list(np.load(CACHE, allow_pickle=True)['cases'])
    for c in cs: c['Rt'] = target(c['a'], c['thr'], c['rat']); c['startup'] = False
    return cs


# ---- the further probe sets and the case list ----------------------------------------------------------------------
NULLSET = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'build', 'hc_nullset_cases.npz')
WEIGHT = 3          # the null-test material counts this many times


def collect_nullset():
    """the null-test signals' envelopes on the alternating probe, at the null test's first six settings"""
    import co_ref as N, co_ref as CO
    S = N.signals(48000)
    out = []
    for st in N.SETTINGS[:6]:
        thr, rat, att, rel, pu = st[:5]
        for name in ('voice-like', 'drum loop', 'bass line', 'white noise -20 rms', 'pink noise -20 rms', 'noise bursts', 'AM 5 Hz 3 kHz', 'sine 100 Hz -20', 'log sweep -20', 'tone steps -50/-10/-30'):
            e = np.abs(S[name].astype(np.float64))
            x = (e * (1 - 2 * (np.arange(len(e)) % 2))).astype(np.float32)
            y = CO.render_ref(x, 48000, threshold=thr, ratio=rat, attack=att, release=rel, punch=0.0, output=-18.0)[0]
            out.append(dict(name=name, thr=thr, rat=rat, att=att, rel=rel, a=e, g=y / np.where(e > 1e-6, x, 1)))
        print('setting', st[:4], 'done', flush=True)
    np.savez(NULLSET, cases=np.array(out, dtype=object))


PRESETSET = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'build', 'hc_preset_cases.npz')


def collect_presets():
    """program signals' envelopes on the alternating probe at the settings of Flatwater Comp's factory presets (the reference's
    own presets): threshold, ratio, attack and release (tempo-synced presets at their release time at 120 BPM); Punch
    0, Mix 100, Output -18, Limiter Off so the gain stays observable"""
    import co_ref as N, co_ref as CO, null_presets as P
    S = N.signals(48000)
    out = []
    for group, name, v in P.presets():
        sync = int(v['sync']); rel = v['release'] if sync == 2 else P.synced_ms(int(v['releaseNote']), v['bpm'] if sync == 0 else 120.0)
        for sig in P.SIGNALS:
            e = np.abs(S[sig].astype(np.float64))
            x = (e * (1 - 2 * (np.arange(len(e)) % 2))).astype(np.float32)
            y = CO.render_ref(x, 48000, threshold=v['threshold'], ratio=v['ratio'], attack=v['attack'], release=rel, punch=0.0, output=-18.0)[0]
            out.append(dict(name=sig, preset=name, thr=v['threshold'], rat=v['ratio'], att=v['attack'], rel=rel, a=e, g=y / np.where(e > 1e-6, x, 1)))
        print('preset', name, 'done', flush=True)
    np.savez(PRESETSET, cases=np.array(out, dtype=object))


MEMSET = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'build', 'hc_memory_cases.npz')


def collect_memory():
    """double bursts from a settled state: a charge (50 or 500 ms), a quiet gap, a 5 ms burst: how long the reference
    remembers the first burst (its release of the second)"""
    import co_ref as CO
    out = []
    for thr, rat, L in ((-30.0, 4.0, 0.08), (-30.0, 4.0, 0.2), (-30.0, 50.0, 1.0)):
        for rel in (30.0, 100.0, 300.0):
            for first in (0.05, 0.5):
                for G in (0.02, 0.05, 0.1, 0.2, 0.4):
                    e = np.concatenate([np.full(LEAD, QUIET), np.full(int(first * 48000), L), np.full(int(G * 48000), QUIET),
                                        np.full(int(0.005 * 48000), L), np.full(int(0.15 * 48000), QUIET)])
                    x = alt(e)
                    y = CO.render_ref(x, 48000, threshold=thr, ratio=rat, attack=0.5, release=rel, punch=0.0, output=-18.0)[0]
                    g = (y / x).astype(np.float64); g /= np.abs(g[LEAD - 4000:LEAD - 3000]).mean()
                    out.append(dict(a=e[SKIP:], g=np.abs(g[SKIP:]), thr=thr, rat=rat, att=0.5, rel=rel, name='memory L %.2f %g ms gap %g ms' % (L, first * 1000, G * 1000)))
        print('depth', thr, rat, L, 'done', flush=True)
    np.savez(MEMSET, cases=np.array(out, dtype=object))


def cases():
    cs = load_settled()
    if os.path.exists(MEMSET):
        mem = list(np.load(MEMSET, allow_pickle=True)['cases'])
        for c in mem: c['Rt'] = target(c['a'], c['thr'], c['rat']); c['startup'] = False
        cs += mem
    for c in cs: c['ref'] = c['g']; c['from'] = 0
    for c in IR.prepare(list(np.load(IR.CACHE, allow_pickle=True)['cases'])):
        ok = ~np.isnan(c['g'])
        ref = np.abs(c['g'] * c['norm']); ref[~ok] = np.nan
        c['ref'] = ref; c['from'] = int(0.4 * 48000)                  # past the reference's start-up
        c['name'] = 'real ' + c['name']
        cs.append(c)
    if os.path.exists(NULLSET):
        for c in IR.prepare(list(np.load(NULLSET, allow_pickle=True)['cases'])):
            ok = (c['a'] > 1e-4)
            ref = np.abs(c['g'] * c['norm']); ref[~ok] = np.nan
            c['ref'] = ref; c['from'] = int(0.4 * 48000); c['name'] = 'null ' + c['name']
            for _ in range(WEIGHT): cs.append(c)
    if os.path.exists(PRESETSET):
        for c in IR.prepare(list(np.load(PRESETSET, allow_pickle=True)['cases'])):
            ok = (c['a'] > 1e-4)
            ref = np.abs(c['g'] * c['norm']); ref[~ok] = np.nan
            c['ref'] = ref; c['from'] = int(0.4 * 48000); c['name'] = 'preset ' + c['name']
            for _ in range(WEIGHT): cs.append(c)
    return cs


def law(q, x0, nF, nA):
    x = x0.copy(); x[:nF + nA] = q[:nF + nA]
    return W.apply(q[nF + nA:], x)


def residuals(q, cs, x0, nF, nA, dec=16):
    X = law(q, x0, nF, nA)
    r = []
    for c in cs:
        m = IR.run(c, X, c['Rt'])
        s = slice(c['from'], None, dec)
        ref = c['ref'][s]; ok = ~np.isnan(ref)
        r.append(20 * np.log10(np.maximum(m[s][ok], 1e-9)) - 20 * np.log10(np.maximum(ref[ok], 1e-9)))
    return np.concatenate(r)


def setup():
    LAW.dll.hc_set_charge_from_r(1)
    x0 = np.array(json.load(open(LAW.OUT))['x'])
    z0 = np.array(json.load(open(W.OUT))['z'])
    nF, nA = len(LAW.KN['Fs']), len(LAW.KN['As'])
    return x0, z0, nF, nA


def refit(nfev):
    x0, z0, nF, nA = setup()
    cs = cases()
    q0 = np.concatenate([x0[:nF + nA], z0])
    r = residuals(q0, cs, x0, nF, nA); print('start rms %.3f dB worst %.2f (%d cases)' % (np.sqrt(np.mean(r ** 2)), np.abs(r).max(), len(cs)), flush=True)
    s = optimize.least_squares(residuals, q0, args=(cs, x0, nF, nA), diff_step=1e-3, max_nfev=nfev)
    r = residuals(s.x, cs, x0, nF, nA); print('fit rms %.3f dB worst %.2f' % (np.sqrt(np.mean(r ** 2)), np.abs(r).max()), flush=True)
    x = x0.copy(); x[:nF + nA] = s.x[:nF + nA]
    lw = json.load(open(LAW.OUT)); lw['x'] = list(map(float, x)); json.dump(lw, open(LAW.OUT, 'w'), indent=1)
    json.dump({'S': W.SX.tolist(), 'z': list(map(float, s.x[nF + nA:]))}, open(W.OUT, 'w'))
    report(cs)


def report(cs=None):
    x0, z0, nF, nA = setup()
    cs = cases() if cs is None else cs
    q = np.concatenate([x0[:nF + nA], z0])
    groups = {}
    for c in cs:
        r = residuals(q, [c], x0, nF, nA, dec=4)
        groups.setdefault(c['name'].split()[0], []).append(np.sqrt(np.mean(r ** 2)))
    for k, v in groups.items(): print('  %-8s median rms %.3f dB, mean %.3f (%d)' % (k, np.median(v), np.mean(v), len(v)))


# ---- the first fit -----------------------------------------------------------------------------------------------
def first(stages):
    """the law's first fit, stage by stage, on the burst cases of fit_release.py (least squares)"""
    cs = load_bursts()
    x = LAW.pack(LAW.initial())
    if os.path.exists(LAW.OUT):
        x = np.array(json.load(open(LAW.OUT))['x'])
    sus = [c for c in cs if c['dur'] == 2.0]
    short = [c for c in cs if c['dur'] < 2.0 and c['rel'] in (30.0, 100.0, 1000.0) and c['att'] == 0.5]
    for st in stages:
        if st == 'slow': x = LAW.fit_subset(x, ['Fs', 'As'], sus, 'slow release')
        elif st == 'fast': x = LAW.fit_subset(x, ['C', 'Ff'], short, 'charge + fast')
        elif st == 'joint': x = LAW.fit_subset(x, LAW.ORDER, sus[::2] + short, 'joint', nfev=40)
        elif st == 'report': LAW.report(x, cs)
        json.dump({'knots': LAW.KN, 'order': LAW.ORDER, 'x': list(map(float, x))}, open(LAW.OUT, 'w'), indent=1)


# ---- the CMA-ES fit -------------------------------------------------------------------------------------------
DEC = 32
POOL = ThreadPoolExecutor(8)
MODE = os.environ.get('HC_OBJ', 'null')       # null: the mean null (dB) over the cases, energy-weighted as a null test; db: rms dB error


EXTRA = os.environ.get('HC_FIT_MEMFLOOR') is not None      # one more parameter: ln of the memory's release-time floor (ms)
FITBETA = os.environ.get('HC_FIT_BETA') is not None         # one more: logit of the charge blend (before the floor's)


def objective(q, cs, x0, nF, nA):
    if FITBETA:
        LAW.dll.hc_set_charge_blend(__import__('ctypes').c_double(float(1 / (1 + np.exp(-q[-2 if EXTRA else -1])))))
    if EXTRA:
        LAW.dll.hc_set_mem_floor(__import__('ctypes').c_double(float(np.exp(q[-1]))))
        q = q[:-1]
    if FITBETA: q = q[:-1]
    X = law(q, x0, nF, nA)                 # sets the C model's w0 / K / p globals for this candidate

    def one(c):
        m = IR.run(c, X, c['Rt'])
        s = slice(c['from'], None, DEC)
        ref = c['ref'][s]; ok = ~np.isnan(ref)
        if MODE == 'null':
            a = c['a'][s][ok]
            num = np.sum(((m[s][ok] - ref[ok]) * a) ** 2); den = np.sum((ref[ok] * a) ** 2) + 1e-30
            return 10 * np.log10(num / den + 1e-12), 1
        d = 20 * np.log10(np.maximum(m[s][ok], 1e-9)) - 20 * np.log10(np.maximum(ref[ok], 1e-9))
        return np.sum(np.minimum(d * d, 100.0)), len(d)     # each sample's error capped at 10 dB (robust)
    r = list(POOL.map(one, cs))
    if MODE == 'null': return sum(a for a, _ in r) / len(r)
    return np.sqrt(sum(a for a, _ in r) / sum(b for _, b in r))


def cma(f, x0, sigma, gens, lam=16, seed=1):
    rng = np.random.default_rng(seed)
    n = len(x0); mu = lam // 2
    w = np.log(mu + 0.5) - np.log(np.arange(1, mu + 1)); w /= w.sum(); mueff = 1 / np.sum(w ** 2)
    cc = (4 + mueff / n) / (n + 4 + 2 * mueff / n); cs_ = (mueff + 2) / (n + mueff + 5)
    c1 = 2 / ((n + 1.3) ** 2 + mueff); cmu = min(1 - c1, 2 * (mueff - 2 + 1 / mueff) / ((n + 2) ** 2 + mueff))
    damps = 1 + 2 * max(0, np.sqrt((mueff - 1) / (n + 1)) - 1) + cs_
    chiN = np.sqrt(n) * (1 - 1 / (4 * n) + 1 / (21 * n * n))
    m = x0.copy(); C = np.eye(n); pc = np.zeros(n); ps = np.zeros(n); B = np.eye(n); D = np.ones(n)
    best = (f(m), m.copy()); print('start %.4f dB' % best[0], flush=True)
    for g in range(gens):
        z = rng.standard_normal((lam, n)); y = z @ (B * D).T; X = m + sigma * y
        F = np.array([f(x) for x in X]); idx = np.argsort(F)
        if F[idx[0]] < best[0]: best = (F[idx[0]], X[idx[0]].copy())
        yw = w @ y[idx[:mu]]; m = m + sigma * yw
        Cinvh = B @ np.diag(1 / D) @ B.T
        ps = (1 - cs_) * ps + np.sqrt(cs_ * (2 - cs_) * mueff) * (Cinvh @ yw)
        hs = np.linalg.norm(ps) / np.sqrt(1 - (1 - cs_) ** (2 * (g + 1))) < (1.4 + 2 / (n + 1)) * chiN
        pc = (1 - cc) * pc + hs * np.sqrt(cc * (2 - cc) * mueff) * yw
        C = (1 - c1 - cmu) * C + c1 * (np.outer(pc, pc) + (1 - hs) * cc * (2 - cc) * C) + cmu * (y[idx[:mu]].T * w) @ y[idx[:mu]]
        sigma *= np.exp((cs_ / damps) * (np.linalg.norm(ps) / chiN - 1))
        C = (C + C.T) / 2; D2, B = np.linalg.eigh(C); D = np.sqrt(np.maximum(D2, 1e-20))
        print('gen %3d best %.4f  generation best %.4f  median %.4f  sigma %.3f' % (g, best[0], F[idx[0]], np.median(F), sigma), flush=True)
    return best


def main():
    if os.environ.get('HC_ORDER'): LAW.dll.hc_set_order(int(os.environ['HC_ORDER']))      # 1: the attack before the release
    gens = int(sys.argv[1]) if len(sys.argv) > 1 else 60
    sigma = float(sys.argv[2]) if len(sys.argv) > 2 else 0.2
    x0, z0, nF, nA = setup()
    if os.environ.get('HC_ORDER'): LAW.dll.hc_set_order(int(os.environ['HC_ORDER']))
    if os.environ.get('HC_CHARGE_OUT'): LAW.dll.hc_set_charge_from_out(int(os.environ['HC_CHARGE_OUT']))
    if os.environ.get('HC_BETA'): LAW.dll.hc_set_charge_blend(__import__('ctypes').c_double(float(os.environ['HC_BETA'])))
    _L = json.load(open(LAW.OUT))
    if not EXTRA: LAW.dll.hc_set_mem_floor(__import__('ctypes').c_double(_L.get('memory_floor_ms', 0.0)))
    if os.environ.get('HC_RFLOOR'): LAW.dll.hc_set_r_floor(int(os.environ['HC_RFLOOR']))
    LAW.dll.hc_set_startup(__import__('ctypes').c_double(json.load(open(LAW.OUT)).get('startup_ms', 0.0)))
    cs = cases()
    q0 = np.concatenate([x0[:nF + nA], z0])
    if FITBETA: b0 = float(os.environ['HC_FIT_BETA']); q0 = np.concatenate([q0, [np.log(b0 / (1 - b0))]])
    if EXTRA: q0 = np.concatenate([q0, [np.log(float(os.environ['HC_FIT_MEMFLOOR']))]])
    t0 = time.time(); f0 = objective(q0, cs, x0, nF, nA); print('%d cases, one evaluation %.1f s' % (len(cs), time.time() - t0), flush=True)
    best, q = cma(lambda q: objective(q, cs, x0, nF, nA), q0, sigma, gens)
    print('best %.4f dB (start %.4f)' % (best, f0))
    for p in (LAW.OUT, W.OUT):
        shutil.copy(p, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'build', os.path.basename(p).replace('.json', '.before_cma.json')))
    if EXTRA:
        floor = float(np.exp(q[-1])); q = q[:-1]; print('memory release floor %.1f ms' % floor)
    if FITBETA:
        beta = float(1 / (1 + np.exp(-q[-1]))); q = q[:-1]; print('charge blend %.4f' % beta)
    x = x0.copy(); x[:nF + nA] = q[:nF + nA]
    lw = json.load(open(LAW.OUT)); lw['x'] = list(map(float, x)); json.dump(lw, open(LAW.OUT, 'w'), indent=1)
    json.dump({'S': W.SX.tolist(), 'z': list(map(float, q[nF + nA:]))}, open(W.OUT, 'w'))
    if os.environ.get('HC_ORDER'):
        lw = json.load(open(LAW.OUT)); lw['attack_first'] = int(os.environ['HC_ORDER']); json.dump(lw, open(LAW.OUT, 'w'), indent=1)
    if os.environ.get('HC_CHARGE_OUT'):
        lw = json.load(open(LAW.OUT)); lw['charge_from_output'] = int(os.environ['HC_CHARGE_OUT']); json.dump(lw, open(LAW.OUT, 'w'), indent=1)
    if os.environ.get('HC_BETA'):
        lw = json.load(open(LAW.OUT)); lw['charge_blend'] = float(os.environ['HC_BETA']); json.dump(lw, open(LAW.OUT, 'w'), indent=1)
    if EXTRA:
        lw = json.load(open(LAW.OUT)); lw['memory_floor_ms'] = floor; json.dump(lw, open(LAW.OUT, 'w'), indent=1)
    if FITBETA:
        lw = json.load(open(LAW.OUT)); lw['charge_blend'] = beta; json.dump(lw, open(LAW.OUT, 'w'), indent=1)
    if os.environ.get('HC_RFLOOR'):
        lw = json.load(open(LAW.OUT)); lw['r_floor_at_s'] = int(os.environ['HC_RFLOOR']); json.dump(lw, open(LAW.OUT, 'w'), indent=1)


if __name__ == '__main__':
    cmd = sys.argv[1] if len(sys.argv) > 1 else ''
    if cmd == 'collectsettled': collect_settled()
    elif cmd == 'collect': collect_nullset()
    elif cmd == 'collectmem': collect_memory()
    elif cmd == 'collectpresets': collect_presets()
    elif cmd == 'first': first(sys.argv[2:] or ['slow', 'fast', 'joint', 'report'])
    elif cmd == 'refit': refit(int(sys.argv[2]) if len(sys.argv) > 2 else 30)
    elif cmd == 'report': report()
    else: main()
