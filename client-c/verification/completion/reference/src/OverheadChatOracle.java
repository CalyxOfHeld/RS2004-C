import deob.client;
import jagex2.io.*;
import jagex2.graphics.*;
import jagex2.config.NpcType;
import jagex2.dash3d.entity.*;
import java.io.*;
import java.nio.file.*;
import java.util.*;
// Actual unchanged draw2DEntityElements and PixFont; expected pixels sparse only
// for storage, reconstructed in full by native comparison.
public class OverheadChatOracle {
 static client c=new client();static DataOutputStream out;static int cases,failures;static long changed;
 static class Actor {
  int kind,x=6400,z=6400,height,colour,style,timer=150;boolean visible=true;String name="Self Test",text="Abc W @red@012!";
  Actor(int k){kind=k;}
 }
 static void text(String s)throws Exception{if(s==null){out.writeInt(-1);return;}out.writeInt(s.length());for(char ch:s.toCharArray())out.writeChar(ch);}
 static void one(List<Actor> input,int mode,int effects,int cycle,boolean clipped)throws Exception {
  c.playerCount=c.npcCount=0;c.publicChatSetting=mode;c.chatEffects=effects;c.sceneCycle=cycle;c.friendCount=1;c.friendName[0]="aLiCe";
  c.cameraX=6400;c.cameraY=-500;c.cameraZ=5800;c.cameraPitch=256;c.cameraYaw=0;c.currentLevel=0;client.loopCycle=1000;
  out.writeInt(mode);out.writeInt(effects);out.writeInt(cycle);out.writeInt(clipped?70:0);out.writeInt(clipped?30:0);out.writeInt(clipped?430:512);out.writeInt(clipped?250:334);
  for(int v:new int[]{c.cameraX,c.cameraY,c.cameraZ,c.cameraPitch,c.cameraYaw})out.writeInt(v);
  out.writeInt(input.size());
  for(int i=0;i<input.size();i++) {
   Actor a=input.get(i);PathingEntity p;
   if(a.kind<2){PlayerEntity player=new PlayerEntity();player.visible=a.visible;player.name=a.name;p=player;if(a.kind==0){c.localPlayer=player;c.players[2047]=player;}else{int id=3+i;c.players[id]=player;c.playerIds[c.playerCount++]=id;}}
   else {NpcEntity npc=new NpcEntity();npc.type=a.visible?new NpcType():null;p=npc;int id=2+i;c.npcs[id]=npc;c.npcIds[c.npcCount++]=id;}
   p.x=a.x;p.z=a.z;p.height=a.height;p.chat=a.text;p.chatColor=a.colour;p.chatStyle=a.style;p.chatTimer=a.text==null?0:a.timer;
   out.writeInt(a.kind);out.writeBoolean(a.visible);text(a.name);out.writeInt(a.x);out.writeInt(a.z);out.writeInt(a.height);text(a.text);out.writeInt(a.colour);out.writeInt(a.style);out.writeInt(p.chatTimer);
   c.projectFromGround(p.height,false,p);out.writeInt(c.projectX);out.writeInt(c.projectY);
  }
  int[] pixels=new int[512*334];Arrays.fill(pixels,0x123456);Pix2D.bind(512,pixels,-657,334);Pix2D.setClipping(clipped?250:334,clipped?30:0,clipped?430:512,789,clipped?70:0);
  boolean ok=true;try{c.draw2DEntityElements(-1);}catch(ArrayIndexOutOfBoundsException ex){if(c.chatCount!=50 || effects!=0 || (c.chatStyles[49]!=1 && c.chatStyles[49]!=2))throw ex;ok=false;failures++;}
  out.writeBoolean(ok);out.writeInt(c.chatCount);
  for(int i=0;i<50;i++){for(int v:new int[]{c.chatX[i],c.chatY[i],c.chatWidth[i],c.chatHeight[i],c.chatColors[i],c.chatStyles[i],c.chatTimers[i]})out.writeInt(v);text(c.chats[i]);}
  for(int v:new int[]{Pix2D.boundLeft,Pix2D.boundTop,Pix2D.boundRight,Pix2D.boundBottom})out.writeInt(v);
  int count=0;for(int v:pixels)if(v!=0x123456)count++;out.writeInt(count);changed+=count;for(int i=0;i<pixels.length;i++)if(pixels[i]!=0x123456){out.writeInt(i);out.writeInt(pixels[i]);}cases++;
 }
 public static void main(String[] args)throws Exception {
  Pix3D.centerW3D=256;Pix3D.centerH3D=167;
  c.fontBold12=new PixFont(new Jagfile(Files.readAllBytes(Paths.get(args[0])),false),"b12",530);c.levelHeightmap=new int[4][105][105];c.levelTileFlags=new byte[4][104][104];
  ByteArrayOutputStream bytes=new ByteArrayOutputStream();out=new DataOutputStream(new BufferedOutputStream(bytes));
  int[] timers={150,149,101,100,99,51,50,49,1,0},cycles={-1,0,1,9,10,19,20,199,255,1024};
  for(int colour=0;colour<=12;colour++)for(int style=0;style<=3;style++)for(int timer:timers)for(int effects=0;effects<2;effects++){
   Actor a=new Actor(0);a.colour=colour;a.style=style;a.timer=timer;one(Arrays.asList(a),0,effects,cycles[cases%cycles.length],cases%5==0);
  }
  for(int cycle=0;cycle<256;cycle++){Actor a=new Actor(0);a.text="iW @red@0123456789 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ";a.style=1;one(Arrays.asList(a),0,0,cycle,cycle%2==0);}
  for(int mode=0;mode<4;mode++)for(int effects=0;effects<2;effects++)for(int cycle:new int[]{0,9,10,19,20}){
   List<Actor> actors=new ArrayList<>();actors.add(new Actor(0));
   for(int i=0;i<12;i++){Actor a=new Actor(1);a.name=new String[]{"Alice","Stranger","sELF tEST",null}[i%4];a.visible=i%5!=0;a.style=i%4;a.colour=i;a.height=i%3*30;a.text=i==1?"":i==2?null:"actor "+i;actors.add(a);}
   for(int i=0;i<4;i++){Actor a=new Actor(2);a.visible=i!=1;a.style=i;a.colour=6+i;a.x+=i*30;a.text="npc "+i;actors.add(a);}one(actors,mode,effects,cycle,false);
  }
  for(int count:new int[]{0,1,49,50,51,60})for(int style=0;style<3;style++)for(int effects=0;effects<2;effects++){
   List<Actor> actors=new ArrayList<>();Actor local=new Actor(0);if(count==0)local.text=null;actors.add(local);
   for(int i=1;i<count;i++){Actor a=new Actor(1);a.name="Alice";a.text="overlap "+i;a.style=i==49?style:0;actors.add(a);}one(actors,0,effects,10,false);
  }
  for(int x:new int[]{127,128,5999,6000,6008,6400,7000,13056,13057})for(int z:new int[]{127,128,5800,6400,13056,13057}){
   Actor a=new Actor(0);a.x=x;a.z=z;a.style=cases%3;one(Arrays.asList(a),0,0,9,false);
  }
  if(cases!=1426 || changed==0 || failures!=6)throw new AssertionError("missing overhead coverage");
  out.flush();try(DataOutputStream file=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[1])))){file.writeInt(0x4f483235);file.writeInt(cases);file.write(bytes.toByteArray());}
  System.out.println("Original overhead frames="+cases+" complete viewport words="+((long)cases*512*334)+" sparse changed words="+changed+" expected styled50th exceptions="+failures+"; actual draw2DEntityElements/b12, complete50-slot state, clipping/projection");
 }
}
