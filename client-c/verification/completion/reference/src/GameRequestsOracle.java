import deob.client;
import jagex2.io.ClientStream;
import jagex2.datastruct.JString;
import java.io.*;
import java.net.*;
import java.nio.charset.StandardCharsets;
import java.util.*;

// Unchanged original packet4 dispatch, including addMessage/modal side effects.
public class GameRequestsOracle {
 static client c=new client();static DataOutputStream out;static int cases,added,trades,duels;
 static String repeat(char ch,int n){char[] chars=new char[n];Arrays.fill(chars,ch);return new String(chars);}
 static void text(String s)throws Exception {if(s==null){out.writeInt(-1);return;}out.writeInt(s.length());for(char ch:s.toCharArray())out.writeChar(ch);}
 static void one(String message,int count,int match,int override,int sticky,int dialog)throws Exception {
  byte[] payload=(message+"\n").getBytes(StandardCharsets.US_ASCII);if(payload.length>255)throw new Exception("length");
  c.ignoreCount=count;for(int i=0;i<100;i++)c.ignoreName37[i]=100000+i;if(match>=0)c.ignoreName37[match]=JString.toBase37(message.substring(0,Math.max(0,message.indexOf(':'))));
  c.overrideChat=override;c.stickyChatInterfaceId=sticky;c.chatInterfaceId=dialog;c.modalMessage="modal kept";c.mouseClickButton=1;c.redrawChatback=false;
  out.writeInt(count);for(long id:c.ignoreName37)out.writeLong(id);out.writeInt(override);out.writeInt(sticky);out.writeInt(dialog);out.writeInt(payload.length);out.write(payload);
  byte[] frame=new byte[payload.length+2];frame[0]=4;frame[1]=(byte)payload.length;System.arraycopy(payload,0,frame,2,payload.length);
  final ByteArrayInputStream input=new ByteArrayInputStream(frame);
  Socket socket=new Socket(){public void setSoTimeout(int n){}public void setTcpNoDelay(boolean b){}public InputStream getInputStream(){return input;}public OutputStream getOutputStream(){return new ByteArrayOutputStream();}};
  c.stream=new ClientStream(c,(byte)2,socket);c.randomIn=null;c.packetType=-1;
  // Observe the discarded FIFO tail: constant request text can reuse top identity.
  String tail=c.messageText[99],marker=new String("capture tail observer");c.messageText[99]=marker;
  if(!c.read(false) || input.available()!=0 || c.packetType!=-1)throw new Exception("frame not consumed");
  boolean accepted=c.messageText[99]!=marker;if(!accepted)c.messageText[99]=tail;out.writeBoolean(accepted);if(accepted){added++;if(c.messageType[0]==4)trades++;if(c.messageType[0]==8)duels++;}
  for(int i=0;i<100;i++){out.writeInt(c.messageType[i]);text(c.messageSender[i]);text(c.messageText[i]);}
  text(c.modalMessage);out.writeInt(c.mouseClickButton);out.writeBoolean(c.redrawChatback);cases++;
 }
 public static void main(String[] args)throws Exception {
  Locale.setDefault(Locale.ENGLISH);ByteArrayOutputStream bytes=new ByteArrayOutputStream();out=new DataOutputStream(new BufferedOutputStream(bytes));
  String[] messages={"","ordinary = @red@ text","Alice:tradereq:","Alice:duelreq:",":tradereq:",":duelreq:","aLiCe_123:tradereq:","Alice:ignored text:duelreq:","Alice:tradereq:duelreq:","Alice:duelreq:tradereq:","Alice:TRADEREQ:","Alice:tradereq: ","Alice:duelreq:!","tradereq:","duelreq:"," :tradereq:","@@!$%:duelreq:","ABCDEFGHIJKLMNO:tradereq:","Alice\nBob:duelreq:"};
  for(String message:messages)for(int override:new int[]{-1,0,1,2})for(int count:new int[]{0,1,99,100})for(int match:new int[]{-1,0,99})one(message,count,match,override,cases%2==0?5875:-1,cases%3==0?27:-1);
  for(int count=0;count<=100;count++)for(String suffix:new String[]{":tradereq:",":duelreq:"}){one("Alice"+suffix,count,count==0?-1:count-1,0,5875,-1);one("Bob"+suffix,count,count<100?count:-1,0,-1,27);}
  for(int n=0;n<=254;n++)one(repeat('x',n),0,-1,n%3,5875,-1);
  for(String suffix:new String[]{":tradereq:",":duelreq:"})one(repeat('A',254-suffix.length())+suffix,100,-1,0,5875,-1);
  Random random=new Random(2254);for(int i=0;i<500;i++){String name="Player_"+random.nextInt(100000);String suffix=i%3==0?":tradereq:":i%3==1?":duelreq:":":unknown:";one(name+suffix,i%101,i%4==0 && i%101<100?i%101:-1,i%7==0?1:0,i%2==0?5875:-1,i%3==0?27:-1);}
  out.flush();try(DataOutputStream file=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[0])))){file.writeInt(0x47523235);file.writeInt(cases);file.write(bytes.toByteArray());}
  System.out.println("Original game requests:"+cases+" packet4 cases,"+added+" history additions,"+trades+" trades,"+duels+" duels;all100 history slots,ignore/override and modal/redraw/click state;actual unchanged client.read/ClientStream");
 }
}
