"""Reads the reference's presets (the factory preset file of the local install, named by local.yaml's FwComp.ref_presets) and
writes their settings, converted by the reference itself, to build/ref_presets_FwComp.json: each preset array is
written into the loaded plug-in's state and the plug-in's own parameter values are read back. Nothing from the file is
published; Flatwater Comp presets are made from the values with new names (scripts/make_presets.py)."""
import sys, os, re, json
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'scripts', 'nulltest'))   # scripts/nulltest/
import local_config_lib as L
import refhost as R
import ref_state_lib as W

SRC = L.get('FwComp.ref_presets')
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "build", "ref_presets_FwComp.json")
PARAMS = ("threshold", "ratio", "punch", "mix", "output", "attack", "release", "releasebpm", "sync", "analog", "limiter", "meter_select")


def num(t):
    t = t.strip().split()[0] if t.strip() else t
    try: return float(t)
    except ValueError: return t.strip()


t = open(SRC, encoding="utf-8").read()
p = R.load(L.ref_component('FwComp'))
base = p.raw_state
inner0 = W.get_inner(base).decode("latin-1")
assert re.search(r'<Parameters Type="RealWorld">', inner0), inner0[:400]
out = []
for m in re.finditer(r'<Preset [^>]*Name="([^"]*)".*?<Parameters Type="(\w+)">([^<]*)</Parameters>', t, re.S):
    name, kind, vals = m.group(1), m.group(2), m.group(3).split()
    done = None
    for pad in range(16):
        text = ' '.join(vals) + ' ' * (1 + pad)
        inner = re.sub(r'<Parameters Type="RealWorld">([^<]*)</Parameters>', lambda q: '<Parameters Type="%s">%s</Parameters>' % (kind, text), inner0)
        p.raw_state = W.set_inner(base, inner.encode("latin-1"))
        got = W.get_values(p.raw_state)
        if got is not None and (got != W.get_values(base) or not out):
            done = got; break
    if done is None:
        print("%-32s NOT taken by the reference" % name, flush=True)
        out.append(dict(name=name, raw=vals, settings=None)); continue
    st = {k: p.parameters[k].string_value.strip() for k in PARAMS}
    out.append(dict(name=name, raw=vals, strings=st, settings={k: num(v) for k, v in st.items()}))
    print("%-32s %s" % (name, st), flush=True)
    p.raw_state = base
json.dump(out, open(OUT, "w"), indent=1)
print("wrote", OUT, len(out))
