"""Harness of Flatwater BassX Lift's null tests and measurements (nulltest/bx_ref.py): the reference through the suite's
host (scripts/nulltest/refhost.py) with pedalboard or tools/RefRender, our build and our renderer (BxRender). The
reference's component name comes from local.yaml (FwBassXLift.ref_name, its stereo component in the shell).

    import bx_ref as BX
    y = BX.render_ref(x, 48000, frequency=60, ratio=2.0)     # the reference, the rest at REF_DEFAULTS
    y = BX.render_vst3(x); y = BX.render_model(x)           # our built VST3, our renderer, at the defaults
"""
import os, sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'scripts', 'nulltest'))   # scripts/nulltest/
import numpy as np
import refhost as R


# ---- the hosts of first_null.py and nulltest.py --vst3 ----------------------------------------------------------------
import local_config_lib as L
PLUGIN, PRODUCT = 'FwBassXLift', 'Flatwater BassX Lift'
REF_NAME = L.get('FwBassXLift.ref_name', required=False)      # the reference's stereo component in the shell


def component():
    """the reference's stereo component (local.yaml's FwBassXLift.ref_name)"""
    if not REF_NAME:
        raise SystemExit("local.yaml: set FwBassXLift.ref_name (the reference's stereo component in the shell, as"
                         " RefRender --list prints it)")
    return REF_NAME


_BUILD = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'build')
VST3 = os.path.join(_BUILD, PLUGIN + '_artefacts', 'Release', 'VST3', PRODUCT + '.vst3')
RENDER = os.path.join(_BUILD, 'BxRender_artefacts', 'Release', 'BxRender.exe')


def render_ref_host(x, fs=48000, params=None, extra=()):
    """The reference through tools/RefRender (latency removed); params by display text, 'Name:' for a normalised value"""
    return R.render_ref(x, fs, component(), params=params, extra=extra)[0]


def render_vst3(x, fs=48000, params=None, extra=()):
    """our built VST3 through tools/RefRender"""
    return R.render_ref(x, fs, PRODUCT, params=params, extra=extra, vst3=VST3)[0]


def render_model(x, fs=48000):
    """our processor through BxRender at the plug-in's defaults"""
    return R.render_exe(RENDER, x, fs)


# ---- the reference: every control is a VST3 parameter; pedalboard snaps values to the plug-in's own steps ---------
REF_DEFAULTS = dict(input_gain=0.0, orig_bass_gain=0.0, harmonics_gain=float("-inf"), ratio=1.0, response=20.0,
                    harmonics_decay=-15.0, highpass=1, frequency=80.0, monitor=0)


def set_ref(p, **kw):
    """the parameters (REF_DEFAULTS for the rest); the choices (High-pass, Monitor) by their index"""
    v = {**REF_DEFAULTS, **kw}
    for k in ("input_gain", "orig_bass_gain", "harmonics_gain", "harmonics_decay", "response", "frequency"):
        setattr(p, k, float(v[k]))
    p.ratio = '%.2f:1 ' % float(v["ratio"])
    p.highpass = p.parameters['highpass'].valid_values[int(v["highpass"])]
    p.monitor = p.parameters['monitor'].valid_values[int(v["monitor"])]
    return v


def render_ref(x, fs, name=None, buffer=512, **kw):
    """Render through the reference with the given parameters (the rest at their defaults)."""
    name = name or component()
    p = R.load(name)
    set_ref(p, **kw)
    return R.render_with(p, x, fs, buffer, stereo=name.endswith('Stereo'))
