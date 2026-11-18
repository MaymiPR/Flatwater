"""The C++ ride (Flatwater Vocal Leveler's processor, run by VlRender) against the fitting model (RideModel.c with the values in
plugins/FwVocalLeveler/Source/dsp/Fit.h, as plugins/FwVocalLeveler/scripts/export_fit.py wrote them): the gains should agree to float precision.

    python -I check_port.py
"""
import sys, os, re, subprocess, tempfile
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import numpy as np
import render_reference as Q
import compare_model as E
import model_lib as M

HERE = os.path.dirname(os.path.abspath(__file__))
EXE = os.environ.get('FWVOCALLEVELER_RENDER', os.path.join(HERE, '..', '..', '..', 'build', 'VlRender_artefacts', 'Release', 'VlRender.exe'))
FIT_H = os.path.join(HERE, '..', '..', '..', 'plugins', 'FwVocalLeveler', 'Source', 'dsp', 'Fit.h')
CPP = dict(c='c', bias='bias', followerAttackSlow='fatt_s', followerAttackFast='fatt_f', followerRelease='frel', followerHold='fhold',
           followerPower='fpow', vadAttack='vatt', vadRelease='vrel', vk='vk', rideAttackSlow='ratt_s',
           rideAttackFast='ratt_f', rideRelease='rrel', ridePower='rpow', parkTime='park', parkHoldSlow='phold_s',
           parkHoldFast='phold_f', spillOffset='spill', initialV='v0')


def header_params():
    t = open(FIT_H, encoding='utf-8').read()
    p = {CPP[m.group(1)]: float(m.group(2)) for m in re.finditer(r'double (\w+) = ([-0-9.e+]+);', t)}
    return p


def render_cpp(x, st, music=None, sens=0.0):
    d = tempfile.mkdtemp()
    fi, fo = os.path.join(d, 'in.f32'), os.path.join(d, 'out.f32')
    np.asarray(x, np.float32).tofile(fi)
    args = [EXE, fi, fo, '1', '48000', 'target=%g' % st['target'], 'rangeMin=%g' % st['rmin'], 'rangeMax=%g' % st['rmax'],
            'vocalSensitivity=%g' % st['vs'], 'spill=%g' % st['spill'], 'attack=%g' % st['fast'], 'mode=%d' % (3 if st['live'] else 1),
            'idle=%g' % st['idle']]
    if music is not None:                                  # the side-chain's mid, one channel
        fk = os.path.join(d, 'key.f32'); np.asarray(music, np.float32).tofile(fk)
        args += ['sc=' + fk, 'scch=1', 'musicSensitivity=%g' % sens]
    subprocess.run(args, check=True)
    return np.fromfile(fo, np.float32)


def main():
    p = header_params()
    worst = 0.0
    for key in ('step-18-30', 'gap0.60-30', 'stop-24', 'syll0', 'syll2'):
        for s in ('default', 'fast', 'range-3+9', 'fast12', 'live-30'):
            x, _ = Q.load('probes', key, s)
            st = E.model_settings(s)
            gm = M.ride(x, st, p).astype(np.float64)
            y = render_cpp(x, st).astype(np.float64)
            ok = np.abs(x) > 1e-3
            gc = 20 * np.log10(np.abs(y[ok]) / np.abs(x[ok]))
            d = np.abs(gc - gm[ok]).max()
            worst = max(worst, d)
            print('%-12s %-10s max |C++ - model| %.4f dB' % (key, s, d))
    # with music on the side-chain: pink noise under two probes, at two Music Sensitivities
    import nulltest_lib as nl
    for key, s, sens in (('syll0', 'default', 0.0), ('step-18-30', 'range-3+9', 6.0)):
        x, _ = Q.load('probes', key, s)
        st = E.model_settings(s)
        m = nl.pink(len(x) / 48000.0 + 1.0, -12.0, fs=48000, seed=4)[:len(x)]
        gm = M.ride(x, st, p, music=m, music_sensitivity=sens).astype(np.float64)
        y = render_cpp(x, st, m, sens).astype(np.float64)
        ok = np.abs(x) > 1e-3
        d = np.abs(20 * np.log10(np.abs(y[ok]) / np.abs(x[ok])) - gm[ok]).max()
        worst = max(worst, d)
        print('%-12s %-10s music, Sensitivity %g: max |C++ - model| %.4f dB' % (key, s, sens, d))
    print('worst %.4f dB' % worst)


if __name__ == '__main__':
    main()
