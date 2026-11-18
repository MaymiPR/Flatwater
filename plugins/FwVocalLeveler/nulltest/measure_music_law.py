"""Measurement: the law of the music on the reference's side-chain bus (MEASUREMENTS.md section 4). The reference
renders through tools/RefRender, which enables the bus and feeds it (sc=). The voice is a 1 kHz sine, which the ride's
law of section 3 gives exactly; the music is a tone, noise, tone bursts or two tones, the same on both channels unless
said. From the settled gain g the music's term is read as

    E = (v level / c)^2 - Target^2,    v = g - gMin / 2,    c = 0.738 (section 3),

in dB (10 log10 E, powers re full scale): what the music adds to the Target's power.

    python -I measure_music_law.py [law] [weight] [time] [kinds] [--save]

  law     E for the music's level, at several voice levels, Targets and ranges
  weight  E by the music's frequency, and by Music Sensitivity
  time    the gain over time while the music steps up and down
  kinds   how noise, bursts and two tones are read against a steady tone of the same rms level
With no part named, all four. --save writes measurements/FwVocalLeveler/music_law.json. Renders are kept under
nulltest/renders/music (git-ignored), so a second run renders nothing.
"""
import os, sys, json, hashlib
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.append(HERE)
sys.path.append(os.path.join(HERE, '..', '..', '..', 'scripts', 'nulltest'))   # scripts/nulltest/
import numpy as np
import refhost as R
import nulltest_lib as nl
import vl_ref as H

FS = 48000
C = 0.738
CACHE = os.path.join(HERE, 'renders', 'music')
OUT = os.path.join(HERE, '..', '..', '..', 'measurements', 'FwVocalLeveler', 'music_law.json')


def sine(f, secs, rms_db):
    t = np.arange(int(secs * FS)) / FS
    return (np.sqrt(2.0) * 10 ** (rms_db / 20) * np.sin(2 * np.pi * f * t)).astype(np.float32)


def ref(x, music, params, tag):
    """the reference's output for the voice x and the music (None: the bus not fed), cached by its inputs"""
    os.makedirs(CACHE, exist_ok=True)
    key = hashlib.sha1((tag + repr(sorted(params.items()))).encode() + np.asarray(x, np.float32).tobytes()
                       + (b'' if music is None else np.asarray(music, np.float32).tobytes())).hexdigest()[:16]
    f = os.path.join(CACHE, key + '.npy')
    if os.path.exists(f): return np.load(f)
    y = R.render_ref(nl.stereo(x), FS, H.REF_NAME, sc=None if music is None else nl.stereo(music), params=params)[0]
    np.save(f, y)
    return y


def gain_db(y, x, a, b):
    s = slice(int(a * FS), int(b * FS))
    return nl.rms_db(np.asarray(y)[0, s]) - nl.rms_db(np.asarray(x)[s])


def term(g_db, voice_db, target_db, range_min):
    g = 10 ** (g_db / 20); v = g - 0.5 * 10 ** (range_min / 20)
    e = (v * 10 ** (voice_db / 20) / C) ** 2 - 10 ** (target_db / 10)
    return 10 * np.log10(e) if e > 0 else float('nan')


def settled(voice_db, music_f, music_db, sens='0', target=-21.0, rmin=-12.0, rmax=12.0, secs=16.0):
    """the gain over the last two seconds, and its drift against the two seconds before"""
    x = sine(1000.0, secs, voice_db)
    m = None if music_db is None else sine(music_f, secs, music_db)
    p = {'Target': target, 'Range Min': rmin, 'Range Max': rmax}
    if sens == 'Off': p['Music Sensitivity:'] = 0          # (the bottom of its range; the text does not parse. A choice such
                                                           # as Attack is set by its normalised value too: its text is not taken)
    else: p['Music Sensitivity'] = sens
    y = ref(x, m, p, 'settled')
    g1, g2 = gain_db(y, x, secs - 4, secs - 2), gain_db(y, x, secs - 2, secs)
    return g2, g2 - g1


def kinds(secs=20.0):
    """the music signals of the kinds part, each at -12 dBFS rms while it sounds"""
    n = int(secs * FS); t = np.arange(n) / FS
    tone = sine(1000.0, secs, -12)
    burst = lambda on, off: tone * ((t % (on + off)) < on).astype(np.float32)
    return {'tone 1 kHz': tone,
            'pink noise': nl.pink(secs, -12.0, fs=FS, seed=4), 'white noise': nl.noise(secs, -12.0, fs=FS, seed=2),
            'bursts 10 ms on, 90 off': burst(0.010, 0.090), 'bursts 50 ms on, 50 off': burst(0.050, 0.050),
            'bursts 100 ms on, 400 off': burst(0.100, 0.400), 'bursts 500 ms on, 1500 off': burst(0.5, 1.5),
            'two tones 1000 + 1100 Hz': sine(1000.0, secs, -15) + sine(1100.0, secs, -15),
            'two tones 1000 + 1010 Hz': sine(1000.0, secs, -15) + sine(1010.0, secs, -15),
            'tone 1 kHz, left channel alone': np.stack([tone, np.zeros_like(tone)])}


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    parts = args or ['law', 'weight', 'time', 'kinds']
    out = {}
    if 'law' in parts:
        print('law: the music a 1 kHz tone, Music Sensitivity 0. Gain (dB), its drift over the last 2 s, E (dB), E less the music\'s rms level')
        g0, d0 = settled(-21, 1000, None)
        print('   no music: gain %.3f dB (drift %.3f)' % (g0, d0))
        rows = []
        for vdb, tdb, rmin, rmax in ((-21, -21, -12.0, 12.0), (-15, -21, -12.0, 12.0), (-27, -21, -12.0, 12.0), (-21, -12, -12.0, 12.0),
                                     (-21, -30, -12.0, 12.0), (-21, -21, -6.0, 6.0)):
            for mdb in ((None, -50, -40, -30, -24, -18, -12, -6, 0) if (vdb, tdb, rmin) == (-21, -21, -12.0) else (None, -18, -6)):
                g, d = settled(vdb, 1000, mdb, target=tdb, rmin=rmin, rmax=rmax)
                e = term(g, vdb, tdb, rmin) if mdb is not None else float('nan')
                rows.append(dict(voice=vdb, target=tdb, range_min=rmin, range_max=rmax, music=mdb, gain=round(g, 3), drift=round(d, 3),
                                 e=None if np.isnan(e) else round(e, 2)))
                print('   voice %4d Target %4d range %+3.0f..%+3.0f music %5s: gain %7.3f (drift %6.3f)  E %7.2f  E - music %6.2f'
                      % (vdb, tdb, rmin, rmax, mdb, g, d, e, e - mdb if mdb is not None else float('nan')), flush=True)
        out['law'] = rows
    if 'weight' in parts:
        print('weight: the music a tone at -12 dBFS rms, by frequency: E re 1 kHz (dB)')
        e1 = term(settled(-21, 1000, -12)[0], -21, -21, -12)
        rows = []
        for f in (50, 100, 200, 500, 2000, 4000, 6500, 10000, 16000):
            g, d = settled(-21, f, -12)
            rows.append(dict(hz=f, gain=round(g, 3), e_re_1k=round(term(g, -21, -21, -12) - e1, 2)))
            print('   %6d Hz: gain %7.3f  E re 1 kHz %6.2f' % (f, g, rows[-1]['e_re_1k']), flush=True)
        out['weight'] = rows
        print('Music Sensitivity, the music a 1 kHz tone at -18 dBFS rms: E re Sensitivity 0 (dB)')
        e0 = term(settled(-21, 1000, -18)[0], -21, -21, -12)
        rows = []
        for s in ('Off', '-12', '-6', '-3', '3', '6', '12'):
            g, d = settled(-21, 1000, -18, sens=s)
            e = term(g, -21, -21, -12)
            rows.append(dict(sensitivity=s, gain=round(g, 3), e_re_0=None if np.isnan(e) else round(e - e0, 2)))
            print('   Sensitivity %4s: gain %7.3f  E re 0: %6.2f' % (s, g, e - e0), flush=True)
        out['sensitivity'] = rows
    if 'time' in parts:
        print('time: the voice steady; the music a 1 kHz tone at -40 dBFS rms, at -6 from 4 s to 10 s; the gain per 50 ms')
        secs = 18.0
        x = sine(1000.0, secs, -21)
        m = sine(1000.0, secs, -40); m[int(4 * FS):int(10 * FS)] *= 10 ** (34 / 20)
        for att in ('Slow', 'Fast'):
            y = ref(x, m, {'Target': -21.0, 'Range Min': -12.0, 'Range Max': 12.0, 'Music Sensitivity': '0', 'Attack:': 1 if att == 'Fast' else 0}, 'time')
            tr = [round(float(gain_db(y, x, k * 0.05, (k + 1) * 0.05)), 3) for k in range(int(secs / 0.05))]
            out['time_' + att.lower()] = tr
            print('   Attack %s: ' % att + ', '.join('%.2f s %.3f' % (t, tr[int(round(t / 0.05))]) for t in
                  (3.9, 4.0, 4.05, 4.1, 4.2, 4.3, 4.5, 5.0, 5.5, 9.9, 10.0, 10.5, 11.0, 12.0, 14.0, 16.0, 17.9)), flush=True)
    if 'kinds' in parts:
        print('kinds: E over the last 6 s of 20 s (dB), its lowest and highest 0.1 s, and its mean re the steady tone')
        secs = 20.0
        x = sine(1000.0, secs, -21)
        p = {'Target': -21.0, 'Range Min': -12.0, 'Range Max': 12.0, 'Music Sensitivity': '0'}
        rows, tone_mean = [], None
        for name, m in kinds(secs).items():
            y = ref(x, m, p, 'kind')
            tail = np.array([term(gain_db(y, x, k * 0.1, (k + 1) * 0.1), -21, -21, -12) for k in range(140, int(secs / 0.1))])
            mean = float(np.nanmean(tail))
            tone_mean = mean if tone_mean is None else tone_mean
            rows.append(dict(music=name, e=round(mean, 2), e_min=round(float(np.nanmin(tail)), 2), e_max=round(float(np.nanmax(tail)), 2),
                             re_tone=round(mean - tone_mean, 2)))
            print('   %-32s E %7.2f  (%7.2f .. %7.2f)  re the tone %6.2f' % (name, mean, rows[-1]['e_min'], rows[-1]['e_max'], rows[-1]['re_tone']), flush=True)
        out['kinds'] = rows
    if '--save' in sys.argv:
        old = json.load(open(OUT)) if os.path.exists(OUT) else {}
        old.update(out)
        with open(OUT, 'w', newline='\n') as f:
            json.dump(old, f, indent=1); f.write('\n')
        print('wrote', os.path.relpath(OUT, os.path.join(HERE, '..', '..', '..')))


if __name__ == '__main__':
    main()
