"""Reads the reference's factory and artist presets (the preset files of Flatwater Bass Lift's and Flatwater Vocal
Comp's references in the local install, each in the folder its local.yaml key <Id>.ref_presets names) and writes their
parameter values, converted to real-world units, to build/ref_presets_<plugin id>.json.

    python -I read_ref_presets.py [FwBassLift] [FwVocalComp]

Presets stored as "RealWorld" are read directly. Presets stored as "States" (0..1000) are converted by the reference
itself: the array is written into the loaded plugin's state with Type="States" and read back as RealWorld, so the
plugin's own scale handling (logarithmic Freq, toggles) is used. Nothing from these files is published; FwBassLift/FwVocalComp
presets are made from the values with new names (scripts/make_presets.py)."""
import sys, os, re, json, glob
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import refhost as R
import ref_state_lib as W
import local_config_lib as L

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "build")
PLUGINS = ("FwBassLift", "FwVocalComp")


def parse(path):
    t = open(path, encoding="utf-8", errors="replace").read()
    name = re.search(r'<Preset[^>]*Name="([^"]*)"', t).group(1)
    m = re.search(r'<Parameters Type="(\w+)">([^<]*)</Parameters>', t)
    return name, m.group(1), m.group(2).split()


# The reference ParamXML scales (all linear): min, max, resolution. The plug-in's own conversion of the two the reference "States"
# artist presets (which it accepts) matches this formula exactly; the factory ones it rejects are converted with it.
VOCAL_COMP_SCALES = [(-36, 0, 0.1), (-80, 0, 0.1), (10, 250, 0.1), None, None, None, None, (-36, 0, 0.1)]


def vocal_comp_states(vals):
    out = []
    for i, v in enumerate(vals):
        sc = VOCAL_COMP_SCALES[i] if i < len(VOCAL_COMP_SCALES) else None
        if sc is None or v == '*':
            out.append('*'); continue
        lo, hi, res = sc
        out.append('%g' % (round((lo + (hi - lo) * float(v) / 1000.0) / res) * res))
    return out


def to_realworld(plugin, base, kind, vals):
    if kind == "RealWorld":
        return vals
    inner0 = W.get_inner(base).decode('latin-1')
    for pad in range(16):
        text = ' '.join(vals) + ' ' * (1 + pad)
        inner = re.sub(r'<Parameters Type="RealWorld">([^<]*)</Parameters>',
                       lambda m: '<Parameters Type="States">' + text + '</Parameters>', inner0)
        plugin.raw_state = W.set_inner(base, inner.encode('latin-1'))
        got = W.get_values(plugin.raw_state)
        before = W.get_values(base)
        if got is not None and got != before:
            return got
    raise RuntimeError("the plugin did not take the States array %s" % vals)


only = sys.argv[1:] or list(PLUGINS)
assert vocal_comp_states("836 893 500 0 0 0 0 1000".split())[:3] == ['-5.9', '-8.6', '130'], vocal_comp_states("836 893 500".split())
for plugin in PLUGINS:
    if plugin not in only: continue
    folder = L.get(plugin + '.ref_presets')
    p = R.load(L.ref_component(plugin))
    base = p.raw_state
    out = []
    for path in sorted(glob.glob(os.path.join(folder, "**", "*.xps"), recursive=True)):
        rel = os.path.relpath(path, folder)
        name, kind, vals = parse(path)
        if plugin == "FwVocalComp" and kind == "States":
            rw = vocal_comp_states(vals)
        else:
            rw = to_realworld(p, base, kind, vals)
        p.raw_state = base
        out.append({"file": rel, "name": name, "kind": kind, "raw": vals, "realworld": rw})
        print("%-55s %-9s %s" % (rel, kind, " ".join(rw)), flush=True)
    json.dump(out, open(os.path.join(OUT, "ref_presets_%s.json" % plugin), "w"), indent=1)
