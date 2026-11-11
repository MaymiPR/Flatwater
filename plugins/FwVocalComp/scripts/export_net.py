"""Writes the trained Flatwater Vocal Comp slow-path network (build/fwvocalcompnet/<name>.pt, from plugins/FwVocalComp/nulltest/train_net.py with
BASE=analytical) to plugins/FwVocalComp/Source/dsp/Net.h as constant arrays. Run in the PyTorch environment.

    python plugins/FwVocalComp/scripts/export_net.py build/fwvocalcompnet/fwvocalcompnet_h48r5_last.pt
"""
import sys, os
import torch

HERE = os.path.dirname(os.path.abspath(__file__))
sd = torch.load(sys.argv[1], map_location="cpu")
H = sd["gru.weight_hh_l0"].shape[1]; I = sd["gru.weight_ih_l0"].shape[1]


def arr(name, t):
    v = t.detach().double().flatten().tolist()
    body = ",\n    ".join(", ".join("%.9gf" % x for x in v[i:i + 8]) for i in range(0, len(v), 8))
    return "inline constexpr float %s[%d] = {\n    %s\n};\n" % (name, len(v), body)


out = os.path.join(HERE, "..", "..", "..", "plugins", "FwVocalComp", "Source", "dsp", "Net.h")
with open(out, "w", newline="\n") as f:
    f.write("#pragma once\n// Learned correction of the Flatwater Vocal Comp slow path (GRU, %d hidden units), trained on renders of the reference by\n"
            "// plugins/FwVocalComp/nulltest/train_net.py and written by plugins/FwVocalComp/scripts/export_net.py from %s. Do not edit by hand.\n"
            "// Inputs per 12 kHz step: see vc::SlowNet in Engine.h. Output: ln correction of the slow ratio.\n"
            "// PyTorch GRU layout: rows [reset; update; new].\n\nnamespace vc::dsp::slownet\n{\n" % (H, os.path.basename(sys.argv[1])))
    f.write("inline constexpr int hidden = %d, inputs = %d;\n" % (H, I))
    f.write(arr("Wih", sd["gru.weight_ih_l0"]))
    f.write(arr("Whh", sd["gru.weight_hh_l0"]))
    f.write(arr("bih", sd["gru.bias_ih_l0"]))
    f.write(arr("bhh", sd["gru.bias_hh_l0"]))
    f.write(arr("Wout", sd["out.weight"]))
    f.write(arr("bout", sd["out.bias"]))
    f.write("}\n")
print("wrote", out, "hidden", H, "inputs", I)
