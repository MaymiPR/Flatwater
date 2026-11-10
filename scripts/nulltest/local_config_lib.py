"""Machine paths for the null-test and measurement scripts, read from local.yaml at the repository root (git-ignored;
local.example.yaml lists the standard keys, scripts/nulltest/README.md the keys this suite adds). Flat "key: value" lines; a
value may be quoted with single or double quotes; an empty value counts as not set.

    import local_config_lib as L
    L.get('ref_vst3')                  # the value, or exit with a message naming the key
    L.get('fl_packs', required=False)  # the value, or None
    L.local_file('build/x.json')       # a local data file, under an earlier name while only that one exists
"""
import os

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
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
        raise SystemExit(f'local.yaml: set {key} (copy local.example.yaml to local.yaml; scripts/nulltest/README.md lists the keys)')
    return None



def local_file(path):
    """a local data file (an absolute path or one from the repository root) under its current name, or under its earlier
    name (renamed_local_files.txt beside this file) while only that one exists, so files written before a rename keep
    working for one release"""
    full = os.path.abspath(os.path.join(ROOT, path))
    if os.path.exists(full):
        return full
    rel = os.path.relpath(full, ROOT).replace(os.sep, '/')
    table = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'renamed_local_files.txt')
    for line in open(table, encoding='utf-8'):
        if line.strip() and not line.startswith('#'):
            new, old = (s.strip() for s in line.split('|'))
            if rel == new or rel.startswith(new + '/'):          # the file, or a file inside a renamed folder
                earlier = os.path.join(ROOT, old + rel[len(new):])
                if os.path.exists(earlier):
                    return earlier
    return full
