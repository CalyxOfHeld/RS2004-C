import deob.client;
import jagex2.io.*;
import jagex2.graphics.*;
import jagex2.config.Component;
import java.io.*;
import java.nio.file.*;
import java.util.*;
public class ReportAbuseOracle {
 static client c=new client();static DataOutputStream out;static int cases,reports;static final int[] seed={225,190,231,12};static int root,editor,mute;static int[] category=new int[12];
 static void text(DataOutputStream f,String s)throws Exception{if(s==null){f.writeInt(-1);return;}f.writeInt(s.length());for(char ch:s.toCharArray())f.writeChar(ch);}
 static int[] keys(String s){int[] k=new int[s.length()];for(int i=0;i<k.length;i++)k[i]=s.charAt(i);return k;}
 static void button(int id){c.menuAction[0]=951;c.menuParamC[0]=id;c.useMenuOption(6412,0);}
 static void one(int mode,String name,boolean rights,int toggles,int end,int[] keys,int x,int y)throws Exception {
  c.viewportInterfaceId=mode==2?root:123;c.sidebarInterfaceId=55;c.chatInterfaceId=27;c.reportAbuseInterfaceID=mode==2?root:77;c.reportAbuseInput=mode==2?name:"old Report";c.reportAbuseMuteOption=true;c.rights=rights;
  c.showSocialInput=true;c.socialAction=3;c.socialInput="old social";c.socialMessage="old header";c.socialName37=77;c.chatbackInputOpen=true;c.chatbackInput="42";c.chatTyped="ordinary kept";c.out.pos=0;
  out.writeInt(mode);text(out,name);out.writeBoolean(rights);out.writeInt(toggles);out.writeInt(end);out.writeInt(x);out.writeInt(y);out.writeInt(keys.length);for(int key:keys)out.writeInt(key);
  if(mode==0){c.mouseClickButton=1;c.mouseClickX=x;c.mouseClickY=y;c.handleChatSettingsInput(0);}
  if(mode==1){c.menuAction[0]=cases%2==0?2034:34;c.menuOption[0]=name.equals("<missing-tag>")?"Report abuse plain":"Report abuse @whi@  "+name+"  ";c.useMenuOption(6412,0);}
  for(int key:keys){c.keyQueue[0]=key;c.keyQueueReadPos=0;c.keyQueueWritePos=1;c.handleInputKey(7);}
  for(int i=0;i<toggles;i++)button(mute);
  if(end==13){c.menuAction[0]=947;c.useMenuOption(6412,0);}else if(end>0){button(category[end-1]);if(c.out.pos>2)reports++;}
  out.writeInt(c.out.pos);out.write(c.out.data,0,c.out.pos);
  out.writeInt(c.viewportInterfaceId);out.writeInt(c.chatInterfaceId);out.writeInt(c.sidebarInterfaceId);out.writeInt(c.reportAbuseInterfaceID);text(out,c.reportAbuseInput);out.writeBoolean(c.reportAbuseMuteOption);
  out.writeBoolean(c.showSocialInput);out.writeInt(c.socialAction);text(out,c.socialInput);text(out,c.socialMessage);out.writeLong(c.socialName37);out.writeBoolean(c.chatbackInputOpen);text(out,c.chatbackInput);text(out,c.chatTyped);
  Isaac random=c.out.random;out.writeInt(random.count);out.writeInt(random.a);out.writeInt(random.b);out.writeInt(random.c);for(int v:random.rsl)out.writeInt(v);for(int v:random.mem)out.writeInt(v);cases++;
 }
 public static void main(String[] args)throws Exception {
  Jagfile title=new Jagfile(Files.readAllBytes(Paths.get(args[0],"title")),false),media=new Jagfile(Files.readAllBytes(Paths.get(args[0],"media")),false),interfaces=new Jagfile(Files.readAllBytes(Paths.get(args[0],"interface")),false);
  PixFont[] fonts={new PixFont(title,"p11",530),new PixFont(title,"p12",530),new PixFont(title,"b12",530),new PixFont(title,"q8",530)};Component.unpack(media,fonts,1,interfaces);
  for(Component comp:Component.instances)if(comp!=null){int code=comp.clientCode;if(code==600){editor=comp.id;root=comp.layer;}if(code==613)mute=comp.id;if(code>=601 && code<=612)category[code-601]=comp.id;}
  System.out.println("Actual report interface root="+root+" editor="+editor+" mute="+mute+" dimensions="+Component.instances[root].width+"x"+Component.instances[root].height);
  c.out=new Packet(5,new byte[5000]);c.out.random=new Isaac((byte)1,seed);ByteArrayOutputStream bytes=new ByteArrayOutputStream();out=new DataOutputStream(new BufferedOutputStream(bytes));
  String[] names={"","Alice","Bob 123","            ","ABCDEFGHIJKLMNO","@whi@Name","Case_Sender","Self Test"};
  for(int mode=0;mode<3;mode++)for(boolean rights:new boolean[]{false,true})for(String name:names)for(int end=0;end<=12;end++)for(int toggles=0;toggles<3;toggles++)one(mode,name,rights,toggles,end,mode==0?keys(name):new int[]{'A',8,'Z'},466,505);
  for(int key=0;key<256;key++)one(0,"",false,0,1,new int[]{key},466,505);
  for(int x:new int[]{415,416,516,517})for(int y:new int[]{489,490,522,523})one(0,"",false,0,0,new int[0],x,y);
  one(1,"<missing-tag>",false,0,0,new int[0],0,0);one(2,"AB",true,1,12,new int[]{8,8,8,'X','_',13,27},0,0);
  for(int mode=0;mode<3;mode++)for(String name:new String[]{"","Alice","            "})for(boolean rights:new boolean[]{false,true})for(int toggles=0;toggles<2;toggles++)one(mode,name,rights,toggles,13,new int[0],466,505);
  out.flush();try(DataOutputStream f=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[1])))){f.writeInt(0x52413235);f.writeInt(cases);f.writeInt(root);for(int v:seed)f.writeInt(v);f.write(bytes.toByteArray());}
  System.out.println("Original report cases="+cases+" category wire cases="+reports+"; actual footer/menu/key/category/close order and full cipher/state");
  try(DataOutputStream f=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[2])))) {
   f.writeInt(0x52553235);f.writeInt(24);f.writeInt(root);
   int n=0;for(String name:new String[]{"","Alice","SELF TEST","            ","ABCDEFGHIJKL","@raw_long_sender_1234567890"})for(boolean rights:new boolean[]{false,true})for(boolean muted:new boolean[]{false,true}) {
    c.reportAbuseInput=name;c.rights=rights;c.reportAbuseMuteOption=muted;client.loopCycle=new int[]{0,9,10,19,20,Integer.MAX_VALUE}[n/4];f.writeInt(client.loopCycle);text(f,name);f.writeBoolean(rights);f.writeBoolean(muted);
    int[] pixels=new int[512*334];Arrays.fill(pixels,0x123456);Pix2D.bind(512,pixels,-657,334);c.drawInterface(0,0,38682,Component.instances[root],0);
    text(f,Component.instances[editor].text);text(f,Component.instances[mute].text);f.writeInt(Component.instances[mute].colour);for(int pixel:pixels)f.writeInt(pixel);n++;
   }
  }
  System.out.println("Original report UI:24 complete512x334 panels/4104192 pixels and editor/mute labels/colour; real interface/title/media");
 }
}
