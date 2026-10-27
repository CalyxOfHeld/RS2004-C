"""Load original SGR1 scene identities and all twelve first-draw snapshots.

The reader supplies graph state to a future native traversal adapter. It does
not itself generate or compare painter callbacks.
"""
from inspect_scene_graph225 import Reader, scene_heights


def _array(r):
    count = r.ints()
    if count == -1:
        return None
    if not 0 <= count <= 100000:
        raise ValueError(f"invalid array count {count}")
    return r.ints(count) if count else ()


def _model(r):
    return r.digest()


def _object(r, scalars, models=0):
    if not r.ints():
        return None
    result = {"fields": r.ints(scalars)}
    result["models"] = tuple(_model(r) for _ in range(models))
    return result


def _occluder(r):
    return r.ints(18)


def load(path, heights_path):
    r = Reader(path)
    heights_by_digest = scene_heights(heights_path)
    assert r.ints() == 0x47523235
    frames = []
    grid = heights = grounds = locations = None
    all_models = set()
    for frame in range(12):
        assert r.ints() == 0x53475231
        header = r.ints(15)
        assert header[0] == frame and header[7:10] == (4, 104, 104)
        if frame == 0:
            grid_count = r.ints()
            heights = r.ints(4 * 105 * 105)
            grid = [-1] * (4 * 104 * 104)
            for _ in range(grid_count):
                level, x, z, identity = r.ints(4)
                assert 0 <= level < 4 and 0 <= x < 104 and 0 <= z < 104
                cell = (level * 104 + x) * 104 + z
                assert grid[cell] == -1
                grid[cell] = identity
            ground_count = r.ints()
            grounds = []
            for identity in range(ground_count):
                assert r.ints() == identity
                fields = r.ints(18)
                assert fields[0] == identity and 0 <= fields[6] <= 5
                r.skip(8)  # Linkable.key is unused by drawTile
                underlay = _object(r, 7)
                if r.ints():
                    overlay = {"fields": r.ints(5),
                               "arrays": tuple(_array(r) for _ in range(10))}
                else:
                    overlay = None
                wall = _object(r, 7, 2)
                decor = _object(r, 7, 1)
                ground_decor = _object(r, 5, 1)
                ground_obj = _object(r, 5, 3)
                slots = tuple(r.ints(2) for _ in range(fields[6]))
                ground = {"fields": fields, "underlay": underlay,
                          "overlay": overlay, "wall": wall, "decor": decor,
                          "ground_decor": ground_decor,
                          "ground_obj": ground_obj, "slots": slots}
                for obj in (wall, decor, ground_decor, ground_obj):
                    if obj:
                        all_models.update(obj["models"])
                grounds.append(ground)
            location_count = r.ints()
            locations = []
            for identity in range(location_count):
                fields = r.ints(17)
                assert fields[0] == identity
                r.utf()  # entity class name; resolution field is preserved
                model = _model(r)
                all_models.add(model)
                locations.append({"fields": fields, "model": model})
        else:
            assert r.ints() == len(grounds)
            ground_state = []
            for identity in range(len(grounds)):
                state = r.ints(9)
                assert state[0] == identity
                ground_state.append(state[1:])
        assert r.ints() == len(locations)
        location_state = []
        for identity in range(len(locations)):
            state = r.ints(3)
            assert state[0] == identity
            location_state.append(state[1:])
        cycles = r.ints(4 * 105 * 105)
        pools = []
        for _ in range(4):
            count = r.ints()
            assert 0 <= count <= 500
            pools.append(tuple(_occluder(r) for _ in range(count)))
        active_count = r.ints()
        assert 0 <= active_count <= 500
        active = tuple(_occluder(r) for _ in range(active_count))
        if frame == 0:
            ground_state = tuple((g["fields"][8],g["fields"][9],g["fields"][10],
                                  g["fields"][7],*g["fields"][11:15])
                                 for g in grounds)
        frames.append({"header": header, "ground_state": ground_state,
                       "location_state": location_state,
                       "tile_cycles": cycles, "occluder_pools": pools,
                       "active_occluders": active})
    footer = r.ints(8)
    assert footer[:4] == (0x454e4431, 12, len(grounds), len(locations))
    assert r.pos == len(r.data)
    assert all_models - {b''} == set(heights_by_digest)
    return {"grid": grid, "heights": heights, "grounds": grounds,
            "locations": locations, "frames": frames,
            "model_heights": heights_by_digest}


if __name__ == "__main__":
    import sys
    graph = load(*sys.argv[1:])
    print(f"Loaded {len(graph['grounds'])} grounds, {len(graph['locations'])} "
          f"shared locations, {len(graph['frames'])} frames, "
          f"{len(graph['model_heights'])} model heights")
