import deob.client;
import sign.signlink;
import jagex2.client.*;
import jagex2.io.*;
import java.io.*;
import java.net.*;
import java.awt.*;
import java.awt.event.*;
import java.util.*;
public class InputTrackingOracle {
 static client c=new client();static Canvas canvas=new Canvas();static DataOutputStream out;static int cases,packets,flushes;static int[] types=new int[14];static long clock=100000;static int[] seed={225,81,133,226};
 static void guard(Class<?> type)throws Exception{try(InputStream in=type.getResourceAsStream(type.getSimpleName()+".class")){ByteArrayOutputStream bytes=new ByteArrayOutputStream();byte[] b=new byte[4096];int n;while((n=in.read(b))!=-1)bytes.write(b,0,n);if(!new String(bytes.toByteArray(),"ISO-8859-1").contains("TrackingClock"))throw new Exception("Use verifier instrumented source classes: "+type);}}
 static void packet(int opcode)throws Exception{final ByteArrayInputStream input=new ByteArrayInputStream(new byte[]{(byte)opcode});Socket socket=new Socket(){public void setSoTimeout(int n){}public void setTcpNoDelay(boolean value){}public InputStream getInputStream(){return input;}public OutputStream getOutputStream(){return new ByteArrayOutputStream();}};c.stream=new ClientStream(c,(byte)2,socket);c.packetType=-1;c.randomIn=null;c.ingame=true;if(!c.read(false)||!c.ingame||input.available()!=0||c.packetType!=-1)throw new Exception("read control "+opcode);packets++;}
 static void buffer(Packet p)throws Exception{out.writeInt(p==null?-1:p.pos);if(p!=null)out.write(p.data,0,p.pos);}
 static void snapshot()throws Exception{out.writeBoolean(InputTracking.enabled);out.writeInt(InputTracking.trackedCount);out.writeLong(InputTracking.lastTime);out.writeLong(InputTracking.lastMoveTime);out.writeInt(InputTracking.lastX);out.writeInt(InputTracking.lastY);buffer(InputTracking.outBuffer);buffer(InputTracking.oldBuffer);out.writeInt(TrackingClock.calls);out.writeInt(c.out.pos);out.write(c.out.data,0,c.out.pos);Isaac cipher=c.out.random;for(int v:new int[]{cipher.count,cipher.a,cipher.b,cipher.c})out.writeInt(v);for(int v:cipher.rsl)out.writeInt(v);for(int v:cipher.mem)out.writeInt(v);}
 static void one(int op,int x,int y,int code,long time)throws Exception{
  out.writeInt(op);out.writeInt(x);out.writeInt(y);out.writeInt(code);out.writeLong(time);TrackingClock.time=time;clock=time;c.out.pos=0;Packet before=InputTracking.outBuffer;int beforePos=before==null?0:before.pos,beforeCount=InputTracking.trackedCount;
  if(op==16){InputTracking.setDisabled((byte)65);InputTracking.lastTime=InputTracking.lastMoveTime=0;InputTracking.trackedCount=InputTracking.lastX=InputTracking.lastY=0;TrackingClock.calls=0;c.out.random=new Isaac((byte)1,seed);}
  else if(op==226 || op==133)packet(op);
  else if(op==0)InputTracking.setDisabled((byte)65);
  else if(op==14){c.ingame=true;c.idleNetCycles=0;c.waveCount=0;c.nextMusicDelay=0;c.stream=null;boolean stopped=false;try{c.updateGame(true);}catch(TrackingClock.PrefixDone done){stopped=true;}if(!stopped)throw new Exception("prefix boundary missing");flushes++;}
  else if(op==17){if(code==0)InputTracking.keyPressed(x,true);else InputTracking.keyReleased(x,1);}
  else if(op==18)InputTracking.trackedCount=x;
  else if(op==1 || op==3 || op==5 || op==6 || op==12 || op==13){int type=op==1?MouseEvent.MOUSE_PRESSED:op==3?MouseEvent.MOUSE_RELEASED:op==6?MouseEvent.MOUSE_DRAGGED:op==12?MouseEvent.MOUSE_ENTERED:op==13?MouseEvent.MOUSE_EXITED:MouseEvent.MOUSE_MOVED;int modifiers=code==1?InputEvent.META_DOWN_MASK:0;MouseEvent e=new MouseEvent(canvas,type,time,modifiers,x,y,1,false,code==1?MouseEvent.BUTTON3:MouseEvent.BUTTON1);if(op==1)c.mousePressed(e);if(op==3)c.mouseReleased(e);if(op==5)c.mouseMoved(e);if(op==6)c.mouseDragged(e);if(op==12)c.mouseEntered(e);if(op==13)c.mouseExited(e);}
  else if(op==8 || op==9){KeyEvent e=new KeyEvent(canvas,op==8?KeyEvent.KEY_PRESSED:KeyEvent.KEY_RELEASED,time,code,x,(char)y);if(op==8)c.keyPressed(e);else c.keyReleased(e);}
  else if(op==10)c.focusGained(new FocusEvent(canvas,FocusEvent.FOCUS_GAINED));
  else if(op==11)c.focusLost(new FocusEvent(canvas,FocusEvent.FOCUS_LOST));
  else throw new Exception("input op "+op);
  if(((op>=1 && op<=13)||op==17) && InputTracking.outBuffer!=null && InputTracking.trackedCount==beforeCount+1){int at=before==InputTracking.outBuffer?beforePos:0;if(InputTracking.outBuffer.pos>at){int type=InputTracking.outBuffer.data[at]&255;if(type>=1 && type<=13)types[type]++;}}
  snapshot();cases++;
 }
 static void event(int op,int x,int y,int code,int delta)throws Exception{one(op,x,y,code,clock+delta);}
 static void ascii(int ch)throws Exception{
  int vk=KeyEvent.getExtendedKeyCodeForChar(ch),mods=0;if(ch>='a' && ch<='z')vk=KeyEvent.VK_A+ch-'a';if(ch>='A' && ch<='Z'){vk=KeyEvent.VK_A+ch-'A';mods=InputEvent.SHIFT_DOWN_MASK;}
  int[][] shifted={{'!',KeyEvent.VK_1},{'@',KeyEvent.VK_2},{'#',KeyEvent.VK_3},{'$',KeyEvent.VK_4},{'%',KeyEvent.VK_5},{'^',KeyEvent.VK_6},{'&',KeyEvent.VK_7},{'*',KeyEvent.VK_8},{'(',KeyEvent.VK_9},{')',KeyEvent.VK_0},{'_',KeyEvent.VK_MINUS},{'+',KeyEvent.VK_EQUALS},{'{',KeyEvent.VK_OPEN_BRACKET},{'}',KeyEvent.VK_CLOSE_BRACKET},{'|',KeyEvent.VK_BACK_SLASH},{':',KeyEvent.VK_SEMICOLON},{'"',KeyEvent.VK_QUOTE},{'<',KeyEvent.VK_COMMA},{'>',KeyEvent.VK_PERIOD},{'?',KeyEvent.VK_SLASH},{'~',KeyEvent.VK_BACK_QUOTE}};
  for(int[] row:shifted)if(ch==row[0]){vk=row[1];mods=InputEvent.SHIFT_DOWN_MASK;}
  event(8,vk,ch,mods,10);event(9,vk,ch,mods,10);
 }
 public static void main(String[] args)throws Exception{
  guard(client.class);guard(InputTracking.class);signlink.active=false;signlink.reporterror=false;c.out=new Packet(5,new byte[5000]);ByteArrayOutputStream body=new ByteArrayOutputStream();out=new DataOutputStream(new BufferedOutputStream(body));
  one(16,0,0,0,100000);for(int op:new int[]{1,3,5,8,9,10,11,12,13})event(op,op==8||op==9?65:4,op==8||op==9?65:4,0,10);event(14,0,0,0,0);event(133,0,0,0,0);event(226,0,0,0,0);
  for(int[] xy:new int[][]{{-1,0},{0,-1},{789,0},{0,532},{788,531},{0,0}})for(int op:new int[]{1,5})event(op,xy[0],xy[1],op%2,50);
  for(int[] xy:new int[][]{{7,7},{15,15},{7,7},{0,0},{127,127},{255,255},{127,127},{0,0},{788,531},{0,0}}){event(5,xy[0],xy[1],0,50);event(6,xy[0],xy[1],0,49);event(5,xy[0],xy[1],0,1);}
  for(int delta:new int[]{0,1,9,10,49,50,51,2499,2500,2510,100000,-1,-9,-10,-2501})event(3,0,0,delta%2,delta);
  for(int vk:new int[]{37,39,38,40,17,8,127,9,10,27,16,18,112,113,123,36,35,33,34,0})for(int ch:new int[]{0,29,30,31,65,127,255,65535}){event(8,vk,ch,0,10);event(9,vk,ch,0,10);}
  for(int ch=32;ch<127;ch++)ascii(ch);for(int raw:new int[]{Integer.MIN_VALUE,-1,0,255,256,999,1000,1001,1002,1003,1007,1008,1019,65535,Integer.MAX_VALUE}){event(17,raw,0,0,10);event(17,raw,0,1,10);}
  for(int op:new int[]{10,11,12,13})for(int i=0;i<4;i++)event(op,0,0,0,10);event(14,0,0,0,0);event(133,0,0,0,0);event(226,0,0,0,0);event(226,0,0,0,1);
  for(int i=0;i<170;i++)event(8,65,65,0,10);event(14,0,0,0,0);event(133,0,0,0,0);event(226,0,0,0,0);for(int i=0;i<520;i++){if(i==0)event(8,65,65,0,10);else event(3,0,0,i%2,10);if(i==249)event(14,0,0,0,0);}event(133,0,0,0,0);event(14,0,0,0,0);
  one(16,0,0,0,Long.MAX_VALUE-100);event(226,0,0,0,0);one(8,65,65,0,Long.MAX_VALUE);one(8,65,65,0,Long.MIN_VALUE+10);event(18,Integer.MAX_VALUE,0,0,0);event(11,0,0,0,10);event(0,0,0,0,0);event(226,0,0,0,0);event(5,2,3,0,10);event(133,0,0,0,0);
  one(16,0,0,0,100000);event(226,0,0,0,0);Random random=new Random(22581133);for(int i=0;i<4000;i++){int choice=random.nextInt(25),op=choice<8?5:choice<12?1:choice<16?3:choice<20?17:choice==20?14:choice==21?133:choice==22?226:choice==23?0:10+random.nextInt(4);int x=op==17?random.nextInt():random.nextInt(850)-30,y=random.nextInt(580)-20,code=random.nextInt(2);event(op,x,y,code,random.nextInt(2600)-50);}event(133,0,0,0,0);
  for(int i=1;i<=13;i++)if(types[i]==0)throw new Exception("missing event encoding "+i);System.out.println("Original event encoding counts="+Arrays.toString(types));
  out.flush();try(DataOutputStream f=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[0])))){f.writeInt(0x49543235);f.writeInt(cases);for(int v:seed)f.writeInt(v);f.write(body.toByteArray());}System.out.println("Original input tracking:"+cases+" states,"+packets+" read133/226 packets,"+flushes+" actual updateGame prefixes;GameShell callbacks/all13codes,full buffers/time/count/positions/clock calls/wire/ISAAC;clock andprefix observer boundaries only");
 }
}
