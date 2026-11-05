"""Rewrites the two root indexes from the plug-in folders' documents: MEASUREMENTS.md (every section of every
plugins/<Name>/MEASUREMENTS.md, so a section a comment cites by name is found from the root) and the "Per plug-in"
list at the end of RESEARCH.md. Run it after adding a section to a plug-in's MEASUREMENTS.md or RESEARCH.md:

    python scripts/doc_index.py
"""
import os, re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def rd(rel):
    with open(os.path.join(ROOT, rel), encoding='utf-8') as h:
        return h.read().replace('\r\n', '\n')


def wr(rel, text):
    with open(os.path.join(ROOT, rel), 'w', encoding='utf-8', newline='\n') as h:
        h.write(text)


def headings(text):
    """[(level, plain text, anchor)] as GitHub computes anchors (duplicates numbered)."""
    out, seen, fence = [], {}, False
    for line in text.split('\n'):
        if line.startswith('```'):
            fence = not fence
        m = None if fence else re.match(r'^(#{1,6}) (.+?)\s*$', line)
        if not m:
            continue
        plain = re.sub(r'[*`]|\[([^\]]*)\]\([^)]*\)', lambda x: x.group(1) or '', m.group(2))
        slug = re.sub(r'[^\w\- ]', '', plain.strip().lower()).replace(' ', '-')
        n = seen.get(slug, 0); seen[slug] = n + 1
        out.append((len(m.group(1)), plain, slug if n == 0 else f'{slug}-{n}'))
    return out


def plugins():
    """(target, display name, original) in packaging/plugins.txt's order, then the plug-ins not shipped yet. The
    original's name is read from the Reference row of the Setup table in the plug-in's MEASUREMENTS.md."""
    order = [l.split('|')[0].strip() for l in rd('packaging/plugins.txt').split('\n') if l.strip() and not l.startswith('#')]
    have = sorted(d for d in os.listdir(os.path.join(ROOT, 'plugins'))
                  if os.path.isfile(os.path.join(ROOT, 'plugins', d, 'MEASUREMENTS.md')))
    out = []
    for t in [t for t in order if t in have] + [t for t in have if t not in order]:
        text = rd(f'plugins/{t}/MEASUREMENTS.md')
        name = re.match(r'# (.+?): measurements', text).group(1)
        orig = re.search(r'^\| Reference \| (.+?) \(', text, re.M)   # the Setup table's first row: the original's name
        out.append((t, name, orig.group(1) if orig else ''))
    return out


def main():
    pl = plugins()
    idx = ['# Flatwater: measurements', '',
           "What was measured on each original and how each part of each model was identified, one file per plug-in. "
           "Every section is listed here, so a section a comment or a document cites by name is found below. The data "
           "is in [measurements/](measurements). Written by `scripts/doc_index.py`.", '']
    for t, name, orig in pl:
        p = f'plugins/{t}/MEASUREMENTS.md'
        idx += [f'## {name}', '', f'[{p}]({p})' + (f' ({orig}):' if orig else ':'), '']
        for lvl, text, slug in headings(rd(p))[1:]:
            idx.append('  ' * (lvl - 2) + f'- [{text}]({p}#{slug})')
        idx.append('')
    wr('MEASUREMENTS.md', '\n'.join(idx).rstrip('\n') + '\n')
    r = rd('RESEARCH.md')
    i = r.index('## Per plug-in')
    lines = ['## Per plug-in', '']
    for t, name, orig in pl:
        p = f'plugins/{t}/RESEARCH.md'
        if not os.path.isfile(os.path.join(ROOT, p)):
            continue
        secs = ', '.join(f'[{text}]({p}#{slug})' for lvl, text, slug in headings(rd(p))[1:] if lvl == 2)
        lines.append(f'- [{name}]({p})' + (f' ({orig})' if orig else '') + ': ' + secs)
    wr('RESEARCH.md', r[:i] + '\n'.join(lines) + '\n')
    print(f'MEASUREMENTS.md and RESEARCH.md indexes written for {len(pl)} plug-ins')


if __name__ == '__main__':
    main()
