import deob.client;
import sign.signlink;
import sign.SoundBackendBoundary;
import java.io.*;
public class SoundBackendOracle {
 static client c=new client();static byte[][] buffers={new byte[441000],new byte[441000]};static DataOutputStream f;static int count;
 static long hash(byte[] b,int start,int n){if(b==null)return 0;long h=1469598103934665603L;for(int i=start;i<start+n;i++){h^=b[i]&255;h*=1099511628211L;}return h;}
 static void one(int op,int a,int b,int seed)throws Exception{
  for(int v:new int[]{op,a,b,seed})f.writeInt(v);int result=-2;
  if(op==1){for(int i=0;i<buffers[a].length;i++)buffers[a][i]=(byte)(i*17+seed);result=c.saveWave(buffers[a],b,0)?1:0;}
  else if(op==2)result=c.replayWave(c.field1143)?1:0;
  else if(op==3)buffers[a][b]^=(byte)seed;
  else if(op==4)SoundBackendBoundary.service();
  else if(op==5)result=c.saveWave(null,b,0)?1:0;
  f.writeInt(result);f.writeInt(signlink.wavepos);f.writeBoolean(signlink.savereq!=null);f.writeBoolean(signlink.savebuf!=null);f.writeInt(signlink.savelen);f.writeBoolean(signlink.waveplay);
  for(int i=0;i<5;i++){byte[] slot=SoundBackendBoundary.slots[i];f.writeInt(slot==null?0:slot.length);f.writeLong(slot==null?0:hash(slot,0,slot.length));}
  f.writeLong(signlink.savebuf==null?0:hash(signlink.savebuf,0,signlink.savelen));
  byte[] published=signlink.wave==null?null:SoundBackendBoundary.slots[signlink.wave.charAt(5)-'0'];int n=published==null||published.length<44?0:published.length-44;f.writeInt(n);f.writeLong(n==0?0:hash(published,44,n));count++;
 }
 public static void main(String[] args)throws Exception{
  ByteArrayOutputStream cb=new ByteArrayOutputStream();InputStream input=signlink.class.getResourceAsStream("signlink.class");int b;while((b=input.read())!=-1)cb.write(b);if(!new String(cb.toByteArray(),"ISO-8859-1").contains("sign/SoundBackendBoundary"))throw new Exception("Wrong signlink worker class");
  signlink.threadliveid=1;signlink.savereq=null;signlink.savebuf=null;signlink.wavepos=signlink.savelen=0;signlink.wave=null;signlink.waveplay=false;Thread worker=new Thread(new signlink());worker.setDaemon(true);worker.start();SoundBackendBoundary.ready();
  ByteArrayOutputStream bytes=new ByteArrayOutputStream();f=new DataOutputStream(bytes);
  one(0,0,0,0);one(2,0,0,0);one(4,0,0,0);one(1,0,128,1);one(1,1,64,2);one(3,0,50,7);one(4,0,0,0);one(2,0,0,0);one(2,0,0,0);one(4,0,0,0);
  for(int i=0;i<9;i++){one(1,0,64+i*37,10+i);one(1,0,64,31+i);one(5,0,2000001,0);one(4,0,0,0);one(2,0,0,0);one(4,0,0,0);}
  one(1,1,2000001,9);one(5,0,0,0);one(4,0,0,0);
  signlink.threadliveid=2;SoundBackendBoundary.gate.release();worker.join(3000);if(worker.isAlive())throw new Exception("worker stop");
  try(DataOutputStream output=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[0])))){output.writeInt(0x53423235);output.writeInt(count);output.write(bytes.toByteArray());}
  System.out.println("Original signlink wave backend:"+count+" request/service states;5 rotating files,busy/replay/null/oversize/aliased bytes;actual worker with startup/file-output/sleep boundaries replaced,no cache files written");
 }
}
