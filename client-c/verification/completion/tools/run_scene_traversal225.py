"""Drive isolated native painter traversal from SGR1, without supplied order."""
import ctypes as c
import hashlib
import pathlib
import sys
from scene_graph225_reader import load
from inspect_scene_graph225 import Reader


I = c.c_int32
B = c.c_bool
U = c.c_uint8
S = c.c_size_t
P = c.c_void_p
D = U * 32


class Wall(c.Structure):
    _fields_ = [("present", B), ("x", I), ("y", I), ("z", I),
                ("bitset", I), ("type_a", I), ("type_b", I),
                ("max_y_a", I), ("max_y_b", I), ("model_a", D), ("model_b", D)]


class Decor(c.Structure):
    _fields_ = [("present", B), ("x", I), ("y", I), ("z", I),
                ("bitset", I), ("type", I), ("angle", I), ("max_y", I),
                ("model", D)]


class GroundDecor(c.Structure):
    _fields_ = [("present", B), ("x", I), ("y", I), ("z", I),
                ("bitset", I), ("max_y", I), ("model", D)]


class GroundObject(c.Structure):
    _fields_ = [("present", B), ("x", I), ("y", I), ("z", I),
                ("bitset", I), ("offset", I), ("bottom", D),
                ("middle", D), ("top", D)]


class Ground(c.Structure):
    _fields_ = [(name, I) for name in
                ("id", "level", "x", "z", "occlude_level", "draw_level",
                 "loc_count", "loc_spans", "check_loc_spans", "block_loc_spans",
                 "inverse_block_loc_spans", "back_wall_types")]
    _fields_ += [(name, B) for name in
                 ("visible", "update", "contains_locs", "underlay", "overlay")]
    _fields_ += [(name, I) for name in ("bridge", "prev", "next")]
    _fields_ += [("wall", Wall), ("decor", Decor),
                 ("ground_decor", GroundDecor), ("ground_obj", GroundObject),
                 ("locs", I * 5), ("loc_span", I * 5)]


class Location(c.Structure):
    _fields_ = [(name, I) for name in
                ("id", "level", "x", "y", "z", "yaw", "min_x", "max_x",
                 "min_z", "max_z", "distance", "cycle", "bitset", "info",
                 "max_y", "entity_resolution")]
    _fields_ += [("model", D)]


class Occluder(c.Structure):
    _fields_ = [(name, I) for name in
                ("min_tile_x", "max_tile_x", "min_tile_z", "max_tile_z", "type",
                 "min_x", "max_x", "min_z", "max_z", "min_y", "max_y", "mode",
                 "min_delta_x", "max_delta_x", "min_delta_z", "max_delta_z",
                 "min_delta_y", "max_delta_y")]


class Occlusion(c.Structure):
    _fields_ = [("pool", c.POINTER(Occluder)), ("count", S),
                ("active_count", S), ("active", c.c_uint16 * 500)]
    _fields_ += [(name, I) for name in
                 ("eye_x", "eye_y", "eye_z", "eye_tile_x", "eye_tile_z",
                  "cycle", "max_x", "max_z", "levels")]
    _fields_ += [("heights", c.POINTER(I)), ("tile_cycles", c.POINTER(I))]


class Call(c.Structure):
    _fields_ = [(name, I) for name in
                ("kind", "ground_id", "level", "x", "z", "yaw", "dx", "dy",
                 "dz", "bitset")]
    _fields_ += [("model", D)]


EMIT = c.CFUNCTYPE(B, P, c.POINTER(Call))
ENTITY = c.CFUNCTYPE(B, P, c.POINTER(Location), c.POINTER(U), c.POINTER(I))


class Traversal(c.Structure):
    _fields_ = [(name, I) for name in
                ("max_level", "grid_x", "grid_z", "min_level", "cycle",
                 "tiles_remaining", "eye_x", "eye_y", "eye_z", "eye_tile_x",
                 "eye_tile_z", "top_level", "min_x", "max_x", "min_z", "max_z")]
    _fields_ += [("grounds", c.POINTER(Ground)), ("ground_count", S),
                 ("locations", c.POINTER(Location)), ("location_count", S),
                 ("grid", c.POINTER(I)), ("occlusion", Occlusion),
                 ("taking_input", B), ("emit", EMIT), ("emit_user", P)]
    _fields_ += [("resolve_entity", ENTITY), ("entity_user", P)]


def digest(target, value):
    if value:
        assert len(value) == 32
        target[:] = value


def build_static(graph):
    model_heights = graph["model_heights"]
    grounds = (Ground * len(graph["grounds"]))()
    for source in graph["grounds"]:
        f = source["fields"]
        g = grounds[f[0]]
        (g.id,g.level,g.x,g.z,g.occlude_level,g.draw_level,g.loc_count,
         g.loc_spans,g.visible,g.update,g.contains_locs,g.check_loc_spans,
         g.block_loc_spans,g.inverse_block_loc_spans,g.back_wall_types,
         g.bridge,g.prev,g.next) = f
        g.underlay = source["underlay"] is not None
        g.overlay = source["overlay"] is not None
        for i, (loc_id, span) in enumerate(source["slots"]):
            g.locs[i] = loc_id
            g.loc_span[i] = span
        if source["wall"]:
            w = source["wall"]
            (g.wall.x,g.wall.y,g.wall.z,g.wall.type_a,g.wall.type_b,
             g.wall.bitset,_) = w["fields"]
            g.wall.present = True
            a,b = w["models"]
            digest(g.wall.model_a,a);digest(g.wall.model_b,b)
            g.wall.max_y_a=model_heights.get(a,0)
            g.wall.max_y_b=model_heights.get(b,0)
        if source["decor"]:
            d=source["decor"]
            (g.decor.x,g.decor.y,g.decor.z,g.decor.type,g.decor.angle,
             g.decor.bitset,_) = d["fields"]
            g.decor.present=True
            model=d["models"][0]
            digest(g.decor.model,model)
            g.decor.max_y=model_heights[model]
        if source["ground_decor"]:
            d=source["ground_decor"]
            (g.ground_decor.x,g.ground_decor.y,g.ground_decor.z,
             g.ground_decor.bitset,_) = d["fields"]
            g.ground_decor.present=True
            digest(g.ground_decor.model,d["models"][0])
        if source["ground_obj"]:
            o=source["ground_obj"]
            (g.ground_obj.x,g.ground_obj.y,g.ground_obj.z,
             g.ground_obj.bitset,g.ground_obj.offset)=o["fields"]
            g.ground_obj.present=True
            for name,model in zip(("bottom","middle","top"),o["models"]):
                digest(getattr(g.ground_obj,name),model)
    locations=(Location*len(graph["locations"]))()
    for source in graph["locations"]:
        f=source["fields"]
        p=locations[f[0]]
        (p.id,p.level,p.x,p.y,p.z,p.yaw,p.min_x,p.max_x,p.min_z,p.max_z,
         p.distance,p.cycle,p.bitset,p.info,_,_,p.entity_resolution)=f
        model=source["model"]
        digest(p.model,model)
        p.max_y=model_heights.get(model,0)
    grid=(I*len(graph["grid"]))(*graph["grid"])
    heights=(I*len(graph["heights"]))(*graph["heights"])
    return grounds,locations,grid,heights


def expected_commands(path):
    r=Reader(path)
    assert r.ints()==0x50323235
    r.skip(8)
    assets={};frames=[];current=None
    while True:
        tag=r.ints()
        if tag==0:
            assert current is None
            r.skip(15*4)
            current=[]
        elif tag==1:
            identity,length=r.ints(2)
            assert identity==len(assets) and 0<length<1000000
            assets[identity]=hashlib.sha256(r.data[r.pos:r.pos+length]).digest()
            r.skip(length)
        elif tag==2:
            v=r.ints(12)
            current.append((3,assets[v[0]],v[1],v[6],v[7],v[8],v[9]))
        elif tag in (3,4):
            state=r.ints(7)
            kind,x,z,*_=r.ints(7)
            if tag==3:
                assert kind==0
                current.append((1,state[2],x,z))
                r.skip(7*4)
            else:
                assert kind==1
                current.append((2,x,z))
                *_,vertices,triangles=r.ints(8)
                r.skip((vertices*3+triangles*7)*4)
        elif tag==5:
            *_,pixels=r.ints(5)
            r.skip(pixels*4)
            frames.append(current)
            current=None
        elif tag==9:
            r.skip(5*4)
            assert r.pos==len(r.data) and len(frames)==12
            return frames
        else:
            raise ValueError(f"unknown painter tag {tag}")


def occluder_pool(entries):
    pool=(Occluder*max(1,len(entries)))()
    for i,source in enumerate(entries):
        (pool[i].type,pool[i].min_tile_x,pool[i].max_tile_x,
         pool[i].min_tile_z,pool[i].max_tile_z,pool[i].min_x,pool[i].max_x,
         pool[i].min_z,pool[i].max_z,pool[i].min_y,pool[i].max_y,
         pool[i].mode,pool[i].min_delta_x,pool[i].max_delta_x,
         pool[i].min_delta_z,pool[i].max_delta_z,pool[i].min_delta_y,
         pool[i].max_delta_y)=source
    return pool


def run(graph, library, expected, limit=12, renderer=None, native_prepare=False):
    dll=c.CDLL(str(pathlib.Path(library).resolve()))
    traverse=dll.world3d_traverse_frame225
    traverse.argtypes=[c.POINTER(Traversal),I]
    traverse.restype=B
    construction=getattr(renderer,"construction",None)
    if construction is None:
        grounds,locations,grid,heights=build_static(graph)
    else:
        native=renderer.native_scene.contents
        grounds=c.cast(native.grounds,c.POINTER(Ground*native.ground_count)).contents
        locations=c.cast(native.locations,c.POINTER(Location*native.location_count)).contents
        grid=c.cast(native.grid,c.POINTER(I*(native.max_level*native.grid_x*native.grid_z))).contents
        heights=c.cast(native.occlusion.heights,c.POINTER(I*(native.max_level*(native.grid_x+1)*(native.grid_z+1)))).contents
    if native_prepare:
        assert renderer is not None
        for ground in grounds:
            ground.visible=ground.update=ground.contains_locs=False
            ground.check_loc_spans=ground.block_loc_spans=ground.inverse_block_loc_spans=ground.back_wall_types=0
            ground.prev=ground.next=-1
        for loc in locations:
            loc.cycle=loc.distance=0
        if construction is None:
            native_cycles=(I*len(graph["heights"]))()
            native_pools=[occluder_pool([(*v[:11],0,0,0,0,0,0,0) for v in entries])
                          for entries in graph["frames"][0]["occluder_pools"]]
        else:
            native_cycles=native.occlusion.tile_cycles
            native_pools=renderer.native_pools
    matched=True
    for index,frame in enumerate(graph["frames"][:limit]):
        h=frame["header"]
        if native_prepare:
            cycles=native_cycles
            pool=native_pools[h[3]]
            active=(range(renderer.pool_counts[h[3]]) if construction is not None else
                    graph["frames"][0]["occluder_pools"][h[3]])
        else:
            for ground,state in zip(grounds,frame["ground_state"]):
                (ground.visible,ground.update,ground.contains_locs,
                 ground.loc_spans,ground.check_loc_spans,ground.block_loc_spans,
                 ground.inverse_block_loc_spans,ground.back_wall_types)=state
                ground.prev=ground.next=-1
            for loc,state in zip(locations,frame["location_state"]):
                loc.cycle,loc.distance=state
            cycles=(I*len(frame["tile_cycles"]))(*frame["tile_cycles"])
            active=frame["active_occluders"]
            pool=occluder_pool(active)
        calls=[]
        def emit(_user,call):
            v=call.contents
            if renderer is not None:
                renderer.observe(index,len(calls))
            calls.append((v.kind,v.ground_id,v.level,v.x,v.z,v.yaw,
                          v.dx,v.dy,v.dz,v.bitset,bytes(v.model)))
            return True
        callback=EMIT(emit)
        scene=Traversal() if construction is None else renderer.native_scene.contents
        if construction is None:
            scene.max_level=h[7];scene.grid_x=h[8];scene.grid_z=h[9]
            scene.min_level=h[10]
            scene.cycle=index if native_prepare else h[12]
        scene.tiles_remaining=0 if native_prepare else h[13]
        scene.eye_x=h[2];scene.eye_y=h[5];scene.eye_z=h[6]
        scene.eye_tile_x=h[2]//128;scene.eye_tile_z=h[6]//128
        scene.top_level=h[3]
        scene.min_x=max(0,scene.eye_tile_x-25)
        scene.max_x=min(scene.grid_x,scene.eye_tile_x+25)
        scene.min_z=max(0,scene.eye_tile_z-25)
        scene.max_z=min(scene.grid_z,scene.eye_tile_z+25)
        scene.grounds=grounds;scene.ground_count=len(grounds)
        scene.locations=locations;scene.location_count=len(locations)
        scene.grid=grid;scene.taking_input=True;scene.emit=callback
        o=scene.occlusion
        o.pool=pool;o.count=len(active);o.active_count=0 if native_prepare else len(active)
        if not native_prepare:
            for i in range(len(active)):o.active[i]=i
        o.eye_x=scene.eye_x;o.eye_y=scene.eye_y;o.eye_z=scene.eye_z
        o.eye_tile_x=scene.eye_tile_x;o.eye_tile_z=scene.eye_tile_z
        o.cycle=scene.cycle;o.max_x=scene.grid_x;o.max_z=scene.grid_z
        o.levels=scene.max_level;o.heights=heights;o.tile_cycles=cycles
        if renderer is None:
            ok=bool(traverse(c.byref(scene),h[14]))
        else:
            if native_prepare and not renderer.prepare(index,scene):
                matched=False
            renderer.begin(index,scene)
            ok=renderer.render(scene,-1 if native_prepare else h[14])
            if not renderer.finish(index,scene):
                matched=False
        actual=[]
        for kind,ground_id,level,x,z,yaw,dx,dy,dz,bitset,model in calls:
            if kind==3:
                if construction is not None:
                    model=renderer.identity(model)
                actual.append((3,model,yaw,dx,dy,dz,bitset))
            elif kind==1:
                actual.append((1,level,x,z))
            else:
                assert kind==2
                actual.append((2,x,z))
        wanted=expected[index]
        mismatch=next((i for i,(a,b) in enumerate(zip(actual,wanted)) if a!=b),None)
        if mismatch is None and len(actual)!=len(wanted):mismatch=min(len(actual),len(wanted))
        print(f"frame={index} traversal={ok} calls={len(actual)}/{len(wanted)} remaining={scene.tiles_remaining} first_mismatch={mismatch}")
        if mismatch is not None:
            matched=False
            print("  native:",actual[mismatch] if mismatch<len(actual) else "<end>")
            print("  source:",wanted[mismatch] if mismatch<len(wanted) else "<end>")
        if not ok or scene.tiles_remaining:
            return False
    return matched


if __name__ == "__main__":
    graph=load(sys.argv[1],sys.argv[2])
    expected=expected_commands(sys.argv[3])
    sys.exit(0 if run(graph,sys.argv[4],expected,
                      int(sys.argv[5]) if len(sys.argv)>5 else 12) else 1)
