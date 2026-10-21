import deob.client;
import jagex2.dash3d.entity.*;
import java.io.*;
public class FacingOracle {
 public static void main(String[] args)throws Exception {
  client app=new client();app.sceneBaseTileX=3152;app.sceneBaseTileZ=3200;app.localPid=11;app.localPlayer=new PlayerEntity();app.localPlayer.x=6336;app.localPlayer.z=6400;app.players[2047]=app.localPlayer;app.players[17]=new PlayerEntity();app.npcs[42]=new NpcEntity();int cases=0;
  try(DataOutputStream out=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[0])))) {
   out.writeInt(0x46414331);out.writeInt(17*17*6*4);
   for(int dx=-1024;dx<=1024;dx+=128)for(int dz=-1024;dz<=1024;dz+=128)for(int mode=0;mode<6;mode++)for(int coord=0;coord<4;coord++) {
    NpcEntity actor=new NpcEntity();actor.x=6464;actor.z=6528;actor.yaw=((dx+dz+2048)*3+mode*32)&2047;actor.dstYaw=(actor.yaw+300)&2047;
    app.npcs[42].x=actor.x+dx;app.npcs[42].z=actor.z+dz;app.players[17].x=actor.x-dx;app.players[17].z=actor.z-dz;
    actor.targetId=mode==0?42:mode==1?32768+17:mode==2?32768+11:mode==3?43:mode==4?32768+18:-1;
    actor.pathLength=coord>=2?1:0;actor.seqTrigger=coord==3?1:0;
    if(coord>0){actor.targetTileX=app.sceneBaseTileX*2+(actor.x+dx)/64;actor.targetTileZ=app.sceneBaseTileZ*2+(actor.z-dz)/64;}
    actor.secondarySeqId=actor.seqStandId=808;actor.seqTurnId=mode%2==0?823:-1;actor.seqWalkId=819;
    for(int v:new int[]{actor.x,actor.z,actor.yaw,actor.dstYaw,actor.targetId,actor.pathLength,actor.seqTrigger,actor.targetTileX,actor.targetTileZ,actor.seqTurnId,app.npcs[42].x,app.npcs[42].z,app.players[17].x,app.players[17].z})out.writeInt(v);
    app.updateFacingDirection(actor,(byte)117);
    for(int v:new int[]{actor.yaw,actor.dstYaw,actor.secondarySeqId,actor.targetTileX,actor.targetTileZ})out.writeInt(v);cases++;
   }
  }
  System.out.println("Original: "+cases+" entity/coordinate facing cases, local-PID remap, absent/coincident targets, movement gating, turn interpolation and animation fallback.");
 }
}
