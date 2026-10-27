"""Build from cache/maps in C, compare construction, prepare and render natively."""
import collections
import ctypes as c
import pathlib
import sys
from run_scene_raster225 import Renderer, Geometry, RESOLVE, I, P, load, expected_commands, run
from run_scene_traversal225 import Traversal, Occluder, S, D, B


class Bounds(c.Structure):
    _fields_ = [(name, I) for name in ("radius", "max_y", "min_depth", "max_depth")]


class Asset(c.Structure):
    _fields_ = [("model", P), ("bounds", Bounds), ("pickable", B)]


class NativeRenderer(Renderer):
    def __init__(self, graph, library, cache, maps, trace):
        super().__init__(graph, library, pathlib.Path(cache) / "textures", trace)
        dll = self.dll
        dll.painter_native_open.argtypes = [c.c_char_p, c.c_char_p, c.c_double]
        dll.painter_native_open.restype = P
        dll.painter_native_close.argtypes = [P]
        dll.painter_native_construct.argtypes = [P]
        dll.painter_native_construct.restype = P
        dll.world3d_scene225.argtypes = [P]
        dll.world3d_scene225.restype = c.POINTER(Traversal)
        dll.world3d_scene_geometry225.argtypes = [P]
        dll.world3d_scene_geometry225.restype = c.POINTER(Geometry)
        dll.world3d_scene_pool225.argtypes = [P, I, c.POINTER(S)]
        dll.world3d_scene_pool225.restype = c.POINTER(Occluder)
        dll.world3d_scene_resolve225.argtypes = [P, c.POINTER(c.c_uint8)]
        dll.world3d_scene_resolve225.restype = P
        dll.world3d_destroy_scene225.argtypes = [P]
        dll.painter_native_compare_asset.argtypes = [P, P]
        dll.painter_native_compare_asset.restype = I
        self.native_fixture = dll.painter_native_open(
            str(pathlib.Path(cache).resolve()).encode(), str(pathlib.Path(maps).resolve()).encode(), 0.8)
        if not self.native_fixture:
            raise RuntimeError("native cache/map construction failed")
        dll.painter_native_brightness.argtypes = [P]
        dll.painter_native_brightness.restype = c.c_double
        dll.painter_trace_set_brightness.argtypes = [P, c.c_double]
        dll.painter_trace_set_brightness(self.fixture, dll.painter_native_brightness(self.native_fixture))
        self.construction = dll.painter_native_construct(self.native_fixture)
        if not self.construction:
            raise RuntimeError("native topology construction failed")
        self.native_scene = dll.world3d_scene225(self.construction)
        native = self.native_scene.contents
        expected_geometry = self.geometry
        self.geometry = c.cast(dll.world3d_scene_geometry225(self.construction),
                               c.POINTER(Geometry * native.ground_count)).contents
        self.native_pools, self.pool_counts = [], []
        for level in range(4):
            count = S()
            self.native_pools.append(dll.world3d_scene_pool225(self.construction, level, c.byref(count)))
            self.pool_counts.append(count.value)
        self.resolver = RESOLVE(("world3d_scene_resolve225", dll))
        self.ground_map, self.location_map, self.identities = {}, {}, {}
        self.errors, self.samples = collections.Counter(), {}
        seen_models = set()

        def error(name, detail):
            self.errors[name] += 1
            self.samples.setdefault(name, detail)

        height_bad = sum(value != native.occlusion.heights[i] for i, value in enumerate(graph["heights"]))
        if height_bad:
            error("heightmap", height_bad)

        def bind_ground(source, actual):
            if source in self.ground_map:
                if self.ground_map[source] != actual:
                    error("ground_alias", (source, actual))
                return
            self.ground_map[source] = actual
            g = native.grounds[actual]
            bridge = graph["grounds"][source]["fields"][15]
            if (bridge < 0) != (g.bridge < 0):
                error("bridge", (source, bridge, g.bridge))
            elif bridge >= 0:
                bind_ground(bridge, g.bridge)

        for index, wanted in enumerate(graph["grid"]):
            actual = native.grid[index]
            if (wanted < 0) != (actual < 0):
                error("grid_presence", (index, wanted, actual))
            elif wanted >= 0:
                bind_ground(wanted, actual)

        def bind_model(key, digest):
            key = bytes(key)
            if not digest:
                if any(key):
                    error("extra_model", key.hex())
                return
            if (key, digest) in seen_models:
                return
            seen_models.add((key, digest))
            ptr = dll.world3d_scene_resolve225(self.construction, (c.c_uint8 * 32)(*key))
            if not ptr:
                error("missing_model", digest.hex())
                return
            asset = c.cast(ptr, c.POINTER(Asset)).contents
            if asset.bounds.max_y != graph["model_heights"][digest]:
                error("model_max_y", (key.hex(), asset.bounds.max_y, graph["model_heights"][digest]))
            if digest in self.models:
                code = dll.painter_native_compare_asset(ptr, self.models[digest])
                if code:
                    error(f"model_fields_{code}", (key.hex(), digest.hex()))
                else:
                    self.identities[key] = digest

        for source_id, actual_id in self.ground_map.items():
            source = graph["grounds"][source_id]
            g = native.grounds[actual_id]
            names = ("level", "x", "z", "occlude_level", "draw_level", "loc_count", "loc_spans")
            actual = tuple(getattr(g, name) for name in names)
            if actual != source["fields"][1:8]:
                error("ground_fields", (source_id, actual, source["fields"][1:8]))
            for name in ("underlay", "overlay"):
                a, b = getattr(self.geometry[actual_id], name), getattr(expected_geometry[source_id], name)
                if bool(a) != bool(b):
                    error(name + "_presence", source_id)
                elif a:
                    a, b = a.contents, b.contents
                    for field, _ in a._fields_:
                        av, bv = getattr(a, field), getattr(b, field)
                        if field == "texture_id" and name == "overlay":
                            av, bv = av >= 0, bv >= 0
                        elif isinstance(av, c.Array):
                            av, bv = tuple(av), tuple(bv)
                        if av != bv:
                            error(name + "_" + field, (source_id, av, bv))
            for name, scalars, model_names in (
                ("wall", ("x", "y", "z", "type_a", "type_b", "bitset"), ("model_a", "model_b")),
                ("decor", ("x", "y", "z", "type", "angle", "bitset"), ("model",)),
                ("ground_decor", ("x", "y", "z", "bitset"), ("model",)),
            ):
                item = getattr(g, name)
                wanted = source[name]
                if bool(item.present) != bool(wanted):
                    error(name + "_presence", source_id)
                elif wanted:
                    values = tuple(getattr(item, field) for field in scalars)
                    if values != wanted["fields"][:-1]:
                        error(name + "_fields", (source_id, values, wanted["fields"][:-1]))
                    for field, digest in zip(model_names, wanted["models"]):
                        bind_model(getattr(item, field), digest)
            if len(source["slots"]) != g.loc_count:
                error("slot_count", source_id)
            for i, (loc_id, span) in enumerate(source["slots"][:g.loc_count]):
                native_id = g.locs[i]
                if loc_id in self.location_map and self.location_map[loc_id] != native_id:
                    error("location_alias", (loc_id, native_id))
                self.location_map[loc_id] = native_id
                if span != g.loc_span[i]:
                    error("location_span", (source_id, i))
        for source_id, actual_id in self.location_map.items():
            source = graph["locations"][source_id]
            loc = native.locations[actual_id]
            names = ("level", "x", "y", "z", "yaw", "min_x", "max_x", "min_z", "max_z")
            values = tuple(getattr(loc, name) for name in names)
            if values != source["fields"][1:10] or (loc.bitset, loc.info) != source["fields"][12:14]:
                error("location_fields", (source_id, values, source["fields"][1:10], loc.bitset, loc.info))
            bind_model(loc.model, source["model"])
        if len(self.ground_map) != len(graph["grounds"]):
            error("ground_count", (len(self.ground_map), len(graph["grounds"])))
        if len(self.location_map) != len(graph["locations"]) or native.location_count != len(graph["locations"]):
            error("location_count", (len(self.location_map), native.location_count, len(graph["locations"])))
        names = ("type", "min_tile_x", "max_tile_x", "min_tile_z", "max_tile_z", "min_x", "max_x",
                 "min_z", "max_z", "min_y", "max_y")
        for level in range(4):
            actual = tuple(tuple(getattr(self.native_pools[level][i], name) for name in names)
                           for i in range(self.pool_counts[level]))
            wanted = tuple(values[:11] for values in graph["frames"][0]["occluder_pools"][level])
            if actual != wanted:
                error("occluders", (level, len(actual), len(wanted),
                                     next(((a, b) for a, b in zip(actual, wanted) if a != b), None)))
        print(f"Native construction: grounds={len(self.ground_map)} locations={native.location_count} "
              f"validated_drawn_models={len(set(self.identities.values()))} pools={self.pool_counts}")
        print("Construction differences:", dict(self.errors))
        for name, detail in self.samples.items():
            print(name, detail)

    def identity(self, key):
        return self.identities.get(key, key)

    def begin(self, index, scene):
        super().begin(index, scene)
        self.raster.resolve_user = self.construction

    def close(self):
        self.dll.world3d_destroy_scene225(self.construction)
        self.dll.painter_native_close(self.native_fixture)
        super().close()


if __name__ == "__main__":
    if len(sys.argv) != 7:
        sys.exit("usage: run_native_scene225.py graph heights trace native-library cache maps")
    graph = load(sys.argv[1], sys.argv[2])
    renderer = NativeRenderer(graph, sys.argv[4], sys.argv[5], sys.argv[6], sys.argv[3])
    try:
        matched = run(graph, sys.argv[4], expected_commands(sys.argv[3]), renderer=renderer, native_prepare=True)
    finally:
        renderer.close()
    sys.exit(0 if matched and renderer.passed == 12 and not renderer.errors else 1)
