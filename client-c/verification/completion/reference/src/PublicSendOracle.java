import deob.client;
import jagex2.io.*;
import jagex2.wordenc.*;
import jagex2.dash3d.entity.PlayerEntity;
import java.io.*;
import java.nio.file.*;
import java.util.*;
// Actual unchanged handleInputKey consumes queued GameShell key codes.
public class PublicSendOracle {
 static client c=new client();static DataOutputStream out;static int cases,wireCases;static final int[] seed={225,158,244,80};
 static void text(String s)throws Exception{if(s==null){out.writeInt(-1);return;}out.writeInt(s.length());for(char ch:s.toCharArray())out.writeChar(ch);}
 static int[] keys(String s){int[] k=new int[s.length()+1];for(int i=0;i<s.length();i++)k[i]=s.charAt(i);k[s.length()]=13;return k;}
 static void one(int[] keys,int pub,int priv,int trade,boolean blocked)throws Exception {
  c.localPlayer=new PlayerEntity();c.localPlayer.name="Alice";c.localPlayer.chat="old";c.localPlayer.chatColor=13;c.localPlayer.chatStyle=17;c.localPlayer.chatTimer=7;
  c.chatTyped="";c.publicChatSetting=pub;c.privateChatSetting=priv;c.tradeChatSetting=trade;c.chatInterfaceId=blocked?27:-1;c.out.pos=0;
  for(int key:keys){c.keyQueue[0]=key;c.keyQueueReadPos=0;c.keyQueueWritePos=1;c.handleInputKey(7);}
  out.writeInt(pub);out.writeInt(priv);out.writeInt(trade);out.writeBoolean(blocked);out.writeInt(keys.length);for(int k:keys)out.writeInt(k);
  out.writeInt(c.out.pos);out.write(c.out.data,0,c.out.pos);if(c.out.pos>0)wireCases++;
  text(c.chatTyped);out.writeInt(c.publicChatSetting);out.writeInt(c.privateChatSetting);out.writeInt(c.tradeChatSetting);
  text(c.localPlayer.chat);out.writeInt(c.localPlayer.chatColor);out.writeInt(c.localPlayer.chatStyle);out.writeInt(c.localPlayer.chatTimer);
  Isaac random=c.out.random;out.writeInt(random.count);out.writeInt(random.a);out.writeInt(random.b);out.writeInt(random.c);for(int v:random.rsl)out.writeInt(v);for(int v:random.mem)out.writeInt(v);
  for(int i=0;i<100;i++){out.writeInt(c.messageType[i]);text(c.messageSender[i]);text(c.messageText[i]);}cases++;
 }
 public static void main(String[] args)throws Exception {
  Locale.setDefault(Locale.ENGLISH);WordFilter.unpack(new Jagfile(Files.readAllBytes(Paths.get(args[0])),false));c.out=new Packet(5,new byte[5000]);c.out.random=new Isaac((byte)1,seed);
  ByteArrayOutputStream b=new ByteArrayOutputStream();out=new DataOutputStream(new BufferedOutputStream(b));
  String[] colours={"","yellow:","red:","green:","cyan:","purple:","white:","flash1:","flash2:","flash3:","glow1:","glow2:","glow3:"};String[] styles={"","wave:","scroll:","wave:scroll:"};
  for(String colour:colours)for(String style:styles)for(int pub=0;pub<4;pub++)for(int priv=0;priv<3;priv++)for(int trade=0;trade<3;trade++)one(keys(colour+style+"Hello WORLD! fuck cook x.com/path"),pub,priv,trade,false);
  for(String first:colours)for(String second:colours)one(keys(first+second+"Mixed_CASE? next! YES."),2,1,2,false);
  for(String colour:colours)for(String style:styles){one(keys(colour.toUpperCase(Locale.ENGLISH)+style.toUpperCase(Locale.ENGLISH)+"Hello"),2,0,0,false);one(keys(colour+style),2,2,2,false);}
  for(String s:new String[]{""," ","   ","_","@red@word","<not supported>","red:red:text","wave:red:text","scroll:wave:text","wave:wave:text","yellow:red:green:cyan:purple:white:flash1:flash2:glow1:text","AbC? DeF. GhI! JkL","red:wave:","normal"})for(int mode=0;mode<4;mode++){one(keys(s),mode,1,2,false);one(keys(s),mode,1,2,true);}
  for(int key=0;key<256;key++)one(new int[]{key,13},2,1,1,false);
  for(int n:new int[]{1,79,80,81,100,120}){char[] s=new char[n];Arrays.fill(s,'W');one(keys(new String(s)),2,2,2,false);}
  one(new int[]{'A','B',8,'C',10},2,0,2,false);one(new int[]{'x',13,'y',13,13},2,1,1,false);
  Random rand=new Random(225158);String alphabet="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .,!@#$%&*()-_/'\\|[]{}<>:";
  for(int i=0;i<1000;i++){char[] s=new char[1+rand.nextInt(120)];s[0]='A';for(int j=1;j<s.length;j++)s[j]=alphabet.charAt(rand.nextInt(alphabet.length()));one(keys(new String(s)),i%4,i%3,(i/3)%3,false);}
  out.flush();try(DataOutputStream file=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[1])))){file.writeInt(0x50533235);file.writeInt(cases);for(int v:seed)file.writeInt(v);file.write(b.toByteArray());}
  System.out.println("Original public-send key cases="+cases+" wire-producing cases="+wireCases+"; all ISAAC state/history/actor/input/modes captured; actual handleInputKey; real wordenc, English locale");
 }
}
