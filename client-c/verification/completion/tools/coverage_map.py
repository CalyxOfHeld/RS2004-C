"""Generate verification/completion/coverage-map.md.

Measures, from source alone, how much of the bundled revision-225 Java client
has a native counterpart:
  1. every server opcode handled by client.read() vs. a native dispatch branch;
  2. every opcode the Java client sends vs. a native encoder;
  3. per-class Java method coverage by name/citation match (an upper bound:
     a match means "a native function exists", not "it is a faithful port").
Run from the rewrite root:  python client-c/verification/completion/tools/coverage_map.py
"""
import collections, datetime, os, re

JAVA = 'javaclient/src/main/java'
CLIENT = os.path.join(JAVA, 'deob', 'client.java')
NATIVE = 'client-c'
OUT = os.path.join(NATIVE, 'verification', 'completion', 'coverage-map.md')


def read(path):
    return open(path, encoding='utf-8', errors='replace').read()


def native_sources():
    srcs = {}
    for d in ('src', 'platform', 'include'):
        for dp, _, fs in os.walk(os.path.join(NATIVE, d)):
            for f in fs:
                if f.endswith(('.c', '.h')):
                    p = os.path.join(dp, f)
                    srcs[os.path.relpath(p, NATIVE).replace(os.sep, '/')] = read(p)
    return srcs


def prot_names(path, cls):
    names = {}
    for line in read(path).split('\n'):
        m = re.search(r'static readonly (\w+)\s*=\s*new %s\((\d+)' % cls, line)
        if m:
            names[int(m.group(2))] = m.group(1)
    return names


def java_methods():
    sig = re.compile(r'^\t(?:public|private|protected)?\s*(?:static\s+)?(?:final\s+)?(?:synchronized\s+)?'
                     r'(?:[\w\[\]<>,]+)\s+(\w+)\s*\(([^)]*)\)\s*(?:throws [\w., ]+)?\s*\{\s*$')
    out = []
    for d, _, fs in os.walk(JAVA):
        for f in fs:
            if not f.endswith('.java'):
                continue
            p = os.path.join(d, f)
            lines = read(p).split('\n')
            cls = os.path.relpath(p, JAVA)[:-5].replace(os.sep, '.')
            i = 0
            while i < len(lines):
                m = sig.match(lines[i])
                if m and m.group(1) not in ('if', 'for', 'while', 'switch', 'catch'):
                    j = i + 1
                    while j < len(lines) and lines[j] != '\t}':
                        j += 1
                    out.append(dict(cls=cls, name=m.group(1), start=i + 1, lines=j - i + 1))
                    i = j
                i += 1
    return out


def main():
    srcs = native_sources()
    allc = '\n'.join(srcs.values())
    client = read(CLIENT).split('\n')
    md = ['# Native coverage of the revision-225 Java client', '',
          f'Generated {datetime.date.today()} by `tools/coverage_map.py` from source alone. '
          'A present branch/encoder/function means native code exists, not that it is a faithful or verified port; '
          'see `test-grades.md` for what is actually verified.', '']

    # 1. server -> client
    names = prot_names('engine/src/network/game/server/ServerGameProt.ts', 'ServerGameProt')
    start = next(i for i, l in enumerate(client) if re.search(r'public final boolean read\(', l))
    ops = []
    for i in range(start, len(client)):
        m = re.search(r'this\.packetType == (\d+)\)', client[i])
        if m:
            ops.append((int(m.group(1)), i + 1))
        if i > start and client[i] == '\t}':
            break
    rows = []
    for op, line in ops:
        pats = [rf'opcode\s*==\s*{op}\b', rf'case\s+{op}\s*:', rf'type\s*==\s*{op}\b', rf'\bop\s*==\s*{op}\b']
        rows.append((op, names.get(op, '?'), line, any(re.search(p, allc) for p in pats)))
    md += [f'## Server packets handled by `client.read()`: {sum(r[3] for r in rows)} of {len(rows)} have a native branch', '',
           '| Opcode | Name | client.java | Native |', '| --- | --- | --- | --- |']
    md += [f'| {o} | {n} | {l} | {"branch" if h else "**absent**"} |' for o, n, l, h in sorted(rows, key=lambda r: (r[3], r[0]))]
    md.append('')

    # 2. client -> server
    cnames = prot_names('engine/src/network/game/client/ClientGameProt.ts', 'ClientGameProt')
    jops = collections.Counter(int(x) for x in re.findall(r'this\.out\.p1isaac\(\(byte\)\s*-?\d+,\s*(\d+)\)', '\n'.join(client)))
    proto = srcs.get('src/net/protocol.c', '')
    cops = set(int(x) for x in re.findall(r'\(\s*(\d+)\s*\+\s*isaac_next', allc))
    cops |= set(int(x) for x in re.findall(r'\b(\d{1,3})\s*\+\s*isaac_next', allc))
    for table in re.findall(r'static const int \w*opcodes?\w*\s*\[\d*\]\s*=\s*\{([^}]*)\}', proto):
        cops |= set(int(x) for x in re.findall(r'\d+', table))
    cops |= set(int(x) for x in re.findall(r'send_\w+\([^;]*?\b(\d{1,3})\s*[,)]', allc) if 0 < int(x) < 256)
    cops |= set(int(x) for x in re.findall(r'send_short_fields\([^;]*?\?(\d+):(\d+)', proto) for x in x)
    md += [f'## Client packets sent by the Java client: {sum(1 for o in jops if o in cops)} of {len(jops)} have a native encoder', '',
           '| Opcode | Name | Java send sites | Native |', '| --- | --- | --- | --- |']
    md += [f'| {o} | {cnames.get(o, "?")} | {jops[o]} | {"encoder" if o in cops else "**absent**"} |'
           for o in sorted(jops, key=lambda o: (o in cops, o))]
    md.append('')

    # 3. per-class method coverage
    cdefs = set(m.group(1) for t in srcs.values() for m in re.finditer(r'^[A-Za-z_][\w \*]*?\b(\w+)\s*\([^;{]*\)\s*\{', t, re.M))
    comments = '\n'.join(l for t in srcs.values() for l in t.split('\n') if '//' in l or '/*' in l or l.strip().startswith('*'))
    snake = lambda n: re.sub(r'(?<=[a-z0-9])([A-Z])', r'_\1', n).lower()
    per = collections.OrderedDict()
    for m in sorted(java_methods(), key=lambda m: m['cls']):
        s = snake(m['name'])
        cls = m['cls'].split('.')[-1]
        hit = any(f == s or f.endswith('_' + s) for f in cdefs) or (
            len(m['name']) > 4 and re.search(r'\b(?:%s\.)?%s\b' % (re.escape(cls), re.escape(m['name'])), comments))
        e = per.setdefault(m['cls'], dict(n=0, lines=0, hit=0, hitlines=0, missing=[]))
        e['n'] += 1; e['lines'] += m['lines']
        if hit:
            e['hit'] += 1; e['hitlines'] += m['lines']
        else:
            e['missing'].append((m['lines'], m['name']))
    md += ['## Java methods with a name-matched native function (upper bound)', '',
           'Name matching misses native code with different names (for example `useMenuOption` lives in '
           '`client_actions.c`/`mouse_picking.c`, and most of `updateGame`/`drawGame` is inlined in `platform/sdl2_main.c` `main()`), '
           'so the `client` row understates; every other row overstates fidelity.', '',
           '| Class | Methods matched | Lines matched | Largest unmatched |', '| --- | --- | --- | --- |']
    for cls, e in sorted(per.items(), key=lambda kv: -kv[1]['lines']):
        big = ', '.join(f'{n}({l})' for l, n in sorted(e['missing'], reverse=True)[:6])
        md.append(f'| {cls} | {e["hit"]}/{e["n"]} | {e["hitlines"]}/{e["lines"]} | {big} |')
    md.append('')
    open(OUT, 'w', encoding='utf-8', newline='\n').write('\n'.join(md))
    print('wrote', OUT, '| server', sum(r[3] for r in rows), '/', len(rows), '| client', sum(1 for o in jops if o in cops), '/', len(jops))


if __name__ == '__main__':
    main()
