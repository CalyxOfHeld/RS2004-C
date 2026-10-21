import jagex2.config.*;
import jagex2.dash3d.*;
import jagex2.dash3d.type.*;
import jagex2.datastruct.*;
import jagex2.graphics.*;
import jagex2.io.*;
import java.io.*;
import java.nio.file.*;
import java.util.*;

/** Placement and draw calls come from original World / World3D methods. */
public class WallDecorationOracle {
 static Jagfile jag(String n)throws Exception{return new Jagfile(Files.readAllBytes(Paths.get("engine/data/pack/client",n)),false);}
 public static void main(String[] args)throws Exception {
  Model.unpack(3,jag("models"));LocType.unpack(jag("config"));World.lowMemory=false;
  for(boolean[][][] a:World3D.visibilityMatrix)for(boolean[][] b:a)for(boolean[] c:b)Arrays.fill(c,true);
  int wall=-1;
  for(int id=0;id<LocType.count;id++) {LocType l=LocType.get(id);if(l.wallwidth==16||l.shapes==null)continue;for(int shape:l.shapes)if(shape==0&&l.getModel(0,0,0,0,0,0,-1)!=null){wall=id;break;}if(wall>=0)break;}
  if(wall<0)throw new IOException("No real nondefault wall-width definition");
  int calls=0,cases=120;
  try(DataOutputStream o=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[0])))) {
   o.writeInt(0x57444331);o.writeInt(cases);
   for(int shape=4;shape<=8;shape++)for(int rot=0;rot<4;rot++)for(int slope=0;slope<2;slope++)for(int order=0;order<3;order++) {
    int[][][] h=new int[4][105][105];int[] v=slope==0?new int[4]:new int[]{-123,-76,-40,-91};
    h[0][40][40]=v[0];h[0][41][40]=v[1];h[0][41][41]=v[2];h[0][40][41]=v[3];
    World w=new World(104,new byte[4][104][104],104,h,-35388);World3D s=new World3D(1,h,104,4,104);LinkList list=new LinkList(0);
    if(order==1)w.addLoc(null,true,0,40,rot,0,s,list,wall,40);
    w.addLoc(null,true,0,40,rot,shape,s,list,1938,40);
    if(order==2)w.addLoc(null,true,0,40,rot,0,s,list,wall,40);
    Decor d=s.levelTiles[0][40][40].decor;if(d==null)throw new IOException("Missing decoration");
    o.writeInt(shape);o.writeInt(rot);o.writeInt(order);o.writeInt(wall);for(int n:v)o.writeInt(n);
    o.writeInt(d.x);o.writeInt(d.y);o.writeInt(d.z);o.writeInt(d.model.vertexCount);
    for(int i=0;i<d.model.vertexCount;i++){o.writeInt(d.model.vertexX[i]);o.writeInt(d.model.vertexY[i]);o.writeInt(d.model.vertexZ[i]);}
    int[] offsets={-256,-64,0,64,256};o.writeInt(25);
    for(int dx:offsets)for(int dz:offsets) {
     int ex=40*128+64+dx,ez=40*128+64+dz;Model.referenceDraws=new ArrayList<>();
     s.draw(0,ex,0,256,-500,ez,0);
     ArrayList<int[]> found=new ArrayList<>();for(int[] draw:Model.referenceDraws)if(draw[4]==d.bitset)found.add(draw);
     o.writeInt(ex);o.writeInt(ez);o.writeInt(found.size());
     for(int[] draw:found){o.writeInt(draw[0]);o.writeInt(draw[1]+ex);o.writeInt(draw[2]-500);o.writeInt(draw[3]+ez);calls++;}
    }
   }
  }
  System.out.println("Original World.addLoc / World3D.draw: "+cases+" placement cases, 3000 camera positions, "+calls+" actual decoration draw calls; real arrow-slit1938 and wall"+wall+" across shapes4..8, all rotations, slopes and wall insertion orders.");
 }
}
