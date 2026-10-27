"""Inspect the observation stream, bind it to original frames, damage private copies."""
import collections
import json
import hashlib
import pathlib
import struct
import sys
import zipfile


def agent(classes, destination):
    with zipfile.ZipFile(destination, "w") as archive:
        archive.writestr(zipfile.ZipInfo("META-INF/MANIFEST.MF"),
                        b"Manifest-Version: 1.0\r\nPremain-Class: PainterTraceAgent\r\n\r\n")
        for path in sorted(pathlib.Path(classes).glob("PainterTraceAgent*.class")):
            archive.writestr(zipfile.ZipInfo(path.name), path.read_bytes())


def inspect(trace_path, scene_path, output):
    raw = pathlib.Path(trace_path).read_bytes()
    scene = pathlib.Path(scene_path).read_bytes()
    output = pathlib.Path(output)
    pos = 0

    def take(count):
        nonlocal pos
        result = struct.unpack_from(">" + "i" * count, raw, pos)
        pos += count * 4
        return result

    assert take(1) == (0x50323235,)
    brightness = raw[pos:pos + 8]
    pos += 8
    assert struct.unpack_from(">4i", scene) == (0x53445731, 512, 334, 12)
    assert brightness == scene[16:24]
    source_pos = 32
    frames, current = [], None
    assets, commands = 0, collections.Counter()
    modes = collections.Counter()
    alpha_assets = priority_assets = bounded_assets = 0
    model_hashes = set()
    first_pixel = None
    while True:
        start = pos
        tag, = take(1)
        if tag == 0:
            assert current is None
            header = take(15)
            assert header[0] == len(frames) and header[1:3] == (512, 334)
            current = {"header": header, "begin": raw[start:pos], "defs": [], "commands": []}
        elif tag == 1:
            assert current is not None
            identity, length = take(2)
            assert identity == assets and 0 < length < 1000000
            stop = pos + length
            model_hashes.add(hashlib.sha256(raw[pos:stop]).digest())
            v, f, t, priority, radius, max_y, min_y, min_d, max_d, pickable = take(10)
            assert 0 <= v <= 4096 and 0 <= f <= 4096 and 0 <= t <= 4096
            arrays = []
            for count in [v]*3 + [f]*10 + [t]*3:
                count_read, = take(1)
                assert count_read == -1 or count_read == count
                arrays.append(None if count_read == -1 else take(count))
            assert pos == stop
            info = arrays[10]
            modes.update("hidden" if value == -1 else str(value & 3)
                         for value in (info if info is not None else [0]*f))
            alpha_assets += arrays[12] is not None and any(arrays[12])
            priority_assets += arrays[11] is not None
            # Only summarize source bounds; never derive rendering expectations.
            bounded_assets += max_d > 0
            assets += 1
            current["defs"].append(raw[start:pos])
        elif tag == 2:
            assert current is not None
            command = take(12)
            assert 0 <= command[0] < assets
            commands["model"] += 1
            current["commands"].append(raw[start:pos])
        elif tag in (3, 4):
            assert current is not None
            take(7)
            kind, x, z, *heights = take(7)
            if kind == 0:
                assert tag == 3
                take(7)
                commands["underlay"] += 1
            else:
                assert kind == 1 and tag == 4
                flat, shape, rotation, bg, fg, textures, vertices, triangles = take(8)
                assert 0 <= vertices <= 6 and 0 <= triangles <= 10
                take(vertices*3 + triangles*7)
                commands["overlay"] += 1
            current["commands"].append(raw[start:pos])
        elif tag == 5:
            assert current is not None
            index, alpha, clip, remaining, count = take(5)
            assert index == len(frames) and remaining == 0 and count == 512*334+8
            if first_pixel is None:
                first_pixel = pos
            source_header = struct.unpack_from(">11i", scene, source_pos)
            source_pos += 44
            h = current["header"]
            assert source_header[:8] == (h[4], h[7], h[8], h[6], h[3], h[5], h[11], h[10])
            assert source_header[8:] == (alpha, clip, count)
            assert raw[pos:pos+count*4] == scene[source_pos:source_pos+count*4]
            pos += count*4
            source_pos += count*4
            current["end"] = raw[start:pos]
            frames.append(current)
            current = None
        elif tag == 9:
            footer = take(5)
            assert current is None and footer == (len(frames), commands["model"], commands["underlay"], commands["overlay"], assets)
            assert pos == len(raw) and source_pos == len(scene) and len(frames) == 12
            end = raw[start:pos]
            break
        else:
            raise ValueError(f"unknown tag {tag} at {start}")
    report = {"frames": len(frames), "commands": dict(commands), "assets": assets,
              "face_modes_across_snapshots": dict(modes), "alpha_assets": alpha_assets,
              "priority_array_assets": priority_assets, "nonzero_bound_assets": bounded_assets,
              "source_frame_binding": "exact: original camera fields, final raster state and all pixels/guards"}
    graph_path = output / "scene-graph-1.bin"
    graph = graph_path.read_bytes()
    assert struct.unpack_from(">I", graph)[0] == 0x47523235
    graph_frames = []
    marker = struct.pack(">I", 0x53475231)
    cursor = 4
    while True:
        at = graph.find(marker, cursor)
        if at < 0:
            break
        values = struct.unpack_from(">15i", graph, at + 4)
        graph_frames.append(values)
        cursor = at + 4 + 15 * 4
    assert len(graph_frames) == len(frames) == 12
    for index, (values, frame) in enumerate(zip(graph_frames, frames)):
        h = frame["header"]
        assert values[0] == index
        assert values[1:7] == (h[3], h[4], h[5], h[6], h[7], h[8])
        assert values[7:10] == (4, 104, 104)
        assert values[11] == (1 if index == 0 else 0)
        assert values[12] == index + 1 and values[13] > 0 and values[14] >= 0
    graph_footer = struct.unpack_from(">8i", graph, len(graph) - 32)
    assert graph_footer == (0x454E4431, 12, 12533, 1046, 1350, 304, 0, 0)
    matched_model_hashes = sum(graph.count(model_hash) > 0 for model_hash in model_hashes)
    assert matched_model_hashes > 0, "scene graph and original painter trace share no exact model snapshots"
    report["scene_graph_frames_bound_to_painter_trace"] = len(graph_frames)
    report["scene_graph_identity_footer"] = {"grounds": graph_footer[2], "locations": graph_footer[3], "location_slots": graph_footer[4], "shared_slots": graph_footer[5], "unresolved_entity_models": graph_footer[6]}
    report["painter_model_snapshots_referenced_by_graph"] = len(model_hashes)
    report["painter_model_snapshots_also_present_in_scene_graph"] = matched_model_hashes
    report["scene_graph_sha256"] = __import__("hashlib").sha256(graph).hexdigest()
    (output / "trace-summary.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    damaged = bytearray(raw)
    damaged[first_pixel] ^= 1
    (output / "wrong-pixel.bin").write_bytes(damaged)
    (output / "truncated.bin").write_bytes(raw[:-1])
    # Keep asset definitions before all references, then reverse draw commands.
    with (output / "reverse-order.bin").open("wb") as stream:
        stream.write(raw[:12])
        for frame in frames:
            stream.write(frame["begin"])
            for segment in frame["defs"]:
                stream.write(segment)
            for segment in reversed(frame["commands"]):
                stream.write(segment)
            stream.write(frame["end"])
        stream.write(end)
    print(json.dumps(report))


if __name__ == "__main__":
    if sys.argv[1] == "agent":
        agent(*sys.argv[2:])
    else:
        inspect(*sys.argv[2:])
