import deob.client;
import sign.signlink;
import sign.MixedAudioBoundary;
import java.io.*;
public final class MixedAudioOracle {
 static client c=new client();static DataOutputStream f;static int count;
 static long hash(byte[] b){if(b==null)return 0;long h=1469598103934665603L;for(byte v:b){h^=v&255;h*=1099511628211L;}return h;}
 static void slot(byte[] b)throws Exception{f.writeInt(b==null?0:b.length);f.writeLong(hash(b));}
 static void one(int op,int length,int seed,boolean fade)throws Exception{
  f.writeInt(op);f.writeInt(length);f.writeInt(seed);f.writeBoolean(fade);byte[] data=new byte[8192];for(int i=0;i<data.length;i++)data[i]=(byte)(i*17+seed);int result=-2;
  if(op==1)result=c.saveWave(data,length,0)?1:0;
  if(op==2)result=c.replayWave(c.field1143)?1:0;
  if(op==3){int before=signlink.midipos;c.saveMidi(data,625,length,fade);result=before!=signlink.midipos?1:0;}
  if(op==4)MixedAudioBoundary.service();
  f.writeInt(result);f.writeInt(signlink.wavepos);f.writeInt(signlink.midipos);f.writeInt(signlink.savereq==null?0:signlink.savereq.startsWith("sound")?1:3);f.writeInt(signlink.savelen);f.writeBoolean(signlink.waveplay);f.writeBoolean(signlink.midiplay);f.writeInt(signlink.midifade);
  for(byte[] b:MixedAudioBoundary.waves)slot(b);for(byte[] b:MixedAudioBoundary.midis)slot(b);
  slot(signlink.wave==null?null:MixedAudioBoundary.waves[signlink.wave.charAt(5)-'0']);slot(signlink.midi==null?null:MixedAudioBoundary.midis[signlink.midi.charAt(6)-'0']);count++;
 }
 public static void main(String[] args)throws Exception{
  ByteArrayOutputStream bytes=new ByteArrayOutputStream();InputStream in=signlink.class.getResourceAsStream("signlink.class");int b;while((b=in.read())!=-1)bytes.write(b);if(!new String(bytes.toByteArray(),"ISO-8859-1").contains("sign/MixedAudioBoundary"))throw new Exception("Wrong mixed worker class");
  signlink.threadliveid=1;Thread worker=new Thread(new signlink());worker.setDaemon(true);worker.start();MixedAudioBoundary.ready();bytes=new ByteArrayOutputStream();f=new DataOutputStream(bytes);
  one(0,0,0,false);
  for(int i=0;i<7;i++){one(1,128+i,1+i,false);one(3,96,31,true);one(4,0,0,false);one(3,256+i,17+i,true);one(3,64,42,false);one(1,128,99,false);one(4,0,0,false);one(2,0,0,false);one(4,0,0,false);}
  one(3,2000001,7,true);one(4,0,0,false);one(3,64,8,false);one(4,0,0,false);
  signlink.threadliveid=2;MixedAudioBoundary.gate.release();worker.join(3000);if(worker.isAlive())throw new Exception("join");
  try(DataOutputStream out=new DataOutputStream(new FileOutputStream(args[0]))){out.writeInt(0x4d423235);out.writeInt(count);out.write(bytes.toByteArray());}
  System.out.println("PASS original mixed audio:"+count+" wave/MIDI shared request/service states;both5-slot rotations,ordered busy/drop/replay,publication bytes,fade even onrejected MIDI;actual original worker");
 }
}
