package deob;
import java.io.*;
import jagex2.io.Packet;
import jagex2.sound.Wave;
public final class SoundQueueBoundary {
 public static int specification,clockCalls,saveThrows,replayThrows;
 public static long time,step;public static DataOutputStream trace;public static int events;
 static byte[] synthetic=new byte[441000];
 public static long hash(byte[] bytes,int n){if(bytes==null)return 0;long h=1469598103934665603L;for(int i=0;i<n;i++){h^=bytes[i]&255;h*=1099511628211L;}return h;}
 public static void event(int kind,int a,int b,long value){try{trace.writeInt(kind);trace.writeInt(a);trace.writeInt(b);trace.writeLong(value);events++;}catch(IOException e){throw new RuntimeException(e);}}
 public static Packet generate(int id,int loops){
  Packet p;try{if(specification==-1)throw new RuntimeException("controlled generation");if(specification==-3)p=Wave.generate((byte)-16,loops,id);else {for(int i=0;i<specification;i++)synthetic[i]=(byte)(i*17+id*31+loops);p=new Packet(5,synthetic);p.pos=specification;if(specification==0)p.data=null;}if(p==null)throw new RuntimeException("null generated packet");}
  catch(Exception e){event(1,id,loops,-1);throw e;}
  event(1,id,loops,p.pos);event(5,p.pos,0,hash(p.data,p.pos));return p;
 }
 public static long now(){long value=time+step*clockCalls++;event(2,0,0,value);return value;}
 public static boolean save(client c,byte[] data,int length){if(saveThrows!=0){event(3,length,-1,hash(data,length));throw new RuntimeException("controlled save");}boolean ok=c.saveWave(data,length,0);event(3,length,ok?1:0,hash(data,length));return ok;}
 public static boolean replay(client c){if(replayThrows!=0){event(4,-1,0,0);throw new RuntimeException("controlled replay");}boolean ok=c.replayWave(c.field1143);event(4,ok?1:0,0,0);return ok;}
}
