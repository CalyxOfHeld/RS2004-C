import deob.client;
import jagex2.config.*;
import jagex2.dash3d.*;
import jagex2.dash3d.entity.*;
import jagex2.dash3d.type.*;
import jagex2.graphics.*;
import jagex2.io.*;
import java.io.*;
import java.nio.file.*;
import java.util.*;

/** Uses original zone23, timed scene updates and PlayerEntity.draw. */
public class MergeOracle {
 static Jagfile jag(String dir,String name)throws Exception{return new Jagfile(Files.readAllBytes(Paths.get(dir,name)),false);}
 static void mesh(DataOutputStream out,Model m)throws Exception {
  out.writeInt(m.vertexCount);out.writeInt(m.faceCount);out.writeInt(m.texturedFaceCount);
  for(int i=0;i<m.vertexCount;i++)for(int v:new int[]{m.vertexX[i],m.vertexY[i],m.vertexZ[i]})out.writeInt(v);
  for(int i=0;i<m.faceCount;i++)for(int v:new int[]{m.faceVertexA[i],m.faceVertexB[i],m.faceVertexC[i],m.faceColor==null?0:m.faceColor[i],m.faceColorA[i],m.faceColorB[i],m.faceColorC[i],m.faceInfo==null?0:m.faceInfo[i],m.facePriority==null?m.priority:m.facePriority[i],m.faceAlpha==null?0:m.faceAlpha[i]})out.writeInt(v);
  for(int i=0;i<m.texturedFaceCount;i++)for(int v:new int[]{m.texturedVertexA[i],m.texturedVertexB[i],m.texturedVertexC[i]})out.writeInt(v);
 }
 static int bitset(client app,int layer) {
  return layer==0?app.scene.getWallBitset(0,50,50):layer==1?app.scene.getWallDecorationBitset(0,50,3,50):layer==2?app.scene.getLocBitset(0,50,50):app.scene.getGroundDecorationBitset(0,50,50);
 }
 public static void main(String[] args)throws Exception {
  Jagfile config=jag(args[0],"config"),models=jag(args[0],"models");
  Model.unpack(3,models);AnimBase.unpack(false,models);AnimFrame.unpack(false,models);SeqType.unpack(config,1);LocType.unpack(config);IdkType.unpack(config,1);ObjType.unpack(config);
  PlayerEntity p=new PlayerEntity();p.visible=true;p.primarySeqId=p.secondarySeqId=-1;
  for(int type=0;type<7;type++)for(int id=0;id<IdkType.count;id++)if(IdkType.instances[id].type==type && !IdkType.instances[id].disable){p.appearances[type]=256+id;break;}
  Model body=p.getSequencedModel(false);if(body.vertexCount==0)throw new IOException("Empty original player fixture");
  int[] representative=new int[23];Arrays.fill(representative,-1);
  for(int id=0;id<LocType.count;id++){LocType l=LocType.get(id);if(l.shapes!=null)for(int shape:l.shapes)if(shape<23 && representative[shape]<0)representative[shape]=id;}
  for(int i=5;i<=8;i++)representative[i]=representative[4];representative[11]=representative[10];
  int cases=0;for(int id:representative)if(id>=0)cases+=4;
  try(DataOutputStream out=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[1])))) {
   out.writeInt(0x4d524731);mesh(out,body);out.writeInt(cases);
   for(int shape=0;shape<23;shape++)if(representative[shape]>=0)for(int rotation=0;rotation<4;rotation++) {
    client app=new client();app.sceneState=2;app.localPid=7;app.localPlayer=p;app.players[42]=p;app.baseX=app.baseZ=48;app.currentLevel=0;
    app.out.random=new Isaac((byte)1,new int[4]);
    app.levelHeightmap=new int[4][105][105];app.levelTileFlags=new byte[4][104][104];
    for(int l=0;l<4;l++)for(int x=0;x<105;x++)for(int z=0;z<105;z++)app.levelHeightmap[l][x][z]=-l*240-x*3+z*2;
    app.scene=new World3D(1,app.levelHeightmap,104,4,104);
    for(int l=0;l<4;l++)app.levelCollisionMap[l]=new CollisionMap(104,-1,104);
    int id=representative[shape],slot=rotation%2==0?7:42,layer=LocationOracle.layer(shape),yaw=((shape*4+rotation)%8)*256;
    client.loopCycle=1000;app.addLoc(rotation,50,50,layer,id,shape,-27819,0);
    byte[] packet=new byte[]{0x22,(byte)(shape*4+rotation),(byte)(id>>8),(byte)id,0,5,0,15,0,(byte)slot,3,-2,-1,4};
    p.locModel=null;app.readZonePacket((byte)-45,new Packet(1,packet),23);
    for(int v:new int[]{id,shape,rotation,slot,yaw})out.writeInt(v);
    for(int v:new int[]{p.locStartCycle,p.locStopCycle,p.locOffsetX,p.locOffsetY,p.locOffsetZ,p.minTileX,p.maxTileX,p.minTileZ,p.maxTileZ})out.writeInt(v);
    out.writeInt(p.locModel==null?0:1);
    int[] ticks={1000,1004,1005,1006,1014,1015,1016};out.writeInt(ticks.length);
    for(int tick:ticks) {
     client.loopCycle=tick;app.updateMergeLocs(1);p.x=6464+(tick-1000)*3;p.z=6464-(tick-1000)*2;p.y=app.getHeightmapY(0,p.x,(byte)5,p.z);p.dstYaw=yaw;
     out.writeInt(tick);out.writeInt(p.x);out.writeInt(p.y);out.writeInt(p.z);
     int code=bitset(app,layer);out.writeInt(code==0?-1:(code>>14)&32767);out.writeInt(app.levelCollisionMap[0].flags[50][50]);
     mesh(out,p.draw(true));out.writeInt(p.locModel==null?0:1);
    }
   }
  }
  System.out.println("Original: "+cases+" zone23 local/remote packet -> timed scene removal/restoration -> PlayerEntity.draw cases, signed bounds, all map shapes, four rotations and eight facing values.");
 }
}
