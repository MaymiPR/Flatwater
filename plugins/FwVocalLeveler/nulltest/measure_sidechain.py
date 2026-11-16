"""Measurement: what the music on the reference's side-chain bus does to its riding. The reference renders through
tools/RefRender, which enables the bus and feeds it (sc=); pedalboard cannot. The voice is a steady buzz (140 Hz and
its harmonics) at -21 dBFS, the music pink noise; the reference's defaults but Music Sensitivity. Prints three
tables; nothing is saved.

    python -I measure_sidechain.py

  1. The settled gain on the voice for each music level and each Music Sensitivity.
  2. Timing: the gain over time while the music steps up and down under the steady voice.
  3. The music's channels and kind: both channels, one alone, a tone in place of the noise, the voice itself.
"""
import os, sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'scripts', 'nulltest'))   # scripts/nulltest/
import numpy as np
import refhost as R
import nulltest_lib as nl
import vl_ref as H

FS = 48000
SECS = 10.0


def buzz(secs=SECS, db=-21.0):
    t = np.arange(int(secs * FS)) / FS
    v = sum(np.sin(2 * np.pi * 140 * k * t) / k for k in range(1, 14))
    return (v / np.sqrt(np.mean(v ** 2)) * 10 ** (db / 20)).astype(np.float32)


def ref(x, music=None, sens=None):
    """the reference's output for the voice x and the music (None: the bus not fed); sens: Music Sensitivity's text
    ('Off' is the bottom of its range, set by the normalised value: the text does not parse)"""
    params = {} if sens is None else {'Music Sensitivity:': 0} if sens == 'Off' else {'Music Sensitivity': sens}
    return R.render_ref(nl.stereo(x), FS, H.REF_NAME, sc=None if music is None else nl.stereo(music), params=params)[0]


def level(y, a, b):
    return nl.rms_db(np.asarray(y)[0, int(a * FS):int(b * FS)])


def gain(y, x, a=SECS - 2.0, b=SECS):
    return level(y, a, b) - level(nl.stereo(x), a, b)


def main():
    x = buzz()
    pink = lambda db: nl.pink(SECS, db, fs=FS, seed=4)
    levels = (-60, -40, -30, -20, -12, -6)
    senses = ('Off', '-12', '-6', '0', '6', '12')

    print('1. The gain on the voice over the last two seconds (dB). Rows: Music Sensitivity; columns: the music (dBFS rms).')
    print('   %-8s %8s %8s' % ('', 'no bus', 'silent') + ''.join(' %8d' % k for k in levels))
    for s in senses:
        row = [gain(ref(x, None, s), x), gain(ref(x, np.zeros_like(x), s), x)] + [gain(ref(x, pink(k), s), x) for k in levels]
        print('   %-8s' % s + ''.join(' %8.2f' % v for v in row), flush=True)

    print('2. Timing at Music Sensitivity 0: the music at -40 dBFS, -6 dBFS from 3 s to 6 s, then -40 dBFS again.')
    m = pink(-40.0).copy(); a, b = int(3 * FS), int(6 * FS)
    m[a:b] *= 10 ** (34 / 20)
    y = ref(x, m, '0'); y0 = ref(x, pink(-40.0), '0')
    for t in (2.5, 3.05, 3.2, 3.5, 4.0, 5.0, 5.9, 6.05, 6.2, 6.5, 7.0, 8.0, 9.5):
        print('   t %4.2f s: gain %7.2f dB (the music steady at -40 dBFS: %7.2f dB)'
              % (t, gain(y, x, t, t + 0.1), gain(y0, x, t, t + 0.1)), flush=True)

    print('3. The music\'s channels and kind at Music Sensitivity 0, -12 dBFS.')
    k = pink(-12.0); z = np.zeros_like(k)
    for label, music in (('pink noise, both channels', np.stack([k, k])), ('the left alone', np.stack([k, z])), ('the right alone', np.stack([z, k])),
                         ('a 1 kHz tone', nl.tone(1000.0, SECS, -9.0, fs=FS)), ('a 100 Hz tone', nl.tone(100.0, SECS, -9.0, fs=FS)),
                         ('the voice itself', x)):
        print('   %-28s gain %7.2f dB' % (label, gain(ref(x, music, '0'), x)), flush=True)


if __name__ == '__main__':
    main()
