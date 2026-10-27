import deob.client;
import deob.SoundQueueBoundary;
import sign.signlink;
import jagex2.io.*;
import jagex2.sound.*;
import java.io.*;
import java.nio.file.*;
import java.util.*;
public class SoundQueueOracle {
 static client c=new client();static int cases,realCases;
 static void state(DataOutputStream f)throws Exception{for(int v:new int[]{c.waveCount,c.lastWaveId,c.lastWaveLoops,c.lastWaveLength})f.writeInt(v);f.writeLong(c.lastWaveStartTime);for(int i=0;i<50;i++){f.writeInt(c.waveIds[i]);f.writeInt(c.waveLoops[i]);f.writeInt(c.waveDelay[i]);}}
 static void one(DataOutputStream f,int specification,long time,long step,boolean busy,boolean saveThrows,boolean replayThrows)throws Exception{
  f.writeInt(specification);f.writeLong(time);f.writeLong(step);f.writeBoolean(busy);f.writeBoolean(saveThrows);f.writeBoolean(replayThrows);state(f);
  signlink.savereq=busy?"busy.wav":null;signlink.savebuf=null;signlink.wavepos=0;signlink.savelen=0;SoundQueueBoundary.specification=specification;SoundQueueBoundary.time=time;SoundQueueBoundary.step=step;SoundQueueBoundary.clockCalls=SoundQueueBoundary.events=0;SoundQueueBoundary.saveThrows=saveThrows?1:0;SoundQueueBoundary.replayThrows=replayThrows?1:0;ByteArrayOutputStream events=new ByteArrayOutputStream();SoundQueueBoundary.trace=new DataOutputStream(events);
  c.updateGame(true);state(f);f.writeInt(SoundQueueBoundary.clockCalls);f.writeInt(SoundQueueBoundary.events);f.write(events.toByteArray());cases++;if(specification==-3)realCases++;
 }
 static void reset(){c.waveCount=0;c.lastWaveId=c.lastWaveLoops=-1;c.lastWaveLength=0;c.lastWaveStartTime=0;for(int i=0;i<50;i++){c.waveIds[i]=i*3;c.waveLoops[i]=i+1;c.waveDelay[i]=100+i;}}
 public static void main(String[] args)throws Exception{
  ByteArrayOutputStream klass=new ByteArrayOutputStream();InputStream in=client.class.getResourceAsStream("client.class");int b;while((b=in.read())!=-1)klass.write(b);if(!new String(klass.toByteArray(),"ISO-8859-1").contains("deob/SoundQueueBoundary"))throw new Exception("Wrong class:original queue slice not loaded");
  Jagfile sounds=new Jagfile(Files.readAllBytes(Paths.get(args[0],"sounds")),false);Wave.unpack(new Packet(363,sounds.read("sounds.dat",null,(byte)0)),1);int noise=0x12345678;for(int i=0;i<Tone.noise.length;i++){noise=noise*1103515245+12345;Tone.noise[i]=(noise&0x80000000)!=0?1:-1;}
  ByteArrayOutputStream bytes=new ByteArrayOutputStream();DataOutputStream f=new DataOutputStream(bytes);
  for(int delay:new int[]{-6,-5,-1,0,1,2,Integer.MAX_VALUE})for(int length:new int[]{-1,0,44,5512})for(boolean replay:new boolean[]{false,true})for(boolean busy:new boolean[]{false,true}){reset();c.waveCount=1;c.waveIds[0]=2;c.waveLoops[0]=1;c.waveDelay[0]=delay;if(replay){c.lastWaveId=2;c.lastWaveLoops=1;c.lastWaveLength=5512;c.lastWaveStartTime=999999;}one(f,length,1000000,1,busy,false,false);}
  for(long now:new long[]{0,1,-1,Long.MIN_VALUE,Long.MAX_VALUE,1000000})for(int prior:new int[]{Integer.MIN_VALUE,0,44,5512,Integer.MAX_VALUE})for(long delta:new long[]{-1,0,1}){reset();c.waveCount=1;c.waveIds[0]=2;c.waveLoops[0]=1;c.waveDelay[0]=0;c.lastWaveLength=prior;c.lastWaveStartTime=now;one(f,5512,now,delta,false,false,false);}
  Random random=new Random(2255512);for(int row=0;row<384;row++){reset();c.waveCount=new int[]{0,1,2,8,50}[row%5];c.lastWaveId=row%3==0?2:-1;c.lastWaveLoops=row%3==0?1:-1;c.lastWaveLength=random.nextInt();c.lastWaveStartTime=row%7==0?Long.MAX_VALUE:999000+random.nextInt(2000);for(int i=0;i<c.waveCount;i++){c.waveIds[i]=random.nextInt(4);c.waveLoops[i]=random.nextInt(3);c.waveDelay[i]=new int[]{-6,-5,-1,0,1,2}[random.nextInt(6)];}one(f,new int[]{-1,0,44,5512,22050,441000}[row%6],1000000,row%3-1,row%4==0,row%11==0,row%13==0);}
  for(int id=0;id<1000;id++)if(Wave.tracks[id]!=null)for(int loops:new int[]{0,1,2,255}){reset();c.waveCount=1;c.waveIds[0]=id;c.waveLoops[0]=loops;c.waveDelay[0]=0;one(f,-3,1000000,1,false,false,false);}
  try(DataOutputStream output=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[1])))){output.writeInt(0x53513235);output.writeInt(cases);output.writeInt(realCases);output.write(bytes.toByteArray());}
  System.out.println("Original sound queue:"+cases+" full50-slot states/traces including"+realCases+" real-cache generations;exact source queue slice,2 clock/1 generator/1 save/1 replay external boundaries;no full tick ordevice");
 }
}
