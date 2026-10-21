"""Compare per-frame region hashes from ParityOracle (Java) and rs2replay (C).

usage: parity_compare.py java-regions.txt c-regions.txt java-report.txt

Only frames after the Java login are compared (title-screen flames come from an
unseeded thread in the original). Exit 0 only when every compared region of every
compared frame matches and the oracle reported no divergence.
"""
import sys
from collections import OrderedDict


def load(path):
    table = {}
    for line in open(path):
        parts = line.split()
        if len(parts) == 3:
            table[(int(parts[0]), parts[1])] = parts[2]
    return table


java = load(sys.argv[1])
native = load(sys.argv[2])
report = open(sys.argv[3]).read().splitlines()

print('== oracle report')
for line in report:
    print('  ' + (line if len(line) < 300 else line[:300] + '...'))

# Compare from the first frame where Java drew an in-game area.
ingame_areas = ('areaViewport', 'areaSidebar', 'areaMapback', 'areaChatback')
frames = sorted({f for f, _ in java})
start = next((f for f in frames if any((f, a) in java for a in ingame_areas)), None)
if start is None:
    print('== no in-game frames drawn by the Java client')
    sys.exit(1)

regions = OrderedDict()
for f, name in sorted(java):
    if f >= start:
        regions.setdefault(name, None)

print('== regions from frame %d (first in-game frame)' % start)
ok = True
for name in regions:
    compared = [f for f in frames if f >= start and (f, name) in java and (f, name) in native]
    bad = [f for f in compared if java[(f, name)] != native[(f, name)]]
    if bad:
        ok = False
    print('  %-16s %4d frames, %4d differ%s' % (name, len(compared), len(bad), ', first at frame %d' % bad[0] if bad else ''))

diverged = any(line.startswith('DIVERGE') for line in report) or not any(line == 'RESULT MATCH' for line in report)
if diverged:
    ok = False
print('== PARITY ' + ('MATCH' if ok else 'DIFFERS'))
sys.exit(0 if ok else 1)
