"""Null test: Flatwater Bass Leveler (its own processor, run offline by build/.../BlRender.exe) against the reference Bass
Rider (rendered through pedalboard) at 48 kHz, both lined up with the input (the reported latency removed), stereo, the
same input on both channels unless the case says otherwise. Reports per case the output residual 20 log10 (rms (ours - reference) / rms (reference)) after the first
0.1 s, and the applied gain's error (ours vs the reference's, sample by sample where the input is loud enough).
Numbers are reported as they come: no case is called a null unless the data shows it.

    python -I nulltest.py [--quick] [--save] [--report] [--vst3]
    python -I nulltest.py --report     NULLTESTS.md again from measurements/FwBassLeveler/nulltests.json, rendering nothing
    python -I nulltest.py --vst3       our built VST3 against our renderer (the check before a number is published)
"""
import sys, os, json
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'scripts', 'nulltest'))   # scripts/nulltest/
import numpy as np
import subprocess
import refhost as R
import local_config_lib as L
import measure_reference as M
import compare_model as C

HERE = os.path.dirname(os.path.abspath(__file__))
EXE = os.environ.get('FWBASSLEVELER_RENDER', os.path.join(HERE, '..', '..', '..', 'build', 'BlRender_artefacts', 'Release', 'BlRender.exe'))
TMP = os.path.join(HERE, 'renders'); os.makedirs(TMP, exist_ok=True)
FS = 48000
KEYS = {'target': 'target', 'sensitivity': 'sensitivity', 'spill': 'spill', 'artifacts': 'artifacts', 'range_max': 'rangeMax',
        'range_min': 'rangeMin', 'output': 'output', 'idle': 'idle'}


def render_ours(x, **params):
    """x: (channels, n). The reference's host parameter names and units"""
    x = np.atleast_2d(np.asarray(x, np.float32)); ch, n = x.shape
    fi, fo = os.path.join(TMP, 'brin%d.f32' % os.getpid()), os.path.join(TMP, 'brout%d.f32' % os.getpid())
    x.T.astype(np.float32).tofile(fi)
    args = [EXE, fi, fo, str(ch), str(FS), 'mode=%d' % (0 if ch == 2 else 1)]
    for k, v in params.items():
        if k == 'response': args.append('response=%d' % (1 if v.strip() == 'Fast' else 0))
        else: args.append('%s=%g' % (KEYS[k], v))
    subprocess.run(args, check=True, capture_output=True)
    return np.fromfile(fo, np.float32).reshape(-1, ch).T


def render_ref(x, **params):
    name = L.ref_component('FwBassLeveler')
    R.fresh(name); p = R.load(name)
    for k, v in params.items(): setattr(p, k, v)
    return p(np.asarray(x, np.float32), FS, buffer_size=512, reset=True)


def cases(quick):
    t = np.arange(int(4 * FS)) / FS
    out = [('steady 80 Hz -20 dBFS', M.seg(80.0, [-20], [4.0]), {}),
           ('steps 1 kHz', M.seg(1000.0, [-30, -16, -30, -20], [1.0, 1.0, 1.0, 1.0]), {}),
           ('bass line', C.bassline(), {}),
           ('synthetic part 6', C.synth_notes(seed=6, secs=10.0), {}),
           ('synthetic part 7, Fast', C.synth_notes(seed=7, secs=10.0), {'response': 'Fast '}),
           ('bass line, Target -24', C.bassline(), {'target': -24.0}),
           ('bass line, Sensitivity 80', C.bassline(), {'sensitivity': 80.0}),
           ('bass line, Artifacts 20', C.bassline(), {'artifacts': 20.0}),
           ('synthetic part 8, range -6 .. +6', C.synth_notes(seed=8, secs=10.0), {'range_min': -6.0, 'range_max': 6.0})]
    return out[:4] if quick else out


def main():
    if '--report' in sys.argv:
        import report_lib as RL
        print('wrote', os.path.relpath(RL.write('FwBassLeveler')))
        return
    if '--vst3' in sys.argv:
        sys.exit(0 if vst3_check() is not False else 1)
    quick = '--quick' in sys.argv
    rows = []
    for name, x, kw in cases(quick):
        x = np.asarray(x, np.float32)
        xs = np.stack([x, x])
        y_ref = render_ref(xs, **kw)
        ours_kw = {k: v for k, v in kw.items()}
        y_ours = render_ours(xs, **ours_kw)
        s = int(0.1 * FS)
        res = 20 * np.log10(np.sqrt(np.mean((y_ours[:, s:] - y_ref[:, s:]) ** 2)) / np.sqrt(np.mean(y_ref[:, s:] ** 2)) + 1e-30)
        g_ref, g_ours = M.gtrace(x, y_ref[0]), M.gtrace(x, y_ours[0])
        env = np.convolve(x.astype(float) ** 2, np.ones(96) / 96, 'same'); ok = env > 10 ** (-55 / 10); ok[:s] = False
        d = np.abs(g_ours - g_ref)[ok]
        rows.append((name, res, np.median(d), np.percentile(d, 90)))
        print('%-36s residual %6.1f dB   gain error median %.3f dB, 90%% %.3f dB' % rows[-1], flush=True)
    if '--save' in sys.argv:
        import report_lib as RL
        with RL.Part('FwBassLeveler', 'fixed') as f:
            f.write('| case | residual (dB) | gain error median (dB) | 90 % (dB) |\n|---|---|---|---|\n')
            for r in rows: f.write('| %s | %.1f | %.3f | %.3f |\n' % r)


# ---- --vst3 ----
import time, tempfile
from scipy.signal import resample_poly
import measure_reference as M, compare_model as C, bass_corpus_lib as K


HERE = os.path.dirname(os.path.abspath(__file__))
VST3 = os.path.join(HERE, '..', '..', '..', 'build', 'FwBassLeveler_artefacts', 'Release', 'VST3', 'Flatwater Bass Leveler.vst3')


def res_db(x, y, r):
    """the residual over the loud parts, dB (x: the input, mono or (ch, n))"""
    xm = np.atleast_2d(x).mean(0)
    env = np.convolve(xm.astype(float) ** 2, np.ones(96) / 96, 'same'); ok = env > 1e-6; ok[:4800] = False
    y, r = np.atleast_2d(y), np.atleast_2d(r)
    return 10 * np.log10(np.sum((y - r)[:, ok] ** 2) / np.sum(r[:, ok] ** 2) + 1e-30)


def vst3(x, fs, params):
    """Flatwater Bass Leveler's VST3 through RefRender (stereo in / out), parameters by their display text"""
    d = tempfile.mkdtemp(prefix='brv_'); fin, fout = os.path.join(d, 'in.f32'), os.path.join(d, 'out.f32')
    x = np.atleast_2d(np.asarray(x, np.float32)); x = np.vstack([x, x]) if x.shape[0] == 1 else x
    x.T.astype(np.float32).tofile(fin)
    cmd = [R.REFRENDER, VST3, 'Flatwater Bass Leveler', fin, fout, 'fs=%d' % fs, 'block=512'] + ['%s=%s' % kv for kv in params.items()]
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
    if not os.path.exists(fout): raise RuntimeError(r.stdout + r.stderr)
    return np.fromfile(fout, np.float32).reshape(-1, 2).T


def ours(x, fs, **st):
    x = np.atleast_2d(np.asarray(x, np.float32))
    return render_ours(x, **st) if fs == 48000 else render_fs(x, fs, **st)


def render_fs(x, fs, **st):
    ch, n = x.shape
    d = tempfile.mkdtemp(prefix='brf_'); fi, fo = os.path.join(d, 'in.f32'), os.path.join(d, 'out.f32')
    x.T.astype(np.float32).tofile(fi)
    args = [EXE, fi, fo, str(ch), str(fs), 'mode=%d' % (0 if ch == 2 else 1)]
    for k, v in st.items():
        if k == 'mode': args[-1] = 'mode=%d' % v
        elif k == 'response': args.append('response=%d' % (1 if v.strip() == 'Fast' else 0))
        else: args.append('%s=%g' % (KEYS[k], v))
    subprocess.run(args, check=True, capture_output=True)
    return np.fromfile(fo, np.float32).reshape(-1, ch).T


def vst3_check():
    """--vst3: Phase 5 checks for Flatwater Bass Leveler: (1) the built VST3 (hosted by tools/RefRender) against the offline renderer"""
    bass = C.bassline().astype(np.float32)
    rows = []
    # (1) the VST3 against the offline renderer, stereo, at 48 and 96 kHz
    for fs in (48000, 96000):
        x = bass if fs == 48000 else resample_poly(bass, 2, 1).astype(np.float32)
        xs = np.vstack([x, 0.5 * np.roll(x, 1000)])
        for label, p_txt, p_ours in (('defaults', {}, {}), ('Fast, Sensitivity 75', {'Response': 'Fast', 'Sensitivity': '75'}, {'response': 'Fast ', 'sensitivity': 75.0}),
                                      ('Artifacts 100, Range -6 .. +6', {'Artifacts': '100', 'Range Max': '6', 'Range Min': '-6'}, {'artifacts': 100.0, 'range_max': 6.0, 'range_min': -6.0})):
            yv = vst3(xs, fs, p_txt); yo = render_fs(xs, fs, **p_ours)
            m = min(yv.shape[1], yo.shape[1])
            rows.append(('VST3 vs offline renderer, %s, %g kHz' % (label, fs / 1000), res_db(xs[:, :m], yv[:, :m], yo[:, :m])))
            print(rows[-1], flush=True)
    # (2) the reference against Flatwater Bass Leveler: other rates (the bass line resampled), Live, stereo
    for fs in (44100, 96000):
        x = resample_poly(bass, 147, 160).astype(np.float32) if fs == 44100 else resample_poly(bass, 2, 1).astype(np.float32)
        R.fresh(M.NAME); p = R.load(M.NAME); r = p(x[None], fs, buffer_size=512, reset=True)[0]
        y = render_fs(x[None], fs)[0]
        rows.append(('reference vs Flatwater Bass Leveler, bass line at %g kHz' % (fs / 1000), res_db(x, y, r))); print(rows[-1], flush=True)
    for label, name, mode, st in (('Live Mono, bass line', L.ref_component('FwBassLeveler', 'Live Mono'), 3, {}), ('Live Mono, bass line, Fast', L.ref_component('FwBassLeveler', 'Live Mono'), 3, {'response': 'Fast '}),
                                  ('Live Mono, bass line, Spill -25', L.ref_component('FwBassLeveler', 'Live Mono'), 3, {'spill': -25.0}),
                                  ('Live Mono, bass line, Spill -15', L.ref_component('FwBassLeveler', 'Live Mono'), 3, {'spill': -15.0})):
        r = M.render(bass, name=name, **st); y = render_fs(bass[None], 48000, mode=mode, **st)[0]
        rows.append(('reference vs Flatwater Bass Leveler, %s' % label, res_db(bass, y, r))); print(rows[-1], flush=True)
    xs = np.vstack([bass, K.part(30)[0][:len(bass)] * 0.7]).astype(np.float32)
    for label, name, mode in (('Stereo, two different bass parts (L / R)', L.ref_component('FwBassLeveler'), 0), ('Live Stereo, the same', L.ref_component('FwBassLeveler', 'Live Stereo'), 2)):
        r = M.render(xs, name=name); y = render_fs(xs, 48000, mode=mode)
        rows.append(('reference vs Flatwater Bass Leveler, %s' % label, res_db(xs, y, r))); print(rows[-1], flush=True)
    # (3) CPU: 60 s of stereo through the offline renderer, 512-sample blocks
    x = np.tile(xs, (1, 6)); t0 = time.perf_counter(); render_fs(x, 48000); dt = time.perf_counter() - t0
    rows.append(('CPU: 60 s of stereo at 48 kHz rendered in (s; % of one core in real time)', dt)); print('cpu %.2f s = %.2f %%' % (dt, dt / 60 * 100))
    for k, v in rows: print('| %s | %.1f |' % (k, v))

if __name__ == '__main__':
    main()
