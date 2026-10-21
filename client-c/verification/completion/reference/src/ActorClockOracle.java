import deob.client;
import jagex2.config.*;
import jagex2.dash3d.entity.*;
import jagex2.graphics.*;
import jagex2.io.*;
import java.io.*;
import java.nio.file.*;
public class ActorClockOracle {
 static Jagfile jag(String dir,String name)throws Exception{return new Jagfile(Files.readAllBytes(Paths.get(dir,name)),false);}
 static void fields(DataOutputStream out,NpcEntity n)throws Exception{for(int v:new int[]{n.primarySeqId,n.primarySeqFrame,n.primarySeqCycle,n.primarySeqLoop,n.primarySeqDelay,n.secondarySeqId,n.secondarySeqFrame,n.secondarySeqCycle,n.seqStretches?1:0})out.writeInt(v);}
 public static void main(String[] args)throws Exception {
  Jagfile config=jag(args[0],"config"),models=jag(args[0],"models");Model.unpack(3,models);AnimBase.unpack(false,models);AnimFrame.unpack(false,models);SeqType.unpack(config,1);client clock=new client();int ticks=0;
  try(DataOutputStream out=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[1])))) {
   out.writeInt(0x41434c31);out.writeInt(SeqType.count*2);
   for(int id=0;id<SeqType.count;id++)for(int variant=0;variant<2;variant++) {
    SeqType seq=SeqType.instances[id];NpcEntity n=new NpcEntity();n.primarySeqId=n.secondarySeqId=id;
    int duration=0;for(int delay:seq.delay)duration+=Math.max(0,delay)+1;
    int steps=variant==0?duration*2+20:4;
    if(variant==0)n.primarySeqDelay=5;
    else{n.primarySeqFrame=n.secondarySeqFrame=seq.frameCount-1;n.primarySeqCycle=n.secondarySeqCycle=seq.delay[seq.frameCount-1];n.primarySeqLoop=seq.replaycount-1;}
    out.writeInt(id);out.writeInt(variant);fields(out,n);out.writeInt(steps);
    for(int tick=0;tick<steps;tick++){client.loopCycle=tick;clock.updateSequences(true,n);fields(out,n);ticks++;}
   }
  }
  System.out.println("Original: "+(SeqType.count*2)+" secondary/primary clocks, "+ticks+" complete states including delays, frame wrap, replay-count boundary, placeholder delays and stretch flags.");
 }
}
