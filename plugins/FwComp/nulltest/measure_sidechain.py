"""Measurement: what a signal on the reference's side-chain bus (the key) does to its gain. The reference renders
through tools/RefRender, which enables the bus and feeds it (sc=); pedalboard cannot. Analog Off, Threshold -30,
Ratio 10, the rest at the defaults. Prints eight tables; nothing is saved.

    python -I measure_sidechain.py

  1. Steady tones: the gain on a main tone for each key level, beside the gain the same main tone gets without a key
     when it has the key's level. Equal columns mean the detector reads the key and nothing of the main signal.
  2. Timing: the gain over time while the key steps in level under a steady main tone, against the gain the same
     steps get as the main signal without a key.
  3. The key's channels: both, the left alone, the right alone.
  4. The key's frequency at one level.
  5. Timing at the sample: the alternating signal +a, -a is a constant level to a peak detector, so the gain is y / x
     at every sample; the gain under a stepping key against the gain the same steps get as the main signal.
  6. The limiter: with no compression and Output +6, which signal the output stage reads.
  7. Mix 50 with a key: the dry part is the main signal.
  8. 96 kHz: the steady gains again.
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

    alt = lambda db, m: (10 ** (db / 20) * np.where(np.arange(m) % 2 == 0, 1.0, -1.0)).astype(np.float32)
    gdb = lambda y, x: 20 * np.log10(np.maximum(np.abs(y), 1e-12) / np.maximum(np.abs(x), 1e-12))
    host = lambda x, key, fs=FS, **p: np.asarray(R.render_ref(nl.stereo(x), fs, H.REF_NAME, sc=None if key is None else nl.stereo(key),
                                                             params={**SET, **p})[0])[0]
    n = 3 * FS
    steady = lambda y, x, fs=FS: float(np.mean(gdb(y[2 * fs:len(x) - 200], x[2 * fs:len(x) - 200])))

    print('5. Timing at the sample: a key stepping -50 / -10 / -50 dBFS, one second each, under a steady -20 dBFS main')
    print('   signal, against the same steps as the main signal without a key.')
    step = np.concatenate([alt(-50, FS), alt(-10, FS), alt(-50, FS)]); x = alt(-20, n)
    for label, p in (('the defaults', {}), ('Attack 1, Punch 15', {'Attack': 1, 'Punch': 15}),
                     ('Attack 30, Release 500, Punch 0', {'Attack': 30, 'Release': 500, 'Punch': 0})):
        yk, yo = host(x, step, **p), host(step, None, **p)
        m = min(len(yk), len(yo), n); a = int(0.1 * FS)
        d = (gdb(yk[:m], x[:m]) - gdb(yo[:m], step[:m]))[a:]
        print('   %-34s largest difference of the two gains %.4f dB (rms %.4f)' % (label, np.abs(d).max(), np.sqrt(np.mean(d ** 2))), flush=True)

    print('6. The limiter: Threshold 0, Ratio 1 (no compression), Output +6. Output peak (dBFS) and gain (dB).')
    flat = {'Threshold': 0, 'Ratio': 1, 'Output': 6}
    for lim in ('Off', 'On'):
        for label, m_db, k_db in (('main 0 dBFS, no key', 0, None), ('main 0 dBFS, key -20 dBFS', 0, -20), ('main -20 dBFS, key 0 dBFS', -20, 0)):
            x = alt(m_db, n)
            y = host(x, None if k_db is None else alt(k_db, n), Limiter=lim, **flat)
            print('   Limiter %-3s %-27s peak %6.2f, gain %6.2f' % (lim, label, 20 * np.log10(np.abs(y[FS:n - 200]).max() + 1e-12), steady(y, x)), flush=True)

    x = alt(-20, n); k = alt(-10, n)
    print('7. Mix with a -10 dBFS key under a -20 dBFS main signal: gain %.2f dB at Mix 100, %.2f dB at Mix 50.'
          % (steady(host(x, k, Mix=100), x), steady(host(x, k, Mix=50), x)), flush=True)

    print('8. 96 kHz, a -20 dBFS main signal: gain', end='')
    n2 = 3 * 96000; x = alt(-20, n2)
    for label, key in (('a silent key', np.zeros(n2, np.float32)), ('key -20 dBFS', alt(-20, n2)), ('key -10 dBFS', alt(-10, n2))):
        print(' %.2f dB with %s%s' % (steady(host(x, key, fs=96000), x, 96000), label, '.' if label.endswith('-10 dBFS') else ','), end='', flush=True)
    print()


if __name__ == '__main__':
    main()
