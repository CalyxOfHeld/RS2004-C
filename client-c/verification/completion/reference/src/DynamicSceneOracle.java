import deob.client;
import jagex2.config.*;
import jagex2.dash3d.*;
import jagex2.dash3d.entity.*;
import jagex2.datastruct.*;
import jagex2.graphics.*;
import jagex2.io.*;
import java.io.*;
import java.nio.file.*;
import java.util.*;

/** Bounded source-call scenario; the original client supplies every expectation. */
public final class DynamicSceneOracle {
    static class Observed extends Entity {
        Entity actor; int calls;
        Observed(Entity actor){this.actor=actor;}
        public Model draw(boolean value){calls++;return actor.draw(value);}
    }
    static void textures(String cache,boolean low,double brightness)throws Exception {
        Pix3D.lowDetail=low;Pix3D.clearTexels(false);
        Pix3D.unpackTextures((byte)2,StaticSceneOracle.jag(cache,"textures"));
        StaticSceneOracle.seed();Pix3D.setBrightness(true,brightness);Pix3D.initPool(20,-20);
    }
    static World3D scene(String maps,byte[][][] flags,int[][][] heights,LinkList animations,CollisionMap[] collision)throws Exception {
        World.randomHueOffset=World.randomLightnessOffset=0;
        World world=new World(104,flags,104,heights,-35388);World3D result=new World3D(1,heights,104,4,104);
        result.setMinLevel(0,0);for(int l=0;l<4;l++)collision[l]=new CollisionMap(104,-1,104);
        world.loadGround(Files.readAllBytes(Paths.get(maps,"m50_50.raw")),3180,1,20,20,3180);
        StaticSceneOracle.seed();world.loadLocations(Files.readAllBytes(Paths.get(maps,"l50_50.raw")),result,collision,animations,true,20,20);
        world.build(result,-270,collision);return result;
    }
    public static void main(String[] args)throws Exception {
        String cache=args[0],maps=args[1];
        String pickingPath=System.getProperty("rs2.scene.picking");boolean picking=pickingPath!=null;
        boolean runtime=Boolean.getBoolean("rs2.scene.runtime");
        World.lowMemory=false;World.randomHueOffset=World.randomLightnessOffset=0;
        Jagfile models=StaticSceneOracle.jag(cache,"models"),cfg=StaticSceneOracle.jag(cache,"config");
        Model.unpack(3,models);AnimBase.unpack(false,models);AnimFrame.unpack(false,models);
        SeqType.unpack(cfg,1);LocType.unpack(cfg);StaticSceneOracle.seed();FloType.unpack(cfg,473);
        NpcType.unpack(cfg);IdkType.unpack(cfg,1);ObjType.unpack(cfg);SpotAnimType.unpack(cfg,1);
        textures(cache,true,0.8);
        byte[][][] flags=new byte[4][104][104];int[][][] heights=new int[4][105][105];
        CollisionMap[] collision=new CollisionMap[4];LinkList animations=new LinkList(0);
        World3D scene=scene(maps,flags,heights,animations,collision);
        LocEntity selected=null;
        for(LocEntity a=(LocEntity)animations.head();a!=null;a=(LocEntity)animations.next(551)){
            System.out.println("animated="+a.index+" level="+a.level+" layer="+a.type+" at="+a.x+","+a.z);
            if(selected==null && a.level==0 && a.x==32 && a.z==39 && (flags[1][a.x][a.z]&2)==0)selected=a;
        }
        if(selected==null)throw new AssertionError("no bounded animated scenery");
        animations=new LinkList(0);animations.addTail(selected);
        client app=new client();app.scene=scene;app.levelHeightmap=heights;app.locList=animations;
        int[] distances=new int[9];for(int i=0;i<9;i++){int angle=i*32+143;distances[i]=(angle*3+600)*Pix3D.sinTable[angle]>>16;}
        World3D.init(distances,800,512,(byte)15,334,500);
        NpcEntity npc=new NpcEntity();npc.type=NpcType.get(0);npc.seqStandId=npc.secondarySeqId=npc.type.readyanim;
        Model held=npc.draw(true);PlayerEntity player=new PlayerEntity();
        if(picking)held.pickable=false;
        Observed n=new Observed(npc),p=new Observed(player),unused=new Observed(npc);
        int spot=-1;for(SpotAnimType s:SpotAnimType.instances)if(s.seq!=null){spot=s.index;break;}
        try(DataOutputStream out=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[2])));
            DataOutputStream pickOut=picking?new DataOutputStream(new BufferedOutputStream(new FileOutputStream(pickingPath))):null){
            PainterTraceHooks.ints(out,0x44534331,selected.level,selected.x,selected.z,selected.type,selected.index,12);
            if(picking)PainterTraceHooks.ints(pickOut,0x504b5331,12);
            for(int step=0;step<12;step++){
                scene.clearTemporaryLocs(0);
                if(runtime && step==6){
                    flags=new byte[4][104][104];heights=new int[4][105][105];LinkList rebuiltAnimations=new LinkList(0);
                    scene=scene(maps,flags,heights,rebuiltAnimations,collision);selected=null;
                    for(LocEntity a=(LocEntity)rebuiltAnimations.head();a!=null;a=(LocEntity)rebuiltAnimations.next(551))
                        if(a.level==0 && a.x==32 && a.z==39 && a.index==196)selected=a;
                    if(selected==null)throw new AssertionError("missing rebuilt animation");
                    animations=new LinkList(0);animations.addTail(selected);app.locList=animations;app.scene=scene;app.levelHeightmap=heights;
                    World3D.init(distances,800,512,(byte)15,334,500);
                }
                int delta=new int[]{0,1,7,20}[step%4];app.sceneDelta=delta;app.pushLocs(1);
                boolean clear=step==4 || step==8;
                if(clear){NpcType.modelCache.clear();PlayerEntity.modelCache.clear();}
                npc.primarySeqId=-1;npc.secondarySeqId=step%2==0?npc.type.readyanim:npc.type.walkanim;
                npc.secondarySeqFrame=step%3==0?0:ActorOracle.last(npc.secondarySeqId);
                byte[] appearance=ActorOracle.appearance(step%2,step%4,step%3);player.read(false,new Packet(1,appearance));
                player.primarySeqId=step%2==0?-1:749;player.primarySeqFrame=step%2==0?0:ActorOracle.last(749)/2;
                player.secondarySeqId=step%2==0?808:819;player.secondarySeqFrame=step%2==0?0:ActorOracle.last(819);
                npc.spotanimId=player.spotanimId=step>=6?spot:-1;
                npc.spotanimFrame=player.spotanimFrame=0;npc.spotanimOffset=player.spotanimOffset=13;
                int yaw=new int[]{0,256,768,1280,1792,2047}[step%6];
                PainterTraceHooks.ints(out,delta,clear?1:0,yaw,npc.spotanimId);
                ActorOracle.selection(out,npc);ActorOracle.selection(out,player);out.writeInt(appearance.length);out.write(appearance);
                // Preview changes global texture detail/brightness and projection scratch.
                textures(cache,step>=6,0.7);int[] preview=new int[512*334+8];Arrays.fill(preview,0x19372b);
                Pix2D.bind(512,preview,-1,334);Pix3D.init3D(334,512,0);Pix3D.trans=0;Pix3D.hclip=false;
                held.drawSimple(128,256,0,0,0,20,500);
                PainterTraceHooks.ints(out,Pix3D.trans,Pix3D.hclip?1:0);for(int v:preview)out.writeInt(v);
                textures(cache,step<6,0.8);World3D.lowMemory=step<6;
                n.calls=p.calls=unused.calls=0;
                int directBitset=picking && step==6?0:picking && step==7?-1:picking && step==2?0x20000000:0x20000002;
                boolean reverse=picking && step==11;
                for(int slot=0;!runtime && slot<3;slot++){
                    int actor=reverse?2-slot:slot;
                    int tx=picking?42:42+actor,tz=!picking && actor==1?36:35;
                    int x=tx*128+64,z=tz*128+64,y=heights[0][tx][tz];
                    if(!scene.addTemporary(-44713,z,60,yaw,x,actor==2?directBitset:0x20000000+actor,true,actor==2?held:null,actor==0?n:actor==1?p:unused,y,0))throw new AssertionError("temporary capacity");
                }
                if(runtime){
                    app.currentLevel=0;app.levelTileFlags=flags;app.localPlayer=player;
                    client.loopCycle=100+step;
                    app.levelCollisionMap=collision;
                    if(step==3)app.addLoc(3,41,30,2,-1,10,-27819,0);
                    if(step==4)app.addLoc(3,41,30,2,879,10,-27819,0);
                    player.x=42*128+64;player.z=35*128+64;player.yaw=yaw;
                    player.locModel=step==1?held:null;
                    if(player.locModel!=null){
                        player.locStartCycle=100;player.locStopCycle=104;player.locOffsetX=player.x+64;player.locOffsetZ=player.z+64;
                        player.locOffsetY=heights[0][42][35];player.minTileX=42;player.maxTileX=43;player.minTileZ=34;player.maxTileZ=35;
                    }
                    npc.x=44*128+64;npc.z=35*128+64;npc.yaw=yaw;app.npcs[0]=npc;app.npcIds[0]=0;app.npcCount=1;
                    app.sceneCycle++;app.pushPlayers(284);app.pushNpcs(false);
                    LinkList stack=new LinkList(0);
                    for(int id:new int[]{995,1277,379}){ObjStackEntity item=new ObjStackEntity();item.index=id;item.count=id==995?25000:1;stack.addTail(item);}
                    app.levelObjStacks[0][43][36]=stack;app.sortObjStacks(43,36);
                    app.projectiles=new LinkList(0);app.spotanims=new LinkList(0);client.loopCycle=100+step;app.sceneDelta=1;
                    ProjectileEntity shot=new ProjectileEntity(0,28,36*128+64,client.loopCycle+30,0,0,client.loopCycle,0,0,heights[0][41][36]-80,spot,41*128+64);
                    shot.updateVelocity(heights[0][45][38]-100,38*128+64,45*128+64,-855,client.loopCycle);app.projectiles.addTail(shot);
                    app.spotanims.addTail(new SpotAnimEntity(45*128+64,spot,false,37*128+64,0,heights[0][45][37],0,client.loopCycle));
                    app.pushProjectiles((byte)-26);app.pushSpotanims((byte)106);
                }
                int[] pixels=new int[512*334+8];Arrays.fill(pixels,0x19372b);
                Pix2D.bind(512,pixels,-1,334);Pix3D.init3D(334,512,0);Pix3D.trans=0;Pix3D.hclip=false;Model.checkHover=false;
                if(picking){
                    int[][] mouse={{256,50},{256,30},{260,50},{240,50},{256,65},{256,-1},{256,50},{256,50},{256,50},{1000000,1000000},{202,290},{256,50}};
                    int[][] ground={{256,300},{256,167},{0,333},{511,333},{256,50},{-1,-1},{256,300},{256,300},{256,300},{1000000,1000000},{511,0},{256,300}};
                    boolean hover=step!=8,input=step!=6 && step!=7 && step!=11;
                    Model.checkHover=hover;Model.mouseX=mouse[step][0];Model.mouseZ=mouse[step][1];
                    if(runtime && step==0){Model.mouseX=312;Model.mouseZ=36;}
                    Arrays.fill(Model.pickedBitsets,0x76543210);Model.pickedCount=runtime?0:2;
                    if(!runtime)Model.pickedBitsets[0]=Model.pickedBitsets[1]=0x11223344;
                    World3D.clickTileX=9;World3D.clickTileZ=10;World3D.takingInput=false;
                    if(input)scene.click(4,ground[step][1],ground[step][0]);
                    PainterTraceHooks.ints(pickOut,hover?1:0,Model.mouseX,Model.mouseZ,input?1:0,ground[step][0],ground[step][1],World3D.clickTileX,World3D.clickTileZ,runtime?spot:directBitset,runtime?step:reverse?1:0);
                }
                int[][] cameras={{42,28,256,0},{56,42,256,512},{42,56,256,1024},{28,42,256,1536},{42,28,320,0},{48,30,256,1792}};
                int[] camera=cameras[runtime?step%6:0];
                app.cameraX=camera[0]*128+64;app.cameraY=heights[0][camera[0]][camera[1]]-600;app.cameraZ=camera[1]*128+64;app.cameraPitch=camera[2];app.cameraYaw=camera[3];
                int top=runtime?app.getTopLevel((byte)106):0;
                scene.draw(camera[3],app.cameraX,top,camera[2],app.cameraY,app.cameraZ,0);
                if(runtime)System.out.println("runtime="+step+" roof_level="+top+" player="+player.height+" npc="+npc.height);
                if(picking){
                    PainterTraceHooks.ints(pickOut,Model.checkHover?1:0,World3D.takingInput?1:0,World3D.clickTileX,World3D.clickTileZ,Model.pickedCount);
                    for(int v:Model.pickedBitsets)pickOut.writeInt(v);
                    System.out.println("picking="+step+" count="+Model.pickedCount+" bits="+Arrays.toString(Arrays.copyOf(Model.pickedBitsets,Model.pickedCount))+" tile="+World3D.clickTileX+","+World3D.clickTileZ);
                }
                PainterTraceHooks.ints(out,n.calls,p.calls,unused.calls,npc.height,player.height);
                System.out.println("dynamic="+step+" resolves="+n.calls+"/"+p.calls+"/"+unused.calls+" heights="+npc.height+"/"+player.height);
                if(!runtime && (n.calls!=1 || p.calls!=1 || unused.calls!=0))throw new AssertionError("resolution coverage");
            }
        }
        System.out.println("DynamicSceneOracle: 12 original scenes and interleaved previews; animated scenery="+selected.index+" at "+selected.x+","+selected.z);
    }
}
