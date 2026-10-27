"""Strict structural inspection of the SGR1 source scene snapshot.

This does not run native traversal or establish painter parity. It verifies the
binary layout and identity/state counts before a C fixture loader is added.
"""
import pathlib
import hashlib
import struct
import sys


class Reader:
    def __init__(self, path):
        self.data = pathlib.Path(path).read_bytes()
        self.pos = 0

    def ints(self, count=1):
        if count < 0 or self.pos + count * 4 > len(self.data):
            raise ValueError(f"invalid int count {count} at {self.pos}")
        values = struct.unpack_from(">" + "i" * count, self.data, self.pos)
        self.pos += count * 4
        return values if count != 1 else values[0]

    def skip(self, count):
        if count < 0 or self.pos + count > len(self.data):
            raise ValueError(f"invalid byte count {count} at {self.pos}")
        self.pos += count

    def array(self):
        count = self.ints()
        if count < -1 or count > 100000:
            raise ValueError(f"invalid array length {count}")
        if count >= 0:
            self.skip(count * 4)

    def digest(self):
        count = self.ints()
        if count not in (0, 32):
            raise ValueError(f"invalid model digest length {count}")
        value = self.data[self.pos:self.pos + count]
        self.skip(count)
        return value

    def utf(self):
        count = struct.unpack_from(">H", self.data, self.pos)[0]
        self.skip(2 + count)


def painter_models(path):
    r = Reader(path)
    assert r.ints() == 0x50323235
    r.skip(8)  # brightness
    models = {}
    while True:
        tag = r.ints()
        if tag == 0:
            r.skip(15 * 4)
        elif tag == 1:
            identity, length = r.ints(2)
            assert identity == len(models) and 0 < length < 1000000
            payload = r.data[r.pos:r.pos + length]
            assert len(payload) == length
            models[identity] = (hashlib.sha256(payload).digest(),
                                struct.unpack_from(">i", payload, 20)[0])
            r.skip(length)
        elif tag == 2:
            r.skip(12 * 4)
        elif tag in (3, 4):
            r.skip(7 * 4)  # state/camera
            kind, *_ = r.ints(7)  # geometry kind, tile, heights
            if kind == 0:
                assert tag == 3
                r.skip(7 * 4)
            else:
                assert kind == 1 and tag == 4
                *_, vertices, triangles = r.ints(8)
                r.skip((vertices * 3 + triangles * 7) * 4)
        elif tag == 5:
            *_, pixels = r.ints(5)
            r.skip(pixels * 4)
        elif tag == 9:
            r.skip(5 * 4)
            assert r.pos == len(r.data)
            return models
        else:
            raise ValueError(f"unknown painter tag {tag} at {r.pos - 4}")


def scene_heights(path):
    r = Reader(path)
    assert r.ints() == 0x534d5931
    count = r.ints()
    assert 0 <= count <= 100000
    values = {}
    for _ in range(count):
        digest = r.data[r.pos:r.pos + 32]
        assert len(digest) == 32 and digest not in values
        r.skip(32)
        values[digest] = r.ints()
    assert r.pos == len(r.data)
    return values


def inspect(path, trace_path=None, heights_path=None):
    r = Reader(path)
    assert r.ints() == 0x47523235
    grounds = locations = slots = None
    roots = []
    model_refs = set()
    for frame in range(12):
        assert r.ints() == 0x53475231
        h = r.ints(15)
        assert h[0] == frame and h[7:10] == (4, 104, 104)
        assert h[11] == (frame == 0) and h[12] == frame + 1
        roots.append(h[14])
        if frame == 0:
            grid_count = r.ints()
            r.skip(4 * 4 * 105 * 105)
            r.skip(grid_count * 4 * 4)
            grounds = r.ints()
            links = []
            slots = 0
            for index in range(grounds):
                assert r.ints() == index
                g = r.ints(18)
                assert g[0] == index and 0 <= g[6] <= 5
                links.append((g[16], g[17]))
                r.skip(8)  # Linkable.key
                if r.ints():
                    r.skip(7 * 4)  # TileUnderlay fields
                if r.ints():
                    r.skip(5 * 4)  # TileOverlay scalar fields
                    for _ in range(10):
                        r.array()
                if r.ints():
                    r.skip(7 * 4)  # Wall fields after presence
                    model_refs.update((r.digest(), r.digest()))
                if r.ints():
                    r.skip(7 * 4)  # Decor fields after presence
                    model_refs.add(r.digest())
                if r.ints():
                    r.skip(5 * 4)  # GroundDecor fields after presence
                    model_refs.add(r.digest())
                if r.ints():
                    r.skip(5 * 4)  # GroundObject fields after presence
                    model_refs.update((r.digest(), r.digest(), r.digest()))
                r.skip(g[6] * 8)  # (shared Location id, tile span)
                slots += g[6]
            locations = r.ints()
            for index in range(locations):
                loc = r.ints(17)
                assert loc[0] == index and loc[16] in (0, 1, 2)
                r.utf()
                model_refs.add(r.digest())
            assert links[roots[0]] == (-1, -1), "first root already linked"
            assert all(link == (-1, -1) for link in links), "non-empty drawTile queue at first entry"
        else:
            assert r.ints() == grounds
            for index in range(grounds):
                state = r.ints(9)
                assert state[0] == index
        assert r.ints() == locations
        for index in range(locations):
            state = r.ints(3)
            assert state[0] == index
        r.skip(4 * 4 * 105 * 105)  # tile occlusion cycles
        for _ in range(4):
            count = r.ints()
            assert 0 <= count <= 500
            r.skip(count * 18 * 4)
        active = r.ints()
        assert 0 <= active <= 500
        r.skip(active * 18 * 4)
    footer = r.ints(8)
    assert footer == (0x454e4431, 12, grounds, locations, slots,
                      slots - locations, 0, 0)
    assert r.pos == len(r.data)
    print(f"SGR1 structural check: 12 frames, {grounds} grounds, "
          f"{locations} shared locations, {slots} slots, "
          f"{len(model_refs - {b''})} distinct referenced models; roots={roots}")
    if trace_path:
        assets = painter_models(trace_path)
        assert all(digest in model_refs for digest, _ in assets.values())
        heights = {digest: height for digest, height in assets.values()}
        assert len(heights) == len({digest for digest, _ in assets.values()})
        print(f"P225 model metadata: {len(assets)} snapshots, "
              f"{len(heights)} unique digests, all referenced by SGR1")
    if heights_path:
        all_heights = scene_heights(heights_path)
        assert set(all_heights) == model_refs - {b''}
        if trace_path:
            assert all(all_heights[digest] == height
                       for digest, height in assets.values())
        print(f"SMY1 model heights: {len(all_heights)} references, "
              "all SGR1 digests covered and P225 heights agree")


if __name__ == "__main__":
    inspect(*sys.argv[1:])
