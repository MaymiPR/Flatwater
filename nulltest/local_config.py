"""Machine paths for the null-test and measurement scripts, read from local.yaml at the repository root (git-ignored;
local.example.yaml lists the standard keys, nulltest/README.md the keys this suite adds). Flat "key: value" lines; a
value may be quoted with single or double quotes; an empty value counts as not set.

    import local_config as L
    L.get('ref_vst3')                  # the value, or exit with a message naming the key
    L.get('fl_packs', required=False)  # the value, or None
"""
import os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
_values = None


def _load():
    out = {}
    path = os.path.join(ROOT, 'local.yaml')
    if not os.path.isfile(path):
        return out
    with open(path, encoding='utf-8') as h:
        for line in h:
            line = line.strip()
            if not line or line.startswith('#') or ':' not in line:
                continue
            key, value = line.split(':', 1)
            value = value.strip()
            if value[:1] in ('"', "'"):
                q = value[0]
                value = value[1:value.rfind(q)] if value.rfind(q) > 0 else value[1:]
                if q == "'":
                    value = value.replace("''", "'")
            else:
                value = value.split(' #', 1)[0].strip()
            if value:
                out[key.strip()] = value
    return out


def get(key, required=True):
    global _values
    if _values is None:
        _values = _load()
    if key in _values:
        return _values[key]
    if required:
        raise SystemExit(f'local.yaml: set {key} (copy local.example.yaml to local.yaml; nulltest/README.md lists the keys)')
    return None
