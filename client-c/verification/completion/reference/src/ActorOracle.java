import deob.client;
import jagex2.config.*;
import jagex2.dash3d.entity.*;
import jagex2.graphics.*;
import jagex2.io.*;
import java.io.*;
import java.nio.file.*;
import java.util.*;

/** Calls original appearance decoding and NPC/player sequenced-model builders. */
public class ActorOracle {
 static Jagfile jag(String dir,String name)throws Exception{return new Jagfile(Files.readAllBytes(Paths.get(dir,name)),false);}
 static AnimBase base(int seq){if(seq<0)return null;int frame=SeqType.instances[seq].frames[0];return frame<0?null:AnimFrame.instances[frame].base;}
 static int last(int seq){return seq<0?0:SeqType.instances[seq].frameCount-1;}
 static void selection(DataOutputStream out,PathingEntity a)throws Exception {
  for(int v:new int[]{a.seqStandId,a.primarySeqId,a.primarySeqFrame,a.primarySeqDelay,a.secondarySeqId,a.secondarySeqFrame})out.writeInt(v);
 }
 static byte[] appearance(int gender,int gear,int palette)throws Exception {
  int[] slots={8,11,4,6,9,7,10},appearance=new int[12];
  for(int part=0;part<7;part++)for(int id=0;id<IdkType.count;id++)if(IdkType.instances[id].type==part+gender*7 && !IdkType.instances[id].disable){appearance[slots[part]]=256+id;break;}
  if(gear==1){appearance[5]=1205+512;appearance[3]=1171+512;}if(gear==2)appearance[5]=841+512;if(gear==3)appearance[5]=1307+512;
  ByteArrayOutputStream bytes=new ByteArrayOutputStream();DataOutputStream out=new DataOutputStream(bytes);out.writeByte(gender);out.writeByte(0);
  for(int v:appearance){if(v==0)out.writeByte(0);else out.writeShort(v);}
  for(int i=0;i<5;i++)out.writeByte(palette==2?255:palette==0?0:(i+3)%client.field1307[i].length);
  for(int seq:new int[]{808,823,819,820,821,822,824})out.writeShort(seq);out.writeLong(123456789L);out.writeByte(3);return bytes.toByteArray();
 }
 static void player(DataOutputStream out,byte[] appearance,int primary,int frame,int delay,int secondary,int secondaryFrame)throws Exception {
  PlayerEntity p=new PlayerEntity();p.read(false,new Packet(1,appearance));p.primarySeqId=primary;p.primarySeqFrame=frame;p.primarySeqDelay=delay;p.secondarySeqId=secondary;p.secondarySeqFrame=secondaryFrame;
  out.writeInt(appearance.length);out.write(appearance);out.writeLong(p.appearanceHashcode);selection(out,p);MergeOracle.mesh(out,p.draw(true));out.writeInt(p.height);
 }
 public static void main(String[] args)throws Exception {
  Jagfile config=jag(args[0],"config"),models=jag(args[0],"models");Model.unpack(3,models);AnimBase.unpack(false,models);AnimFrame.unpack(false,models);SeqType.unpack(config,1);NpcType.unpack(config);IdkType.unpack(config,1);ObjType.unpack(config);SpotAnimType.unpack(config,1);
  int npcCases=0,npcDefinitions=0,nonrenderable=0,playerCases=0;
  try(DataOutputStream out=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[1])))) {
   out.writeInt(0x41435431);
   for(int id=0;id<NpcType.count;id++) {
    NpcType type=NpcType.get(id);if(type.models==null || type.models.length==0){nonrenderable++;continue;}npcDefinitions++;
    NpcEntity n=new NpcEntity();n.type=type;n.seqStandId=type.readyanim;
    for(int variant=0;variant<3;variant++) {
     n.primarySeqId=-1;n.primarySeqFrame=0;n.primarySeqDelay=0;n.secondarySeqId=variant==0?type.readyanim:type.walkanim;n.secondarySeqFrame=variant==0?0:last(n.secondarySeqId);
     if(variant==2) {
      AnimBase wanted=base(type.walkanim>=0?type.walkanim:type.readyanim);
      for(int seq=0;wanted!=null && seq<SeqType.count;seq++)if(SeqType.instances[seq].walkmerge!=null && base(seq)==wanted){n.primarySeqId=seq;break;}
     }
     out.writeInt(id);selection(out,n);out.writeInt(n.height);MergeOracle.mesh(out,n.getSequencedModel(false));out.writeInt(n.height);npcCases++;
    }
   }
   out.writeInt(-1);out.writeInt(nonrenderable);
   // Real defaults/equipment, both genders, palette variation and invalid palette clamping.
   for(int gender=0;gender<2;gender++)for(int gear=0;gear<4;gear++)for(int palette=0;palette<3;palette++) {
    byte[] bytes=appearance(gender,gear,palette);player(out,bytes,-1,0,0,808,0);playerCases++;
    player(out,bytes,749,last(749)/2,0,819,last(819));playerCases++;
   }
   // Every cache sequence, first/last transforms, compatible walk blending and delays/held substitutions.
   for(int gender=0;gender<2;gender++) {
    byte[] bytes=appearance(gender,1,1);
    for(int seq=0;seq<SeqType.count;seq++) {
     int secondary=SeqType.instances[seq].walkmerge!=null && base(seq)!=base(819)?808:819;
     player(out,bytes,seq,0,0,secondary,last(secondary));playerCases++;
     player(out,bytes,seq,last(seq),0,808,0);playerCases++;
     if(SeqType.instances[seq].walkmerge!=null || SeqType.instances[seq].righthand>=0 || SeqType.instances[seq].lefthand>=0){player(out,bytes,seq,0,3,819,0);playerCases++;}
    }
   }
   out.writeInt(-1);
   int combined=0;for(SpotAnimType spot:SpotAnimType.instances)if(spot.seq!=null)combined+=4;
   out.writeInt(combined);
   for(int actor=0;actor<2;actor++)for(SpotAnimType spot:SpotAnimType.instances)if(spot.seq!=null)for(int variant=0;variant<2;variant++) {
    int spotFrame=variant==0?0:spot.seq.frameCount-1;out.writeInt(actor);out.writeInt(spot.index);out.writeInt(spotFrame);out.writeInt(13);
    if(actor==0) {
     NpcEntity n=new NpcEntity();n.type=NpcType.get(0);n.secondarySeqId=n.seqStandId=n.type.readyanim;n.spotanimId=spot.index;n.spotanimFrame=spotFrame;n.spotanimOffset=13;
     selection(out,n);MergeOracle.mesh(out,n.draw(true));out.writeInt(n.height);
    } else {
     PlayerEntity p=new PlayerEntity();p.read(false,new Packet(1,appearance(0,1,1)));p.primarySeqId=749;p.primarySeqFrame=3;p.secondarySeqId=819;p.spotanimId=spot.index;p.spotanimFrame=spotFrame;p.spotanimOffset=13;
     selection(out,p);MergeOracle.mesh(out,p.draw(true));out.writeInt(p.height);
    }
   }
   // A returned alpha-sharing NPC model stays alive after LRU eviction/clear.
   NpcType.modelCache.clear();int retainedId=-1;
   for(int id=0;id<NpcType.count;id++){NpcType t=NpcType.get(id);if(t.models!=null && !t.animHasAlpha && t.getSequencedModel(-1,-1,null).faceAlpha!=null){retainedId=id;break;}}
   out.writeInt(retainedId);
   if(retainedId>=0) {
    Model held=NpcType.get(retainedId).getSequencedModel(-1,-1,null);MergeOracle.mesh(out,held);
    for(int id=0;id<40;id++)if(id!=retainedId)NpcType.get(id).getSequencedModel(-1,-1,null);
    MergeOracle.mesh(out,held);NpcType.modelCache.clear();MergeOracle.mesh(out,held);
   }
  }
  System.out.println("Original: "+npcDefinitions+" rendered NPC definitions / "+npcCases+" ready/walk/blended models; "+nonrenderable+" nonrenderable definitions; "+playerCases+" decoded player appearance/sequence models covering both genders, real held items, all "+SeqType.count+" sequences, rest-pose shades, delays, blending and held substitutions.");
 }
}
