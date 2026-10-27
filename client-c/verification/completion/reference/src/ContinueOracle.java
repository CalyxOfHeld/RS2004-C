import deob.client;
import jagex2.config.Component;
import jagex2.graphics.*;
import jagex2.io.*;
import java.io.*;
import java.net.Socket;
import java.nio.file.*;
import sign.signlink;

/** Calls the unchanged bundled revision225 classes. No original instrumentation. */
public class ContinueOracle {
 static final int[] seed={225,235,129,44};
 static client c=new client();static int count;
 static Component root=new Component(),child=new Component();
 static void read(int op,byte[] payload)throws Exception {
  byte[] framed=new byte[payload.length+1];framed[0]=(byte)op;System.arraycopy(payload,0,framed,1,payload.length);
  final ByteArrayInputStream input=new ByteArrayInputStream(framed);
  Socket socket=new Socket(){public void setSoTimeout(int n){}public void setTcpNoDelay(boolean b){}public InputStream getInputStream(){return input;}public OutputStream getOutputStream(){return new ByteArrayOutputStream();}};
  c.stream=new ClientStream(c,(byte)2,socket);c.packetType=-1;c.randomIn=null;c.ingame=true;
  if(!c.read(false)||input.available()!=0||c.packetType!=-1)throw new Exception("original read "+op);
 }
 static void click(int id){c.menuAction[0]=count%2==0?44:2044;c.menuParamC[0]=id;c.useMenuOption(6412,0);}
 static void cipher(DataOutputStream f)throws Exception{Isaac i=c.out.random;for(int v:new int[]{i.count,i.a,i.b,i.c})f.writeInt(v);for(int v:i.rsl)f.writeInt(v);for(int v:i.mem)f.writeInt(v);}
 static void one(DataOutputStream f,int op,boolean wait,int v,int s,int ch,int id,int clicks)throws Exception {
  f.writeInt(op);f.writeBoolean(wait);f.writeInt(v);f.writeInt(s);f.writeInt(ch);f.writeInt(id);f.writeInt(clicks);
  c.objSelected=c.spellSelected=1;c.pressedContinueOption=wait;c.viewportInterfaceId=v;c.sidebarInterfaceId=s;c.chatInterfaceId=ch;c.out.pos=0;c.out.random=new Isaac((byte)1,seed);
  if(op==0){for(int i=0;i<clicks;i++)click(i==0?id:(id+1)&65535);}
  else if(op==1){c.menuAction[0]=947;c.useMenuOption(6412,0);}
  else if(op==140)read(140,new byte[]{(byte)(id==0?0:192),0,2,1,0,1,(byte)201,0,1});
  else {byte[] payload=op==129?new byte[0]:op==28?new byte[]{0,0,0,0}:new byte[]{0,0};read(op,payload);}
  f.writeBoolean(c.pressedContinueOption);f.writeBoolean(c.objSelected!=0);f.writeBoolean(c.spellSelected!=0);f.writeInt(c.viewportInterfaceId);f.writeInt(c.sidebarInterfaceId);f.writeInt(c.chatInterfaceId);
  f.writeInt(c.out.pos);f.write(c.out.data,0,c.out.pos);cipher(f);
  Component saved=Component.instances[0];Component.instances[0]=child;c.menuSize=0;
  c.handleInterfaceInput(20,20,0,root,5082,0,0);
  f.writeInt(c.menuSize);if(c.menuSize>0){f.writeInt(c.menuAction[0]);f.writeInt(c.menuParamC[0]);}
  Component.instances[0]=saved;count++;
 }
 static Jagfile jag(String path,String name)throws Exception{return new Jagfile(Files.readAllBytes(Paths.get(path,name)),false);}
 public static void main(String[] args)throws Exception {
  Jagfile title=jag(args[0],"title"),media=jag(args[0],"media"),interfaces=jag(args[0],"interface");
  PixFont[] fonts={new PixFont(title,"p11",530),new PixFont(title,"p12",530),new PixFont(title,"b12",530),new PixFont(title,"q8",530)};
  Component.unpack(media,fonts,1,interfaces);signlink.active=false;signlink.reporterror=false;
  root.type=0;root.width=512;root.height=334;root.childId=new int[]{0};root.childX=new int[]{12};root.childY=new int[]{10};
  child.type=4;child.id=0;child.buttonType=6;child.width=180;child.height=30;child.overLayer=-1;child.text="Click here to continue";child.activeText="";child.option="Click here to continue";child.colour=0xffff00;child.font=fonts[1];
  c.out=new Packet(5,new byte[5000]);ByteArrayOutputStream bytes=new ByteArrayOutputStream();DataOutputStream f=new DataOutputStream(bytes);
  for(boolean wait:new boolean[]{false,true})for(int v:new int[]{-1,0,42})for(int s:new int[]{-1,0,55})for(int ch:new int[]{-1,0,27}){
   for(int id:new int[]{0,1,32767,65535})for(int clicks:new int[]{1,2,5})one(f,0,wait,v,s,ch,id,clicks);
   for(int op:new int[]{1,14,28,129,168,195,185})one(f,op,wait,v,s,ch,0,0);
   for(int id:new int[]{0,1})one(f,140,wait,v,s,ch,id,0);
  }
  try(DataOutputStream output=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[1])))){output.writeInt(0x43543235);output.writeInt(count);for(int v:seed)output.writeInt(v);output.write(bytes.toByteArray());}
  int pictures=48;try(DataOutputStream output=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[2])))){
   output.writeInt(0x43553235);output.writeInt(pictures);
   Component saved=Component.instances[0];Component.instances[0]=child;
   for(int i=0;i<pictures;i++){
    c.pressedContinueOption=(i&1)!=0;child.font=fonts[(i/2)%3];child.center=(i&8)!=0;child.shadowed=(i&16)!=0;child.buttonType=i<40?6:1;
    child.colour=0xffff00;child.activeColour=0xff0000;child.text="Click here to continue";child.activeText="Active continue";
    child.scriptComparator=new int[]{0};child.scriptOperand=new int[]{1};child.scripts=new int[][]{{11,0}};c.energy=(i&4)!=0?1:0;
    output.writeInt(i);int[] pixels=new int[512*334];Pix2D.bind(512,pixels,-657,334);c.drawInterface(0,0,38682,root,0);for(int pixel:pixels)output.writeInt(pixel);
   }
   Component.instances[0]=saved;
  }
  System.out.println("Original continue:"+count+" action/read/close states,full wire/ISAAC/menu;48 complete512x334 panels from original drawInterface/cache fonts. No instrumentation.");
 }
}
