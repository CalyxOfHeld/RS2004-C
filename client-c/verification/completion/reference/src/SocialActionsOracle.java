import deob.client;
import jagex2.io.*;
import jagex2.datastruct.JString;
import jagex2.dash3d.entity.PlayerEntity;
import jagex2.config.Component;
import java.io.*;
import java.util.*;
public class SocialActionsOracle {
 static client c=new client();static DataOutputStream out;static int cases,wires,messages;static final int[] seed={225,118,79,171};
 static void text(String s)throws Exception{if(s==null){out.writeInt(-1);return;}out.writeInt(s.length());for(char ch:s.toCharArray())out.writeChar(ch);}
 static int[] keys(String s){int[] k=new int[s.length()+1];for(int i=0;i<s.length();i++)k[i]=s.charAt(i);k[s.length()]=13;return k;}
 static void setup(int friends,int ignores,int kind,int self) {
  c.localPlayer=new PlayerEntity();c.localPlayer.name=self==0?"Self Test":self==1?"SELF TEST":null;c.friendCount=friends;c.ignoreCount=ignores;
  for(int i=0;i<100;i++){c.friendName[i]="Friend "+i;c.friendName37[i]=JString.toBase37(c.friendName[i]);c.friendWorld[i]=i%3==0?0:10+i;c.ignoreName37[i]=JString.toBase37("Ignore "+i);}
  if(kind==9){c.friendName37[0]=c.ignoreName37[0]=JString.toBase37("Both");c.friendName[0]="Both";}
  Arrays.fill(c.messageText,null);Arrays.fill(c.messageSender,null);Arrays.fill(c.messageType,0);
  for(int i=0;i<3;i++){c.messageText[i]="old text "+i;c.messageSender[i]="old sender "+i;c.messageType[i]=i;}
  c.showSocialInput=false;c.socialAction=3;c.socialInput="old input";c.socialMessage="old header";c.socialName37=77;c.chatbackInputOpen=true;c.chatbackInput="42";c.chatTyped="ordinary kept";c.chatInterfaceId=27;c.out.pos=0;
 }
 static long target(int kind,int friends,int ignores) {
  switch(kind){case 0:return 0;case 1:return JString.toBase37("New Name");case 2:return JString.toBase37("Self Test");case 3:return c.friendName37[0];case 4:return c.friendName37[Math.max(0,friends/2)];case 5:return c.friendName37[Math.max(0,friends-1)];case 6:return c.ignoreName37[0];case 7:return c.ignoreName37[Math.max(0,ignores/2)];case 8:return c.ignoreName37[Math.max(0,ignores-1)];default:return JString.toBase37("Both");}
 }
 static void one(int friends,int ignores,int kind,int self,int action,int form,int[] supplied)throws Exception {
  setup(friends,ignores,kind,self);long id=target(kind,friends,ignores);int[] input=supplied!=null?supplied:keys(id==0?"":JString.fromBase37(id,false));
  out.writeInt(friends);out.writeInt(ignores);out.writeInt(kind);out.writeInt(self);out.writeInt(action);out.writeInt(form);out.writeLong(id);out.writeInt(form==2?input.length:0);if(form==2)for(int key:input)out.writeInt(key);
  if(form==0){if(action==1)c.addFriend(id,-460);if(action==2)c.removeFriend(43808,id);if(action==4)c.addIgnore(id,(byte)3);if(action==5)c.removeIgnore(1,id);}
  if(form==1){c.menuAction[0]=(action==1?406:action==2?557:action==4?436:556)+(cases%2==0?2000:0);c.menuOption[0]="Action @whi@  "+(id==0?"":JString.fromBase37(id,false))+"  ";c.useMenuOption(6412,0);}
  if(form==2){Component button=new Component();button.clientCode=action==1?201:action==2?202:action==4?501:502;c.handleInterfaceAction(false,button);for(int key:input){c.keyQueue[0]=key;c.keyQueueReadPos=0;c.keyQueueWritePos=1;c.handleInputKey(7);}}
  out.writeInt(c.out.pos);out.write(c.out.data,0,c.out.pos);if(c.out.pos>0)wires++;
  out.writeInt(c.friendCount);out.writeInt(c.ignoreCount);
  for(int i=0;i<100;i++){out.writeLong(c.friendName37[i]);text(c.friendName[i]);out.writeInt(c.friendWorld[i]);out.writeLong(c.ignoreName37[i]);}
  out.writeBoolean(c.showSocialInput);out.writeInt(c.socialAction);text(c.socialInput);text(c.socialMessage);out.writeLong(c.socialName37);out.writeBoolean(c.chatbackInputOpen);text(c.chatbackInput);text(c.chatTyped);
  Isaac random=c.out.random;out.writeInt(random.count);out.writeInt(random.a);out.writeInt(random.b);out.writeInt(random.c);for(int v:random.rsl)out.writeInt(v);for(int v:random.mem)out.writeInt(v);
  if(c.messageText[0]!=null && !c.messageText[0].startsWith("old text"))messages++;
  for(int i=0;i<100;i++){out.writeInt(c.messageType[i]);text(c.messageSender[i]);text(c.messageText[i]);}cases++;
 }
 public static void main(String[] args)throws Exception {
  Locale.setDefault(Locale.ENGLISH);c.out=new Packet(5,new byte[5000]);c.out.random=new Isaac((byte)1,seed);ByteArrayOutputStream b=new ByteArrayOutputStream();out=new DataOutputStream(new BufferedOutputStream(b));
  for(int count=0;count<=100;count++)for(int action:new int[]{1,2,4,5})for(int kind:new int[]{0,1,4,7})one(count,100-count,kind,count%3,action,(count+kind+action)%3,null);
  for(int f:new int[]{0,1,99,100})for(int i:new int[]{0,1,99,100})for(int action:new int[]{1,2,4,5})for(int kind:new int[]{2,3,5,6,8,9})one(f,i,kind,(f+i)%3,action,kind%3,null);
  for(int key=0;key<256;key++)for(int action:new int[]{1,2,4,5})one(3,3,1,0,action,2,key==10||key==13?new int[]{key}:new int[]{key,13});
  for(int action:new int[]{1,2,4,5})for(String name:new String[]{""," ","SELF_TEST","self test","abcdefghijklmnop","  Friend 1  ","Ignore_1","!@#$%^&*()","New_Name99","::literal"})one(3,3,1,0,action,2,keys(name));
  Random random=new Random(225118);String alphabet="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 _!@#$%^&*()-+=?./";
  for(int i=0;i<200;i++){char[] s=new char[random.nextInt(121)];for(int j=0;j<s.length;j++)s[j]=alphabet.charAt(random.nextInt(alphabet.length()));one(i%101,(100-i%101),1,i%3,new int[]{1,2,4,5}[i%4],2,keys(new String(s)));}
  one(3,3,1,0,1,2,new int[]{'A','B',8,'C',10});one(3,3,1,0,4,2,new int[]{'X',8,8,13});
  out.flush();try(DataOutputStream file=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[0])))){file.writeInt(0x534f3235);file.writeInt(cases);for(int v:seed)file.writeInt(v);file.write(b.toByteArray());}
  System.out.println("Original social actions:cases="+cases+" wireCases="+wires+" errorMessages="+messages+"; actual direct methods/menu/prompt/key branches,all100 list tails/history/full cipher");
 }
}
