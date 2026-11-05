"""Writes Flatwater Comp's trained gain correction (build/fwcompnet/<name>.pt, from nulltest/train_fwcomp.py) to
plugins/FwComp/FwCompNet.h as constant arrays. Run in the PyTorch environment.

    python tools/export_fwcompnet.py build/fwcompnet/fwcompnet_h48.pt
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


out = os.path.join(HERE, "..", "plugins", "FwComp", "FwCompNet.h")
with open(out, "w", newline="\n") as f:
    f.write("#pragma once\n// Learned correction of Flatwater Comp's gain (GRU, %d hidden units), trained on renders of the reference by\n"
            "// nulltest/train_fwcomp.py and written by tools/export_fwcompnet.py from %s. Do not edit by hand.\n"
            "// Inputs per step: see fwcomp::GainNet in FwCompDsp.h. Output: the correction in dB, 9 tanh (o / 9).\n"
            "// PyTorch GRU layout: rows [reset; update; new].\n\nnamespace fwcomp::gainnet\n{\n" % (H, os.path.basename(sys.argv[1])))
    f.write("inline constexpr int hidden = %d, inputs = %d;\n" % (H, I))
    f.write(arr("Wih", sd["gru.weight_ih_l0"]))
    f.write(arr("Whh", sd["gru.weight_hh_l0"]))
    f.write(arr("bih", sd["gru.bias_ih_l0"]))
    f.write(arr("bhh", sd["gru.bias_hh_l0"]))
    f.write(arr("Wout", sd["out.weight"]))
    f.write(arr("bout", sd["out.bias"]))
    f.write("}\n")
print("wrote", out, "hidden", H, "inputs", I)
