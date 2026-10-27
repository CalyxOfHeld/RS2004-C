import deob.client;
import deob.MouseModeBoundary;
import jagex2.io.*;
import jagex2.graphics.*;
import jagex2.config.*;
import java.io.*;
import java.net.Socket;
import java.nio.file.*;
import java.util.*;
public class MouseModeOracle {
 static client c=new client();static final int[] seed={225,5,406,235};static int settings,inputs,releases,id,neutral;
 static Jagfile jag(String dir,String name)throws Exception{return new Jagfile(Files.readAllBytes(Paths.get(dir,name)),false);}
 static void wire(DataOutputStream f)throws Exception{f.writeInt(c.out.pos);f.write(c.out.data,0,c.out.pos);Isaac a=c.out.random;for(int v:new int[]{a.count,a.a,a.b,a.c})f.writeInt(v);for(int v:a.rsl)f.writeInt(v);for(int v:a.mem)f.writeInt(v);}
 static void reset(){c.out.pos=0;c.out.random=new Isaac((byte)1,seed);c.pressedContinueOption=false;c.menuVisible=false;c.objDragArea=0;c.spellSelected=0;}
 static void read(int op,byte[] payload)throws Exception{byte[] frame=new byte[payload.length+1];frame[0]=(byte)op;System.arraycopy(payload,0,frame,1,payload.length);final ByteArrayInputStream input=new ByteArrayInputStream(frame);Socket s=new Socket(){public void setSoTimeout(int n){}public void setTcpNoDelay(boolean b){}public InputStream getInputStream(){return input;}public OutputStream getOutputStream(){return new ByteArrayOutputStream();}};c.stream=new ClientStream(c,(byte)2,s);c.randomIn=null;c.packetType=-1;c.ingame=true;if(!c.read(false)||input.available()!=0)throw new Exception("read"+op);}
 static void setting(DataOutputStream f,int op,int target,int prior,int option,int value)throws Exception{
  reset();Arrays.fill(c.varps,0);Arrays.fill(c.varCache,0);c.varps[target]=prior;c.varCache[target]=77;c.mouseButtonsOption=option;
  for(int v:new int[]{op,target,prior,option,value})f.writeInt(v);
  if(op==0){c.varps[target]=value;c.updateVarp(target,49);}
  else if(op==6){Component b=Component.instances[value];c.menuAction[0]=b.buttonType==4?465:960;c.menuParamC[0]=value;c.useMenuOption(6412,0);}
  else if(op==4||op==5){Component b=Component.instances[0];b.scripts=new int[][]{{5,target,0}};b.scriptOperand=new int[]{value};c.menuAction[0]=op==4?465:960;c.menuParamC[0]=0;c.useMenuOption(6412,0);}
  else if(op==193){c.varCache[target]=value;read(193,new byte[0]);}
  else {ByteArrayOutputStream b=new ByteArrayOutputStream();DataOutputStream d=new DataOutputStream(b);d.writeShort(target);if(op==150)d.writeByte(value);else d.writeInt(value);read(op,b.toByteArray());}
  f.writeInt(c.varps[target]);f.writeInt(c.varCache[target]);f.writeInt(c.mouseButtonsOption);wire(f);settings++;
 }
 static void input(DataOutputStream f,int release,int option,int size,int action,int button,int x,int y,int spell,int drag)throws Exception{
  reset();c.mouseButtonsOption=option;c.menuSize=size;c.viewportInterfaceId=c.sidebarInterfaceId=c.chatInterfaceId=1;c.spellSelected=spell;c.objDragArea=drag;
  for(int i=0;i<size;i++){c.menuOption[i]=i==size-1?"Option @yel@Test":"Cancel";c.menuAction[i]=i==size-1?action:0;c.menuParamC[i]=0;}
  c.mouseClickButton=button;c.mouseClickX=x;c.mouseClickY=y;c.mouseX=x;c.mouseY=y;
  for(int v:new int[]{release,option,size,action,button,x,y,spell,drag})f.writeInt(v);
  if(release!=0){int before=MouseModeBoundary.calls;c.objDragArea=1;c.objGrabThreshold=false;c.objDragCycles=0;c.objGrabX=x;c.objGrabY=y;c.mouseButton=0;c.updateGame(true);if(MouseModeBoundary.calls!=before+1)throw new Exception("release boundary");releases++;}
  else c.handleMouseInput((byte)3);
  f.writeBoolean(c.menuVisible);if(c.menuVisible){f.writeInt(c.menuArea);f.writeInt(c.menuX);f.writeInt(c.menuY);f.writeInt(c.menuWidth);f.writeInt(c.menuHeight);}
  f.writeBoolean(c.pressedContinueOption);f.writeInt(c.objDragArea);wire(f);inputs++;
 }
 public static void main(String[] args)throws Exception{
  ByteArrayOutputStream classBytes=new ByteArrayOutputStream();InputStream classInput=client.class.getResourceAsStream("client.class");int next;while((next=classInput.read())!=-1)classBytes.write(next);byte[] clazz=classBytes.toByteArray();if(!new String(clazz,java.nio.charset.StandardCharsets.ISO_8859_1).contains("deob/MouseModeBoundary"))throw new Exception("Wrong client class:release slice not loaded");
  VarpType.unpack(jag(args[0],"config"),1);id=-1;neutral=-1;for(int i=0;i<VarpType.count;i++){if(VarpType.instances[i].clientcode==5 && id<0)id=i;if(VarpType.instances[i].clientcode==0 && neutral<0)neutral=i;}
  if(id<0||neutral<0)throw new Exception("cache varps");PixFont[] fonts={new PixFont(jag(args[0],"title"),"p11",530),new PixFont(jag(args[0],"title"),"p12",530),new PixFont(jag(args[0],"title"),"b12",530),new PixFont(jag(args[0],"title"),"q8",530)};Component.unpack(jag(args[0],"media"),fonts,1,jag(args[0],"interface"));Component.instances[0]=new Component();c.fontBold12=new PixFont(jag(args[0],"title"),"b12",530);c.out=new Packet(5,new byte[5000]);
  ByteArrayOutputStream body=new ByteArrayOutputStream();DataOutputStream f=new DataOutputStream(body);
  for(int op:new int[]{0,150,175,193,4,5})for(int target:new int[]{id,neutral})for(int prior:new int[]{Integer.MIN_VALUE,0,1,Integer.MAX_VALUE})for(int option:new int[]{-1,0,1,77})for(int value:new int[]{Integer.MIN_VALUE,-128,-1,0,1,2,127,128,Integer.MAX_VALUE})setting(f,op,target,prior,option,value);
  for(Component b:Component.instances)if(b!=null && (b.buttonType==4||b.buttonType==5) && b.scripts!=null && b.scripts[0][0]==5 && b.scripts[0][1]==id){System.out.println("Cached mouse button:"+b.id+" type="+b.buttonType+" root="+b.layer+" operand="+b.scriptOperand[0]);for(int prior:new int[]{0,1})for(int option:new int[]{0,1})setting(f,6,id,prior,option,b.id);}
  int[][] points={{100,100},{600,300},{100,400},{600,200}};
  for(int option:new int[]{Integer.MIN_VALUE,-1,0,1,2,Integer.MAX_VALUE})for(int size:new int[]{0,1,2,3,20})for(int action:new int[]{44,2044,406,2406,436})for(int button:new int[]{1,2})for(int[] p:points)for(int spell=0;spell<2;spell++)for(int drag=0;drag<2;drag++)input(f,0,option,size,action,button,p[0],p[1],spell,drag);
  for(int option:new int[]{-1,0,1,2})for(int size:new int[]{0,1,2,3,20})for(int action:new int[]{44,406,2406,436})for(int[] p:points)input(f,1,option,size,action,1,p[0],p[1],0,1);
  try(DataOutputStream output=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[1])))){output.writeInt(0x4d4d3235);output.writeInt(settings);output.writeInt(inputs);output.writeInt(releases);output.writeInt(id);output.writeInt(neutral);for(int v:seed)output.writeInt(v);output.write(body.toByteArray());}
  System.out.println("Original mouse mode:"+settings+" setting states,"+inputs+" input states including"+releases+" original drag-release slices;cache clientcode5 varp="+id+" neutral="+neutral+";full wire/ISAAC/menu rectangles. Other original methods unchanged.");
 }
}
