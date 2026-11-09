"""Generates the factory preset tables of Flatwater Bass Lift, Flatwater Vocal Comp, Flatwater BassX Lift, Flatwater Comp, Flatwater Delay and Flatwater Bass Leveler (plugins/<P>/Source/Presets.h).

Input: build/ref_presets_RBass.json and build/ref_presets_RVox.json, written by scripts/nulltest/read_ref_presets.py, and
build/ref_presets_MaxxBass.json, written by plugins/FwBassXLift/nulltest/read_ref_presets.py, and build/ref_presets_HComp.json, written
by plugins/FwComp/nulltest/read_ref_presets.py, and build/ref_presets_HDelay.json, written by plugins/FwDelay/nulltest/read_ref_presets.py, and
build/ref_presets_BassRider.json, written by plugins/FwBassLeveler/nulltest/read_ref_presets.py,
from the reference presets of the local install (setting values only). Each table maps a preset's position in the
reference's preset file (or in its sorted preset folder), in the order the reader lists them, to our group and name; no
reference preset name or path is kept here. The preset names are our own: no artist names, no vendor product names,
no other trademarks; each one describes what the setting is for, so it stays recognisable. Groups: "Essentials" (the reference's factory set) and "Studio" (its artist set).

    python scripts/make_presets.py
"""
import json, os

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..")

# position in the reference's preset folder (read_ref_presets.py: the sorted file list) -> (group, our name); the
# positions left out are its reset presets
BASS_NAMES = {
    0: ('Studio', 'Electric Fiddle Low Lift'),
    1: ('Studio', 'Synth Bass Weight'),
    2: ('Studio', 'Vocal Bass Line'),
    3: ('Studio', 'Serious Kick'),
    4: ('Studio', 'Low Glue'),
    5: ('Studio', 'Heavy Low Line'),
    6: ('Studio', 'Kick Low End'),
    7: ('Studio', 'Stage Bass Round'),
    8: ('Studio', 'Bass Workstation'),
    9: ('Studio', 'Boom Sub'),
    10: ('Studio', 'Rock Bass Guitar'),
    11: ('Studio', 'Four-String Sub'),
    12: ('Studio', 'Trap Boom'),
    13: ('Studio', 'Sub Push'),
    14: ('Studio', 'Boom Fight'),
    15: ('Studio', 'Club System Low'),
    16: ('Studio', 'Pop Bass Tone'),
    17: ('Studio', 'Live Bass Warmth'),
    18: ('Studio', 'Poly Synth Octave'),
    19: ('Studio', 'Boom Low'),
    20: ('Studio', 'Low Bloom'),
    21: ('Studio', 'Sub Feed'),
    22: ('Studio', 'Tom Depth'),
    23: ('Studio', 'Drum Bus Weight'),
    24: ('Studio', 'Vintage Snare'),
    25: ('Studio', 'Near-Miked Hits'),
    26: ('Studio', 'Small Tablas'),
    27: ('Studio', 'Bigger Than It Is'),
    28: ('Studio', 'Dusty Loop Low End'),
    29: ('Studio', 'Bass Shape 1'),
    30: ('Studio', 'Bass Shape 2'),
    31: ('Studio', 'Bass Shape 3'),
    32: ('Studio', 'Door Knock'),
    33: ('Studio', 'Gunshot Low Thump'),
    34: ('Studio', 'Gunshot Mid Boost'),
    35: ('Studio', 'Score Low End'),
    36: ('Studio', 'Low Punch'),
    37: ('Essentials', 'Basic Bass'),
    38: ('Essentials', 'Deep Floor'),
    39: ('Essentials', 'Bass Bomb'),
    40: ('Essentials', 'Bottom Lift'),
    41: ('Essentials', 'Grit Bass'),
    42: ('Essentials', 'Upper Bass Lift'),
    43: ('Essentials', 'Earbud Weight'),
    44: ('Essentials', 'Low Drive'),
}
# the same for Flatwater Vocal Comp
VOCAL_NAMES = {
    0: ('Studio', 'Background Vocal'),
    1: ('Studio', 'Main Voice Control'),
    2: ('Studio', 'Gentle Lead'),
    3: ('Studio', 'Up Front Vocal'),
    4: ('Studio', 'Grounded Voice'),
    5: ('Studio', 'First Pass'),
    6: ('Studio', 'Soft Vocal Squeeze'),
    7: ('Studio', 'Snare Choke'),
    8: ('Studio', 'Steady Vocal'),
    9: ('Studio', 'Vocal Forward'),
    10: ('Studio', 'Breath Control'),
    11: ('Studio', 'Tracking Vocal'),
    12: ('Studio', 'Backing Stack'),
    13: ('Studio', 'Lead Up Front'),
    14: ('Studio', 'BGV Pile'),
    15: ('Studio', 'Light Touch'),
    16: ('Studio', 'Handheld Mic Hold'),
    17: ('Essentials', 'Heavy Squash Ungated'),
    18: ('Essentials', 'Stadium Announcer'),
    19: ('Essentials', 'Noisy Lecture Cleanup'),
    20: ('Essentials', 'Outdoor Reporter'),
}


# Flatwater BassX Lift: the reference's presets in the order of its preset file -> (group, our name). "Essentials": the
# reference's factory set, "Studio": its artist set.
MAXX_NAMES = [
    ('Essentials', 'Default'),
    ('Essentials', 'PA System'),
    ('Essentials', 'Deepest Reach'),
    ('Essentials', 'Subtle Lift'),
    ('Essentials', 'Even Lift'),
    ('Essentials', 'Full Lift'),
    ('Essentials', 'Desktop Speakers'),
    ('Essentials', 'Laptop Speakers'),
    ('Essentials', 'Deep Sub Push'),
    ('Essentials', 'Portable Radio'),
    ('Essentials', 'Home Stereo Polish'),
    ('Essentials', 'Cheap Speaker Polish'),
    ('Studio', 'Bass Mix Weight'),
    ('Studio', 'Bass Focus 1'),
    ('Studio', 'Bass Focus 2'),
    ('Studio', 'Bass Focus 3'),
    ('Studio', 'Vocal Warmth'),
    ('Studio', 'Mix Low Repair'),
    ('Studio', 'Electric Bass Body'),
    ('Studio', 'Low Stack'),
    ('Studio', 'Electric Bass Lift'),
    ('Studio', 'Kick Harmonics'),
]


# Flatwater Comp: the reference's presets in the order of its preset file -> (group, our name). "Studio": the
# reference's guest presets.
HCOMP_NAMES = [
    ('Essentials', 'Default'),
    ('Essentials', 'Live Sound Start'),
    ('Essentials', 'Kit Smack'),
    ('Essentials', 'Drum Body'),
    ('Essentials', 'Light Kit'),
    ('Essentials', 'Heavy Drums'),
    ('Essentials', 'Pumping Kit'),
    ('Essentials', 'Snug Drums'),
    ('Essentials', 'Round Kick'),
    ('Essentials', 'Transient Lift'),
    ('Essentials', 'Hard Squash'),
    ('Essentials', 'Firm Voice'),
    ('Essentials', 'Parallel Blend'),
    ('Essentials', 'Thick Bass Hold'),
    ('Essentials', 'Punchy Keys'),
    ('Essentials', 'Solo Guitar Sustain'),
    ('Essentials', 'Dirty Guitar Hold'),
    ('Essentials', 'Orchestral Section'),
    ('Essentials', 'Master Glue'),
    ('Studio', 'Gentle Snare'),
    ('Studio', 'Classic Kit Crush'),
    ('Studio', 'Drum Knockout'),
    ('Studio', 'Kit Bus'),
    ('Studio', 'Loud Band Voice'),
    ('Studio', 'Stage Drums Blend'),
    ('Studio', 'Lo-Fi Radio'),
]


def num(v, lo, hi, nd=2):
    x = float(v)
    if abs(x) < 1e-9: x = 0.0
    return round(min(hi, max(lo, x)), nd)


def bass_values(rw):
    # reference order: Input, Freq, Response, Ratio, Clip, Intensity, HiPass (1 = Original Bass out), Gain, Decay,
    # 4 meters, SmoothTc, Tc, XFadeTc, Smooth, (Skin)
    v = [("inGain", num(rw[0], -30, 0, 1)), ("freq", num(rw[1], 32, 256, 0)), ("response", num(rw[2], 0, 30, 1)),
         ("ratio", num(rw[3], 0.25, 1)), ("intensity", num(rw[5], -24, 24, 1)),
         ("origBass", 0.0 if float(rw[6]) > 0.5 else 1.0), ("gain", num(rw[7], -30, 0, 1)), ("decay", num(rw[8], 0, 1, 1))]
    for i, pid, lo, hi in ((13, "smoothTc", 0, 1000), (14, "tc", 0, 2000), (15, "xfadeTc", 0, 5000)):
        if i < len(rw) and rw[i] != '*': v.append((pid, num(rw[i], lo, hi, 1)))
    if len(rw) > 16 and rw[16] != '*': v.append(("smooth", 1.0 if float(rw[16]) > 0.5 else 0.0))
    return v


def vocal_values(rw):
    # reference order: Compression, Gate, GateRel, 4 meters, Gain
    return [("comp", num(rw[0], -36, 0, 1)), ("gate", num(rw[1], -80, 0, 1)), ("gateRel", num(rw[2], 10, 250, 1)),
            ("gain", num(rw[7], -36, 0, 1))]


def write(pid, product, src, names, conv):
    data = json.load(open(os.path.join(ROOT, "build", src)))
    missing = [k for k, d in enumerate(data) if k not in names and "reset" not in d["file"].lower()]
    assert not missing, missing
    rows = []
    for k, (group, name) in names.items():
        vals = conv(data[k]["realworld"])
        rows.append('    { "%s", "%s", { %s } },' % (group, name, ", ".join('{ "%s", %rf }' % (kv[0], float(kv[1])) for kv in vals)))
    rows.sort(key=lambda r: (0 if '"Essentials"' in r else 1, r))
    out = os.path.join(ROOT, "plugins", pid, "Source", "Presets.h")
    open(out, "w", newline="\n").write(
        "#pragma once\n// Factory presets of %s, generated by scripts/make_presets.py (do not edit by hand).\n"
        "// Setting values carried over from the reference's presets; names are Flatwater's own.\n"
        '#include "shared/Presets.h"\n\nnamespace %s\n{\ninline const std::vector<fw::FactoryPreset>& factoryPresets()\n{\n'
        "    static const std::vector<fw::FactoryPreset> list {\n%s\n    };\n    return list;\n}\n}\n"
        % (product, pid.lower(), "\n".join(rows)))
    print(out, len(rows), "presets")


NL = "\n"


def write_maxx():
    data = json.load(open(os.path.join(ROOT, "build", "ref_presets_MaxxBass.json")))
    assert len(data) == len(MAXX_NAMES) == 22, "preset list changed"
    db = lambda v, hi: -100.0 if v == float("-inf") or v <= -99.9 else num(v, -100, hi, 1)
    rows = []
    for k, d in enumerate(data):
        group, name = MAXX_NAMES[k]; s = d["settings"]
        vals = [("freq", num(s["frequency"], 32, 256, 0)), ("decay", num(s["harmonics_decay"], -24, -9, 1)),
                ("response", num(s["response"], 10, 30, 1)), ("ratio", num(s["ratio"], 1, 4, 2)), ("highpass", int(s["highpass"])),
                ("origBass", db(s["orig_bass_gain"], 12)), ("harmonics", db(s["harmonics_gain"], 12)), ("monitor", 0)]
        rows.append('    { "%s", "%s", { %s } },' % (group, name, ", ".join('{ "%s", %rf }' % (k, float(v)) for k, v in vals)))
    out = os.path.join(ROOT, "plugins", "FwBassXLift", "Source", "Presets.h")
    head = ("#pragma once" + NL + "// Factory presets of Flatwater BassX Lift, generated by scripts/make_presets.py (do not edit by hand)." + NL
            + "// Setting values carried over from the reference's presets (converted by the reference itself, see" + NL
            + "// plugins/FwBassXLift/nulltest/read_ref_presets.py); names are Flatwater's own. Presets leave Input alone, as the reference does." + NL
            + '#include "shared/Presets.h"' + NL + NL + "namespace fwbassxlift" + NL + "{" + NL
            + "inline const std::vector<fw::FactoryPreset>& factoryPresets()" + NL + "{" + NL
            + "    static const std::vector<fw::FactoryPreset> list {" + NL)
    tail = NL + "    };" + NL + "    return list;" + NL + "}" + NL + "}" + NL
    open(out, "w", newline=NL).write(head + NL.join(rows) + tail)
    print(out, len(rows), "presets")


def write_hcomp():
    # the preset arrays are stored as real-world values; positions found by loading presets into the reference and
    # reading its parameters back (plugins/FwComp/nulltest/read_ref_presets.py): 0 Threshold, 1 Ratio, 3 Punch, 4 Mix, 5 Output,
    # 6 Attack, 7 Release, 8 Release Note (index), 9 Sync (0 BPM, 1 Host, 2 ms), 10 BPM, 13 Limiter (1 On), 28 Analog
    # (0 Off .. 4); '*' leaves a parameter at its default
    data = json.load(open(os.path.join(ROOT, "build", "ref_presets_HComp.json")))
    assert len(data) == len(HCOMP_NAMES) == 26, "preset list changed"
    rows = []
    for k, d in enumerate(data):
        group, name = HCOMP_NAMES[k]; r = d["raw"]
        get = lambda i: None if i >= len(r) or r[i] == '*' else float(r[i])
        spec = ((0, "threshold", -48, 0, 1), (1, "ratio", 1, 50, 2), (3, "punch", 0, 30, 1), (4, "mix", 0, 100, 1),
                (5, "output", -18, 18, 1), (6, "attack", 0.5, 100, 2), (7, "release", 3, 3000, 2), (8, "releaseNote", 0, 19, 0),
                (9, "sync", 0, 2, 0), (10, "bpm", 30, 300, 0), (13, "limiter", 0, 1, 0), (28, "analog", 0, 4, 0))
        vals = [(pid, num(get(i), lo, hi, nd)) for i, pid, lo, hi, nd in spec if get(i) is not None]
        rows.append('    { "%s", "%s", { %s } },' % (group, name, ", ".join('{ "%s", %rf }' % (k, float(v)) for k, v in vals)))
    rows.sort(key=lambda row: (0 if '"Essentials"' in row else 1))
    out = os.path.join(ROOT, "plugins", "FwComp", "Source", "Presets.h")
    head = ("#pragma once" + NL + "// Factory presets of Flatwater Comp, generated by scripts/make_presets.py (do not edit by hand)." + NL
            + "// Setting values carried over from the reference's presets (array positions confirmed by the reference itself," + NL
            + "// see plugins/FwComp/nulltest/read_ref_presets.py); names are Flatwater's own. Presets leave Mix alone, as the reference does." + NL
            + '#include "shared/Presets.h"' + NL + NL + "namespace fwcomp" + NL + "{" + NL
            + "inline const std::vector<fw::FactoryPreset>& factoryPresets()" + NL + "{" + NL
            + "    static const std::vector<fw::FactoryPreset> list {" + NL)
    tail = NL + "    };" + NL + "    return list;" + NL + "}" + NL + "}" + NL
    open(out, "w", newline=NL).write(head + NL.join(rows) + tail)
    print(out, len(rows), "presets")


# Flatwater Delay: the reference's presets in their order; the first 68 are its factory set, the last 20 its guest set
HDELAY_NAMES = [
    "Default", "Live Sound Start", "Side to Side", "Plain Stereo", "Plain Stereo Worn", "Worn Slap", "Band-Pass Echo",
    "Plain Echo", "Tape Echo", "Dub Echo", "Wide Echo", "Tape Runaway (Loud)", "Quarter Side to Side", "Just Right Echo",
    "Filtered Dub 1/4T", "Vintage Echo", "Even Eighths", "Sixteenths", "Murky Dub 1/8D", "Retro Insert",
    "Filtered Echo", "Narrow Bounce 1/8T", "Short Bounce Insert", "Plain Slap", "Chorus Echo", "Thick Shimmer", "Bouncy Kit",
    "Rap Vocal Echo", "Hard Left Six-String", "Deep Chorus", "Soft Jet Insert", "Clang Double", "Flanger",
    "Moving Repeats", "Wobble Engine", "Far Away", "Washroom", "Fluttering Repeats", "Ceramic Room", "Identity Blur",
    "Lost Space", "Spinning Cabinet", "Coil Stereo", "Coil Mono", "Through the Wall", "Dawn", "Thin Straw", "Vintage Wireless",
    "Whisker Wobble", "Rough Edge", "Broken Machine", "Haunted Voices", "Strange Voices", "Low Rumble (Loud)",
    "Quick Alarm", "Springy Wobble", "Robot Voice", "Saucer (Loud)", "Spectre", "Dark Haunt (Loud)", "Granular Drizzle",
    "Phone Line", "Faulty Filter", "Machines Talking", "Worn Infinite Loop", "Siren Drive (Loud)", "Hard Slap",
    "Simple Echo",
    "Bouncing Balls", "Top Echo", "Vocal Echo", "Beat Vocal 1", "Beat Vocal 2", "Long Vocal", "Dizzy Spiral",
    "Vocal 1/16D", "Blur Echo", "Filtered Studio Echo", "Studio Vocal Slap", "Lazy Vocal Trail", "Voice Effects Rack", "Go-To Echo",
    "Roaming Wireless", "Off Stage", "Rotor (Loud)", "Quarter Note Echo", "Eighth Note Echo", "Rock Vocal Echo"]


def write_hdelay():
    # the preset arrays hold real-world values (positions checked against the reference's own parameters, see
    # plugins/FwDelay/nulltest/read_ref_presets.py): 1 Sync (0 BPM, 1 Host, 2 ms), 2 BPM, 3 Delay ms, 4 Delay note (index), 6 Mix,
    # 7 Feedback, 8 / 9 Phase L / R, 10 Ping Pong, 14 Output, 15 LoPass (20 100 and up: Off), 17 HiPass (19: Off),
    # 19 Depth, 20 Rate, 21 LoFi, 23 Analog. The start side, Dual and the right side's times are not stored.
    # Values as stored: the reference takes a preset's values exactly (FwDelayProcessor uses them while the parameter
    # stays where the preset put it); LoPass 20 100 is Off, 20 139 a filter at 20 139 Hz.
    data = json.load(open(os.path.join(ROOT, "build", "ref_presets_HDelay.json")))
    assert len(data) == len(HDELAY_NAMES) == 88, "preset list changed"
    assert len(set(HDELAY_NAMES)) == len(HDELAY_NAMES)
    rows = []
    for k, d in enumerate(data):
        r = d["raw"]; get = lambda i: float(r[i])
        group = "Essentials" if k < 68 else "Studio"
        spec = ((1, "sync", 0, 2, 0), (2, "bpm", 30, 300, 2), (3, "delay", 1, 3500, 3), (4, "delayNote", 0, 19, 0),
                (6, "mix", 0, 100, 2), (7, "feedback", 0, 200, 2), (8, "phaseL", 0, 1, 0), (9, "phaseR", 0, 1, 0),
                (10, "pingPong", 0, 1, 0), (14, "output", -18, 18, 2), (15, "lopass", 19, 20139, 2), (17, "hipass", 19, 20138.77, 2),
                (19, "depth", 0, 100, 3), (20, "rate", 0.1, 6000, 4), (21, "lofi", 0, 1, 0), (23, "analog", 0, 4, 0))
        vals = [(pid, num(get(i), lo, hi, nd)) for i, pid, lo, hi, nd in spec]
        rows.append('    { "%s", "%s", { %s } },' % (group, HDELAY_NAMES[k], ", ".join('{ "%s", %rf }' % (pid, float(v)) for pid, v in vals)))
    out = os.path.join(ROOT, "plugins", "FwDelay", "Source", "Presets.h")
    head = ("#pragma once" + NL + "// Factory presets of Flatwater Delay, generated by scripts/make_presets.py (do not edit by hand)." + NL
            + "// Setting values carried over from the reference's presets (array positions checked against the reference's" + NL
            + "// own parameters, see plugins/FwDelay/nulltest/read_ref_presets.py); names are Flatwater's own. \"(Loud)\": the reference marks" + NL
            + "// these as able to run away (Feedback over 100). Presets leave Mix alone, as the reference does." + NL
            + '#include "shared/Presets.h"' + NL + NL + "namespace fwdelay" + NL + "{" + NL
            + "inline const std::vector<fw::FactoryPreset>& factoryPresets()" + NL + "{" + NL
            + "    static const std::vector<fw::FactoryPreset> list {" + NL)
    tail = NL + "    };" + NL + "    return list;" + NL + "}" + NL + "}" + NL
    open(out, "w", newline=NL).write(head + NL.join(rows) + tail)
    print(out, len(rows), "presets")


# the reference's 17 presets, in its order, with our names
BASSLEVELER_NAMES = [
    "Default", "Live PA Start", "Soft Touch", "Fingerstyle Slow", "Fretless Slow", "Unplugged Bass Ease", "Upright Bass",
    "Fast Fingers", "Wide Dynamics", "Clean and Lively", "Fast, Artifacts Ignored", "Picked Bass Fast", "Fast Eighths",
    "Squeak Proof", "Slap Through the Amp", "Bass Tamer", "Gig Night"]


def write_bassleveler():
    # the arrays hold real-world values: 4 Output, 8 / 9 Range Min / Max, 12 Target ("*": the preset leaves it), 31
    # Sensitivity, 33 Response (0 Slow, 1 Fast), 34 the Idle arrow's place in the range, 44 the Artifacts gate (-20 .. -3
    # dB, shown 0 .. 100 linearly), 45 Spill. The other stored values are the same in every preset (the hidden ones).
    data = json.load(open(os.path.join(ROOT, "build", "ref_presets_BassRider.json")))
    assert len(data) == len(BASSLEVELER_NAMES) == 17, "preset list changed"
    rows = []
    for k, d in enumerate(data):
        r = d["raw"]; get = lambda i: float(r[i])
        vals = [("output", num(get(4), -12, 12, 1)), ("rangeMin", num(get(8), -12, 11, 1)), ("rangeMax", num(get(9), -11, 12, 1))]
        leave = []
        if r[12] == "*": leave.append("target")
        else: vals.append(("target", num(get(12), -42, 0, 1)))
        vals += [("sensitivity", num(get(31), 0, 100, 2)), ("response", float(int(get(33)))), ("idle", num(get(34), 0.02, 0.98, 2)),
                 ("artifacts", num((get(44) + 20.0) / 17.0 * 100.0, 0, 100, 1)), ("spill", num(get(45), -50, -3, 1))]
        group = "Essentials"
        rows.append('    { "%s", "%s", { %s }%s },' % (group, BASSLEVELER_NAMES[k], ", ".join('{ "%s", %rf }' % (pid, float(v)) for pid, v in vals),
                                                        (", { %s }" % ", ".join('"%s"' % x for x in leave)) if leave else ""))
    out = os.path.join(ROOT, "plugins", "FwBassLeveler", "Source", "Presets.h")
    head = ("#pragma once" + NL + "// Factory presets of Flatwater Bass Leveler, generated by scripts/make_presets.py (do not edit by hand)." + NL
            + "// Setting values carried over from the reference's presets (see plugins/FwBassLeveler/nulltest/read_ref_presets.py); names are Flatwater's" + NL
            + "// own. All but the first two leave Target where it is, as the reference's do." + NL
            + '#include "shared/Presets.h"' + NL + NL + "namespace fwbassleveler" + NL + "{" + NL
            + "inline const std::vector<fw::FactoryPreset>& factoryPresets()" + NL + "{" + NL
            + "    static const std::vector<fw::FactoryPreset> list {" + NL)
    tail = NL + "    };" + NL + "    return list;" + NL + "}" + NL + "}" + NL
    open(out, "w", newline=NL).write(head + NL.join(rows) + tail)
    print(out, len(rows), "presets")


# the reference's 13 presets, in its order, with our names
VOCALRIDER_NAMES = [
    "Default", "Gentle Ride", "Steady Lead", "Quick Ride", "Floor Ride", "Upper Ride", "Big Swings", "Loud Mix",
    "Loud Lead", "Auto Fader", "Level Tamer", "Ceiling Ride", "Open Up"]


def write_vocalrider():
    # the arrays hold real-world values: 0 Target, 1 Music Sensitivity (-12.1 = Off), 2 / 3 Range Min / Max, 6 Vocal
    # Sensitivity, 20 Output, 30 Automation ("*": the preset leaves it), 32 the Idle arrow's place in the range (some
    # presets use 0 and 1, outside the arrow's 0.02 .. 0.98: kept exactly, see FwVocalLevelerProcessor), 42 Attack (which
    # sets the Attack-tied hidden values the presets also store). Spill is not stored in presets.
    data = json.load(open(os.path.join(ROOT, "build", "ref_presets_VocalRider.json")))
    assert len(data) == len(VOCALRIDER_NAMES) == 13, "preset list changed"
    rows = []
    for k, d in enumerate(data):
        r = d["raw"]; get = lambda i: float(r[i])
        vals = [("target", num(get(0), -42, 0, 1)), ("musicSensitivity", num(get(1), -12.1, 12, 1)), ("rangeMin", num(get(2), -12, 11, 1)),
                ("rangeMax", num(get(3), -11, 12, 1)), ("vocalSensitivity", num(get(6), -12, 12, 1)), ("output", num(get(20), -12, 12, 1)),
                ("idle", num(get(32), 0.0, 1.0, 2)), ("attack", float(int(get(42))))]
        if r[30] != "*": vals.append(("automation", float(int(get(30)))))
        rows.append('    { "Essentials", "%s", { %s } },' % (VOCALRIDER_NAMES[k], ", ".join('{ "%s", %rf }' % (pid, float(v)) for pid, v in vals)))
    out = os.path.join(ROOT, "plugins", "FwVocalLeveler", "Source", "Presets.h")
    head = ("#pragma once" + NL + "// Factory presets of Flatwater Vocal Leveler, generated by scripts/make_presets.py (do not edit by hand)." + NL
            + "// Setting values carried over from the reference's presets (see plugins/FwVocalLeveler/nulltest/read_ref_presets.py); names are Flatwater's" + NL
            + "// own. An Idle of 0 or 1 is outside the arrow's range; the processor keeps it exactly while the arrow stays put." + NL
            + '#include "shared/Presets.h"' + NL + NL + "namespace fwvocalleveler" + NL + "{" + NL
            + "inline const std::vector<fw::FactoryPreset>& factoryPresets()" + NL + "{" + NL
            + "    static const std::vector<fw::FactoryPreset> list {" + NL)
    tail = NL + "    };" + NL + "    return list;" + NL + "}" + NL + "}" + NL
    open(out, "w", newline=NL).write(head + NL.join(rows) + tail)
    print(out, len(rows), "presets")


write("FwBassLift", "Flatwater Bass Lift", "ref_presets_RBass.json", BASS_NAMES, bass_values)
write("FwVocalComp", "Flatwater Vocal Comp", "ref_presets_RVox.json", VOCAL_NAMES, vocal_values)
write_maxx()
write_hcomp()
write_hdelay()
write_bassleveler()
write_vocalrider()


# Flatwater Vocal Chain (self-contained: its names, values and writer). The reference's 43 presets, in its order, with
# our names; read straight from its preset file on the local install (setting values only): local.yaml's
# FwVocalChain.ref_presets, or FW_CLAV_PRESETS. A preset stores the six faders, their colours and Balance; Sensitivity, Output and the In/Out link stay
# where they are (the reference ignores the Sensitivity and Output it stores).
VOCALCHAIN_NAMES = [
    ("Essentials", "Default"), ("Essentials", "Lead Starter"), ("Essentials", "Lead Starter Dry"),
    ("Essentials", "Wide Double"), ("Essentials", "Up Front Dry"), ("Essentials", "Up Front"),
    ("Essentials", "Everything Up"), ("Essentials", "Everything Up Dry"), ("Essentials", "Slap Lead"),
    ("Essentials", "Chamber Lead"), ("Essentials", "Open Room"), ("Essentials", "Brooding"),
    ("Essentials", "Bright Wall Dry"), ("Essentials", "Heavy Wall"), ("Essentials", "Rock Lead High"),
    ("Essentials", "Rock Lead High Dry"), ("Essentials", "Rock Lead Low"), ("Essentials", "Rock Lead Low Dry"),
    ("Essentials", "Space Lead"), ("Essentials", "Spoken Word"), ("Essentials", "All Clear"),
    ("Backing", "Backing Starter"), ("Backing", "Airy Backing"), ("Backing", "Answer Line"),
    ("Backing", "Backing Wash"), ("Backing", "Backing Left"), ("Backing", "Backing Right"),
    ("Backing", "Backing Orbit"), ("Backing", "Backing Close"), ("Backing", "Backing Chamber"),
    ("Backing", "Backing Rock"), ("Studio", "Distant Choir"), ("Studio", "Presence Vocal"),
    ("Studio", "Broad Snare"), ("Studio", "Parallel Air"), ("Studio", "Robotic Vocal"),
    ("Studio", "Eerie Vocal"), ("Studio", "Wider Percussion"), ("Backing", "Backing Lo-Fi"),
    ("Studio", "Background Space"), ("Studio", "Vocal Space"), ("Studio", "Squeezed Voice"),
    ("Studio", "Wet Ad-Libs")]


def write_vocalchain():
    import re
    import sys
    sys.path.append(os.path.join(HERE, "nulltest"))
    import local_config as L
    path = os.environ.get("FW_CLAV_PRESETS") or L.get("FwVocalChain.ref_presets", required=False) or ""
    if not os.path.exists(path):
        print("skipped Flatwater Vocal Chain: no", path)
        return
    txt = open(path, encoding="utf-8", errors="replace").read()
    found = re.findall(r'<Preset[^>]*Name="[^"]*".*?<Parameters Type="RealWorld">([^<]*)</Parameters>', txt, re.S)
    assert len(found) == len(VOCALCHAIN_NAMES) == 43, "preset list changed"
    # the array's 54 .. 65: the six stages' fader and colour (0 the clear position), 66 Balance
    stages = ("bass", "treble", "compress", "reverb", "delay", "pitch")
    rows = []
    for (group, name), raw in zip(VOCALCHAIN_NAMES, found):
        r = raw.split()
        vals = []
        for j, st in enumerate(stages):
            vals += [(st, num(r[54 + 2 * j], -10, 10, 1)), (st + "Color", float(int(float(r[55 + 2 * j]))))]
        vals.append(("balance", num(r[66], -6, 6, 1)))
        rows.append('    { "%s", "%s", { %s }, { "sensitivity", "output", "ioLink" } },'
                    % (group, name, ", ".join('{ "%s", %rf }' % (pid, float(v)) for pid, v in vals)))
    out = os.path.join(ROOT, "plugins", "FwVocalChain", "Source", "Presets.h")
    head = ("#pragma once" + NL + "// Factory presets of Flatwater Vocal Chain, generated by scripts/make_presets.py (do not edit by hand)." + NL
            + "// Setting values carried over from the reference's presets; names are Flatwater's own. Sensitivity, Output and" + NL
            + "// the In/Out link stay where they are when a preset loads." + NL
            + '#include "shared/Presets.h"' + NL + NL + "namespace fwvocalchain" + NL + "{" + NL
            + "inline const std::vector<fw::FactoryPreset>& factoryPresets()" + NL + "{" + NL
            + "    static const std::vector<fw::FactoryPreset> list {" + NL)
    tail = NL + "    };" + NL + "    return list;" + NL + "}" + NL + "}" + NL
    open(out, "w", newline=NL).write(head + NL.join(rows) + tail)
    print(out, len(rows), "presets")


write_vocalchain()
