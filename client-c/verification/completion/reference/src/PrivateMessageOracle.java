import deob.client;
import jagex2.io.*;
import jagex2.wordenc.*;
import jagex2.datastruct.JString;
import java.io.*;
import java.net.*;
import java.nio.file.*;
import java.util.*;

// Actual unchanged client.read dispatch through the original ClientStream.
public class PrivateMessageOracle {
 static client c;static DataOutputStream out;static int cases,accepted,failed;static boolean reset;
 static void fresh(){c=new client();reset=true;}
 static byte[] packed(String s){Packet p=new Packet(5,new byte[256]);WordPack.pack(p,true,s);return Arrays.copyOf(p.data,p.pos);}
 static byte[] pm(long sender,int id,int rights,byte[] text)throws Exception{
  ByteArrayOutputStream b=new ByteArrayOutputStream();DataOutputStream d=new DataOutputStream(b);d.writeLong(sender);d.writeInt(id);d.writeByte(rights);d.write(text);return b.toByteArray();
 }
 static byte[] ignores(long... ids)throws Exception{ByteArrayOutputStream b=new ByteArrayOutputStream();DataOutputStream d=new DataOutputStream(b);for(long id:ids)d.writeLong(id);return b.toByteArray();}
 static void text(String s)throws Exception{if(s==null){out.writeInt(-1);return;}out.writeInt(s.length());for(char ch:s.toCharArray())out.writeChar(ch);}
 static void step(int opcode,byte[] payload,int override,boolean success)throws Exception{
  if(payload.length>255 && opcode==41)throw new Exception("size");
  ByteArrayOutputStream b=new ByteArrayOutputStream();DataOutputStream d=new DataOutputStream(b);d.writeByte(opcode);if(opcode==21)d.writeShort(payload.length);else d.writeByte(payload.length);d.write(payload);
  final ByteArrayInputStream input=new ByteArrayInputStream(b.toByteArray());
  Socket sock=new Socket(){public void setSoTimeout(int n){}public void setTcpNoDelay(boolean n){}public InputStream getInputStream(){return input;}public OutputStream getOutputStream(){return new ByteArrayOutputStream();}};
  c.stream=new ClientStream(c,(byte)2,sock);c.randomIn=null;c.packetType=-1;c.overrideChat=override;
  String previous=c.messageText[0];if(!c.read(false) || input.available()!=0 || c.packetType!=-1)throw new Exception("original handler did not consume frame");
  if(c.messageText[0]!=previous)accepted++;if(!success)failed++;
  out.writeBoolean(reset);reset=false;out.writeByte(opcode);out.writeInt(override);out.writeBoolean(success);out.writeInt(payload.length);out.write(payload);
  out.writeInt(c.ignoreCount);for(long id:c.ignoreName37)out.writeLong(id);
  out.writeInt(c.privateMessageCount);for(int id:c.messageIds)out.writeInt(id);
  for(int i=0;i<100;i++){out.writeInt(c.messageType[i]);text(c.messageSender[i]);text(c.messageText[i]);}
  cases++;
 }
 static void msg(long sender,int id,int rights,String s,int override)throws Exception{step(41,pm(sender,id,rights,packed(s)),override,true);}
 public static void main(String[] args)throws Exception{
  Locale.setDefault(Locale.ENGLISH);WordFilter.unpack(new Jagfile(Files.readAllBytes(Paths.get(args[0])),false));
  ByteArrayOutputStream bytes=new ByteArrayOutputStream();out=new DataOutputStream(new BufferedOutputStream(bytes));fresh();long alice=JString.toBase37("alice"),bob=JString.toBase37("bob_test");
  msg(alice,0,0,"zero ID initially duplicate",0);msg(alice,1,0,"Hello WORLD!",0);msg(alice,1,255,"duplicate high rights",0);
  step(21,ignores(alice),0,true);
  for(int rights:new int[]{0,1,2,3,127,128,255}){msg(alice,10+rights,rights,"cook fuck x.com/path 127.0.0.1",0);msg(alice,1000+rights,rights,"override",1);msg(bob,2000+rights,rights,"unignored",0);}
  step(21,ignores(),0,true);msg(alice,-1,0,"ignore cleared",0);
  long[] names={0,1,37,Long.MIN_VALUE,Long.MAX_VALUE,6582952005840035280L,6582952005840035281L,bob,JString.toBase37("a_b_c_1234567")};
  for(long name:names)for(int rights:new int[]{0,1,2,255})msg(name,3000+cases,rights,"Mixed CASE. pounds \u00a3!",0);
  long[] all=new long[100];for(int i=0;i<100;i++)all[i]=i+1;step(21,ignores(all),0,true);step(21,ignores(bob,alice),0,true);step(21,ignores(),0,true);
  fresh();for(int i=1;i<=205;i++)msg(alice,i,i%4,"ring "+i,0);msg(bob,205,2,"duplicate",0);msg(bob,1,0,"evicted ID",0);msg(bob,0,0,"zero after wrap",0);
  fresh();for(int i=0;i<256;i++){msg(alice,1+i,i,WordFilter.ALLOWLIST[i%5]+" example.com/path 255.0.1.2",0);}
  // Every real bad word plus domain and TLD patterns runs through packet decoding/filtering.
  for(char[][] group:new char[][][]{WordFilter.badWords,WordFilter.domains,WordFilter.tlds})for(char[] word:group)msg(bob,10000+cases,cases%4,"name@"+new String(word)+".com/path",0);
  fresh();for(int n=0;n<=50;n++){byte[] raw=new byte[n];step(41,pm(alice,1+n,0,raw),0,true);}
  // Above original100-unit scratch capacity: original catches after consuming ring ID.
  step(41,pm(alice,900,0,new byte[51]),0,false);step(41,pm(alice,900,2,packed("duplicate failed ID")),0,true);
  // Suppression happens before unpack, even when the payload would overflow.
  step(21,ignores(alice),0,true);step(41,pm(alice,901,1,new byte[242]),0,true);step(41,pm(alice,902,2,new byte[242]),1,true);
  step(21,ignores(),0,true);byte[] unmatched={(byte)0xf};step(41,pm(alice,903,2,unmatched),0,true);
  Random random=new Random(22541);for(int i=0;i<1000;i++){byte[] raw=new byte[random.nextInt(51)];random.nextBytes(raw);step(41,pm(i%5==0?bob:alice,100000+i,i%256,raw),i%11==0?1:0,true);}
  out.flush();try(DataOutputStream file=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[1])))){file.writeInt(0x504d3235);file.writeInt(cases);file.write(bytes.toByteArray());}
  System.out.println("Original incoming private packets="+cases+" history additions="+accepted+" expected decode failures="+failed+"; actual client.read/ClientStream, English locale, real wordenc");
 }
}

