import deob.client;
import sign.signlink;
import jagex2.io.*;
import jagex2.config.*;
import java.io.*;
import java.net.Socket;
import java.nio.file.*;
import java.util.Arrays;
public final class MusicSettingOracle {
 static client c=new client();static int id,count;static final int[] seed={225,3,168,155};
 static void read(int op,byte[] payload)throws Exception{byte[] frame=new byte[payload.length+1];frame[0]=(byte)op;System.arraycopy(payload,0,frame,1,payload.length);final ByteArrayInputStream input=new ByteArrayInputStream(frame);Socket socket=new Socket(){public void setSoTimeout(int n){}public void setTcpNoDelay(boolean b){}public InputStream getInputStream(){return input;}public OutputStream getOutputStream(){return new ByteArrayOutputStream();}};c.stream=new ClientStream(c,(byte)2,socket);c.randomIn=null;c.packetType=-1;c.ingame=true;if(!c.read(false)||input.available()!=0)throw new Exception("read"+op);}
 static void one(DataOutputStream f,int op,int prior,boolean active,int volume,int value)throws Exception{
  Arrays.fill(c.varps,0);Arrays.fill(c.varCache,0);c.varps[id]=prior;c.varCache[id]=77;c.midiActive=active;client.lowMemory=false;signlink.midivol=volume;signlink.midi=null;signlink.midifade=7;c.currentMidi="adventure";c.midiCrc=0x12345678;c.midiSize=45;c.nextMusicDelay=21;c.midiSyncName="old";c.midiSyncCrc=33;c.midiSyncLen=44;c.out.pos=0;c.out.random=new Isaac((byte)1,seed);
  f.writeInt(op);f.writeInt(prior);f.writeBoolean(active);f.writeInt(volume);f.writeInt(value);
  if(op==0){c.varps[id]=value;c.updateVarp(id,49);}else if(op==4||op==5){Component b=Component.instances[0];b.scripts=new int[][]{{5,id,0}};b.scriptOperand=new int[]{value};c.menuAction[0]=op==4?465:960;c.menuParamC[0]=0;c.useMenuOption(6412,0);}else if(op==193){c.varCache[id]=value;read(op,new byte[0]);}else {ByteArrayOutputStream bytes=new ByteArrayOutputStream();DataOutputStream d=new DataOutputStream(bytes);d.writeShort(id);if(op==150)d.writeByte(value);else d.writeInt(value);read(op,bytes.toByteArray());}
  f.writeInt(c.varps[id]);f.writeInt(c.varCache[id]);MusicStateOracle.c=c;MusicStateOracle.state(f);f.writeInt(c.out.pos);f.write(c.out.data,0,c.out.pos);Isaac a=c.out.random;for(int v:new int[]{a.count,a.a,a.b,a.c})f.writeInt(v);for(int v:a.rsl)f.writeInt(v);for(int v:a.mem)f.writeInt(v);count++;
 }
 public static void main(String[] args)throws Exception{
  VarpType.unpack(new Jagfile(Files.readAllBytes(Paths.get(args[0],"config")),false),1);id=-1;for(int i=0;i<VarpType.count;i++)if(VarpType.instances[i].clientcode==3){id=i;break;}if(id<0)throw new Exception("cache music varp");Component.instances=new Component[]{new Component()};c.out=new Packet(5,new byte[5000]);ByteArrayOutputStream bytes=new ByteArrayOutputStream();DataOutputStream f=new DataOutputStream(bytes);
  for(int op:new int[]{0,150,175,193,4,5})for(int prior:new int[]{0,1,4,77})for(boolean active:new boolean[]{false,true})for(int volume:new int[]{-1200,0})for(int value:new int[]{-1,0,1,2,3,4,5,128,Integer.MIN_VALUE,Integer.MAX_VALUE})one(f,op,prior,active,volume,value);
  try(DataOutputStream out=new DataOutputStream(new FileOutputStream(args[1]))){out.writeInt(0x4d533235);out.writeInt(count);out.writeInt(id);for(int v:seed)out.writeInt(v);out.write(bytes.toByteArray());}
  System.out.println("PASS original music settings:"+count+" direct/150/175/193/toggle/set states;retained song/request/delay/volume/fade,local/server varps,full wire/ISAAC");
 }
}
