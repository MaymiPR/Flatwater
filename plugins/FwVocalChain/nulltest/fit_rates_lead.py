"""(the doubler at other rates) the LFO lead's correction from a click comparison: ours - reference ~ -slope * dlead"""
import sys
sys.path.append(__import__('os').path.dirname(__import__('os').path.abspath(__file__)))
import numpy as np
import measure_doubler_rates as R
colour, fs = int(sys.argv[1]), int(sys.argv[2])
res, P = R.main(colour, fs, 8.0)
for side, (dr, do) in enumerate(res):
    e = do[:, 1] - dr[:, 1]
    sl = np.gradient(dr[:, 1]) / P                                      # (samples per sample)
    ok = np.abs(np.gradient(dr[:, 1])) < 50                             # (not across a jump)
    t = dr[:, 0] / fs
    A = np.stack([-sl[ok], np.ones(ok.sum()), t[ok]], 1)
    q, *_ = np.linalg.lstsq(A, e[ok], rcond=None)
    r = e[ok] - A @ q
    print('side %d: lead %+.1f samples, offset %+.3f, slope %+.4f samples/s; rms %.3f -> %.3f'
          % (side, q[0], q[1], q[2], np.sqrt(np.mean(e[ok] ** 2)), np.sqrt(np.mean(r ** 2))))
