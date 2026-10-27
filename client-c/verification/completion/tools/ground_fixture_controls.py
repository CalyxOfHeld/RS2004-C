"""Create two damaged fixture copies using streaming I/O (no huge PS slices)."""
import json
import shutil
import struct
import sys
from pathlib import Path

fixture, output = map(Path, sys.argv[1:])
with fixture.open('rb') as f:
    def word():
        data = f.read(4)
        if len(data) != 4:
            raise ValueError('truncated ground fixture')
        return struct.unpack('>I', data)[0]
    if word() != 0x47323235:
        raise ValueError('expected G225')
    f.seek(8, 1)
    tiles = word()
    for _ in range(tiles):
        kind = word()
        f.seek(6 * 4, 1)  # coordinates and four heights
        if kind == 0:
            f.seek(7 * 4, 1)
        elif kind == 1:
            f.seek(6 * 4, 1)
            vertices, faces = word(), word()
            f.seek((vertices * 3 + faces * 7) * 4, 1)
        else:
            raise ValueError('bad tile kind')
    cases = word()
    first_pixel = f.tell() + (17 + 4 + 1) * 4
wrong = output / 'wrong-pixel.bin'
shutil.copyfile(fixture, wrong)
with wrong.open('r+b') as f:
    f.seek(first_pixel)
    value = f.read(1)
    f.seek(first_pixel)
    f.write(bytes([value[0] ^ 1]))
truncated = output / 'truncated.bin'
shutil.copyfile(fixture, truncated)
with truncated.open('r+b') as f:
    f.truncate(truncated.stat().st_size - 1)
(output / 'fixture-controls.json').write_text(json.dumps({'tiles': tiles, 'cases': cases, 'first_pixel_offset': first_pixel}, indent=2) + '\n')
