"""Render generated native traversal with SGR1 geometry and digest-bound models.

P225 supplies model definitions, initial raster state, and comparison data.
Its command order and recorded ground calls never supply rendering inputs.
"""
import array
import ctypes as c
import hashlib
import pathlib
import struct
import sys
from inspect_scene_graph225 import Reader
from scene_graph225_reader import load
from run_scene_traversal225 import (
    I, B, P, S, U, Call, Traversal, expected_commands, run,
)


class Underlay(c.Structure):
    _fields_ = [(name, I) for name in ("sw", "se", "ne", "nw", "texture")]
    _fields_ += [("flat", B), ("rgb", I)]


class Overlay(c.Structure):
    _fields_ = [("vertices", I)]
    _fields_ += [(name, I * 10) for name in ("vx", "vy", "vz")]
    _fields_ += [("triangles", I)]
    _fields_ += [(name, I * 10) for name in ("a", "b", "c", "ca", "cb", "cc")]
    _fields_ += [("texture_id", I), ("textures", I * 10)]
    _fields_ += [(name, I) for name in ("shape", "rotation", "background", "foreground")]
    _fields_ += [("flat", B)]


class Geometry(c.Structure):
    _fields_ = [("underlay", c.POINTER(Underlay)), ("overlay", c.POINTER(Overlay))]


class View(c.Structure):
    _fields_ = [(name, I) for name in ("eye_x", "eye_y", "eye_z", "pitch", "yaw")]
    _fields_ += [("low_memory", B), ("taking_input", B)]
    _fields_ += [(name, I) for name in ("mouse_x", "mouse_y", "click_x", "click_z")]


RESOLVE = c.CFUNCTYPE(P, P, c.POINTER(U))


class Picking(c.Structure):
    _fields_ = [("check_hover", B), ("mouse_x", I), ("mouse_y", I),
                ("bitsets", c.POINTER(I)), ("count", S), ("capacity", S)]


class Raster(c.Structure):
    _fields_ = [("geometry", c.POINTER(Geometry)), ("geometry_count", S),
                ("resolve", RESOLVE), ("resolve_user", P), ("view", View)]
    _fields_ += [(name, c.c_uint64) for name in ("models", "underlays", "overlays")]
    _fields_ += [("error", I), ("failed_call", Call)]
    _fields_ += [("picking", Picking)]


class Visibility(c.Structure):
    _fields_ = [("projection", I * 10), ("matrix", B * (8 * 32 * 51 * 51))]


def comparisons(path):
    r = Reader(path)
    assert r.ints() == 0x50323235
    brightness = struct.unpack_from(">d", r.data, r.pos)[0]
    r.skip(8)
    assets = {}
    frames = []
    frame = None
    while True:
        tag = r.ints()
        if tag == 0:
            assert frame is None
            frame = {"header": r.ints(15), "entry_states": []}
        elif tag == 1:
            identity, length = r.ints(2)
            assert identity == len(assets) and 0 < length < 1000000
            digest = hashlib.sha256(r.data[r.pos:r.pos + length]).digest()
            assert digest not in assets
            assets[digest] = r.pos
            r.skip(length)
        elif tag == 2:
            values = r.ints(12)
            frame["entry_states"].append(values[10:12])
        elif tag in (3, 4):
            state = r.ints(7)
            frame["entry_states"].append(state[:2])
            # Recorded ground geometry is comparison-only and is not retained.
            kind, *_ = r.ints(7)
            if tag == 3:
                assert kind == 0
                r.skip(7 * 4)
            else:
                assert kind == 1
                *_, vertices, triangles = r.ints(8)
                r.skip((vertices * 3 + triangles * 7) * 4)
        elif tag == 5:
            index, alpha, clip, remaining, count = r.ints(5)
            assert index == len(frames) and remaining == 0 and count == 512 * 334 + 8
            pixels = array.array("I", r.data[r.pos:r.pos + count * 4])
            if sys.byteorder == "little":
                pixels.byteswap()
            r.skip(count * 4)
            frame.update(pixels=pixels, final_state=(alpha, clip))
            frames.append(frame)
            frame = None
        elif tag == 9:
            totals = r.ints(5)
            assert totals[0] == len(frames) == 12 and totals[4] == len(assets)
            assert frame is None and r.pos == len(r.data)
            return brightness, assets, frames
        else:
            raise ValueError(f"unknown painter tag {tag}")


class Renderer:
    def __init__(self, graph, library, textures, trace):
        self.dll = dll = c.CDLL(str(pathlib.Path(library).resolve()))
        dll.painter_trace_open.argtypes = [c.c_char_p, c.c_double]
        dll.painter_trace_open.restype = P
        dll.painter_trace_load_asset.argtypes = [c.c_char_p, I]
        dll.painter_trace_load_asset.restype = P
        dll.painter_trace_free_asset.argtypes = [P]
        dll.painter_trace_begin.argtypes = [P, c.POINTER(c.c_uint32), c.POINTER(I)]
        dll.painter_trace_begin.restype = B
        dll.painter_trace_close.argtypes = [P]
        dll.world3d_render_scene225.argtypes = [c.POINTER(Traversal), I, c.POINTER(Raster)]
        dll.world3d_render_scene225.restype = B
        self.alpha = I.in_dll(dll, "pix3d_trans")
        self.clip = B.in_dll(dll, "pix3d_hclip")
        brightness, offsets, self.frames = comparisons(trace)
        self.cameras = [frame["header"] for frame in graph["frames"]]
        self.graph = graph
        self.preparation_bad = 0
        self.fixture = dll.painter_trace_open(str(pathlib.Path(textures).resolve()).encode(), brightness)
        if not self.fixture:
            raise RuntimeError("texture archive initialization failed")
        self.models = {}
        trace_path = str(pathlib.Path(trace).resolve()).encode()
        for digest, offset in offsets.items():
            model = dll.painter_trace_load_asset(trace_path, offset)
            if not model:
                raise RuntimeError(f"model load failed: {digest.hex()}")
            self.models[digest] = model
        self.missing = set()

        def resolve(_user, digest):
            key = bytes(digest[:32])
            model = self.models.get(key)
            if model is None:
                self.missing.add(key.hex())
            return model

        self.resolver = RESOLVE(resolve)
        self.geometry = (Geometry * len(graph["grounds"]))()
        self.keep = []
        for index, source in enumerate(graph["grounds"]):
            if source["underlay"]:
                sw, se, ne, nw, texture, flat, rgb = source["underlay"]["fields"]
                u = Underlay(sw, se, ne, nw, texture, flat, rgb)
                self.keep.append(u)
                self.geometry[index].underlay = c.pointer(u)
            if source["overlay"]:
                data = source["overlay"]
                o = Overlay()
                o.shape, o.rotation, o.flat, o.background, o.foreground = data["fields"]
                arrays = data["arrays"]
                o.vertices, o.triangles = len(arrays[0]), len(arrays[3])
                assert 0 <= o.vertices <= 6 and 0 <= o.triangles <= 10
                for name, values in zip(("vx", "vy", "vz", "ca", "cb", "cc", "a", "b", "c"), arrays):
                    target = getattr(o, name)
                    target[:len(values)] = values
                o.texture_id = -1 if arrays[9] is None else 0
                textures_for_tile = [-1] * o.triangles if arrays[9] is None else arrays[9]
                o.textures[:o.triangles] = textures_for_tile
                self.keep.append(o)
                self.geometry[index].overlay = c.pointer(o)
        self.bad = self.checked = self.state_bad = self.passed = 0

    def prepare(self, index, scene):
        if not hasattr(self, "visibility"):
            self.visibility = Visibility()
            init = self.dll.world3d_visibility_init225
            init.argtypes = [c.POINTER(Visibility), c.POINTER(I), I, I, I, I]
            init.restype = B
            sine = (I * 2048).in_dll(self.dll, "pix3d_sin_table")
            distances = (I * 9)(*[
                ((i * 32 + 143) * 3 + 600) * sine[i * 32 + 143] >> 16 for i in range(9)])
            if not init(c.byref(self.visibility), distances, 800, 512, 334, 500):
                raise RuntimeError("native visibility initialization failed")
            native = self.dll.world3d_prepare_scene225
            native.argtypes = [c.POINTER(Traversal), c.POINTER(Visibility)] + [I] * 6
            native.restype = B
        h = self.cameras[index]
        if not self.dll.world3d_prepare_scene225(c.byref(scene), c.byref(self.visibility),
                                               h[2], h[5], h[6], h[4], h[1], h[3]):
            raise RuntimeError("native scene preparation failed")
        expected = self.graph["frames"][index]
        bad = int(scene.cycle != h[12]) + int(scene.tiles_remaining != h[13])
        for identity, wanted in enumerate(expected["ground_state"]):
            g = scene.grounds[getattr(self, "ground_map", {}).get(identity, identity)]
            actual = (int(g.visible), int(g.update), int(g.contains_locs), g.loc_spans,
                      g.check_loc_spans, g.block_loc_spans, g.inverse_block_loc_spans, g.back_wall_types)
            if actual != wanted:
                if bad < 2:
                    print(f"preparation ground={identity} actual={actual} expected={wanted}")
                bad += 1
        for identity, wanted in enumerate(expected["location_state"]):
            loc = scene.locations[getattr(self, "location_map", {}).get(identity, identity)]
            bad += (loc.cycle, loc.distance) != wanted
        bad += any(value != scene.occlusion.tile_cycles[i]
                   for i, value in enumerate(expected["tile_cycles"]))
        active = []
        names = ("type", "min_tile_x", "max_tile_x", "min_tile_z", "max_tile_z", "min_x", "max_x",
                 "min_z", "max_z", "min_y", "max_y", "mode", "min_delta_x", "max_delta_x",
                 "min_delta_z", "max_delta_z", "min_delta_y", "max_delta_y")
        o = scene.occlusion
        for i in range(o.active_count):
            item = o.pool[o.active[i]]
            active.append(tuple(getattr(item, name) for name in names))
        bad += tuple(active) != expected["active_occluders"]
        self.preparation_bad += bad
        print(f"native preparation frame={index} remaining={scene.tiles_remaining} "
              f"active={o.active_count} state_differences={bad}")
        return not bad

    def begin(self, index, scene):
        frame = self.frames[index]["header"]
        camera = self.cameras[index]
        assert frame[3:9] == camera[1:7]
        assert (frame[4], frame[7], frame[8]) == (scene.eye_x, scene.eye_y, scene.eye_z)
        self.storage = (c.c_uint32 * (512 * 334 + 16))(*([0x19372b] * (512 * 334 + 16)))
        pixels = c.cast(c.byref(self.storage, 8 * 4), c.POINTER(c.c_uint32))
        if not self.dll.painter_trace_begin(self.fixture, pixels, (I * 15)(*frame)):
            raise RuntimeError("raster frame initialization failed")
        self.raster = Raster()
        self.raster.geometry = self.geometry
        self.raster.geometry_count = len(self.geometry)
        self.raster.resolve = self.resolver
        self.raster.view.pitch, self.raster.view.yaw = camera[4], camera[1]
        self.raster.view.low_memory = frame[10] != 0
        self.frame_states = 0
        self.observed = 0

    def observe(self, index, ordinal):
        states = self.frames[index]["entry_states"]
        actual = (self.alpha.value, int(self.clip.value))
        if ordinal >= len(states) or actual != states[ordinal]:
            if self.frame_states < 3:
                print(f"raster entry mismatch frame={index} call={ordinal} actual={actual}")
            self.frame_states += 1
        self.observed += 1

    def render(self, scene, root):
        return bool(self.dll.world3d_render_scene225(c.byref(scene), root, c.byref(self.raster)))

    def finish(self, index, scene):
        frame = self.frames[index]
        diffs = 0
        for offset, expected in enumerate(frame["pixels"]):
            actual = self.storage[offset + 8]
            if actual != expected:
                if diffs < 3:
                    print(f"raster word mismatch frame={index} word={offset} C={actual:08x} Java={expected:08x}")
                diffs += 1
        guards = all(self.storage[i] == 0x19372b for i in range(8))
        state = (self.alpha.value, int(self.clip.value)) == frame["final_state"]
        complete = scene.tiles_remaining == 0 and not scene.taking_input
        counts = self.observed == len(frame["entry_states"]) == (
            self.raster.models + self.raster.underlays + self.raster.overlays)
        good = not (diffs or self.frame_states or self.raster.error) and guards and state and complete and counts
        self.bad += diffs
        self.checked += len(frame["pixels"])
        self.state_bad += self.frame_states + int(not state)
        self.passed += int(good)
        print(f"raster frame={index} exact={good} differences={diffs} entry_state_differences={self.frame_states} "
              f"final_state={state} prefix_guards={guards} complete={complete} counts={counts} error={self.raster.error}")
        if self.raster.error:
            call = self.raster.failed_call
            print(f"  failed kind={call.kind} ground={call.ground_id} digest={bytes(call.model).hex()}")
        return good

    def close(self):
        for model in self.models.values():
            self.dll.painter_trace_free_asset(model)
        self.dll.painter_trace_close(self.fixture)
        print(f"Generated scene raster: {self.passed}/12 frames; {self.checked} framebuffer/guard words; "
              f"{self.bad} differences; {self.state_bad} entry/final state differences; missing_models={len(self.missing)}")
        if self.missing:
            print("Missing model digests:", sorted(self.missing))
        print(f"Native preparation state differences: {self.preparation_bad}")


if __name__ == "__main__":
    if len(sys.argv) not in (6, 7) or (len(sys.argv) == 7 and sys.argv[6] != "--native-prepare"):
        sys.exit("usage: run_scene_raster225.py graph heights trace native-library textures-archive [--native-prepare]")
    graph = load(sys.argv[1], sys.argv[2])
    renderer = Renderer(graph, sys.argv[4], sys.argv[5], sys.argv[3])
    try:
        matched = run(graph, sys.argv[4], expected_commands(sys.argv[3]), renderer=renderer,
                      native_prepare=len(sys.argv) == 7)
    finally:
        renderer.close()
    sys.exit(0 if matched and renderer.passed == 12 else 1)
