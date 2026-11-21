"""Null grid of Flatwater Vocal Comp: every combination of Compression, Gate, Gain and an input trim on four signals,
our renderer against the reference (vc_ref.py's two renders), residual in dB.

    python -I null_grid.py [--save]      renders the grid; --save writes measurements/FwVocalComp/nulltests-grid.json and
                                         the Grid section of NULLTESTS.md
    python -I null_grid.py --report      the Grid section again from that JSON, rendering nothing

Residual = 20 log10( rms(ours - reference) / rms(reference) ), 48 kHz, after the first 0.2 s. Numbers are reported as
they come: no case is labelled a null unless the data shows it.
"""
import os, sys, json
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.append(HERE)
sys.path.append(os.path.join(HERE, '..', '..', '..', 'scripts', 'nulltest'))   # scripts/nulltest/
import numpy as np

COMP = [0, -6, -12, -18, -24, -30, -36]
GATE = ['off', -60, -40, -20]
GAIN = [0, -12]
TRIM = [-24, -12, 0]
SIGNALS = {'voice-like': 'voice-like', 'white noise': 'white noise -20 rms', 'tone steps': 'tone steps -50/-10/-30',
           'sine 1 kHz': 'sine 1 kHz -20 dBFS'}                       # (the column, the signal of vc_ref.signals)
DATA = os.path.join(HERE, '..', '..', '..', 'measurements', 'FwVocalComp', 'nulltests-grid.json')


def run():
    import vc_ref as N
    S = N.signals()
    rows = []
    for col, key in SIGNALS.items():
        for trim in TRIM:
            x = (S[key] * 10 ** (trim / 20)).astype(np.float32)
            for comp in COMP:
                for gate in GATE:
                    for gain in GAIN:
                        g = None if gate == 'off' else gate
                        ref = N.render_ref(x, N.FS, comp=comp, gate=g, gain=gain)[0]
                        mine = N.ours(x, N.FS, comp, g, gain)[0]
                        rows.append(dict(signal=col, trim=trim, comp=comp, gate=gate, gain=gain,
                                         residual_db=round(float(N.residual_db(mine, ref, int(0.2 * N.FS))), 1)))
            print('%-12s trim %4d: median %.1f dB' % (col, trim, np.median([r['residual_db'] for r in rows if r['signal'] == col and r['trim'] == trim])), flush=True)
    return rows


def breakdown(rows):
    v = np.array([r['residual_db'] for r in rows])
    L = ['Every combination of Compression %s, Gate %s, Gain %s and an input trim of %s dB, for %d signals (%d cases). '
         'Residual = 20 log10(rms(FwVocalComp - reference) / rms(reference)), 48 kHz, after 0.2 s. Cells give **median / worst** in dB; '
         'lower is better, -60 dB or less is a null. Produced by `plugins/FwVocalComp/nulltest/null_grid.py` '
         '(`measurements/FwVocalComp/nulltests-grid.json`).' % (COMP, GATE, GAIN, TRIM, len(SIGNALS), len(rows)), '']
    for title, head, key, values in (('By Compression', 'Comp (dB)', 'comp', COMP), ('By Gate', 'Gate (dB)', 'gate', GATE),
                                     ('By Gain', 'Gain (dB)', 'gain', GAIN), ('By input level', 'Input trim (dB)', 'trim', TRIM)):
        L += ['## ' + title, '', '| %s | ' % head + ' | '.join(SIGNALS) + ' |', '|---|' + '---:|' * len(SIGNALS)]
        for val in values:
            cells = []
            for col in SIGNALS:
                w = np.array([r['residual_db'] for r in rows if r[key] == val and r['signal'] == col])
                cells.append('%.1f / %.1f' % (np.median(w), w.max()))
            L.append('| %s | ' % val + ' | '.join(cells) + ' |')
        L.append('')
    L.append('Overall: median %.1f dB, worst %.1f dB, %d of %d cases below -60 dB.' % (np.median(v), v.max(), (v < -60).sum(), len(v)))
    return L


def main():
    args = sys.argv[1:]
    if '--report' in args:
        rows = json.load(open(DATA, encoding='utf-8'))
    else:
        rows = run()
        if '--save' in args: json.dump(rows, open(DATA, 'w', encoding='utf-8', newline='\n'), indent=0)
    L = breakdown(rows)
    print(L[-1])
    if '--save' in args or '--report' in args:
        import report_lib as RL
        print('wrote', RL.save('FwVocalComp', 'grid', L, section='Grid', args=['--save']))


if __name__ == '__main__':
    main()
