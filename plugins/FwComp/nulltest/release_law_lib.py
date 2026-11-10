"""The release law of Flatwater Comp in its two-state form (native/ReleaseLaw.c, the plug-in's form): the C model's
binding, the knot tables and their packing, the target reduction per sample, the least-squares stages and the report.
fit_release_law.py fits it; fit_release_real.py and fit_release_time_factor.py import it.

Knot tables (log rates): Fs (the slow release, x release time), As (an absolute part near unity gain), C (the slow
state's charge), Ff (the fast fall toward the slow state). Results in measurements/FwComp/release_law.json.
"""
import sys, os, json
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import numpy as np
import ctypes as ct
from scipy import optimize
from scipy.ndimage import maximum_filter1d
from model_lib import static_gain_db
from fit_release import load

FS = 48000
HERE = os.path.dirname(os.path.abspath(__file__))
dll = ct.CDLL(os.path.join(HERE, '..', '..', '..', 'build', 'native', 'FwComp', 'ReleaseLaw' + ('.dll' if os.name == 'nt' else '.so')))
OUT = os.path.join(HERE, '..', '..', '..', 'measurements', 'FwComp', 'release_law.json')

KN = {'Fs': [0, 0.5, 1, 2, 3, 4, 6, 8, 10, 12, 16, 20, 25, 30, 40, 50],
      'As': [0, 0.5, 1, 1.5, 2, 3, 5],
      'C': [0, 1, 2, 4, 6, 8, 12, 16, 20, 25, 30, 40, 50],
      'Ff': [0, 1, 2]}                   # not a table: log K (ms), the exponent p of rel_eff = K / (R - S)^p, the charge exponent q
ORDER = ['Fs', 'As', 'C', 'Ff']


def target(c):
    a = c['a']
    H, lead = 64, 1
    pad = np.concatenate([a, np.full(H + lead, a[-1])])
    h = maximum_filter1d(pad, size=H, origin=-(H // 2))[lead:lead + len(a)]
    return np.ascontiguousarray(-static_gain_db(h, -30.0, c['rat']), np.float64)


def run(c, vals, Rt=None):
    Rt = target(c) if Rt is None else Rt
    n = len(Rt)
    g = np.empty(n)
    xs = np.concatenate([np.asarray(KN[k], float) for k in ORDER])
    ns = np.array([len(KN[k]) for k in ORDER], np.int32)
    vs = np.ascontiguousarray(vals, np.float64)
    att_c = np.exp(-1 / (c['att'] * 0.99e-3 * FS))
    dll.hc_release(Rt.ctypes.data_as(ct.c_void_p), g.ctypes.data_as(ct.c_void_p), None, None, n, ct.c_double(FS), ct.c_double(c['rel']),
                   ct.c_double(att_c), 38, 27, ns.ctypes.data_as(ct.c_void_p), xs.ctypes.data_as(ct.c_void_p), vs.ctypes.data_as(ct.c_void_p))
    return g / g[0]


def initial():
    v = {}
    Fs = np.interp(KN['Fs'], [0, 1, 2, 4, 8, 16, 28, 50], [8, 9, 9.6, 5.93, 8.7, 27.8, 125, 600])
    v['Fs'] = np.log(Fs)
    v['As'] = np.log(np.interp(KN['As'], [0, 1, 2, 5], [40, 30, 5, 0.5]))
    v['C'] = np.log(np.full(len(KN['C']), 200.0))
    v['Ff'] = np.array([np.log(56.23), 1.38, 0.5])
    return v


def pack(v): return np.concatenate([v[k] for k in ORDER])


def unpack(x):
    v, i = {}, 0
    for k in ORDER:
        v[k] = x[i:i + len(KN[k])]; i += len(KN[k])
    return v


def residuals(x, cases, Rts, dec=8):
    r = []
    for c, Rt in zip(cases, Rts):
        m = run(c, x, Rt)
        r.append((20 * np.log10(m[::dec]) - 20 * np.log10(c['g'][::dec])))
    return np.concatenate(r)


def fit_subset(x0, free, cases, label, nfev=60):
    Rts = [target(c) for c in cases]
    idx = np.concatenate([np.arange(sum(len(KN[k]) for k in ORDER[:ORDER.index(f)]), sum(len(KN[k]) for k in ORDER[:ORDER.index(f) + 1])) for f in free])

    def f(z):
        x = x0.copy(); x[idx] = z
        return residuals(x, cases, Rts)
    s = optimize.least_squares(f, x0[idx], diff_step=1e-3, max_nfev=nfev, x_scale=1.0)
    x = x0.copy(); x[idx] = s.x
    r = f(s.x)
    print('%s: rms %.3f dB, worst %.2f dB (%d cases)' % (label, np.sqrt(np.mean(r ** 2)), np.abs(r).max(), len(cases)), flush=True)
    return x


def report(x, cases):
    worst = []
    for c in cases:
        e = np.abs(20 * np.log10(run(c, x)) - 20 * np.log10(c['g']))
        worst.append((e.max(), np.sqrt(np.mean(e ** 2)), c['rat'], c['att'], c['rel'], c['dur'], c['lvl']))
    worst.sort()
    print('median worst %.3f dB, mean rms %.3f dB' % (np.median([w[0] for w in worst]), np.mean([w[1] for w in worst])))
    for w in worst[-10:]: print('   worst %.2f (rms %.3f) dB: ratio %g attack %g release %g burst %g level %g' % w)
