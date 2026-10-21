import deob.client;
import jagex2.config.*;
import jagex2.dash3d.entity.*;
import jagex2.graphics.*;
import jagex2.io.*;
import java.io.*;
import java.nio.file.*;
import java.util.*;

/** Production original player-mask decoding and entity updates, no copied formulas. */
public class ExactOracle {
 static Jagfile jag(String dir,String name)throws Exception{return new Jagfile(Files.readAllBytes(Paths.get(dir,name)),false);}
 static void state(DataOutputStream out,PlayerEntity p)throws Exception {
  for(int v:new int[]{p.x,p.z,p.yaw,p.dstYaw,p.seqTrigger,p.pathLength,p.pathTileX[0],p.pathTileZ[0],p.forceMoveEndCycle,p.forceMoveStartCycle,p.primarySeqId,p.primarySeqFrame,p.primarySeqCycle,p.primarySeqDelay,p.primarySeqLoop,p.secondarySeqId,p.secondarySeqFrame,p.secondarySeqCycle})out.writeInt(v);
 }
 public static void main(String[] args)throws Exception {
  Jagfile config=jag(args[0],"config"),models=jag(args[0],"models");Model.unpack(3,models);AnimBase.unpack(false,models);AnimFrame.unpack(false,models);SeqType.unpack(config,1);
  client app=new client();
  try(DataOutputStream out=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[1])))) {
   out.writeInt(0x45584331);int cases=4*3*2;out.writeInt(cases);
   for(int facing=0;facing<4;facing++)for(int variant=0;variant<3;variant++)for(int animated=0;animated<2;animated++) {
    int start=variant==0?0:variant==1?7:30,duration=variant==0?1:variant==1?25:96,seq=animated==0?-1:749,delay=animated==0?0:5;
    PlayerEntity p=new PlayerEntity();app.localPlayer=p;p.x=43*128+64;p.z=44*128+64;p.pathTileX[0]=43;p.pathTileZ[0]=44;p.pathLength=3;p.seqTrigger=5;p.primarySeqId=seq;p.primarySeqDelay=delay;p.secondarySeqId=p.seqStandId=-1;
    client.loopCycle=1000;byte[] packet={46,44,46,47,(byte)(start>>8),(byte)start,(byte)((start+duration)>>8),(byte)(start+duration),(byte)facing};
    app.getPlayerExtended(true,2047,0x200,new Packet(1,packet),p);
    for(int v:new int[]{facing,start,duration,seq,delay})out.writeInt(v);state(out,p);
    int ticks=start+duration+5;out.writeInt(ticks);
    for(int tick=1000;tick<1000+ticks;tick++){client.loopCycle=tick;app.updateEntity(p,(byte)-128,1);out.writeInt(tick);state(out,p);}
   }
   LinkedHashMap<Integer,Integer> priorities=new LinkedHashMap<Integer,Integer>();for(int id=0;id<SeqType.count;id++)priorities.put(SeqType.instances[id].priority,id);
   int pairs=(SeqType.count+1)*(priorities.size()+2);out.writeInt(pairs);
   for(int id=-1;id<SeqType.count;id++) {
    ArrayList<Integer> old=new ArrayList<Integer>(priorities.values());old.add(-1);old.add(id);
    for(int previous:old) {
     PlayerEntity p=new PlayerEntity();p.primarySeqId=previous;p.primarySeqFrame=2;p.primarySeqCycle=3;p.primarySeqLoop=4;p.primarySeqDelay=5;
     app.getPlayerExtended(true,2047,2,new Packet(1,new byte[]{(byte)(id>>8),(byte)id,7}),p);
     out.writeInt(previous);out.writeInt(id);for(int v:new int[]{p.primarySeqId,p.primarySeqFrame,p.primarySeqCycle,p.primarySeqLoop,p.primarySeqDelay})out.writeInt(v);
    }
   }
   System.out.println("Original: "+cases+" full player forced-movement lifecycles; "+pairs+" original player sequence priority/restart decisions across "+priorities.size()+" cache priorities.");
  }
 }
}
