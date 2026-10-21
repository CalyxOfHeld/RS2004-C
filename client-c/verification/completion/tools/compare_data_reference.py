"""Compare DataOracle (Java) and test_data_reference (native) dumps.

Usage: python compare_data_reference.py java.txt native.txt [--examples N]

For every (type, field) it reports:
  match       values equal for every object
  MISMATCH    differing values (with example ids)
  null~zero   Java null vs native all-zero/empty (representation only)
  java-only   field the native structs do not store
  native-only field the Java object does not have
Object presence (null objects, counts) is compared too.
"""
import collections, sys


def load(path):
    objs = collections.defaultdict(dict)   # (type,id) -> {field: value}
    counts, nulls = {}, set()
    with open(path, encoding='ascii', errors='replace') as f:
        for line in f:
            line = line.rstrip('\n')
            head, _, rest = line.partition(' ')
            if rest.startswith('count='):
                counts[head] = int(rest[6:])
                continue
            ident, _, kv = rest.partition(' ')
            if kv == 'null':
                nulls.add((head, int(ident)))
                continue
            field, _, value = kv.partition('=')
            objs[(head, int(ident))][field] = value
    return objs, counts, nulls


def zeroish(v):
    if v in ('null', '[]', '0', '""'):
        return True
    if v.startswith('[') and v.endswith(']'):
        return all(x in ('0', 'null', '[]', '') for x in v[1:-1].split(','))
    return False


def main():
    jpath, npath = sys.argv[1], sys.argv[2]
    examples = int(sys.argv[sys.argv.index('--examples') + 1]) if '--examples' in sys.argv else 3
    J, jc, jn = load(jpath)
    N, nc, nn = load(npath)
    types = sorted(set(t for t, _ in J) | set(t for t, _ in N) | set(jc) | set(nc))
    failures = 0
    for t in types:
        print(f'== {t}: java count={jc.get(t)} native count={nc.get(t)}' + ('' if jc.get(t) == nc.get(t) else '  ** COUNT DIFFERS **'))
        if jc.get(t) != nc.get(t):
            failures += 1
        jn_t = {i for tt, i in jn if tt == t}; nn_t = {i for tt, i in nn if tt == t}
        jids = {i for tt, i in J if tt == t}; nids = {i for tt, i in N if tt == t}
        only_java_obj = sorted((jids - nids) - nn_t)
        null_mismatch = sorted((jn_t & nids) | (nn_t & jids))
        if only_java_obj:
            print(f'   objects missing natively: {len(only_java_obj)} e.g. {only_java_obj[:examples]}'); failures += 1
        if null_mismatch:
            print(f'   null-presence differs: {len(null_mismatch)} e.g. {null_mismatch[:examples]}'); failures += 1
        common = sorted(jids & nids)
        fields = sorted(set(f for i in common for f in J[(t, i)]) | set(f for i in common for f in N[(t, i)]))
        for f in fields:
            jhas = [i for i in common if f in J[(t, i)]]
            nhas = [i for i in common if f in N[(t, i)]]
            if not nhas:
                print(f'   java-only   {f}'); continue
            if not jhas:
                print(f'   native-only {f}'); continue
            bad, rep = [], []
            for i in common:
                a, b = J[(t, i)].get(f), N[(t, i)].get(f)
                if a == b:
                    continue
                if a is not None and b is not None and zeroish(a) and zeroish(b):
                    rep.append(i); continue
                bad.append(i)
            if bad:
                failures += 1
                print(f'   MISMATCH    {f}: {len(bad)}/{len(common)} objects, e.g.')
                for i in bad[:examples]:
                    print(f'      {t} {i}: java={J[(t, i)].get(f, "-")[:160]}')
                    print(f'      {" " * len(t)} {" " * len(str(i))}  natv={N[(t, i)].get(f, "-")[:160]}')
            elif rep:
                print(f'   null~zero   {f}: {len(rep)} objects')
            else:
                print(f'   match       {f}')
    print(f'\n{failures} failing checks')
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
