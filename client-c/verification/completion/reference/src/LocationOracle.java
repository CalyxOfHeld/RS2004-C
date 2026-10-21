import deob.client;
import jagex2.config.*;
import jagex2.dash3d.*;
import jagex2.dash3d.entity.*;
import jagex2.dash3d.type.*;
import jagex2.graphics.*;
import jagex2.io.*;
import java.io.*;
import java.nio.file.*;

/** Expectations come from the bundled original LocType, zone reader and pushLocs. */
public class LocationOracle {
 static Jagfile jag(String dir,String name)throws Exception{return new Jagfile(Files.readAllBytes(Paths.get(dir,name)),false);}
 static int layer(int shape){return shape<4?0:shape<9?1:shape==22?3:2;}
 static void mesh(DataOutputStream out,Model m,boolean light)throws Exception {
  if(m==null || m.vertexCount==0)throw new IOException("Missing original mesh");
  out.writeInt(m.vertexCount);out.writeInt(m.faceCount);out.writeInt(light?1:0);
  for(int i=0;i<m.vertexCount;i++){out.writeInt(m.vertexX[i]);out.writeInt(m.vertexY[i]);out.writeInt(m.vertexZ[i]);}
  for(int i=0;i<m.faceCount;i++) {
   out.writeInt(m.faceAlpha==null?0:m.faceAlpha[i]);
   if(light){out.writeInt(m.faceColorA[i]);out.writeInt(m.faceColorB[i]);out.writeInt(m.faceColorC[i]);}
  }
 }
 static Ground setup(client app,int id,int shape,int rotation) {
  Ground g=new Ground(0,50,50);int bitset=0x40000000|(id<<14)|(50<<7)|50;
  byte info=(byte)(shape|(rotation<<6));
  if(layer(shape)==0){g.wall=new Wall();g.wall.bitset=bitset;g.wall.info=info;}
  else if(layer(shape)==1){g.decor=new Decor();g.decor.bitset=bitset;g.decor.info=info;}
  else if(layer(shape)==3){g.groundDecor=new GroundDecor();g.groundDecor.bitset=bitset;g.groundDecor.info=info;}
  else {g.locCount=1;g.locs[0]=new Location();g.locs[0].bitset=bitset;g.locs[0].info=info;g.locs[0].minSceneTileX=g.locs[0].minSceneTileZ=50;}
  app.scene.levelTiles[0][50][50]=g;return g;
 }
 static void start(client app,int shape,int seq) {
  app.locList.clear();app.baseX=app.baseZ=48;app.currentLevel=0;
  app.readZonePacket((byte)-45,new Packet(1,new byte[]{0x22,(byte)(shape*4+3),(byte)(seq>>8),(byte)seq}),42);
 }
 public static void main(String[] args)throws Exception {
  Jagfile config=jag(args[0],"config"),models=jag(args[0],"models");
  Model.unpack(3,models);AnimBase.unpack(false,models);AnimFrame.unpack(false,models);SeqType.unpack(config,1);LocType.unpack(config);
  int meshCount=0,animated=0,firstId=-1,firstSeq=-1;int[] representative=new int[23];java.util.Arrays.fill(representative,-1);
  try(DataOutputStream out=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[1])))) {
   out.writeInt(0x4c4f4331);
   for(int id=0;id<LocType.count;id++) {
    LocType loc=LocType.get(id);if(loc.shapes==null)continue;
    for(int shape:loc.shapes)if(shape>=0&&shape<23&&representative[shape]<0)representative[shape]=id;
    if(loc.anim<0)continue;animated++;
    if(firstId<0){firstId=id;firstSeq=loc.anim;}
    for(int shape:loc.shapes)for(int rotation=0;rotation<(shape==2?8:4);rotation++)for(int variant=0;variant<2;variant++) {
     int frame=SeqType.instances[loc.anim].frames[variant==0?0:SeqType.instances[loc.anim].frameCount-1];
     out.writeInt(id);out.writeInt(shape);out.writeInt(rotation);out.writeInt(frame);
     int[] h=variant==0?new int[]{0,0,0,0}:new int[]{-123,-76,-40,-91};for(int v:h)out.writeInt(v);
     Model m=loc.getModel(shape,rotation,h[0],h[1],h[2],h[3],frame);
     if(id==521 && rotation==0)System.out.println("[ALPHA] loc521 frame="+frame+" alpha="+(m.faceAlpha==null?0:m.faceAlpha[0])+" flag="+loc.animHasAlpha);
     mesh(out,m,true);meshCount++;
    }
   }
   out.writeInt(-1);
   for(int shape=5;shape<=8;shape++)representative[shape]=representative[4];
   representative[11]=representative[10];
   client app=new client();app.levelHeightmap=new int[4][105][105];app.scene=new World3D(1,app.levelHeightmap,104,4,104);
   // Retain a real cache alpha-sharing model while frames/rotations and caches change.
   LocType.modelCacheStatic.clear();LocType.modelCacheDynamic.clear();
   LocType alpha=LocType.get(521);SeqType alphaSeq=SeqType.instances[alpha.anim];
   int firstFrame=alphaSeq.frames[0],stepsAlpha=alphaSeq.frameCount*2+6;
   out.writeInt(alpha.index);out.writeInt(firstFrame);out.writeInt(stepsAlpha);
   Model held=alpha.getModel(10,0,0,0,0,0,firstFrame);
   for(int step=0;step<stepsAlpha;step++) {
    int clear=step==3?1:step==5?2:0;
    if(clear>0)LocType.modelCacheStatic.clear();if(clear==2)LocType.modelCacheDynamic.clear();
    int frame=alphaSeq.frames[step%alphaSeq.frameCount],rotation=step%4;
    out.writeInt(clear);out.writeInt(frame);out.writeInt(rotation);
    mesh(out,alpha.getModel(10,rotation,0,0,0,0,frame),!alpha.sharelight);mesh(out,held,!alpha.sharelight);
   }
   // Full source sequence lifecycles, including zero-step renders and batched ticks.
   out.writeInt(SeqType.count);
   for(int id=0;id<SeqType.count;id++) {
    setup(app,firstId,10,0);start(app,10,id);LocEntity e=(LocEntity)app.locList.head();if(e==null)throw new IOException("Missing zone animation");
    int duration=0;for(int delay:SeqType.instances[id].delay)duration+=Math.max(0,delay)+1;
    int steps=duration*2+20;out.writeInt(id);out.writeInt(steps);
    for(int step=0;step<steps;step++) {
     int delta=step%11==0?0:step%13==0?7:1;app.sceneDelta=delta;app.pushLocs(1);
     out.writeInt(delta);out.writeInt(e.seqFrame);out.writeInt(e.seqCycle);out.writeInt(e.prev==null?0:1);
    }
   }
   // Actual original zone42 -> pushLocs -> scene models for all real cache shapes.
   LocType.modelCacheStatic.clear();LocType.modelCacheDynamic.clear();
   int sceneCases=0;for(int id:representative)if(id>=0)sceneCases+=4;
   out.writeInt(sceneCases);
   for(int shape=0;shape<23;shape++)if(representative[shape]>=0)for(int rotation=0;rotation<4;rotation++) {
    int id=representative[shape];LocType loc=LocType.get(id);Ground g=setup(app,id,shape,rotation);
    app.levelHeightmap[0][50][50]=-123;app.levelHeightmap[0][51][50]=-76;app.levelHeightmap[0][51][51]=-40;app.levelHeightmap[0][50][51]=-91;
    start(app,shape,firstSeq);app.sceneDelta=1;app.pushLocs(1);
    out.writeInt(id);out.writeInt(shape);out.writeInt(rotation);out.writeInt(firstSeq);
    Model[] parts=layer(shape)==0?(shape==2?new Model[]{g.wall.modelA,g.wall.modelB}:new Model[]{g.wall.modelA}):layer(shape)==1?new Model[]{g.decor.model}:layer(shape)==3?new Model[]{g.groundDecor.model}:new Model[]{g.locs[0].model};
    out.writeInt(parts.length);for(Model m:parts)mesh(out,m,true);
   }
  }
  System.out.println("Original: "+animated+" animated loc definitions; "+meshCount+" meshes; "+SeqType.count+" complete sequence lifecycles; original zone42 scene replacements across cache shapes.");
 }
}
