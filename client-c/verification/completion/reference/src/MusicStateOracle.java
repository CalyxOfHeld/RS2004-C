import deob.client;
import sign.signlink;
import jagex2.io.*;
import jagex2.config.VarpType;
import java.io.*;
import java.net.Socket;
import java.nio.file.*;
public final class MusicStateOracle {
 static client c=new client();static int id,count;static byte[] jingle;
 static void string(DataOutputStream f,String s)throws Exception{f.writeInt(s==null?-1:s.length());if(s!=null)f.write(s.getBytes("US-ASCII"));}
 static void state(DataOutputStream f)throws Exception{f.writeBoolean(c.midiActive);f.writeBoolean(client.lowMemory);string(f,c.currentMidi);f.writeInt(c.midiCrc);f.writeInt(c.midiSize);f.writeInt(c.nextMusicDelay);f.writeInt(signlink.midivol);string(f,c.midiSyncName);f.writeInt(c.midiSyncCrc);f.writeInt(c.midiSyncLen);f.writeInt(signlink.midi==null?0:signlink.midi.equals("voladjust")?1:2);f.writeInt(signlink.midifade);}
 static void read(int op,byte[] payload)throws Exception{
  ByteArrayOutputStream b=new ByteArrayOutputStream();DataOutputStream d=new DataOutputStream(b);d.writeByte(op);int size=Protocol.SERVERPROT_SIZES[op];if(size==-1)d.writeByte(payload.length);if(size==-2)d.writeShort(payload.length);d.write(payload);
  final ByteArrayInputStream input=new ByteArrayInputStream(b.toByteArray());Socket socket=new Socket(){public void setSoTimeout(int n){}public void setTcpNoDelay(boolean b){}public InputStream getInputStream(){return input;}public OutputStream getOutputStream(){return new ByteArrayOutputStream();}};
  c.stream=new ClientStream(c,(byte)2,socket);c.randomIn=null;c.packetType=-1;c.ingame=true;if(!c.read(false)||input.available()!=0)throw new Exception("read"+op);
 }
 static void execute(int op,int value,String song)throws Exception{
  if(op==0){c.varps[id]=value;c.updateVarp(id,49);}
  if(op==1){ByteArrayOutputStream b=new ByteArrayOutputStream();DataOutputStream d=new DataOutputStream(b);d.write(song.getBytes("US-ASCII"));d.writeByte(10);d.writeInt(0x87654321);d.writeInt(12345);read(54,b.toByteArray());}
  if(op==2)c.updateGame(true);
  if(op==3){ByteArrayOutputStream b=new ByteArrayOutputStream();DataOutputStream d=new DataOutputStream(b);d.writeShort(value);d.write(jingle);read(212,b.toByteArray());}
 }
 public static void main(String[] args)throws Exception{
  ByteArrayOutputStream klass=new ByteArrayOutputStream();InputStream in=client.class.getResourceAsStream("client.class");int v;while((v=in.read())!=-1)klass.write(v);if(!new String(klass.toByteArray(),"ISO-8859-1").contains("musicSlice225"))throw new Exception("Wrong music-slice class");
  VarpType.unpack(new Jagfile(Files.readAllBytes(Paths.get(args[0],"config")),false),1);id=-1;for(int i=0;i<VarpType.count;i++)if(VarpType.instances[i].clientcode==3){id=i;break;}if(id<0)throw new Exception("music varp");jingle=Files.readAllBytes(Paths.get(args[0],"jingles","advance cooking.mid"));
  ByteArrayOutputStream bytes=new ByteArrayOutputStream();DataOutputStream f=new DataOutputStream(bytes);
  for(boolean active:new boolean[]{false,true})for(boolean low:new boolean[]{false,true})for(String current:new String[]{null,"","adventure"})for(String pending:new String[]{null,"old"})for(int delay:new int[]{0,1,20,21,65535})for(int op=0;op<4;op++){
   int[] values=op==0?new int[]{-1,0,1,2,3,4,5}:op==3?new int[]{0,1,20,40}:new int[]{0};
   String[] songs=op==1?new String[]{"","adventure","newbie melody"}:new String[]{""};
   for(int value:values)for(String song:songs){
    c.midiActive=active;client.lowMemory=low;c.currentMidi=current;c.midiCrc=0x12345678;c.midiSize=45;c.nextMusicDelay=delay;signlink.midivol=-777;signlink.midifade=7;signlink.midi=null;c.midiSyncName=pending;c.midiSyncCrc=33;c.midiSyncLen=44;signlink.savereq="busy-control";
    state(f);f.writeInt(op);f.writeInt(value);string(f,song);execute(op,value,song);state(f);c.updateGame(true);state(f);count++;
   }
  }
  try(DataOutputStream output=new DataOutputStream(new FileOutputStream(args[1]))){output.writeInt(0x4d553236);output.writeInt(count);output.writeInt(id);output.write(bytes.toByteArray());}
  System.out.println("PASS original music state:"+count+" settings/song54/real-cache jingle212/delay-slice states;varp="+id+";save request busy,not worker/device acceptance");
 }
}
