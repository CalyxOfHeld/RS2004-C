import deob.client;
import jagex2.io.*;
import jagex2.wordenc.*;
import jagex2.datastruct.JString;
import java.io.*;
import java.nio.file.*;
import java.util.*;
// Actual original menu action and queued key processing; persistent cipher/history.
public class PrivateSendOracle {
 static client c=new client();static DataOutputStream out;static int cases,wireCases;static final int[] seed={225,148,244,80};
 static void text(String s)throws Exception{if(s==null){out.writeInt(-1);return;}out.writeInt(s.length());for(char ch:s.toCharArray())out.writeChar(ch);}
 static int[] keys(String s){int[] k=new int[s.length()+1];for(int i=0;i<s.length();i++)k[i]=s.charAt(i);k[s.length()]=13;return k;}
 static void state()throws Exception {out.writeBoolean(c.showSocialInput);text(c.socialInput);text(c.socialMessage);out.writeLong(c.socialName37);out.writeBoolean(c.chatbackInputOpen);text(c.chatbackInput);text(c.chatTyped);}
 static void one(int[] keys,int pub,int priv,int trade,int who,boolean blocked)throws Exception {
  String[] names={"Alice","Bob","Self Test","Missing"};
  c.friendCount=3;for(int i=0;i<3;i++){c.friendName[i]=names[i];c.friendName37[i]=JString.toBase37(names[i]);c.friendWorld[i]=i==1?0:10+i;}
  c.showSocialInput=false;c.socialInput="old prompt";c.socialMessage="old header";c.socialName37=77;c.chatbackInputOpen=true;c.chatbackInput="42";c.chatTyped="ordinary kept";
  c.publicChatSetting=pub;c.privateChatSetting=priv;c.tradeChatSetting=trade;c.chatInterfaceId=blocked?27:-1;c.out.pos=0;
  c.menuAction[0]=679;c.menuOption[0]="Message @whi@"+names[who];c.useMenuOption(6412,0);
  out.writeInt(pub);out.writeInt(priv);out.writeInt(trade);out.writeInt(who);out.writeBoolean(blocked);state();
  out.writeInt(keys.length);for(int key:keys){out.writeInt(key);c.keyQueue[0]=key;c.keyQueueReadPos=0;c.keyQueueWritePos=1;c.handleInputKey(7);}
  out.writeInt(c.out.pos);out.write(c.out.data,0,c.out.pos);if(c.out.pos>0)wireCases++;
  state();out.writeInt(c.publicChatSetting);out.writeInt(c.privateChatSetting);out.writeInt(c.tradeChatSetting);
  Isaac random=c.out.random;out.writeInt(random.count);out.writeInt(random.a);out.writeInt(random.b);out.writeInt(random.c);for(int v:random.rsl)out.writeInt(v);for(int v:random.mem)out.writeInt(v);
  for(int i=0;i<100;i++){out.writeInt(c.messageType[i]);text(c.messageSender[i]);text(c.messageText[i]);}cases++;
 }
 public static void main(String[] args)throws Exception {
  Locale.setDefault(Locale.ENGLISH);WordFilter.unpack(new Jagfile(Files.readAllBytes(Paths.get(args[0])),false));c.out=new Packet(5,new byte[5000]);c.out.random=new Isaac((byte)1,seed);
  ByteArrayOutputStream bytes=new ByteArrayOutputStream();out=new DataOutputStream(new BufferedOutputStream(bytes));
  String[] examples={""," ","   ","_","@red@word","red:wave:Hello WORLD!","AbC? DeF. GhI! JkL","fuck cook x.com/path","::command","Hello WORLD!"};
  for(String text:examples)for(int pub=0;pub<4;pub++)for(int priv=0;priv<3;priv++)for(int trade=0;trade<3;trade++)for(int who:new int[]{0,2})one(keys(text),pub,priv,trade,who,(cases&1)!=0);
  for(int key=0;key<256;key++)one(key==10||key==13?new int[]{key}:new int[]{key,13},2,2,1,0,true);
  for(int n:new int[]{0,1,79,80,81,100,120}){char[] chars=new char[n];Arrays.fill(chars,'W');one(keys(new String(chars)),3,2,2,2,false);}
  one(new int[]{'A','B',8,'C',10},2,0,2,0,true);one(new int[]{'A',8,8,13},0,2,0,2,false);
  for(int who:new int[]{1,3})one(new int[0],0,2,0,who,false);
  Random rand=new Random(225148);String alphabet="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .,!@#$%&*()-_/'\\|[]{}<>:";
  for(int i=0;i<1000;i++){char[] s=new char[1+rand.nextInt(120)];for(int j=0;j<s.length;j++)s[j]=alphabet.charAt(rand.nextInt(alphabet.length()));one(keys(new String(s)),i%4,i%3,(i/3)%3,i%2==0?0:2,i%2==0);}
  out.flush();try(DataOutputStream f=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[1])))){f.writeInt(0x50563235);f.writeInt(cases);for(int v:seed)f.writeInt(v);f.write(bytes.toByteArray());}
  System.out.println("Original private send cases="+cases+" wire-producing cases="+wireCases+"; actual useMenuOption679/handleInputKey,full prompt/ordinary/count input,cipher/history/privacy; real wordenc English locale");
 }
}
