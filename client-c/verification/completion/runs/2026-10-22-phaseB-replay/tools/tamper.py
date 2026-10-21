"""Make negative-control copies of a session log."""
import sys
from pathlib import Path

src = Path(sys.argv[1])
out = Path(sys.argv[2])
out.mkdir(parents=True, exist_ok=True)
lines = src.read_text().split('\n')

# 1. Flip one bit in the payload of a large inbound read midway through the session.
reads = [i for i, l in enumerate(lines) if l.startswith('NR ') and len(l.split()[-1]) > 40]
target = reads[len(reads) // 2]
fields = lines[target].split(' ')
data = bytearray.fromhex(fields[-1])
data[len(data) // 2] ^= 0x10
fields[-1] = data.hex()
flipped = list(lines)
flipped[target] = ' '.join(fields)
(out / 'flipped-inbound.rec').write_text('\n'.join(flipped))
print('flipped inbound byte on line', target + 1, 'of', len(lines))

# 2. Truncate the log at 60%.
cut = int(len(lines) * 0.6)
(out / 'truncated.rec').write_text('\n'.join(lines[:cut]) + '\n')
print('truncated at line', cut)
