"""Reuse the accepted fixture/model comparator with the production runtime adapter."""
import ctypes as c
import pathlib
import sys
from run_dynamic_scene225 import scenarios, picking_scenarios
from run_native_scene225 import NativeRenderer
from run_scene_raster225 import Raster, Geometry, I, P, B, load, expected_commands
from run_scene_traversal225 import EMIT, U


def main(graph_path, heights, trace, scenario, picking, library, cache, maps):
    selection, frames = scenarios(scenario)
    queries = picking_scenarios(picking)
    expected = expected_commands(trace)
    r = NativeRenderer(load(graph_path, heights), library, cache, maps, trace)
    dll = r.dll
    dll.painter_dynamic_open.argtypes = [P, P, c.c_char_p, c.POINTER(I)]
    dll.painter_dynamic_open.restype = P
    dll.painter_dynamic_frame.argtypes = [P, c.POINTER(I), c.POINTER(U), I]
    dll.painter_dynamic_frame.restype = B
    dll.painter_runtime_preview.argtypes = [P]
    dll.painter_runtime_preview.restype = B
    dll.painter_dynamic_stats.argtypes = [P, c.POINTER(I)]
    dll.painter_dynamic_close.argtypes = [P]
    dll.painter_runtime_open.argtypes = [P]
    dll.painter_runtime_open.restype = P
    dll.painter_runtime_rebuild.argtypes = [P, c.c_char_p]
    dll.painter_runtime_rebuild.restype = B
    dll.painter_dynamic_build.argtypes = [P]
    dll.painter_dynamic_build.restype = P
    dll.painter_runtime_frame.argtypes = [P, P, c.POINTER(I), c.POINTER(I), I, EMIT, P]
    dll.painter_runtime_frame.restype = B
    dll.scene_runtime225_build.argtypes = [P]
    dll.scene_runtime225_build.restype = P
    dll.scene_runtime225_result.argtypes = [P]
    dll.scene_runtime225_result.restype = c.POINTER(Raster)
    dll.scene_runtime225_destroy.argtypes = [P]
    context = dll.painter_dynamic_open(r.native_fixture, r.construction,
                                     str(pathlib.Path(cache).resolve()).encode(), (I * 5)(*selection))
    if not context:
        raise RuntimeError("native actor configuration failed")
    runtime = dll.painter_runtime_open(context)
    if not runtime:
        raise RuntimeError("production runtime creation failed")
    preview = dll.painter_trace_open(str((pathlib.Path(cache) / "textures").resolve()).encode(), 0.7)
    dll.painter_trace_set_brightness(preview, dll.painter_native_brightness(r.native_fixture) - 0.1)
    errors = models = preview_words = 0
    try:
        for index, (inputs, appearance, preview_state, preview_pixels, expected_stats) in enumerate(frames):
            if index == 6:
                assert dll.painter_runtime_rebuild(context, str(pathlib.Path(maps).resolve()).encode())
                r.construction = dll.painter_dynamic_build(context)
                r.native_scene = dll.world3d_scene225(r.construction)
                r.geometry = c.cast(dll.world3d_scene_geometry225(r.construction),
                                    c.POINTER(Geometry * r.native_scene.contents.ground_count)).contents
            packet = (U * len(appearance)).from_buffer_copy(appearance)
            assert dll.painter_dynamic_frame(context, (I * 16)(*inputs), packet, len(appearance))
            storage = (c.c_uint32 * (512 * 334 + 16))(*([0x19372b] * (512 * 334 + 16)))
            pixels = c.cast(c.byref(storage, 32), c.POINTER(c.c_uint32))
            h = list(r.frames[index]["header"])
            h[11], h[13], h[14] = int(index >= 6), 0, 0
            assert dll.painter_trace_begin(preview, pixels, (I * 15)(*h))
            assert dll.painter_runtime_preview(context)
            differences = sum(storage[i + 8] != v for i, v in enumerate(preview_pixels))
            preview_words += len(preview_pixels)
            preview_ok = not differences and (r.alpha.value, int(r.clip.value)) == preview_state
            errors += not preview_ok
            print(f"runtime preview={index} exact={preview_ok} differences={differences}")
            h = r.frames[index]["header"]
            r.cameras[index] = (index, *h[3:9])
            # begin only binds the surface/cache and initializes comparison state.
            binding = r.native_scene.contents
            binding.eye_x, binding.eye_y, binding.eye_z = h[4], h[7], h[8]
            r.begin(index, binding)
            calls = []
            def emit(_user, pointer):
                nonlocal errors, models
                v = pointer.contents
                ordinal = len(calls)
                r.observe(index, ordinal)
                if v.kind == 3:
                    model = dll.world3d_scene_resolve225(dll.scene_runtime225_build(runtime), v.model)
                    wanted = expected[index][ordinal] if ordinal < len(expected[index]) else None
                    digest = bytes(v.model)
                    if wanted and wanted[0] == 3:
                        code = dll.painter_native_compare_asset(model, r.models[wanted[1]])
                        models += 1
                        if code:
                            if errors < 8:
                                print(f"runtime model mismatch frame={index} call={ordinal} bitset={v.bitset} fields={code}")
                            errors += 1
                        else:
                            digest = wanted[1]
                    calls.append((3, digest, v.yaw, v.dx, v.dy, v.dz, v.bitset))
                elif v.kind == 1:
                    calls.append((1, v.level, v.x, v.z))
                else:
                    calls.append((2, v.x, v.z))
                return True
            observer = EMIT(emit)
            ok = dll.painter_runtime_frame(runtime, context, (I * 15)(*h), (I * 10)(*queries[index][0]), inputs[2], observer, None)
            r.raster = dll.scene_runtime225_result(runtime).contents
            build = dll.scene_runtime225_build(runtime)
            scene = dll.world3d_scene225(build).contents
            exact = r.finish(index, scene)
            result = (int(r.raster.picking.check_hover), int(r.raster.view.taking_input),
                      r.raster.view.click_x, r.raster.view.click_z, r.raster.picking.count)
            wanted, bitsets = queries[index][1:]
            list_ok = result == wanted and tuple(r.raster.picking.bitsets[:result[4]]) == bitsets[:wanted[4]]
            command_ok = calls == expected[index]
            native_stats = (I * 5)()
            dll.painter_dynamic_stats(context, native_stats)
            stats_ok = tuple(native_stats) == expected_stats
            top_ok = scene.top_level == h[5]
            errors += not (ok and exact and command_ok and list_ok and stats_ok and top_ok)
            mismatch = next((i for i, (a, b) in enumerate(zip(calls, expected[index])) if a != b), None)
            print(f"runtime={index} calls={len(calls)}/{len(expected[index])} exact={command_ok} first_mismatch={mismatch} "
                  f"picks={list_ok} state={result}/{wanted} stats={tuple(native_stats)}/{expected_stats} roof={scene.top_level}/{h[5]}")
            if mismatch is not None:
                print("  C:", calls[mismatch], "Java:", expected[index][mismatch])
            if not ok:
                break
        print(f"Production runtime: models={models} preview_words={preview_words} errors={errors}")
    finally:
        dll.scene_runtime225_destroy(runtime)
        dll.painter_dynamic_close(context)
        dll.painter_trace_close(preview)
        r.close()
    return 0 if r.passed == 12 and not errors and not r.errors else 1


if __name__ == "__main__":
    sys.exit(main(*sys.argv[1:]))
