import deob.client;
import sign.signlink;
import jagex2.io.*;
import jagex2.config.*;
import jagex2.graphics.*;
import jagex2.dash3d.*;
import jagex2.dash3d.entity.PlayerEntity;
import java.awt.*;
import java.awt.image.ImageProducer;
import java.io.*;
import java.net.*;
import java.nio.file.*;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;
import java.util.zip.CRC32;
public class MapDownloadOracle {
 static client c=new client();static DataOutputStream out;static int cases,builds,requests,saves;static boolean allowWrites=true;static int[] seed={225,150,132,220};
 static Map<String,byte[]> cache=new ConcurrentHashMap<>();static SortedSet<String> names=new TreeSet<>();
 static class Area extends PixMap {
  Area(){super(new Canvas(){public Image createImage(ImageProducer producer){return Toolkit.getDefaultToolkit().createImage(producer);}},512,299,334);}
  public void draw(int y,Graphics g,int x,int token){} // presentation only; original pixel bind/font calls still execute
 }
 static Jagfile jag(String dir,String name)throws Exception{return new Jagfile(Files.readAllBytes(Paths.get(dir,name)),false);}
 static void data(byte[] bytes)throws Exception{out.writeInt(bytes==null?-1:bytes.length);if(bytes!=null)out.write(bytes);}
 static void text(String value)throws Exception{byte[] bytes=value.getBytes("US-ASCII");out.writeInt(bytes.length);out.write(bytes);}
 static int crc(byte[] data){CRC32 crc=new CRC32();crc.update(data);return (int)crc.getValue();}
 static byte[] packet(int... fields)throws Exception{ByteArrayOutputStream bytes=new ByteArrayOutputStream();DataOutputStream d=new DataOutputStream(bytes);for(int v:fields)d.writeShort(v);return bytes.toByteArray();}
 static void startCache(){signlink.active=true;Thread worker=new Thread(()->{for(;;){if(signlink.loadreq!=null){byte[] bytes=cache.get(signlink.loadreq);signlink.loadbuf=bytes==null?null:bytes.clone();signlink.loadreq=null;}else if(signlink.savereq!=null){if(allowWrites && signlink.savebuf!=null)cache.put(signlink.savereq,Arrays.copyOf(signlink.savebuf,signlink.savelen));signlink.savereq=null;}else {try{Thread.sleep(1);}catch(InterruptedException e){return;}}}});worker.setDaemon(true);worker.start();}
 static void read(int opcode,byte[] payload)throws Exception{
  ByteArrayOutputStream bytes=new ByteArrayOutputStream();DataOutputStream frame=new DataOutputStream(bytes);frame.writeByte(opcode);if(opcode==237 || opcode==132 || opcode==220 || opcode==184)frame.writeShort(payload.length);frame.write(payload);final ByteArrayInputStream input=new ByteArrayInputStream(bytes.toByteArray());Socket socket=new Socket(){public void setSoTimeout(int n){}public void setTcpNoDelay(boolean b){}public InputStream getInputStream(){return input;}public OutputStream getOutputStream(){return new ByteArrayOutputStream();}};c.stream=new ClientStream(c,(byte)2,socket);c.packetType=-1;c.randomIn=null;if(!c.read(false) || !c.ingame || input.available()!=0 || c.packetType!=-1)throw new Exception("read failed "+opcode);
 }
 static void register(int x,int z){names.add("m"+x+"_"+z);names.add("l"+x+"_"+z);}
 static void snapshot(boolean built)throws Exception{
  for(int value:new int[]{c.sceneCenterZoneX,c.sceneCenterZoneZ,c.sceneBaseTileX,c.sceneBaseTileZ,c.sceneState})out.writeInt(value);out.writeBoolean(c.cutscene);out.writeInt(c.sceneMapIndex.length);
  for(int i=0;i<c.sceneMapIndex.length;i++){out.writeInt(c.sceneMapIndex[i]);data(c.sceneMapLandData[i]);data(c.sceneMapLocData[i]);}
  out.writeInt(c.out.pos);out.write(c.out.data,0,c.out.pos);Isaac cipher=c.out.random;for(int v:new int[]{cipher.count,cipher.a,cipher.b,cipher.c})out.writeInt(v);for(int v:cipher.rsl)out.writeInt(v);for(int v:cipher.mem)out.writeInt(v);
  out.writeInt(names.size());for(String name:names){text(name);data(cache.get(String.valueOf(signlink.gethash(name))));}
  out.writeBoolean(built);if(built){for(int l=0;l<4;l++)for(int x=0;x<105;x++)for(int z=0;z<105;z++)out.writeInt(c.levelHeightmap[l][x][z]);for(int l=0;l<4;l++)for(int x=0;x<104;x++)for(int z=0;z<104;z++){out.writeByte(c.levelTileFlags[l][x][z]);out.writeInt(c.levelCollisionMap[l].flags[x][z]);}builds++;}
 }
 static void one(int opcode,byte[] payload)throws Exception{
  out.writeInt(opcode);out.writeInt(payload.length);out.write(payload);boolean built=false;c.out.pos=0;
  if(opcode==-1){c.ingame=true;cache.clear();names.clear();allowWrites=true;DataInputStream in=new DataInputStream(new ByteArrayInputStream(payload));int n=in.readInt();for(int i=0;i<n;i++){String name=in.readUTF();int size=in.readInt();byte[] bytes=new byte[size];in.readFully(bytes);names.add(name);cache.put(String.valueOf(signlink.gethash(name)),bytes);}c.sceneCenterZoneX=c.sceneCenterZoneZ=c.sceneBaseTileX=c.sceneBaseTileZ=c.mapLastBaseX=c.mapLastBaseZ=0;c.sceneState=0;c.sceneMapIndex=new int[0];c.sceneMapLandData=new byte[0][];c.sceneMapLocData=new byte[0][];c.cutscene=true;c.out.random=new Isaac((byte)1,seed);c.levelHeightmap=new int[4][105][105];c.levelTileFlags=new byte[4][104][104];c.scene=new World3D(1,c.levelHeightmap,104,4,104);for(int i=0;i<4;i++)c.levelCollisionMap[i]=new CollisionMap(104,-1,104);}
  else if(opcode==-2)allowWrites=payload[0]!=0;
  else {if(opcode==237){for(int i=4;i+9<payload.length;i+=10)register(payload[i]&255,payload[i+1]&255);requests++;}else if(opcode==20 || opcode==80 || opcode==132 || opcode==220)register(payload[0]&255,payload[1]&255);if(opcode==184){c.localPlayer.x=c.localPlayer.z=6400;for(int i=0;i<10;i++)c.localPlayer.pathTileX[i]=c.localPlayer.pathTileZ[i]=50;built=c.sceneState==1;}read(opcode,payload);if(built && c.out.pos!=4)throw new Exception("original build did not finish all4 keepalives: "+c.out.pos);if(opcode==20 || opcode==80)saves++;}
  snapshot(built);cases++;
 }
 static void reset(String[] names,byte[][] values)throws Exception{ByteArrayOutputStream bytes=new ByteArrayOutputStream();DataOutputStream d=new DataOutputStream(bytes);d.writeInt(names.length);for(int i=0;i<names.length;i++){d.writeUTF(names[i]);d.writeInt(values[i].length);d.write(values[i]);}one(-1,bytes.toByteArray());}
 static void rebuild(int center,int[][] regions)throws Exception{ByteArrayOutputStream bytes=new ByteArrayOutputStream();DataOutputStream d=new DataOutputStream(bytes);d.writeShort(center);d.writeShort(center);for(int[] r:regions){d.writeByte(r[0]);d.writeByte(r[1]);d.writeInt(r[2]);d.writeInt(r[3]);}one(237,bytes.toByteArray());}
 static void chunk(int op,int x,int z,int offset,int total,byte[] bytes)throws Exception{ByteArrayOutputStream all=new ByteArrayOutputStream();DataOutputStream d=new DataOutputStream(all);d.writeByte(x);d.writeByte(z);d.writeShort(offset);d.writeShort(total);d.write(bytes);one(op,all.toByteArray());}
 static void download(int op,byte[] bytes,boolean reverse)throws Exception{int count=(bytes.length+990)/991;for(int i=0;i<count;i++){int at=(reverse?count-1-i:i)*991;chunk(op,50,50,at,bytes.length,Arrays.copyOfRange(bytes,at,Math.min(bytes.length,at+991)));}}
 public static void main(String[] args)throws Exception{
  Jagfile models=jag(args[0],"models"),config=jag(args[0],"config"),title=jag(args[0],"title");Model.unpack(3,models);AnimBase.unpack(false,models);AnimFrame.unpack(false,models);SeqType.unpack(config,1);LocType.unpack(config);FloType.unpack(config,1);ObjType.unpack(config);Pix3D.unpackTextures((byte)2,jag(args[0],"textures"));Pix3D.setBrightness(true,0.8);client.lowMemory=World3D.lowMemory=false;c.fontPlain12=new PixFont(title,"p12",530);c.areaViewport=new Area();c.localPlayer=new PlayerEntity();c.players[2047]=c.localPlayer;c.out=new Packet(5,new byte[65536]);signlink.reporterror=false;c.imageMinimap=new Pix32(512,512);Jagfile media=jag(args[0],"media");try{for(int i=0;i<50;i++)c.imageMapscene[i]=new Pix8(media,"mapscene",i);}catch(Exception e){}try{for(int i=0;i<50;i++)c.imageMapfunction[i]=new Pix32(media,"mapfunction",i);}catch(Exception e){}startCache();
  byte[] land=Files.readAllBytes(Paths.get(args[0],"maps/m50_50")),loc=Files.readAllBytes(Paths.get(args[0],"maps/l50_50"));int[][] region={{50,50,crc(land),crc(loc)}};ByteArrayOutputStream bytes=new ByteArrayOutputStream();out=new DataOutputStream(new BufferedOutputStream(bytes));
  reset(new String[0],new byte[0][]);rebuild(406,region);one(184,new byte[]{0,127,(byte)240});chunk(132,255,255,65535,0,new byte[]{1,2});download(132,land,true);one(184,new byte[]{0,127,(byte)240});one(80,new byte[]{50,50});one(184,new byte[]{0,127,(byte)240});download(220,loc,false);one(20,new byte[]{50,50});one(184,new byte[]{0,127,(byte)240});one(20,new byte[]{50,50});one(184,new byte[]{0,127,(byte)240});rebuild(406,region);
  reset(new String[]{"m50_50","l50_50"},new byte[][]{land,loc});rebuild(406,region);one(184,new byte[]{0,127,(byte)240});
  byte[] bad=land.clone();bad[bad.length-1]^=1;reset(new String[]{"m50_50","l50_50"},new byte[][]{bad,loc});rebuild(406,region);rebuild(406,region);download(132,land,false);one(-2,new byte[]{0});one(80,new byte[]{50,50});one(184,new byte[]{0,127,(byte)240});one(-2,new byte[]{1});one(80,new byte[]{50,50});one(184,new byte[]{0,127,(byte)240});
  reset(new String[]{"m50_50","l50_50"},new byte[][]{land,loc});rebuild(406,new int[][]{{50,50,0,0}});one(184,new byte[]{0,127,(byte)240});
  reset(new String[0],new byte[0][]);rebuild(406,new int[][]{{50,50,1,2},{51,50,3,4},{50,50,5,6}});for(int op:new int[]{132,220}){chunk(op,50,50,2,8,new byte[]{1,2,3});chunk(op,50,50,0,8,new byte[]{9});chunk(op,50,50,6,8,new byte[]{7,8});chunk(op,50,50,0,4,new byte[]{4,3,2,1});chunk(op,50,50,0,0,new byte[0]);one(op==132?80:20,new byte[]{50,50});chunk(op,50,50,65535,65535,new byte[0]);chunk(op,50,50,0,3,new byte[]{1,2,3});}one(80,new byte[]{99,99});
  Random random=new Random(225132220);for(int i=0;i<300;i++){int size=random.nextInt(200),off=random.nextInt(size+1),n=random.nextInt(size-off+1);byte[] data=new byte[n];random.nextBytes(data);chunk(i%2==0?132:220,i%7==0?51:50,50,off,size,data);if(i%19==0)one(i%2==0?80:20,new byte[]{50,50});}
  out.flush();try(DataOutputStream f=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[1])))){f.writeInt(0x4d443235);f.writeInt(cases);for(int value:seed)f.writeInt(value);f.write(bytes.toByteArray());}System.out.println("Original map lifecycle:"+cases+" states,"+requests+" rebuild requests,"+saves+" done observations,"+builds+" completed actual buildScene snapshots;full buffers/cache snapshots/wire/ISAAC/height/flags/collision;real map files");
 }
}
