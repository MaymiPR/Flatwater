"""Refreshes Flatwater Comp's side of an existing training set (render_trainset_fwcomp.py) after a change to its analytical
model: each case keeps its input, settings and the reference's render (yH); Flatwater Comp's render (yW) and traced
features are made again with the current CoRender (learned=0). Writes <dir>_v2 beside each folder; the reference
is not hosted again.

    python -I refresh_trainset_fwcomp.py data data2 data3 data4 test      -> build/fwcompnet/<dir>_v2/case_NNNN.npz
"""
import glob, os, subprocess, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import numpy as np
import nulltest_fwcomp as N
_argv, sys.argv = sys.argv, sys.argv[:1]
import render_trainset_fwcomp as T                          # (its import reads sys.argv for an output folder)
sys.argv = _argv

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'build', 'fwcompnet')
fi, fo, ft = [os.path.join(N.TMP, '%s%d.f32' % (p, os.getpid())) for p in ('rin', 'rout', 'rtr')]
for d in sys.argv[1:]:
    src, dst = os.path.join(ROOT, d), os.path.join(ROOT, d + '_v2')
    os.makedirs(dst, exist_ok=True)
    files = sorted(glob.glob(os.path.join(src, 'case_*.npz')))
    for k, p in enumerate(files):
        c = np.load(p)
        thr, rat, att, rel, pu, out = [float(v) for v in c['settings']]
        x = c['x']
        np.stack([x, x]).astype(np.float32).T.tofile(fi)
        subprocess.run([N.EXE, fi, fo, '2', str(T.FS), 'threshold=%g' % thr, 'ratio=%g' % rat, 'attack=%g' % att,
                        'release=%g' % rel, 'punch=%g' % pu, 'mix=100', 'output=%g' % out, 'limiter=0', 'analog=0',
                        'learned=0', 'trace=' + ft], check=True, capture_output=True)
        yW = np.fromfile(fo, np.float32).reshape(-1, 2).T[0]
        feat, guard = T.features(np.fromfile(ft, np.float32), (thr, rat, att, rel, pu))
        np.savez_compressed(os.path.join(dst, os.path.basename(p)), x=x, yW=yW, yH=c['yH'], feat=feat, guard=guard,
                            settings=c['settings'], name=c['name'])
        if k % 250 == 0:
            print(d, k, '/', len(files), flush=True)
    print(d, 'done', len(files), flush=True)
