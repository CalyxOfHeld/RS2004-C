import jagex2.dash3d.World3D;
import jagex2.dash3d.Occlude;
import jagex2.dash3d.type.*;
import jagex2.datastruct.Linkable;
import jagex2.graphics.Model;
import java.io.*;
import java.security.MessageDigest;
import java.util.IdentityHashMap;
import java.util.ArrayList;
import java.util.TreeMap;

/** Deterministic snapshot of the original scene immediately before each draw. */
public final class SceneGraphOracle {
    public static final int ENTITY_MODEL_NONE=0, ENTITY_MODEL_DIRECT_MODEL_TAKES_PRECEDENCE=1, ENTITY_MODEL_UNRESOLVED_UNTIL_DRAW=2;
    private static DataOutputStream out;
    private static int frames;
    private static int locationSlots,sharedLocationSlots,unresolvedEntityModels,directModelEntities;
    private static final IdentityHashMap<Location,Integer> locations=new IdentityHashMap<Location,Integer>();
    private static final IdentityHashMap<Ground,Integer> grounds=new IdentityHashMap<Ground,Integer>();
    private static final TreeMap<String,Integer> modelHeights=new TreeMap<String,Integer>();

    private static void ints(int... v)throws IOException { for(int n:v)out.writeInt(n); }
    private static byte[] digest(Model m)throws Exception {
        if(m==null)return new byte[0];
        MessageDigest d=MessageDigest.getInstance("SHA-256");
        update(d,m.vertexCount);update(d,m.faceCount);update(d,m.texturedFaceCount);update(d,m.priority);
        update(d,m.radius);update(d,m.maxY);update(d,m.minY);update(d,m.minDepth);update(d,m.maxDepth);update(d,m.pickable?1:0);
        int[] lengths={m.vertexCount,m.vertexCount,m.vertexCount,m.faceCount,m.faceCount,m.faceCount,m.faceCount,m.faceCount,m.faceCount,m.faceCount,m.faceCount,m.faceCount,m.faceCount,m.texturedFaceCount,m.texturedFaceCount,m.texturedFaceCount};
        int[][] arrays={m.vertexX,m.vertexY,m.vertexZ,m.faceVertexA,m.faceVertexB,m.faceVertexC,m.faceColor,m.faceColorA,m.faceColorB,m.faceColorC,m.faceInfo,m.facePriority,m.faceAlpha,m.texturedVertexA,m.texturedVertexB,m.texturedVertexC};
        for(int i=0;i<arrays.length;i++){int[] a=arrays[i];update(d,a==null?-1:lengths[i]);if(a!=null){if(a.length<lengths[i])throw new IOException("short scene model array");for(int j=0;j<lengths[i];j++)update(d,a[j]);}}
        return d.digest();
    }
    private static void update(MessageDigest d,int v){d.update((byte)(v>>>24));d.update((byte)(v>>>16));d.update((byte)(v>>>8));d.update((byte)v);}
    private static int groundId(Ground g){if(g==null)return -1;Integer n=grounds.get(g);if(n==null){n=grounds.size();grounds.put(g,n);}return n;}
    private static int locationId(Location l){if(l==null)return -1;Integer n=locations.get(l);if(n==null){n=locations.size();locations.put(l,n);}return n;}
    private static int linkId(Linkable n){return n==null?-1:(n instanceof Ground?groundId((Ground)n):-2);}
    private static void collectGround(Ground g,ArrayList<Ground> all,IdentityHashMap<Ground,Boolean> seen){if(g==null||seen.containsKey(g))return;seen.put(g,Boolean.TRUE);groundId(g);all.add(g);collectGround(g.bridge,all,seen);}
    private static ArrayList<Ground> collectGrounds(World3D scene){ArrayList<Ground> all=new ArrayList<Ground>();IdentityHashMap<Ground,Boolean> seen=new IdentityHashMap<Ground,Boolean>();for(int l=0;l<scene.maxLevel;l++)for(int x=0;x<scene.maxTileX;x++)for(int z=0;z<scene.maxTileZ;z++)collectGround(scene.levelTiles[l][x][z],all,seen);return all;}
    private static ArrayList<Location> collectLocations(ArrayList<Ground> all){for(Ground g:all)for(int i=0;i<g.locCount;i++)if(g.locs[i]!=null)locationId(g.locs[i]);ArrayList<Location> ordered=new ArrayList<Location>();for(int i=0;i<locations.size();i++)ordered.add(null);for(Ground g:all)for(int i=0;i<g.locCount;i++){Location l=g.locs[i];if(l!=null)ordered.set(locationId(l),l);}return ordered;}
    private static void model(Model m)throws Exception {byte[] h=digest(m);ints(h.length);out.write(h);if(m!=null){StringBuilder key=new StringBuilder(64);for(byte b:h){int v=b&255;key.append(Character.forDigit(v>>>4,16));key.append(Character.forDigit(v&15,16));}Integer old=modelHeights.put(key.toString(),m.maxY);if(old!=null&&old!=m.maxY)throw new IOException("inconsistent maxY for scene model digest");}}
    private static void tileModel(Model m)throws Exception {model(m);}
    private static void occluder(Occlude o)throws IOException {ints(o.type,o.minTileX,o.maxTileX,o.minTileZ,o.maxTileZ,o.minX,o.maxX,o.minZ,o.maxZ,o.minY,o.maxY,o.mode,o.minDeltaX,o.maxDeltaX,o.minDeltaZ,o.maxDeltaZ,o.minDeltaY,o.maxDeltaY);}
    private static void ground(Ground g)throws Exception {
        ints(groundId(g),g.level,g.x,g.z,g.occludeLevel,g.drawLevel,g.locCount,g.locSpans,g.visible?1:0,g.update?1:0,g.containsLocs?1:0,g.checkLocSpans,g.blockLocSpans,g.inverseBlockLocSpans,g.backWallTypes,groundId(g.bridge),linkId(g.prev),linkId(g.next));out.writeLong(g.key);
        if(g.underlay==null)ints(0);else {TileUnderlay u=g.underlay;ints(1,u.southwestColor,u.southeastColor,u.northeastColor,u.northwestColor,u.textureId,u.flat?1:0,u.rgb);}
        if(g.overlay==null)ints(0);else {TileOverlay o=g.overlay;ints(1,o.shape,o.rotation,o.flat?1:0,o.backgroundRgb,o.foregroundRgb);array(o.vertexX);array(o.vertexY);array(o.vertexZ);array(o.triangleColorA);array(o.triangleColorB);array(o.triangleColorC);array(o.triangleVertexA);array(o.triangleVertexB);array(o.triangleVertexC);array(o.triangleTextureIds);}
        if(g.wall==null)ints(0);else {Wall w=g.wall;ints(1,w.x,w.y,w.z,w.typeA,w.typeB,w.bitset,w.info);tileModel(w.modelA);tileModel(w.modelB);}
        if(g.decor==null)ints(0);else {Decor d=g.decor;ints(1,d.x,d.y,d.z,d.type,d.angle,d.bitset,d.info);tileModel(d.model);}
        if(g.groundDecor==null)ints(0);else {GroundDecor d=g.groundDecor;ints(1,d.x,d.y,d.z,d.bitset,d.info);tileModel(d.model);}
        if(g.groundObj==null)ints(0);else {GroundObject o=g.groundObj;ints(1,o.x,o.y,o.z,o.bitset,o.offset);tileModel(o.bottomObj);tileModel(o.middleObj);tileModel(o.topObj);}
        for(int i=0;i<g.locCount;i++){Location l=g.locs[i];ints(locationId(l),g.locSpan[i]);}
    }
    private static void array(int[] a)throws IOException {if(a==null){ints(-1);return;}ints(a.length);for(int v:a)ints(v);}
    private static void writeFrame(World3D scene,Ground root,int[] camera)throws Exception {
        ArrayList<Ground> all=collectGrounds(scene);ArrayList<Location> allLocations=collectLocations(all);
        for(Ground g:all)if(g.prev!=null||g.next!=null)throw new IOException("non-empty drawTile queue at capture");
        if(frames==0){for(Ground g:all)for(int i=0;i<g.locCount;i++)if(g.locs[i]!=null){locationSlots++;if(g.locs[i].entity!=null&&g.locs[i].model==null)unresolvedEntityModels++;if(g.locs[i].entity!=null&&g.locs[i].model!=null)directModelEntities++;}sharedLocationSlots=locationSlots-allLocations.size();if(grounds.size()!=all.size()||locations.size()!=allLocations.size()||sharedLocationSlots<0)throw new IOException("scene graph identity table is inconsistent");}
        out.writeInt(0x53475231);ints(frames,camera[0],camera[1],camera[2],camera[3],camera[4],camera[5],scene.maxLevel,scene.maxTileX,scene.maxTileZ,scene.minLevel,frames==0?1:0,World3D.cycle,World3D.tilesRemaining,groundId(root));
        if(frames==0){int count=0;for(int l=0;l<scene.maxLevel;l++)for(int x=0;x<scene.maxTileX;x++)for(int z=0;z<scene.maxTileZ;z++)if(scene.levelTiles[l][x][z]!=null)count++;ints(count);
            for(int l=0;l<scene.maxLevel;l++)for(int x=0;x<=scene.maxTileX;x++)for(int z=0;z<=scene.maxTileZ;z++)ints(scene.levelHeightmaps[l][x][z]);
            for(int l=0;l<scene.maxLevel;l++)for(int x=0;x<scene.maxTileX;x++)for(int z=0;z<scene.maxTileZ;z++){Ground g=scene.levelTiles[l][x][z];if(g!=null)ints(l,x,z,groundId(g));}
            ints(all.size());for(Ground g:all){ints(groundId(g));ground(g);}}
        else {ints(all.size());for(Ground g:all)ints(groundId(g),g.visible?1:0,g.update?1:0,g.containsLocs?1:0,g.locSpans,g.checkLocSpans,g.blockLocSpans,g.inverseBlockLocSpans,g.backWallTypes);}
        if(frames==0){ints(allLocations.size());for(int id=0;id<allLocations.size();id++){Location p=allLocations.get(id);int resolution=p.entity==null?ENTITY_MODEL_NONE:(p.model==null?ENTITY_MODEL_UNRESOLVED_UNTIL_DRAW:ENTITY_MODEL_DIRECT_MODEL_TAKES_PRECEDENCE);ints(id,p.level,p.x,p.y,p.z,p.yaw,p.minSceneTileX,p.maxSceneTileX,p.minSceneTileZ,p.maxSceneTileZ,p.distance,p.cycle,p.bitset,p.info,p.model==null?0:1,p.entity==null?0:1,resolution);if(p.entity==null)out.writeUTF("NONE");else out.writeUTF(p.entity.getClass().getName());model(p.model);}
        }
        ints(allLocations.size());for(int id=0;id<allLocations.size();id++){Location p=allLocations.get(id);ints(id,p.cycle,p.distance);}
        for(int l=0;l<scene.maxLevel;l++)for(int x=0;x<=scene.maxTileX;x++)for(int z=0;z<=scene.maxTileZ;z++)ints(scene.levelTileOcclusionCycles[l][x][z]);
        for(int l=0;l<World3D.LEVEL_COUNT;l++){ints(World3D.levelOccluderCount[l]);for(int i=0;i<World3D.levelOccluderCount[l];i++)occluder(World3D.levelOccluders[l][i]);}
        ints(World3D.activeOccluderCount);for(int i=0;i<World3D.activeOccluderCount;i++)occluder(World3D.activeOccluders[i]);
        frames++;
    }
    public static void capture(World3D scene,Ground root,int[] camera,String path)throws Exception {if(out==null){out=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(path)));ints(0x47523235);}writeFrame(scene,root,camera);}
    public static void close(String path)throws IOException {if(out!=null){ints(0x454E4431,frames,grounds.size(),locations.size(),locationSlots,sharedLocationSlots,unresolvedEntityModels,directModelEntities);out.close();out=null;DataOutputStream heightsOut=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(path+".maxy")));try{heightsOut.writeInt(0x534D5931);heightsOut.writeInt(modelHeights.size());for(java.util.Map.Entry<String,Integer> entry:modelHeights.entrySet()){String key=entry.getKey();for(int i=0;i<64;i+=2)heightsOut.writeByte(Integer.parseInt(key.substring(i,i+2),16));heightsOut.writeInt(entry.getValue());}}finally{heightsOut.close();}System.err.println("SceneGraphOracle: frames="+frames+" grounds="+grounds.size()+" locationIdentities="+locations.size()+" locationSlots="+locationSlots+" sharedLocationSlots="+sharedLocationSlots+" unresolvedEntityModels="+unresolvedEntityModels+" directModelEntities="+directModelEntities+" modelHeights="+modelHeights.size());}}
}
