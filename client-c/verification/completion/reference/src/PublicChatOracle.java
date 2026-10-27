import deob.client;
import jagex2.io.*;
import jagex2.wordenc.*;
import jagex2.datastruct.JString;
import jagex2.dash3d.entity.*;
import java.io.*;
import java.nio.file.*;
import java.util.*;
// Calls actual original getPlayerExtended and updateEntityChats, unchanged jar.
public class PublicChatOracle {
 static client c=new client();static DataOutputStream out;static int cases,accepted;
 static void text(String s)throws Exception{if(s==null){out.writeInt(-1);return;}out.writeInt(s.length());for(char ch:s.toCharArray())out.writeChar(ch);}
 static byte[] packed(String s){Packet p=new Packet(5,new byte[256]);WordPack.pack(p,true,s);return Arrays.copyOf(p.data,p.pos);}
 static void one(String name,boolean ignore,int override,int rights,int colour,byte[] raw,String forced,boolean suffix)throws Exception {
  PlayerEntity p=new PlayerEntity();p.name=name;p.chat="old";p.chatColor=13;p.chatStyle=17;p.chatTimer=7;
  c.ignoreCount=ignore?1:0;c.ignoreName37[0]=JString.toBase37(name==null?"alice":name);c.overrideChat=override;
  ByteArrayOutputStream bytes=new ByteArrayOutputStream();DataOutputStream d=new DataOutputStream(bytes);int mask=0;
  if(forced!=null){mask|=8;for(char ch:forced.toCharArray())d.writeByte(ch);d.writeByte(10);}
  if(raw!=null){mask|=64;d.writeShort(colour);d.writeByte(rights);d.writeByte(raw.length);d.write(raw);}
  if(suffix){mask|=256;d.writeShort(65535);d.writeInt((19<<16)|23);}
  byte[] payload=bytes.toByteArray();Packet packet=new Packet(5,payload);client.loopCycle=1000;
  String before=c.messageText[0];c.getPlayerExtended(true,2047,mask,packet,p);if(c.messageText[0]!=before)accepted++;
  text(name);out.writeBoolean(ignore);out.writeInt(override);out.writeInt(mask);out.writeInt(payload.length);out.write(payload);
  out.writeInt(packet.pos);text(p.chat);out.writeInt(p.chatColor);out.writeInt(p.chatStyle);out.writeInt(p.chatTimer);
  out.writeInt(p.spotanimId);out.writeInt(p.spotanimOffset);out.writeInt(p.spotanimLastCycle);out.writeInt(p.spotanimFrame);out.writeInt(p.spotanimCycle);
  for(int i=0;i<100;i++){out.writeInt(c.messageType[i]);text(c.messageSender[i]);text(c.messageText[i]);}cases++;
 }
 static void timer(DataOutputStream file,PathingEntity p)throws Exception{file.writeInt(p.chatTimer);file.writeBoolean(p.chat==null);}
 public static void main(String[] args)throws Exception {
  Locale.setDefault(Locale.ENGLISH);WordFilter.unpack(new Jagfile(Files.readAllBytes(Paths.get(args[0])),false));
  ByteArrayOutputStream b=new ByteArrayOutputStream();out=new DataOutputStream(new BufferedOutputStream(b));
  for(int rights=0;rights<256;rights++)for(boolean ignored:new boolean[]{false,true})for(int override=0;override<2;override++)
   one("Alice",ignored,override,rights,(rights<<8)|(255-rights),packed("fuck cook x.com/path 127.0.0.1"),null,true);
  for(String name:new String[]{null,"Alice","Bob Test","aLiCe","invalid_name"})for(boolean ignore:new boolean[]{false,true})for(int over=0;over<2;over++)
   for(int rights:new int[]{0,1,2,255})one(name,ignore,over,rights,65535,packed("Hello WORLD!"),null,true);
  for(char[][] group:new char[][][]{WordFilter.badWords,WordFilter.domains,WordFilter.tlds})for(char[] word:group)
   one("Bob Test",false,0,cases%256,cases%65536,packed("name@"+new String(word)+".com/path"),null,true);
  for(int n=0;n<=51;n++){byte[] raw=new byte[n];one("Alice",false,0,0,0x1234,raw,null,true);}
  one("Alice",true,0,0,0,new byte[255],null,true);one(null,false,0,2,0,new byte[255],null,true);one("Alice",false,1,255,0,new byte[255],null,true);
  for(String name:new String[]{null,"Alice"})for(String forced:new String[]{"forced","","~forced", "Raw fuck 123"})for(int over=0;over<2;over++) {
   one(name,true,over,0,0,null,forced,true);one(name,true,over,2,0xbeef,packed("followup"),forced,true);
  }
  Random rand=new Random(22564);for(int i=0;i<1000;i++){byte[] raw=new byte[rand.nextInt(51)];rand.nextBytes(raw);one("Alice",i%7==0,i%13==0?1:0,i%256,rand.nextInt(65536),raw,null,true);}
  out.flush();try(DataOutputStream file=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[1])))) {
   file.writeInt(0x50433235);file.writeInt(cases);file.write(b.toByteArray());
   c=new client();PlayerEntity local=new PlayerEntity(),remote=new PlayerEntity(),inactive=new PlayerEntity();NpcEntity npc=new NpcEntity();
   c.players[2047]=local;c.players[3]=remote;c.players[4]=inactive;c.playerCount=1;c.playerIds[0]=3;c.npcs[2]=npc;c.npcCount=1;c.npcIds[0]=2;
   local.chat=remote.chat=inactive.chat=npc.chat="text";local.chatTimer=150;remote.chatTimer=1;inactive.chatTimer=19;npc.chatTimer=100;
   file.writeInt(155);for(int tick=0;tick<155;tick++){c.updateEntityChats((byte)94);timer(file,local);timer(file,remote);timer(file,inactive);timer(file,npc);}
  }
  System.out.println("Original public/forced masks="+cases+" history-changing steps="+accepted+";155 chat ticks/four actors; unchanged getPlayerExtended/updateEntityChats; real wordenc/English locale");
 }
}
