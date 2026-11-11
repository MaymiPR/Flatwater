"""Writes the note detector's fitted constants (measurements/FwBassLeveler/detector_fit.json, from nulltest/fit_detector.py)
into the plug-in's constants (plugins/FwBassLeveler/Source/dsp/Tuning.h), each field's default replaced in place, so the
plug-in rides as the fitted model does.

    python -I export_detector.py
"""
import os, re, json

HERE = os.path.dirname(os.path.abspath(__file__))
FIT = os.path.join(HERE, '..', '..', '..', 'measurements', 'FwBassLeveler', 'detector_fit.json')
HDR = os.path.join(HERE, '..', 'Source', 'dsp', 'Tuning.h')

d = json.load(open(FIT))
g = d['globals']
sens = ['0', '25', '50', '75', '100']
scalars = {'slowAtt': g['slow_att'], 'slowRel': g['slow_rel'], 'refRel': g['ref_rel'], 'hyst': g['hyst'], 'relMs': g['rel_ms'],
           'holdMs': g['settle'], 'followAtt': g['f_att'], 'followRel': g['f_rel'], 'reRideLo': g['rr_lo1'],
           'reRideHi': g['rr_hi1'], 'offMs': g['off_ms'], 'offRelMs': g.get('off_rel_ms', 60.0), 'winMs': g['win_ms']}
arrays = {'onThr': [d['thr_u'][k] for k in sens], 'margin': [d['margin'][k] for k in sens],
          'onRise': [d['on_rise'][k] for k in sens], 'depthSlow': [d['rel_depth']['Slow ' + k] for k in sens],
          'depthFast': [d['rel_depth']['Fast ' + k] for k in sens]}
s = open(HDR, encoding='utf-8').read()
for name, v in scalars.items():
    s, n = re.subn(r'(\n    double %s = )[^;]*;' % name, lambda m: m.group(1) + '%.4f;' % v, s)
    assert n == 1, name
for name, vals in arrays.items():
    s, n = re.subn(r'(\n    double %s\[5\] )\{[^}]*\};' % name, lambda m: m.group(1) + '{ %s };' % ', '.join('%.4f' % x for x in vals), s)
    assert n == 1, name
open(HDR, 'w', encoding='utf-8', newline='\n').write(s)
print('\n'.join('%s = %.4f' % kv for kv in scalars.items()))
