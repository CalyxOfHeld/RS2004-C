"""Compare real-cache native construction with the original graph geometry."""
import collections
import ctypes as c
import pathlib
import struct
import sys
from scene_graph225_reader import load
from run_scene_raster225 import Geometry, I, P


def compare(graph, dll, fixture):
    native_heights = dll.painter_native_heights(fixture)
    height_bad = sum(value != native_heights[i] for i, value in enumerate(graph["heights"]))
    differences = collections.Counter()
    samples = {}
    for source in graph["grounds"]:
        identity, level, x, z, occlude_level, *_ = source["fields"]
        geometry = Geometry()
        draw_level = dll.painter_native_tile(fixture, occlude_level, x, z, c.byref(geometry))
        if draw_level != source["fields"][5]:
            differences["draw_level"] += 1
        for name in ("underlay", "overlay"):
            actual = getattr(geometry, name)
            expected = source[name]
            if bool(actual) != bool(expected):
                differences[name + "_presence"] += 1
                samples.setdefault(name + "_presence", (identity, level, x, z))
                continue
            if not expected:
                continue
            actual = actual.contents
            if name == "underlay":
                values = (actual.sw, actual.se, actual.ne, actual.nw, actual.texture, int(actual.flat), actual.rgb)
                wanted = expected["fields"]
                for i, field in enumerate(("sw", "se", "ne", "nw", "texture", "flat", "rgb")):
                    if values[i] != wanted[i]:
                        differences["underlay_" + field] += 1
                        samples.setdefault("underlay_" + field, ((identity, occlude_level, x, z), values[i], wanted[i]))
            else:
                values = (actual.shape, actual.rotation, int(actual.flat), actual.background, actual.foreground)
                if values != expected["fields"]:
                    differences["overlay_fields"] += 1
                    samples.setdefault("overlay_fields", ((identity, occlude_level, x, z), values, expected["fields"]))
                lengths = (actual.vertices,) * 3 + (actual.triangles,) * 7
                for array_name, length, wanted in zip(
                        ("vx", "vy", "vz", "ca", "cb", "cc", "a", "b", "c", "textures"),
                        lengths, expected["arrays"]):
                    values = tuple(getattr(actual, array_name)[:length])
                    if wanted is None:
                        wanted = (-1,) * actual.triangles
                    if values != wanted:
                        differences["overlay_" + array_name] += 1
                        samples.setdefault("overlay_" + array_name, ((identity, occlude_level, x, z), values, wanted))
    print(f"Native heightmap: {len(graph['heights'])} values; differences={height_bad}")
    print("Native geometry differences:", dict(differences))
    for name, sample in samples.items():
        print(name, sample)
    return not (height_bad or differences)


if __name__ == "__main__":
    if len(sys.argv) != 7:
        sys.exit("usage: compare_scene_build225.py graph heights trace native-library cache maps")
    graph = load(sys.argv[1], sys.argv[2])
    dll = c.CDLL(str(pathlib.Path(sys.argv[4]).resolve()))
    dll.painter_trace_open.argtypes = [c.c_char_p, c.c_double]
    dll.painter_trace_open.restype = P
    dll.painter_trace_close.argtypes = [P]
    dll.painter_native_open.argtypes = [c.c_char_p, c.c_char_p, c.c_double]
    dll.painter_native_open.restype = P
    dll.painter_native_close.argtypes = [P]
    dll.painter_native_heights.argtypes = [P]
    dll.painter_native_heights.restype = c.POINTER(I)
    dll.painter_native_tile.argtypes = [P, I, I, I, c.POINTER(Geometry)]
    dll.painter_native_tile.restype = I
    brightness = struct.unpack_from(">d", pathlib.Path(sys.argv[3]).read_bytes(), 4)[0]
    cache = pathlib.Path(sys.argv[5]).resolve()
    context = dll.painter_trace_open(str(cache / "textures").encode(), brightness)
    fixture = dll.painter_native_open(str(cache).encode(), str(pathlib.Path(sys.argv[6]).resolve()).encode(), 0.8)
    if not fixture:
        sys.exit("native construction failed")
    try:
        matched = compare(graph, dll, fixture)
    finally:
        dll.painter_native_close(fixture)
        dll.painter_trace_close(context)
    sys.exit(0 if matched else 1)
