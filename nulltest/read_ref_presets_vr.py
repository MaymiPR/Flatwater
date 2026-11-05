"""Reads the reference Vocal Rider presets (the factory preset file of the local install, named by local.yaml's
FwVocalLeveler.ref_presets) and writes their stored arrays to build/ref_presets_VocalRider.json. The arrays are RealWorld values in the
ParamXML order of RESEARCH.md section 7 (0 Target, 1 Music Sensitivity, 2 / 3 Range Min / Max, 6 Vocal Sensitivity,
20 Output, 30 Automation or "*" (kept), 32 the Idle arrow's place in the range, 42 Attack; the Attack-tied hidden
values follow Attack). Nothing from the file is published; Flatwater Vocal Leveler presets are made from the values with new names
(tools/make_presets.py)."""
import os, re, json, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import local_config as L

SRC = L.get('FwVocalLeveler.ref_presets')
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "build", "ref_presets_VocalRider.json")
t = open(SRC, encoding="utf-8").read()
out = []
for m in re.finditer(r'<Preset [^>]*Name="([^"]*)".*?<Parameters Type="(\w+)">([^<]*)</Parameters>', t, re.S):
    assert m.group(2) == "RealWorld", m.group(1)
    out.append(dict(name=m.group(1), raw=m.group(3).split()))
json.dump(out, open(OUT, "w"), indent=1)
print("wrote", OUT, len(out))
