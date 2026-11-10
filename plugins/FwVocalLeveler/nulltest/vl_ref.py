"""Harness of Flatwater Vocal Leveler's null tests and measurements (nulltest/vl_ref.py): the reference through the suite's host
(scripts/nulltest/refhost.py) with pedalboard or tools/RefRender, our build and our renderer (VlRender); the components (Stereo, Mono, Live Stereo, Live Mono) under COMPONENTS.
The reference's component names come from local.yaml (FwVocalLeveler.ref_name, its stereo component) or the defaults below.

    import vl_ref as H
    y = H.render_vst3(x); y = H.render_model(x)          # our built VST3, our renderer, at the defaults
    p = H.load('Mono')                                   # a fresh pedalboard instance of one component
"""
import os, sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'scripts', 'nulltest'))   # scripts/nulltest/
import numpy as np
import refhost as R


# ---- the hosts of first_null.py and nulltest.py --vst3 ----------------------------------------------------------------
import local_config_lib as L
PLUGIN, PRODUCT = 'FwVocalLeveler', 'Flatwater Vocal Leveler'
REF_NAME = L.get('FwVocalLeveler.ref_name', required=False) or 'Vocal Rider Stereo'      # the reference's stereo component in the shell
_BUILD = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'build')
VST3 = os.path.join(_BUILD, PLUGIN + '_artefacts', 'Release', 'VST3', PRODUCT + '.vst3')
RENDER = os.path.join(_BUILD, 'VlRender_artefacts', 'Release', 'VlRender.exe')


def render_ref_host(x, fs=48000, params=None, extra=()):
    """The reference through tools/RefRender (latency removed); params by display text, 'Name:' for a normalised value"""
    return R.render_ref(x, fs, REF_NAME, params=params, extra=extra)[0]


def render_vst3(x, fs=48000, params=None, extra=()):
    """our built VST3 through tools/RefRender"""
    return R.render_ref(x, fs, PRODUCT, params=params, extra=extra, vst3=VST3)[0]


def render_model(x, fs=48000, **keys):
    """our processor (keys: renderer keys) through VlRender at the plug-in's defaults"""
    return R.render_exe(RENDER, x, fs, **keys)


_BASE = REF_NAME.rsplit(' ', 1)[0]
COMPONENTS = {k: _BASE + ' ' + k for k in ('Stereo', 'Mono', 'Live Stereo', 'Live Mono')}


def load(kind='Stereo'):
    """a fresh instance of the component kind (Stereo, Mono, Live Stereo, Live Mono)"""
    R.fresh(COMPONENTS[kind])
    return R.load(COMPONENTS[kind])
