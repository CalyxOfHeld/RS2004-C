"""Reduce a DataOracle (Java) dump to the per-object digest fixture that
`test_data_reference --check` verifies in `make test`.

Usage: python data_reference_digest.py java.txt tests/fixtures/data-reference.digest

Digest of an object: SHA-256 (first 16 hex) over its "field=value\\n" lines
sorted by field name, or over "null\\n" for a null object. Fields that the
native structs deliberately do not hold are excluded (JAVA_ONLY); every one
was confirmed default-valued or a reference already covered by another field
(see runs/2026-10-22-phaseA-audit/data-oracle.md). Count lines become
"type -1 <count>".
"""
import hashlib, sys

JAVA_ONLY = {
    # Filled later by calculateNormals/createLabelReferences/bounds; all
    # default after new Model(false, id).
    'model': {'labelFaces', 'labelVertices', 'maxX', 'maxY', 'maxZ', 'minX', 'minY', 'minZ',
              'pickable', 'radius', 'vertexNormal', 'vertexNormalOriginal'},
    # Runtime animation state, 0 after decode.
    'component': {'seqCycle', 'seqFrame'},
    # The font's java.util.Random instance.
    'font': {'random'},
    # index = decode id; seq = SeqType.instances[anim] (native: base pointer).
    'spotanim': {'index', 'seq'},
}


def main():
    src, dst = sys.argv[1], sys.argv[2]
    out = []
    cur = None
    fields = []
    null_obj = False

    def finish():
        if cur is None:
            return
        h = hashlib.sha256()
        if null_obj:
            h.update(b'null\n')
        else:
            for name, line in sorted(fields, key=lambda f: f[0].encode()):
                h.update(line.encode('ascii') + b'\n')
        out.append(f'{cur[0]} {cur[1]} {h.hexdigest()[:16]}')

    with open(src, encoding='ascii') as f:
        for line in f:
            line = line.rstrip('\n')
            typ, _, rest = line.partition(' ')
            if rest.startswith('count='):
                out.append(f'{typ} -1 {int(rest[6:])}')
                continue
            ident, _, kv = rest.partition(' ')
            key = (typ, int(ident))
            if key != cur:
                finish()
                cur, fields, null_obj = key, [], False
            if kv == 'null':
                null_obj = True
                continue
            name = kv.partition('=')[0]
            if name in JAVA_ONLY.get(typ, ()):
                continue
            fields.append((name, kv))
    finish()
    with open(dst, 'w', newline='\n') as f:
        f.write('\n'.join(out) + '\n')
    print(f'{len(out)} entries -> {dst}')


if __name__ == '__main__':
    main()
