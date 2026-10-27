import deob.client;
import sign.signlink;
import jagex2.io.*;
import jagex2.config.*;
import jagex2.sound.Wave;
import java.io.*;
import java.net.Socket;
import java.nio.file.*;
import java.util.*;
public class WaveSettingOracle {
 static client c=new client();static int id,count;static final int[] seed={225,4,12,155};
 static Jagfile jag(String dir,String name)throws Exception{return new Jagfile(Files.readAllBytes(Paths.get(dir,name)),false);}
 static void read(int op,byte[] payload)throws Exception{byte[] frame=new byte[payload.length+1];frame[0]=(byte)op;System.arraycopy(payload,0,frame,1,payload.length);final ByteArrayInputStream input=new ByteArrayInputStream(frame);Socket s=new Socket(){public void setSoTimeout(int n){}public void setTcpNoDelay(boolean b){}public InputStream getInputStream(){return input;}public OutputStream getOutputStream(){return new ByteArrayOutputStream();}};c.stream=new ClientStream(c,(byte)2,s);c.randomIn=null;c.packetType=-1;c.ingame=true;if(!c.read(false)||input.available()!=0)throw new Exception("read"+op);}
 static void setting(DataOutputStream f,int op,int prior,boolean enabled,int volume,int value)throws Exception{
  Arrays.fill(c.varps,0);Arrays.fill(c.varCache,0);c.varps[id]=prior;c.varCache[id]=77;c.waveEnabled=enabled;signlink.wavevol=volume;c.out.pos=0;c.out.random=new Isaac((byte)1,seed);
  f.writeInt(op);f.writeInt(prior);f.writeBoolean(enabled);f.writeInt(volume);f.writeInt(value);
  if(op==0){c.varps[id]=value;c.updateVarp(id,49);}else if(op==4||op==5){Component b=Component.instances[0];b.scripts=new int[][]{{5,id,0}};b.scriptOperand=new int[]{value};c.menuAction[0]=op==4?465:960;c.menuParamC[0]=0;c.useMenuOption(6412,0);}else if(op==193){c.varCache[id]=value;read(op,new byte[0]);}else {ByteArrayOutputStream b=new ByteArrayOutputStream();DataOutputStream d=new DataOutputStream(b);d.writeShort(id);if(op==150)d.writeByte(value);else d.writeInt(value);read(op,b.toByteArray());}
  f.writeInt(c.varps[id]);f.writeInt(c.varCache[id]);f.writeBoolean(c.waveEnabled);f.writeInt(signlink.wavevol);f.writeInt(c.out.pos);f.write(c.out.data,0,c.out.pos);Isaac a=c.out.random;for(int v:new int[]{a.count,a.a,a.b,a.c})f.writeInt(v);for(int v:a.rsl)f.writeInt(v);for(int v:a.mem)f.writeInt(v);count++;
 }
 public static void main(String[] args)throws Exception{
  VarpType.unpack(jag(args[0],"config"),1);id=-1;for(int i=0;i<VarpType.count;i++)if(VarpType.instances[i].clientcode==4){id=i;break;}if(id<0)throw new Exception("cache code4");Component.instances=new Component[]{new Component()};c.out=new Packet(5,new byte[5000]);Wave.unpack(new Packet(363,jag(args[0],"sounds").read("sounds.dat",null,(byte)0)),1);if(Wave.tracks[2]==null)throw new Exception("cache sound2");
  ByteArrayOutputStream bytes=new ByteArrayOutputStream();DataOutputStream f=new DataOutputStream(bytes);
  for(int op:new int[]{0,150,175,193,4,5})for(int prior:new int[]{0,1,4,77})for(boolean enabled:new boolean[]{false,true})for(int volume:new int[]{-1200,0})for(int value:new int[]{-1,0,1,2,3,4,5,128,Integer.MIN_VALUE,Integer.MAX_VALUE})setting(f,op,prior,enabled,volume,value);
  int queues=0;for(boolean enabled:new boolean[]{false,true})for(int prior:new int[]{0,49,50})for(int loops:new int[]{0,1,2,255})for(int delay:new int[]{0,1,65535}){
   c.waveEnabled=enabled;c.waveCount=prior;for(int i=0;i<prior;i++){c.waveIds[i]=2;c.waveLoops[i]=1;c.waveDelay[i]=400+i;}
   f.writeBoolean(enabled);f.writeInt(prior);f.writeInt(loops);f.writeInt(delay);read(12,new byte[]{0,2,(byte)loops,(byte)(delay>>8),(byte)delay});f.writeInt(c.waveCount);for(int i=0;i<c.waveCount;i++){f.writeInt(c.waveIds[i]);f.writeInt(c.waveLoops[i]);f.writeInt(c.waveDelay[i]);}queues++;
  }
  try(DataOutputStream output=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[1])))){output.writeInt(0x57533235);output.writeInt(count);output.writeInt(queues);output.writeInt(id);output.writeInt(Wave.delays[2]);for(int v:seed)output.writeInt(v);output.write(bytes.toByteArray());}
  System.out.println("Original wave setting:"+count+" setting states,"+queues+" read12 admission states;varp="+id+" trim2="+Wave.delays[2]+";full local/server/enabled/retained volume/wire/ISAAC/queue;unchanged original jar,high-memory only");
 }
}
