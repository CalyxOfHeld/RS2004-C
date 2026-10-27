import deob.client;
import jagex2.io.*;
import jagex2.dash3d.World3D;
import jagex2.dash3d.entity.PlayerEntity;
import jagex2.graphics.Pix2D;
import java.io.*;
import java.net.*;
import java.lang.reflect.*;
import java.util.*;
import java.util.concurrent.atomic.AtomicLong;
public class CameraPacketsOracle {
 static client c=new client();static DataOutputStream out;static int cases,draws,ticks,packets;static Random math;static AtomicLong randomState;static int[] view;
 static class Stop extends RuntimeException {private static final long serialVersionUID=1;}
 static class CaptureScene extends World3D {
  CaptureScene(){super(1,c.levelHeightmap,104,4,104);}
  public void draw(int yaw,int x,int top,int pitch,int y,int z,int ignored){view=new int[]{x,y,z,pitch,yaw,top};throw new Stop();}
 }
 static byte[] packet(int x,int z,int h,int speed,int acceleration){return new byte[]{(byte)x,(byte)z,(byte)(h>>8),(byte)h,(byte)speed,(byte)acceleration};}
 static void fresh(int level){c.currentLevel=level;c.cutscene=false;c.cameraX=6400;c.cameraY=-900;c.cameraZ=6000;c.cameraPitch=200;c.cameraYaw=2000;c.cutsceneSrcLocalTileX=c.cutsceneSrcLocalTileZ=c.cutsceneSrcHeight=c.cutsceneMoveSpeed=c.cutsceneMoveAcceleration=0;c.cutsceneDstLocalTileX=c.cutsceneDstLocalTileZ=c.cutsceneDstHeight=c.cutsceneRotateSpeed=c.cutsceneRotateAcceleration=0;Arrays.fill(c.cameraModifierEnabled,false);Arrays.fill(c.cameraModifierJitter,0);Arrays.fill(c.cameraModifierWobbleScale,0);Arrays.fill(c.cameraModifierWobbleSpeed,0);Arrays.fill(c.cameraModifierCycle,0);c.orbitCameraX=c.orbitCameraZ=6400;c.orbitCameraYaw=43;c.orbitCameraPitch=200;c.cameraPitchClamp=50000;c.cameraAnticheatAngle=9;}
 static void state()throws Exception {
  out.writeBoolean(c.cutscene);for(int v:new int[]{c.cutsceneSrcLocalTileX,c.cutsceneSrcLocalTileZ,c.cutsceneSrcHeight,c.cutsceneMoveSpeed,c.cutsceneMoveAcceleration,c.cutsceneDstLocalTileX,c.cutsceneDstLocalTileZ,c.cutsceneDstHeight,c.cutsceneRotateSpeed,c.cutsceneRotateAcceleration,c.cameraX,c.cameraY,c.cameraZ,c.cameraPitch,c.cameraYaw})out.writeInt(v);
  for(int i=0;i<5;i++){out.writeBoolean(c.cameraModifierEnabled[i]);out.writeInt(c.cameraModifierJitter[i]);out.writeInt(c.cameraModifierWobbleScale[i]);out.writeInt(c.cameraModifierWobbleSpeed[i]);out.writeInt(c.cameraModifierCycle[i]);}
 }
 static void one(int opcode,byte[] payload)throws Exception {
  out.writeInt(opcode);out.writeInt(payload.length);out.write(payload);long seed=225000L+cases;out.writeLong(seed);
  if(opcode==-3){fresh(payload[0]);if(payload.length>1)c.cameraX=c.cameraZ=-2000;}
  else if(opcode==-1){if(c.cutscene)c.applyCutscene((byte)2);for(int i=0;i<5;i++)c.cameraModifierCycle[i]++;ticks++;}
  else if(opcode==-2){math.setSeed(seed);int[] before={c.cameraX,c.cameraY,c.cameraZ,c.cameraPitch,c.cameraYaw};view=null;Pix2D.bind(512,new int[512*334],-657,334);try{c.drawScene(0);}catch(Stop stop){}if(view==null)throw new Exception("draw did not reach scene");for(int value:view)out.writeInt(value);out.writeLong(randomState.get());if(c.cutscene){c.cameraX=before[0];c.cameraY=before[1];c.cameraZ=before[2];c.cameraPitch=before[3];c.cameraYaw=before[4];}else { // source restores orbit base after drawScene; reconstruct actual orbit before shake.
   int pitch=Math.max(c.orbitCameraPitch,c.cameraPitchClamp/256);if(c.cameraModifierEnabled[4])pitch=Math.max(pitch,c.cameraModifierWobbleScale[4]+128);c.orbitCamera(c.getHeightmapY(c.currentLevel,c.localPlayer.x,(byte)5,c.localPlayer.z)-50,c.orbitCameraX,(c.orbitCameraYaw+c.cameraAnticheatAngle)&2047,pitch,16418,c.orbitCameraZ,pitch*3+600);
  }draws++;}
  else {byte[] frame=new byte[payload.length+1];frame[0]=(byte)opcode;System.arraycopy(payload,0,frame,1,payload.length);final ByteArrayInputStream input=new ByteArrayInputStream(frame);Socket socket=new Socket(){public void setSoTimeout(int n){}public void setTcpNoDelay(boolean b){}public InputStream getInputStream(){return input;}public OutputStream getOutputStream(){return new ByteArrayOutputStream();}};c.stream=new ClientStream(c,(byte)2,socket);c.packetType=-1;c.randomIn=null;if(!c.read(false) || input.available()!=0 || c.packetType!=-1)throw new Exception("packet not consumed");packets++;}
  state();cases++;
 }
 public static void main(String[] args)throws Exception {
  Field holder=Class.forName("java.lang.Math$RandomNumberGeneratorHolder").getDeclaredField("randomNumberGenerator");holder.setAccessible(true);math=(Random)holder.get(null);Field seed=Random.class.getDeclaredField("seed");seed.setAccessible(true);randomState=(AtomicLong)seed.get(math);
  c.levelHeightmap=new int[4][105][105];c.levelTileFlags=new byte[4][104][104];for(int l=0;l<4;l++)for(int x=0;x<105;x++)for(int z=0;z<105;z++){c.levelHeightmap[l][x][z]=-l*240-x*13-z*7+x*z%37;if(x<104 && z<104)c.levelTileFlags[l][x][z]=(byte)((l==1 && (x+z)%3==0?2:0)|((x+z)%4==0?4:0));}
  c.localPlayer=new PlayerEntity();c.localPlayer.x=c.localPlayer.z=6400;c.localPlayer.visible=false;c.scene=new CaptureScene();ByteArrayOutputStream bytes=new ByteArrayOutputStream();out=new DataOutputStream(new BufferedOutputStream(bytes));
  for(int level=0;level<4;level++)for(int a:new int[]{0,1,99,100,101,255}){one(-3,new byte[]{(byte)level});one(3,packet(20,30,200,7,a));one(74,packet(60,50,100,5,a));for(int i=0;i<32;i++){one(-1,new byte[0]);if(i%8==0)one(-2,new byte[0]);}for(int i=0;i<5;i++)one(13,new byte[]{(byte)i,(byte)(i*25),(byte)(255-i*30),(byte)(i*40)});for(int i=0;i<20;i++){one(-1,new byte[0]);one(-2,new byte[0]);}one(239,new byte[0]);one(-2,new byte[0]);}
  for(int edge:new int[]{0,1,103,255}){one(-3,new byte[]{0});one(3,packet(edge,edge,65535,255,255));one(74,packet(edge,edge,65535,255,255));one(-1,new byte[0]);}
  Random random=new Random(22531374);one(-3,new byte[]{2});for(int i=0;i<300;i++){one(i%2==0?3:74,packet(random.nextInt(104),random.nextInt(104),random.nextInt(65536),random.nextInt(256),random.nextInt(256)));one(-1,new byte[0]);one(13,new byte[]{(byte)(i%5),(byte)random.nextInt(256),(byte)random.nextInt(256),(byte)random.nextInt(256)});one(-2,new byte[0]);if(i%13==0)one(239,new byte[0]);}
  one(-3,new byte[]{0,1});one(74,packet(255,255,200,10,100));
  out.flush();try(DataOutputStream f=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[0])))){f.writeInt(0x43413235);f.writeInt(cases);f.write(bytes.toByteArray());}System.out.println("Original camera:"+cases+" states,"+packets+" actual read packets,"+ticks+" applyCutscene ticks,"+draws+" actual drawScene camera/RNG captures;all5 modifier arrays,base restore and raw tile-height sampling");
 }
}
