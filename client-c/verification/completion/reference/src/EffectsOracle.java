import deob.client;
import jagex2.config.*;
import jagex2.dash3d.entity.*;
import jagex2.graphics.*;
import jagex2.io.*;
import java.io.*;
import java.nio.file.*;
import java.util.Arrays;

/** Calls the bundled revision-225 implementations; no later revision data. */
public class EffectsOracle {
 static Jagfile jag(String dir,String name)throws Exception{return new Jagfile(Files.readAllBytes(Paths.get(dir,name)),false);}
 static void mesh(DataOutputStream out,Model m)throws Exception {
  if(m.vertexCount==0 || m.faceCount==0)throw new IOException("Empty original mesh");
  out.writeInt(m.vertexCount);out.writeInt(m.faceCount);
  for(int i=0;i<m.vertexCount;i++){out.writeInt(m.vertexX[i]);out.writeInt(m.vertexY[i]);out.writeInt(m.vertexZ[i]);}
  for(int i=0;i<m.faceCount;i++){out.writeInt(m.faceColorA[i]);out.writeInt(m.faceColorB[i]);out.writeInt(m.faceColorC[i]);out.writeInt(m.faceAlpha==null?0:m.faceAlpha[i]);}
 }
 public static void main(String[] a)throws Exception {
  Jagfile config=jag(a[0],"config"),models=jag(a[0],"models");
  Model.unpack(3,models);AnimBase.unpack(false,models);AnimFrame.unpack(false,models);
  SeqType.unpack(config,1);SpotAnimType.unpack(config,1);
  try(DataOutputStream out=new DataOutputStream(new FileOutputStream(a[1]))) {
   out.writeInt(0x45464631);out.writeInt(SpotAnimType.count);
   for(SpotAnimType s:SpotAnimType.instances) {
    for(int v:new int[]{s.model,s.anim,s.resizeh,s.resizev,s.orientation,s.ambient,s.contrast,s.animHasAlpha?1:0})out.writeInt(v);
    for(int v:s.recol_s)out.writeInt(v);for(int v:s.recol_d)out.writeInt(v);
    for(int variant=0;variant<3;variant++) {
     int frame=s.seq==null?0:variant==0?0:s.seq.frameCount-1;
     out.writeInt(frame);
     if(variant<2) {
      ProjectileEntity p=new ProjectileEntity(0,0,6400,100,0,0,0,0,0,0,s.index,6400);
      p.seqFrame=frame;p.pitch=variant==0?0:300;mesh(out,p.draw(true));
     }else {
      SpotAnimEntity p=new SpotAnimEntity(6400,s.index,false,6400,0,0,0,0);p.seqFrame=frame;
      if(s.seq==null)p.seqComplete=true;mesh(out,p.draw(true));
     }
    }
   }
   int cases=8;out.writeInt(cases);
   for(int n=0;n<cases;n++) {
    int id=n%SpotAnimType.count,start=100,last=130+n,srcx=6400+n*128,srcz=6400,srcy=-50,peak=n*7,arc=n*4;
    out.writeInt(id);out.writeInt(start);out.writeInt(last);out.writeInt(srcx);out.writeInt(srcz);out.writeInt(srcy);out.writeInt(peak);out.writeInt(arc);
    ProjectileEntity p=new ProjectileEntity(0,peak,srcz,last,0,0,start,arc,0,srcy,id,srcx);
    out.writeInt(last-start+1);
    for(int tick=start;tick<=last;tick++) {
     int tx=7000+(tick-start)*3,tz=6700-(tick-start)*2,ty=-120;
     out.writeInt(tx);out.writeInt(tz);out.writeInt(ty);
     p.updateVelocity(ty,tz,tx,-855,tick);p.update((byte)-30,1);
     for(double v:new double[]{p.x,p.y,p.z,p.velocityX,p.velocityZ,p.velocity,p.velocityY,p.accelerationY})out.writeDouble(v);
     out.writeInt(p.yaw);out.writeInt(p.pitch);out.writeInt(p.seqFrame);out.writeInt(p.seqCycle);
    }
   }
   int animated=0;for(SpotAnimType s:SpotAnimType.instances)if(s.seq!=null)animated++;
   out.writeInt(animated);client clock=new client();
   for(SpotAnimType s:SpotAnimType.instances)if(s.seq!=null) {
    int ticks=12;for(int delay:s.seq.delay)ticks+=delay+1;
    out.writeInt(s.index);out.writeInt(ticks);
    SpotAnimEntity fixed=new SpotAnimEntity(6400,s.index,false,6400,0,0,0,0);
    NpcEntity actor=new NpcEntity();actor.spotanimId=s.index;actor.spotanimFrame=-1;actor.spotanimLastCycle=5;
    for(int tick=0;tick<ticks;tick++) {
     if(!fixed.seqComplete)fixed.update(1,0);
     client.loopCycle=tick;clock.updateSequences(true,actor);
     out.writeInt(fixed.seqFrame);out.writeInt(fixed.seqCycle);out.writeInt(fixed.seqComplete?1:0);
     out.writeInt(actor.spotanimId);out.writeInt(actor.spotanimFrame);out.writeInt(actor.spotanimCycle);
    }
   }
  }
  System.out.println(SpotAnimType.count+" actual spotanim definitions; three original animated/lit meshes each; eight homing projectile trajectories.");
  // Actual original entity-overlay routine, in its original viewport buffer.
  client app=new client();app.localPlayer=null;app.npcCount=1;app.npcIds[0]=42;
  NpcEntity npc=new NpcEntity();npc.type=new NpcType();npc.x=npc.z=6464;npc.height=100;app.npcs[42]=npc;
  app.levelHeightmap=new int[4][105][105];app.levelTileFlags=new byte[4][104][104];
  app.cameraX=6464;app.cameraZ=6000;app.cameraY=-500;app.cameraPitch=256;app.cameraYaw=0;
  Pix3D.centerW3D=256;Pix3D.centerH3D=167;
  Jagfile media=jag(a[0],"media");int marks=0;
  try {for(;marks<20;marks++)app.imageHitmarks[marks]=new Pix32(media,"hitmarks",marks);} catch(Exception end) {}
  app.fontPlain11=new PixFont(jag(a[0],"title"),"p11",530);
  int[] pixels=new int[512*334];Pix2D.bind(512,pixels,-1,334);
  int cases=marks*6;
  try(DataOutputStream out=new DataOutputStream(new FileOutputStream(a[2]))) {
   out.writeInt(0x48495431);out.writeInt(cases);
   for(int n=0;n<cases;n++) {
    Arrays.fill(pixels,0);client.loopCycle=100;
    npc.combatCycle=500-(n/marks)*70;npc.health=n%marks;npc.totalHealth=3;npc.damage=n%marks;npc.damageType=n%marks;
    app.projectFromGround(npc.height+15,false,npc);out.writeInt(app.projectX+8);out.writeInt(app.projectY+11);
    app.projectFromGround(npc.height/2,false,npc);out.writeInt(app.projectX+8);out.writeInt(app.projectY+11);
    out.writeInt(npc.combatCycle);out.writeInt(npc.health);out.writeInt(npc.totalHealth);out.writeInt(npc.damage);out.writeInt(npc.damageType);
    app.draw2DEntityElements(-1);
    int lit=0;for(int v:pixels)if(v!=0)lit++;out.writeInt(lit);
    for(int y=0;y<334;y++)for(int x=0;x<512;x++)if(pixels[y*512+x]!=0){out.writeInt((y+11)*789+x+8);out.writeInt(pixels[y*512+x]);}
   }
  }
  System.out.println((marks*6)+" actual original entity-overlay framebuffers; "+marks+" actual hitmark sprites and expiry boundaries.");
 }
}
