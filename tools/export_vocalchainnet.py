"""Writes Flatwater Vocal Chain's learned Compress correction (build/vocalchainnet/c<colour><tag>.pt, from
nulltest/train_vocalchain_compress.py) to plugins/FwVocalChain/FwVocalChainCompressNet.h as constant arrays: per colour one
net, or several whose corrections the plug-in averages (the same number for every colour). Run in the PyTorch
environment.

    python tools/export_vocalchainnet.py [tag]                 (one tag for all colours, default _r2)
    python tools/export_vocalchainnet.py push spank wall       (a tag per colour; tags joined by + are averaged,
                                                              e.g. _r2+_r4 _r2+_r4 _r2+_r3)
"""
import math
import os
import sys
import torch

HERE = os.path.dirname(os.path.abspath(__file__))
ARGS = sys.argv[1:] or ['_r2']
assert len(ARGS) in (1, 3), 'one tag, or one per colour'
TAGS = dict(zip((1, 2, 3), [a.split('+') for a in (ARGS * 3 if len(ARGS) == 1 else ARGS)]))
M = len(TAGS[1])
assert all(len(t) == M for t in TAGS.values()), 'the same number of nets per colour'
NAMES = {1: 'Push', 2: 'Spank', 3: 'Wall'}


def arr(name, v):
    v = [float(x) for x in v]
    body = ',\n    '.join(', '.join('%.9gf' % x for x in v[i:i + 8]) for i in range(0, len(v), 8))
    return 'inline constexpr float %s[%d] = {\n    %s\n};\n' % (name, len(v), body)


cks = {(c, m): torch.load(os.path.join(HERE, '..', 'build', 'vocalchainnet', 'c%d%s.pt' % (c, TAGS[c][m])), map_location='cpu')
       for c in (1, 2, 3) for m in range(M)}
H = cks[1, 0]['hidden']; I = cks[1, 0]['state']['gru.weight_ih_l0'].shape[1]
assert all(ck['hidden'] == H for ck in cks.values())
out = os.path.join(HERE, '..', 'plugins', 'FwVocalChain', 'FwVocalChainCompressNet.h')
with open(out, 'w', newline='\n') as f:
    f.write('#pragma once\n// Flatwater Vocal Chain\'s learned Compress correction (per colour %d GRU%s of %d hidden units, %s),\n'
            '// trained on renders of the reference by nulltest/train_vocalchain_compress.py and written by\n'
            '// tools/export_vocalchainnet.py from build/vocalchainnet/c<colour><tag>.pt (the tags below). Do not edit.\n'
            '// Inputs per 12 kHz step: see fwvocalchain::CompressNet in FwVocalChainDsp.h, normalised by mean and scale. Output:\n'
            '// cmax tanh (out), the ln correction of the compressor\'s output. PyTorch GRU layout: rows [reset; update; new].\n\n'
            'namespace fwvocalchain::compressnet\n{\n' % (M, 's' if M > 1 else '', H, 'their corrections averaged' if M > 1 else 'one net'))
    f.write('inline constexpr int hidden = %d, inputs = %d, members = %d;\n' % (H, I, M))
    f.write('inline constexpr double cmax = %.17g;\n' % math.log(10 ** (2.0 / 20)))
    for (c, m), ck in cks.items():
        sd = ck['state']; s = '%d_%d' % (c, m)
        f.write('\n// %s (c%d%s.pt, training step %d)\n' % (NAMES[c], c, TAGS[c][m], ck['step']))
        f.write(arr('mean' + s, ck['mean'])); f.write(arr('scale' + s, ck['std']))
        f.write(arr('Wih' + s, sd['gru.weight_ih_l0'].flatten())); f.write(arr('Whh' + s, sd['gru.weight_hh_l0'].flatten()))
        f.write(arr('bih' + s, sd['gru.bias_ih_l0'])); f.write(arr('bhh' + s, sd['gru.bias_hh_l0']))
        f.write(arr('Wout' + s, sd['out.weight'].flatten())); f.write(arr('bout' + s, sd['out.bias']))
    one = lambda s: '{ mean%s, scale%s, Wih%s, Whh%s, bih%s, bhh%s, Wout%s, bout%s }' % ((s,) * 8)
    f.write('\nstruct Net { const float *mean, *scale, *Wih, *Whh, *bih, *bhh, *Wout, *bout; };\n')
    f.write('inline constexpr Net nets[4][members] = { {}, %s };\n' % ', '.join(
        '{ %s }' % ', '.join(one('%d_%d' % (c, m)) for m in range(M)) for c in (1, 2, 3)))
    f.write('}\n')
print('wrote', out, 'hidden', H, 'inputs', I, 'members', M)
