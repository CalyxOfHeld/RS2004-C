"""Bounded dynamic source-call scenario using the existing complete-frame comparator."""
import array
import ctypes as c
import os
import pathlib
import struct
import sys
from run_native_scene225 import NativeRenderer
from run_scene_raster225 import I, P, B, Visibility, load, expected_commands
from run_scene_traversal225 import EMIT, U


def scenarios(path):
    data = pathlib.Path(path).read_bytes()
    offset = 0
    def ints(count):
        nonlocal offset
        values = struct.unpack_from(">" + "i" * count, data, offset)
        offset += count * 4
        return values
    magic, *selection, count = ints(7)
    assert magic == 0x44534331 and count == 12
    frames = []
    for _ in range(count):
        inputs = ints(16)
        length, = ints(1)
        appearance = data[offset:offset + length]
        offset += length
        state = ints(2)
        words = 512 * 334 + 8
        pixels = array.array("I", data[offset:offset + words * 4])
        if sys.byteorder == "little":
            pixels.byteswap()
        offset += words * 4
        frames.append((inputs, appearance, state, pixels, ints(5)))
    assert offset == len(data)
    return selection, frames


def picking_scenarios(path):
    data = pathlib.Path(path).read_bytes()
    magic, count = struct.unpack_from(">2i", data)
    assert magic == 0x504b5331 and count == 12
    offset = 8
    records = []
    for _ in range(count):
        inputs = struct.unpack_from(">10i", data, offset)
        offset += 10 * 4
        final = struct.unpack_from(">5i", data, offset)
        offset += 5 * 4
        bitsets = struct.unpack_from(">1000i", data, offset)
        offset += 1000 * 4
        records.append((inputs, final, bitsets))
    assert offset == len(data)
    return records


def main(graph_path, heights, trace, scenario, library, cache, maps, picking=None):
    dependencies = (os.add_dll_directory(str(pathlib.Path(__file__).resolve().parents[3] / "bin"))
                    if hasattr(os, "add_dll_directory") else None)
    graph = load(graph_path, heights)
    selection, frames = scenarios(scenario)
    pick_frames = picking_scenarios(picking) if picking else None
    expected = expected_commands(trace)
    r = NativeRenderer(graph, library, cache, maps, trace)
    dll = r.dll
    dll.painter_dynamic_open.argtypes = [P, P, c.c_char_p, c.POINTER(I)]
    dll.painter_dynamic_open.restype = P
    dll.painter_dynamic_frame.argtypes = [P, c.POINTER(I), c.POINTER(U), I]
    dll.painter_dynamic_frame.restype = B
    dll.painter_dynamic_preview.argtypes = [P]
    dll.painter_dynamic_preview.restype = B
    dll.painter_dynamic_stats.argtypes = [P, c.POINTER(I)]
    dll.painter_dynamic_close.argtypes = [P]
    dll.painter_dynamic_pick_layout.argtypes = [P, I, B]
    dll.world3d_scene_clear_temporary225.argtypes = [P]
    context = dll.painter_dynamic_open(r.native_fixture, r.construction,
                                     str(pathlib.Path(cache).resolve()).encode(), (I * 5)(*selection))
    if not context:
        raise RuntimeError("dynamic native initialization failed")
    preview = dll.painter_trace_open(str((pathlib.Path(cache) / "textures").resolve()).encode(), 0.7)
    if not preview:
        raise RuntimeError("preview texture initialization failed")
    # Both source setBrightness calls reseed Math.random to 225.
    dll.painter_native_brightness.restype = c.c_double
    dll.painter_trace_set_brightness(preview, dll.painter_native_brightness(r.native_fixture) - 0.1)
    visibility = Visibility()
    dll.world3d_visibility_init225.argtypes = [c.POINTER(Visibility), c.POINTER(I), I, I, I, I]
    dll.world3d_visibility_init225.restype = B
    sine = (I * 2048).in_dll(dll, "pix3d_sin_table")
    distances = (I * 9)(*[((i * 32 + 143) * 3 + 600) * sine[i * 32 + 143] >> 16 for i in range(9)])
    assert dll.world3d_visibility_init225(c.byref(visibility), distances, 800, 512, 334, 500)
    dll.world3d_prepare_scene225.argtypes = [c.POINTER(type(r.native_scene.contents)), c.POINTER(Visibility)] + [I] * 6
    dll.world3d_prepare_scene225.restype = B
    errors = preview_words = model_checks = 0
    old_key = None
    try:
        for index, (inputs, appearance, preview_state, preview_pixels, stats) in enumerate(frames):
            if picking:
                dll.painter_dynamic_pick_layout(context, pick_frames[index][0][8], bool(pick_frames[index][0][9]))
            a = (U * len(appearance)).from_buffer_copy(appearance)
            assert dll.painter_dynamic_frame(context, (I * 16)(*inputs), a, len(appearance))
            if old_key is not None:
                assert not dll.world3d_scene_resolve225(r.construction, (U * 32)(*old_key)), "stale temporary key accepted"
            storage = (c.c_uint32 * (512 * 334 + 16))(*([0x19372b] * (512 * 334 + 16)))
            pixels = c.cast(c.byref(storage, 32), c.POINTER(c.c_uint32))
            header = list(r.frames[index]["header"])
            header[11] = int(index >= 6)
            header[13:15] = [0, 0]
            assert dll.painter_trace_begin(preview, pixels, (I * 15)(*header))
            assert dll.painter_dynamic_preview(context)
            diffs = sum(storage[i + 8] != v for i, v in enumerate(preview_pixels))
            preview_words += len(preview_pixels)
            preview_ok = not diffs and (r.alpha.value, int(r.clip.value)) == preview_state and all(storage[i] == 0x19372b for i in range(8))
            errors += not preview_ok
            print(f"preview={index} exact={preview_ok} differences={diffs}")
            scene = r.native_scene.contents
            header = r.frames[index]["header"]
            yaw, x, level, pitch, y, z = header[3:9]
            scene.occlusion.pool = r.native_pools[level]
            scene.occlusion.count = r.pool_counts[level]
            assert dll.world3d_prepare_scene225(c.byref(scene), c.byref(visibility), x, y, z, pitch, yaw, level)
            # Only scenario camera inputs are supplied; no original per-frame state.
            r.cameras[index] = (index, yaw, x, level, pitch, y, z)
            r.begin(index, scene)
            if picking:
                hover, mx, my, ground, gx, gy, click_x, click_z, _, _ = pick_frames[index][0]
                pick_storage = (I * 1016)(*([0x76543210] * 1016))
                pick_storage[8] = pick_storage[9] = 0x11223344
                p = r.raster.picking
                p.check_hover, p.mouse_x, p.mouse_y = bool(hover), mx, my
                p.bitsets = c.cast(c.byref(pick_storage, 32), c.POINTER(I))
                p.count, p.capacity = 2, 1000
                v = r.raster.view
                v.taking_input, v.mouse_x, v.mouse_y = bool(ground), gx, gy
                v.click_x, v.click_z = click_x, click_z
            calls = []
            def emit(_user, pointer):
                nonlocal errors, model_checks, old_key
                call = pointer.contents
                ordinal = len(calls)
                r.observe(index, ordinal)
                if call.kind == 3:
                    key = bytes(call.model)
                    actual = dll.world3d_scene_resolve225(r.construction, call.model)
                    wanted = expected[index][ordinal] if ordinal < len(expected[index]) else None
                    if wanted and wanted[0] == 3:
                        code = dll.painter_native_compare_asset(actual, r.models[wanted[1]])
                        model_checks += 1
                        if code:
                            if errors < 8:
                                print(f"model mismatch frame={index} call={ordinal} bitset={call.bitset} fields={code}")
                            errors += 1
                        digest = wanted[1] if not code else key
                    else:
                        digest = key
                    if call.bitset == 0x20000002:
                        old_key = key
                    calls.append((3, digest, call.yaw, call.dx, call.dy, call.dz, call.bitset))
                elif call.kind == 1:
                    calls.append((1, call.level, call.x, call.z))
                else:
                    calls.append((2, call.x, call.z))
                return True
            callback = EMIT(emit)
            scene.emit = callback
            complete = r.render(scene, -1)
            exact = r.finish(index, scene)
            native_stats = (I * 5)()
            dll.painter_dynamic_stats(context, native_stats)
            commands_ok = calls == expected[index]
            stats_ok = tuple(native_stats) == stats
            errors += not (complete and exact and commands_ok and stats_ok)
            print(f"dynamic={index} callbacks={len(calls)}/{len(expected[index])} exact={commands_ok} stats={tuple(native_stats)}/{stats}")
            if picking:
                p, v = r.raster.picking, r.raster.view
                result = (int(p.check_hover), int(v.taking_input), v.click_x, v.click_z, p.count)
                desired, bitsets = pick_frames[index][1:]
                guards = all(pick_storage[i] == 0x76543210 for i in (*range(8), *range(1008, 1016)))
                pick_ok = result == desired and tuple(pick_storage[8:1008]) == bitsets and guards
                errors += not pick_ok
                print(f"picking={index} exact={pick_ok} result={result}/{desired} list={list(pick_storage[8:8+p.count])} guards={guards}")
            if not complete:
                break
        print(f"Dynamic checks: models={model_checks} preview_words={preview_words} errors={errors}; stale keys rejected")
        if picking:
            # Bounded caller storage must reject a further append without a
            # write past capacity. This is an API capacity diagnostic.
            scene = r.native_scene.contents
            assert dll.world3d_prepare_scene225(c.byref(scene), c.byref(visibility), x, y, z, pitch, yaw, level)
            scene.emit = EMIT()
            r.begin(11, scene)
            protected = (I * 4)(11, 12, 13, 14)
            p = r.raster.picking
            p.check_hover, p.mouse_x, p.mouse_y = True, 256, 50
            p.bitsets = c.cast(c.byref(protected, 4), c.POINTER(I))
            p.count = p.capacity = 2
            rejected = not r.render(scene, -1) and r.raster.error == 7 and tuple(protected) == (11, 12, 13, 14)
            errors += not rejected
            print(f"Pick capacity: rejected_without_write={rejected} error={r.raster.error}")
        # A bridge moved an old Ground down. Inserting at the vacated level
        # must allocate another identity rather than overwrite that moved tile.
        dll.world3d_scene_temporary225.argtypes = [P] + [I] * 6 + [B, I, P, P, P]
        dll.world3d_scene_temporary225.restype = B
        dll.world3d_scene_clear_temporary225(r.construction)
        scene = r.native_scene.contents
        candidates = [(level, x, z) for level in range(1, 4) for x in range(104) for z in range(104)
                      if scene.grid[(level * 104 + x) * 104 + z] < 0
                      and scene.grid[((level - 1) * 104 + x) * 104 + z] == (level * 104 + x) * 104 + z]
        assert candidates, "no bridge allocation control"
        level, x, z = candidates[0]
        moved = scene.grid[((level - 1) * 104 + x) * 104 + z]
        before = bytes(scene.grounds[moved])
        direct = next(dll.world3d_scene_resolve225(r.construction, loc.model)
                      for loc in scene.locations[:scene.location_count] if any(loc.model))
        assert dll.world3d_scene_temporary225(r.construction, level, x * 128 + 64, 0, z * 128 + 64,
                                             0, 0, False, 1, direct, None, None)
        fresh = scene.grid[(level * 104 + x) * 104 + z]
        alias_ok = fresh != moved and bytes(scene.grounds[moved]) == before
        errors += not alias_ok
        print(f"Bridge allocation: moved={moved} new={fresh} distinct_and_unchanged={alias_ok}")
    finally:
        dll.painter_dynamic_close(context)
        dll.painter_trace_close(preview)
        r.close()
    return 0 if r.passed == 12 and not errors and not r.errors else 1


if __name__ == "__main__":
    sys.exit(main(*sys.argv[1:]))
