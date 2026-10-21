import jagex2.config.*;
import jagex2.dash3d.*;
import jagex2.dash3d.type.*;
import jagex2.datastruct.*;
import jagex2.graphics.*;
import jagex2.io.*;
import java.io.*;
import java.nio.file.*;
import java.util.*;

/** Calls the bundled revision-225 algorithms; contains no native light formula. */
public class SceneLightingOracle {
 static Jagfile jag(String n)throws Exception{return new Jagfile(Files.readAllBytes(Paths.get("engine/data/pack/client",n)),false);}
 static void normals(DataOutputStream o,VertexNormal[] n)throws Exception {
  for(VertexNormal v:n){o.writeInt(v.x);o.writeInt(v.y);o.writeInt(v.z);o.writeInt(v.w);}
 }
 static void before(DataOutputStream o,Model m)throws Exception {
  o.writeInt(m.vertexCount);o.writeInt(m.faceCount);
  for(int i=0;i<m.vertexCount;i++){o.writeInt(m.vertexX[i]);o.writeInt(m.vertexY[i]);o.writeInt(m.vertexZ[i]);}
  normals(o,m.vertexNormalOriginal);
 }
 static void faces(DataOutputStream o,Model m)throws Exception {
  for(int f=0;f<m.faceCount;f++) {
   o.writeInt(m.faceInfo==null?0:m.faceInfo[f]);
   o.writeInt(m.faceColorA[f]);o.writeInt(m.faceColorB[f]);o.writeInt(m.faceColorC[f]);
  }
 }
 static class Saved {
  int level,x,z,id,shape,part;Model model;byte[] pre;
  Saved(int l,int x,int z,int bitset,int info,int part,Model m)throws Exception {
   level=l;this.x=x;this.z=z;id=(bitset>>14)&32767;shape=info&63;this.part=part;model=m;
   ByteArrayOutputStream bytes=new ByteArrayOutputStream();before(new DataOutputStream(bytes),m);pre=bytes.toByteArray();
  }
 }
 static void save(ArrayList<Saved> list,int l,int x,int z,int bitset,int info,int part,Model m)throws Exception {
  if(m!=null && m.vertexNormalOriginal!=null)list.add(new Saved(l,x,z,bitset,info,part,m));
 }
 public static void main(String[] args)throws Exception {
  Jagfile cfg=jag("config"),models=jag("models");Model.unpack(3,models);AnimBase.unpack(false,models);AnimFrame.unpack(false,models);SeqType.unpack(cfg,1);LocType.unpack(cfg);
  ArrayList<int[]> cases=new ArrayList<>();
  for(int id=0;id<LocType.count;id++) {
   LocType loc=LocType.get(id);if(!loc.sharelight || loc.shapes==null)continue;
   for(int shape:loc.shapes)for(int rot=0;rot<4;rot++)for(int variant=0;variant<4;variant++)cases.add(new int[]{id,shape,rot,variant});
  }
  int scale=768*(int)Math.sqrt(5100)>>8,hidden=0;
  try(DataOutputStream out=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[0])))) {
   out.writeInt(0x534c5431);out.writeInt(cases.size());
   World3D merger=new World3D(1,new int[4][105][105],104,4,104);
   for(int[] c:cases) {
    int id=c[0],shape=c[1],rot=c[2],v=c[3];LocType loc=LocType.get(id);
    int[] h=v==3?new int[]{-123,-76,-40,-91}:new int[4];
    int dx=v==2?128:0,dy=0,dz=v==1 || v==3?128:0;boolean hide=v!=2;
    for(int n:c)out.writeInt(n);for(int n:h)out.writeInt(n);out.writeInt(dx);out.writeInt(dy);out.writeInt(dz);out.writeInt(hide?1:0);
    Model a=loc.getModel(shape,rot,h[0],h[1],h[2],h[3],-1),b=loc.getModel(shape,rot,h[0],h[1],h[2],h[3],-1);
    before(out,a);before(out,b);merger.mergeNormals(a,b,dx,dy,dz,hide);
    normals(out,a.vertexNormal);normals(out,b.vertexNormal);
    a.applyLighting(64,scale,-50,-10,-50);b.applyLighting(64,scale,-50,-10,-50);
    faces(out,a);faces(out,b);
    for(int f:a.faceInfo)if(f==-1)hidden++;
   }
   // Full real Lumbridge terrain and scenery, before original setBridge.
   byte[][][] flags=new byte[4][104][104];int[][][] heights=new int[4][105][105];
   World world=new World(104,flags,104,heights,-35388);World.lowMemory=false;
   World3D scene=new World3D(1,heights,104,4,104);CollisionMap[] collision=new CollisionMap[4];
   for(int l=0;l<4;l++)collision[l]=new CollisionMap(104,-1,104);
   world.loadGround(Files.readAllBytes(Paths.get(args[1],"m50_50.raw")),3180,1,20,20,3180);
   world.loadLocations(Files.readAllBytes(Paths.get(args[1],"l50_50.raw")),scene,collision,new LinkList(0),true,20,20);
   ArrayList<Saved> saved=new ArrayList<>();
   for(int l=0;l<4;l++)for(int x=0;x<104;x++)for(int z=0;z<104;z++) {
    Ground g=scene.levelTiles[l][x][z];if(g==null)continue;
    if(g.wall!=null){save(saved,l,x,z,g.wall.bitset,g.wall.info,0,g.wall.modelA);save(saved,l,x,z,g.wall.bitset,g.wall.info,1,g.wall.modelB);}
    for(int j=0;j<g.locCount;j++){Location loc=g.locs[j];if(loc.minSceneTileX==x&&loc.minSceneTileZ==z)save(saved,l,x,z,loc.bitset,loc.info,0,loc.model);}
    if(g.groundDecor!=null)save(saved,l,x,z,g.groundDecor.bitset,g.groundDecor.info,0,g.groundDecor.model);
   }
   scene.buildModels(-10,64,-50,768,-50,false);out.writeInt(saved.size());
   for(Saved s:saved){out.writeInt(s.level);out.writeInt(s.x);out.writeInt(s.z);out.writeInt(s.id);out.writeInt(s.shape);out.writeInt(s.part);out.write(s.pre);faces(out,s.model);}
   System.out.println("Original: "+cases.size()+" real-cache model pair merges, "+hidden+" first-model hidden faces; "+saved.size()+" full Lumbridge map shared-light models including real terrain, bridge source planes, walls, scenery and ground decorations.");
  }
 }
}
