"""The null-test report of a plug-in: plugins/<Id>/NULLTESTS.md, written from measurements/<Id>/nulltests.json and never
edited by hand.

The JSON holds the report as data: a title, the introduction and the sections (Settings, Stages, Presets, Grid), each
a list of blocks: a heading, a paragraph (its lines) or a table (columns and rows of cells). A block may belong to a
part, the output of one null script: the script replaces its part when it runs, and the report is written again.

    import report_lib as RL
    RL.save('FwComp', 'presets', md_lines, section='Presets')   # a script's output (Markdown lines) as its part
    RL.write('FwComp')                                         # NULLTESTS.md again from the JSON
    python report_lib.py FwComp                                # the same from the command line (nulltest.py --report)
"""
import json, os, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SECTIONS = ('Settings', 'Stages', 'Presets', 'Grid')


def data_path(plugin):
    return os.path.join(ROOT, 'measurements', plugin, 'nulltests.json')


def report_path(plugin):
    return os.path.join(ROOT, 'plugins', plugin, 'NULLTESTS.md')


def _cells(line):
    s = line.strip()
    if s.startswith('|'): s = s[1:]
    if s.endswith('|') and not s.endswith('\\|'): s = s[:-1]
    out, cur, i = [], '', 0
    while i < len(s):
        if s[i] == '\\' and i + 1 < len(s) and s[i + 1] == '|':
            cur += '\\|'; i += 2; continue
        if s[i] == '|':
            out.append(cur.strip()); cur = ''
        else:
            cur += s[i]
        i += 1
    out.append(cur.strip())
    return out


def parse(lines, demote=0):
    """Markdown lines -> blocks: {'heading': level, 'text': str}, {'table': True, 'columns': [...], 'align': [...],
    'rows': [[...]]}, {'lines': [...]} (a paragraph or a list, blank lines between blocks dropped)."""
    blocks, para, i = [], [], 0

    def flush():
        if para:
            blocks.append({'lines': list(para)}); para.clear()
    while i < len(lines):
        line = lines[i]
        if line.lstrip().startswith(('```', '~~~')):
            flush()
            fence, j = [line], i + 1
            while j < len(lines) and not lines[j].lstrip().startswith(line.lstrip()[:3]):
                fence.append(lines[j]); j += 1
            fence.append(lines[j] if j < len(lines) else line.lstrip()[:3])
            blocks.append({'lines': fence})
            i = j + 1
            continue
        if line.startswith('#'):
            flush()
            level = len(line) - len(line.lstrip('#'))
            blocks.append({'heading': level + demote, 'text': line[level:].strip()})
        elif line.lstrip().startswith('|') and i + 1 < len(lines) and set(lines[i + 1].replace('|', '').replace(':', '').replace(' ', '')) <= {'-'} \
                and lines[i + 1].strip().startswith('|'):
            flush()
            cols, align = _cells(line), _cells(lines[i + 1])
            rows, i = [], i + 2
            while i < len(lines) and lines[i].lstrip().startswith('|'):
                rows.append(_cells(lines[i])); i += 1
            blocks.append({'table': True, 'columns': cols, 'align': align, 'rows': rows})
            continue
        elif not line.strip():
            flush()
        else:
            para.append(line)
        i += 1
    flush()
    return blocks


def render_blocks(blocks):
    out = []
    for b in blocks:
        if 'heading' in b:
            out += ['#' * b['heading'] + ' ' + b['text'], '']
        elif b.get('table'):
            out.append('| ' + ' | '.join(b['columns']) + ' |')
            out.append('|' + '|'.join(b['align']) + '|')
            out += ['| ' + ' | '.join(r) + ' |' for r in b['rows']]
            out.append('')
        else:
            out += b['lines'] + ['']
    return out


def render(doc):
    out = ['# ' + doc['title'], ''] + render_blocks(doc.get('intro', []))
    for s in doc['sections']:
        out += ['## ' + s['name'], ''] + render_blocks(s['blocks'])
    while out and out[-1] == '':
        out.pop()
    return '\n'.join(out) + '\n'


def load(plugin):
    with open(data_path(plugin), encoding='utf-8') as h:
        return json.load(h)


def store(plugin, doc):
    os.makedirs(os.path.dirname(data_path(plugin)), exist_ok=True)
    with open(data_path(plugin), 'w', encoding='utf-8', newline='\n') as h:
        json.dump(doc, h, indent=1, ensure_ascii=False)
        h.write('\n')


def write(plugin):
    """NULLTESTS.md from the JSON; returns its path"""
    path = report_path(plugin)
    with open(path, 'w', encoding='utf-8', newline='\n') as h:
        h.write(render(load(plugin)))
    return path


def save(plugin, part, lines, section='Settings', from_table=False):
    """a null script's output as its part of the report: Markdown lines as the script writes them (a title line of its
    own is dropped and its headings go one level down, under the section's); from_table: only from its first table on.
    The part's blocks are replaced where they stand, or added at the end of the section; then NULLTESTS.md is written
    again. Returns its path."""
    lines = [x for line in lines for x in line.split(chr(10))]
    if lines and lines[0].startswith('# '):
        lines = lines[1:]
    if from_table:
        lines = lines[next((i for i, x in enumerate(lines) if x.lstrip().startswith('|')), len(lines)):]
    doc = load(plugin)
    new = [dict(b, part=part) for b in parse(lines, demote=1)]
    for s in doc['sections']:
        idx = [i for i, b in enumerate(s['blocks']) if b.get('part') == part]
        if idx:
            s['blocks'][idx[0]:idx[-1] + 1] = new
            break
    else:
        sec = next((s for s in doc['sections'] if s['name'] == section), None)
        if sec is None:
            sec = {'name': section, 'blocks': []}
            doc['sections'].append(sec)
            doc['sections'].sort(key=lambda s: SECTIONS.index(s['name']) if s['name'] in SECTIONS else len(SECTIONS))
        sec['blocks'] += new
    store(plugin, doc)
    return write(plugin)


class Part:
    """collects what a script writes (with f.write) and saves it as its part on leaving:
    with report_lib.Part('FwComp', 'presets', 'Presets') as f: f.write(...)"""
    def __init__(self, plugin, part, section='Settings', from_table=False):
        self.args = (plugin, part, section, from_table)
        self.text = []

    def write(self, s):
        self.text.append(s)

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        if exc[0] is None:
            plugin, part, section, from_table = self.args
            print('wrote', save(plugin, part, ''.join(self.text).rstrip(chr(10)).split(chr(10)), section, from_table))
        return False


if __name__ == '__main__':
    for p in sys.argv[1:]:
        print('wrote', os.path.relpath(write(p), ROOT))
