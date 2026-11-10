"""Null test: Flatwater Vocal Leveler (its own processor, run offline by build/.../VlRender.exe) against the reference Vocal
Rider (rendered through pedalboard, a fresh instance per case). There is no latency on either side. Reports per case
the output residual 20 log10 (rms (ours - reference) / rms (reference)) after the first 0.1 s, and the applied gain's
error (ours vs the reference's, sample by sample where the input is loud enough). The corpus cases are held-out parts
(scripts/nulltest/vocal_corpus_lib.py 24 .. 35), not used to identify the model. Numbers are reported as they come: no case is called a
null unless the data shows it.

    python -I nulltest.py [--quick] [--save] [--report] [--vst3]
    python -I nulltest.py --report     NULLTESTS.md again from measurements/FwVocalLeveler/nulltests.json, rendering nothing
    python -I nulltest.py --vst3       our built VST3 against our renderer (the check before a number is published)
"""
import sys, os
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'scripts', 'nulltest'))   # scripts/nulltest/
import numpy as np
import subprocess
from scipy.signal import resample_poly
import refhost as R
import render_reference as Q
import vocal_corpus_lib as V
from measure_reference import seg, with_pilot

HERE = os.path.dirname(os.path.abspath(__file__))
EXE = os.environ.get('FWVOCALLEVELER_RENDER', os.path.join(HERE, '..', '..', '..', 'build', 'VlRender_artefacts', 'Release', 'VlRender.exe'))
TMP = os.path.join(HERE, 'renders'); os.makedirs(TMP, exist_ok=True)
COMPONENTS = {1: 'Vocal Rider Mono', 2: 'Vocal Rider Stereo'}
LIVE = {1: 'Vocal Rider Live Mono', 2: 'Vocal Rider Live Stereo'}


def render_ours(x, fs, live, s):
    """x: (channels, n); s: the case's settings (dB; idle as its place in the range, 0 .. 1)"""
    ch, n = x.shape
    fi, fo = os.path.join(TMP, 'vrin%d.f32' % os.getpid()), os.path.join(TMP, 'vrout%d.f32' % os.getpid())
    x.T.astype(np.float32).tofile(fi)
    lo, hi = s.get('range_min', -6.0), s.get('range_max', 6.0)
    args = [EXE, fi, fo, str(ch), str(fs), 'mode=%d' % ((2 if live else 0) + (1 if ch == 1 else 0)),
            'target=%g' % s.get('target', -21.0), 'vocalSensitivity=%g' % s.get('vocal_sensitivity', 0.0),
            'rangeMin=%g' % lo, 'rangeMax=%g' % hi, 'idle=%g' % (lo + s.get('idle', 0.5) * (hi - lo)),
            'attack=%d' % (1 if s.get('fast') else 0), 'spill=%g' % s.get('spill', -48.0)]
    subprocess.run(args, check=True, capture_output=True)
    return np.fromfile(fo, np.float32).reshape(-1, ch).T


def render_ref(x, fs, live, s):
    comp = (LIVE if live else COMPONENTS)[x.shape[0]]
    R.fresh(comp); p = R.load(comp)
    for k in ('target', 'vocal_sensitivity', 'range_min', 'range_max'):
        if k in s: setattr(p, k, float(s[k]))
    if 'idle' in s: p.idle = round(s['idle'] * 24.0 - 12.0, 2)       # (its host text: the place over a fixed +-12)
    if s.get('fast'): p.attack = 'Fast '                              # (choice texts end with a space)
    if live and 'spill' in s: p.spill = float(s['spill'])
    return p(np.asarray(x, np.float32), fs, buffer_size=512, reset=True)


def cases(quick):
    part = lambda k: V.part(k)[0]
    out = [('steps 1 kHz', with_pilot(seg(1000.0, [-30, -16, -30, None, -20], [2.0, 2.0, 2.0, 1.0, 2.0])), 48000, False, {}),
           ('syllable train', Q.probes()['syll1'], 48000, False, {}),
           ('vocal part 24', part(24), 48000, False, {}),
           ('vocal part 25, Fast', part(25), 48000, False, dict(fast=True)),
           ('vocal part 26, range -12 .. +12', part(26), 48000, False, dict(range_min=-12.0, range_max=12.0)),
           ('vocal part 27, Target -30', part(27), 48000, False, dict(target=-30.0)),
           ('vocal part 28, Vocal Sensitivity +6', part(28), 48000, False, dict(vocal_sensitivity=6.0)),
           ('vocal part 29, range -3 .. +9, Idle 0.67', part(29), 48000, False, dict(range_min=-3.0, range_max=9.0, idle=0.67)),
           ('vocal part 30, Live, Spill -30', part(30), 48000, True, dict(spill=-30.0)),
           ('vocal part 31, Live Fast, Spill -36', part(31), 48000, True, dict(spill=-36.0, fast=True)),
           ('vocal part 32, stereo (R 4 dB lower)', np.stack([part(32), part(32) * 10 ** (-4 / 20)]), 48000, False, {}),
           ('vocal part 33, 44.1 kHz', resample_poly(part(33), 147, 160), 44100, False, {}),
           ('vocal part 34, 96 kHz, Fast', resample_poly(part(34), 2, 1), 96000, False, dict(fast=True))]
    return out[:3] if quick else out


def main():
    if '--report' in sys.argv:
        import report_lib as RL
        print('wrote', os.path.relpath(RL.write('FwVocalLeveler')))
        return
    if '--vst3' in sys.argv:
        sys.exit(0 if vst3_check() is not False else 1)
    quick = '--quick' in sys.argv
    rows = []
    for name, x, fs, live, s in cases(quick):
        x = np.atleast_2d(np.asarray(x, np.float32))
        y_ref = render_ref(x, fs, live, s)
        y_ours = render_ours(x, fs, live, s)
        a = int(0.1 * fs)
        res = 20 * np.log10(np.sqrt(np.mean((y_ours[:, a:] - y_ref[:, a:]) ** 2)) / np.sqrt(np.mean(y_ref[:, a:] ** 2)) + 1e-30)
        x0 = x[0].astype(np.float64)
        k = np.ones(max(1, int(0.005 * fs)))
        gt = lambda y: 20 * np.log10(np.maximum(np.convolve(x0 * y, k, 'same'), 1e-30) / np.maximum(np.convolve(x0 * x0, k, 'same'), 1e-30))
        g_ref, g_ours = gt(y_ref[0].astype(np.float64)), gt(y_ours[0].astype(np.float64))
        env = np.convolve(x0 ** 2, np.ones(96) / 96, 'same'); ok = env > 10 ** (-60 / 10); ok[:a] = False
        d = np.abs(g_ours - g_ref)[ok]
        rows.append((name, res, np.median(d), np.percentile(d, 90)))
        print('%-42s residual %6.1f dB   gain error median %.3f dB, 90%% %.3f dB' % rows[-1], flush=True)
    if '--save' in sys.argv:
        import report_lib as RL
        with RL.Part('FwVocalLeveler', 'fixed') as f:
            f.write('| case | residual (dB) | gain error median (dB) | 90 % (dB) |\n|---|---|---|---|\n')
            for r in rows: f.write('| %s | %.1f | %.3f | %.3f |\n' % r)


# ---- --vst3 ----
import time, tempfile
from scipy.signal import resample_poly
import vocal_corpus_lib as V


HERE = os.path.dirname(os.path.abspath(__file__))
VST3 = os.path.join(HERE, '..', '..', '..', 'build', 'FwVocalLeveler_artefacts', 'Release', 'VST3', 'Flatwater Vocal Leveler.vst3')


def vst3(x, fs, params):
    """Flatwater Vocal Leveler's VST3 through RefRender (stereo in / out), parameters by their display text"""
    d = tempfile.mkdtemp(prefix='vrv_'); fin, fout = os.path.join(d, 'in.f32'), os.path.join(d, 'out.f32')
    x.T.astype(np.float32).tofile(fin)
    cmd = [R.REFRENDER, VST3, 'Flatwater Vocal Leveler', fin, fout, 'fs=%d' % fs, 'block=512'] + ['%s=%s' % kv for kv in params.items()]
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
    if not os.path.exists(fout): raise RuntimeError(r.stdout + r.stderr)
    return np.fromfile(fout, np.float32).reshape(-1, 2).T


def offline(x, fs, keys):
    d = tempfile.mkdtemp(prefix='vro_'); fi, fo = os.path.join(d, 'in.f32'), os.path.join(d, 'out.f32')
    x.T.astype(np.float32).tofile(fi)
    subprocess.run([EXE, fi, fo, '2', str(fs), 'mode=0'] + ['%s=%g' % kv for kv in keys.items()], check=True, capture_output=True)
    return np.fromfile(fo, np.float32).reshape(-1, 2).T


def res_db(y, r):
    a = 4800
    return 10 * np.log10(np.sum((y[:, a:] - r[:, a:]) ** 2) / np.sum(r[:, a:] ** 2) + 1e-30)


def vst3_check():
    """--vst3: Phase 5 checks for Flatwater Vocal Leveler: (1) the built VST3 (hosted by tools/RefRender) against the offline renderer"""
    v = V.part(24)[0].astype(np.float32)
    rows = []
    for fs in (48000, 96000):
        x = v if fs == 48000 else resample_poly(v, 2, 1).astype(np.float32)
        xs = np.vstack([x, 0.6 * np.roll(x, 2000)]).astype(np.float32)
        for label, p_txt, p_keys in (('defaults', {}, {}),
                                      ('Fast, Target -30', {'Attack': 'Fast', 'Target': '-30'}, {'attack': 1, 'target': -30}),
                                      ('Range -12 .. +12, Vocal Sensitivity 6', {'Range Max': '12', 'Range Min': '-12', 'Vocal Sensitivity': '6'},
                                       {'rangeMax': 12, 'rangeMin': -12, 'vocalSensitivity': 6})):
            yv = vst3(xs, fs, p_txt); yo = offline(xs, fs, p_keys)
            m = min(yv.shape[1], yo.shape[1])
            rows.append(('VST3 vs offline renderer, %s, %g kHz' % (label, fs / 1000), res_db(yv[:, :m], yo[:, :m])))
            print(rows[-1], flush=True)
    xs = np.vstack([v, v]); x = np.tile(xs, (1, 3)); t0 = time.perf_counter(); offline(x, 48000, {}); dt = time.perf_counter() - t0
    rows.append(('CPU: 60 s of stereo at 48 kHz rendered in (s; includes start-up)', dt)); print('cpu %.2f s = %.2f %%' % (dt, dt / 60 * 100))
    for k, val in rows: print('| %s | %.1f |' % (k, val))

if __name__ == '__main__':
    main()
