"""Measurement: what a signal on the reference's side-chain bus (the key) does to its gain. The reference renders
through tools/RefRender, which enables the bus and feeds it (sc=); pedalboard cannot. Analog Off, Threshold -30,
Ratio 10, the rest at the defaults. Prints four tables; nothing is saved.

    python -I measure_sidechain.py

  1. Steady tones: the gain on a main tone for each key level, beside the gain the same main tone gets without a key
     when it has the key's level. Equal columns mean the detector reads the key and nothing of the main signal.
  2. Timing: the gain over time while the key steps in level under a steady main tone, against the gain the same
     steps get as the main signal without a key.
  3. The key's channels: both, the left alone, the right alone.
  4. The key's frequency at one level.
"""
import os, sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'scripts', 'nulltest'))   # scripts/nulltest/
import numpy as np
import refhost as R
import nulltest_lib as nl
import co_ref as H

FS = 48000
SET = {'Analog': 'Off', 'Threshold': -30, 'Ratio': 10}


def ref(x, key=None):
    """the reference's output for the main signal x and the key (None: the bus not fed), both mono or (2, n)"""
    return R.render_ref(nl.stereo(x), FS, H.REF_NAME, sc=None if key is None else nl.stereo(key), params=SET)[0]


def level(y, a, b):
    """rms level of the left channel between a and b seconds, dB"""
    return nl.rms_db(np.asarray(y)[0, int(a * FS):int(b * FS)])


def gain(y, x, a=2.0, b=3.0):
    return level(y, a, b) - level(nl.stereo(x), a, b)


def trace(y, x, win=0.005):
    """the gain every 5 ms: output level over input level, dB"""
    n = int(win * FS); m = min(np.asarray(y).shape[1], len(x)) // n
    e = lambda v: 10 * np.log10(np.maximum(np.mean(np.asarray(v, np.float64)[:m * n].reshape(m, n) ** 2, axis=1), 1e-30))
    return e(np.asarray(y)[0]) - e(x)


def main():
    tone = lambda db, f=1000.0: nl.tone(f, 3.0, db, fs=FS)
    levels = (-40, -30, -20, -10, -6)

    print('1. Steady 1 kHz tones, gain over the last second (dB). Rows: the main tone; columns: the key.')
    print('   %-12s %8s %8s' % ('main (dBFS)', 'no key', 'silent') + ''.join(' %8s' % ('key %d' % k) for k in levels))
    alone = {}
    for m in levels:
        x = tone(m)
        alone[m] = gain(ref(x), x)
        row = [alone[m], gain(ref(x, np.zeros_like(x)), x)] + [gain(ref(x, tone(k)), x) for k in levels]
        print('   %-12d' % m + ''.join(' %8.2f' % v for v in row), flush=True)
    silent = gain(ref(tone(-40), np.zeros(3 * FS, np.float32)), tone(-40))
    print('   without a key, a main tone at the key\'s level gets:' + ''.join(' %d: %.2f' % (k, alone[k]) for k in levels))
    print('   and with a silent key every main tone gets %.2f dB, the gain of no reduction.' % silent)

    print('2. Timing: a key stepping -50 / -10 / -30 dBFS, one second each, under a steady -20 dBFS main tone,')
    print('   against the same steps as the main signal without a key (the gain every 5 ms).')
    steps = nl.steps([-50, -10, -30], 1.0, fs=FS)
    main_tone = nl.tone(1000.0, 3.0, -20.0, fs=FS)[:len(steps)]
    keyed = trace(ref(main_tone, steps), main_tone)
    own = trace(ref(steps), steps)
    d = keyed - own
    skip = int(0.1 / 0.005)
    print('   difference of the two gain traces after the first 0.1 s: rms %.3f dB, largest %.3f dB (at %.3f s)'
          % (np.sqrt(np.mean(d[skip:] ** 2)), np.abs(d[skip:]).max(), (skip + int(np.argmax(np.abs(d[skip:])))) * 0.005))
    for t in (0.5, 1.02, 1.1, 1.5, 2.02, 2.1, 2.5, 2.95):
        i = int(t / 0.005)
        print('   t %.2f s: keyed %7.2f dB, as the main signal %7.2f dB' % (t, keyed[i], own[i]))

    print('3. The key\'s channels: a -10 dBFS key under a -20 dBFS main tone.')
    x = tone(-20); k = tone(-10); z = np.zeros_like(k)
    for label, key in (('both', np.stack([k, k])), ('left alone', np.stack([k, z])), ('right alone', np.stack([z, k]))):
        print('   %-12s gain %7.2f dB' % (label, gain(ref(x, key), x)), flush=True)

    print('4. The key\'s frequency: a -10 dBFS key under a -20 dBFS main tone of 1 kHz; beside it the gain a main tone')
    print('   of that frequency and level gets without a key.')
    for f in (50.0, 100.0, 300.0, 1000.0, 3000.0, 8000.0, 15000.0):
        a = tone(-10, f)
        print('   key %7.0f Hz: gain %7.2f dB   as the main signal %7.2f dB' % (f, gain(ref(x, a), x), gain(ref(a), a)), flush=True)


if __name__ == '__main__':
    main()
