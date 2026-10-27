import jagex2.config.*;
import jagex2.dash3d.*;
import jagex2.datastruct.LinkList;
import jagex2.graphics.*;
import java.io.*;
import java.nio.file.*;
import java.util.*;

/** Direct unchanged activation and occlusion predicates on real scene inputs. */
public final class OcclusionOracle {
    static void ints(DataOutputStream out,int... values)throws Exception {for(int v:values)out.writeInt(v);}
    static int[] fields(Occlude o){return new int[]{o.minTileX,o.maxTileX,o.minTileZ,o.maxTileZ,o.type,o.minX,o.maxX,o.minZ,o.maxZ,o.minY,o.maxY,o.mode,o.minDeltaX,o.maxDeltaX,o.minDeltaZ,o.maxDeltaZ,o.minDeltaY,o.maxDeltaY};}
    static Occlude copy(Occlude input)throws Exception {Occlude out=new Occlude();for(java.lang.reflect.Field f:Occlude.class.getFields())f.setInt(out,f.getInt(input));return out;}
    static long cacheHash(World3D scene){long h=0xcbf29ce484222325L;for(int[][] l:scene.levelTileOcclusionCycles)for(int[] x:l)for(int v:x){h^=(v&0xffffffffL);h*=0x100000001b3L;}return h;}
    static boolean query(World3D scene,int[] q){
        if(q[7]!=0 && q[0]!=0)scene.levelTileOcclusionCycles[q[1]][q[2]][q[q[0]==4?4:3]]=q[8];
        if(q[0]==0){
            if(q[7]==0)return scene.occluded(q[1],q[2],q[3]);
            int count=World3D.activeOccluderCount;Occlude first=World3D.activeOccluders[0];
            World3D.activeOccluderCount=1;World3D.activeOccluders[0]=World3D.levelOccluders[World3D.topLevel][q[8]];
            boolean result=scene.occluded(q[1],q[2],q[3]);World3D.activeOccluderCount=count;World3D.activeOccluders[0]=first;return result;
        }
        if(q[0]==1)return scene.tileVisible(q[1],q[2],q[3]);
        if(q[0]==2)return scene.visible(q[1],q[2],q[3],q[4]);
        if(q[0]==3)return scene.wallVisible(q[1],q[2],q[3],q[4]);
        return scene.locVisible(q[1],q[2],q[3],q[4],q[5],q[6]);
    }
    public static void main(String[] args)throws Exception {
        String cache=args[0],maps=args[1];World.lowMemory=false;World.randomHueOffset=0;World.randomLightnessOffset=0;
        Model.unpack(3,StaticSceneOracle.jag(cache,"models"));AnimBase.unpack(false,StaticSceneOracle.jag(cache,"models"));AnimFrame.unpack(false,StaticSceneOracle.jag(cache,"models"));SeqType.unpack(StaticSceneOracle.jag(cache,"config"),1);LocType.unpack(StaticSceneOracle.jag(cache,"config"));StaticSceneOracle.seed();FloType.unpack(StaticSceneOracle.jag(cache,"config"),473);
        Pix3D.lowDetail=true;Pix3D.unpackTextures((byte)2,StaticSceneOracle.jag(cache,"textures"));StaticSceneOracle.seed();Pix3D.setBrightness(true,0.8);Pix3D.initPool(20,-20);
        int[][][] heights=new int[4][105][105];World world=new World(104,new byte[4][104][104],104,heights,-35388);World3D scene=new World3D(1,heights,104,4,104);scene.setMinLevel(0,0);
        CollisionMap[] collision=new CollisionMap[4];for(int l=0;l<4;l++)collision[l]=new CollisionMap(104,-1,104);
        world.loadGround(Files.readAllBytes(Paths.get(maps,"m50_50.raw")),3180,1,20,20,3180);StaticSceneOracle.seed();world.loadLocations(Files.readAllBytes(Paths.get(maps,"l50_50.raw")),scene,collision,new LinkList(0),true,20,20);world.build(scene,-270,collision);
        int real=0;for(int n:World3D.levelOccluderCount)real+=n;
        for(int level=0;level<4;level++){
            World3D.addOccluder(13312,4096,-1,4096,1,4096,level,-4096,0);
            World3D.addOccluder(4096,0,-1,4096,2,13312,level,-4096,4096);
            World3D.addOccluder(13312,0,-1,-900,4,13312,level,-900,0);
            // A plane exactly at matrix edge 50, and one outside it.
            World3D.addOccluder(13312,7296,-1,0,1,7296,level,-4096,0);
            World3D.addOccluder(13312,7424,-1,0,1,7424,level,-4096,0);
        }
        Occlude[][] base=new Occlude[4][];
        for(int l=0;l<4;l++){
            base[l]=new Occlude[World3D.levelOccluderCount[l]];
            for(int i=0;i<base[l].length;i++){Occlude o=copy(World3D.levelOccluders[l][i]);o.mode=9;o.minDeltaX=-257;o.maxDeltaX=511;o.minDeltaZ=-513;o.maxDeltaZ=1023;o.minDeltaY=-1025;o.maxDeltaY=2047;base[l][i]=o;}
        }
        int[] distance=new int[9];for(int i=0;i<9;i++){int angle=i*32+143;distance[i]=(angle*3+600)*Pix3D.sinTable[angle]>>16;}World3D.init(distance,800,512,(byte)15,334,500);
        boolean[][][] visibility=new boolean[4][][];visibility[0]=World3D.visibilityMatrix[4][0];
        for(int k=1;k<4;k++){visibility[k]=new boolean[51][51];for(int x=0;x<51;x++)for(int z=0;z<51;z++)visibility[k][x][z]=k==2 || (k==3 && (x*3+z)%7==0);}
        int[][] cameras={{4063,-1029,4063},{4064,-1029,4064},{4096,-1029,4096},{4128,-1029,4128},{4129,-1029,4129},{4160,-1027,4160},{4160,-1028,4160},{4160,-1029,4160},{0,-1500,0},{13311,-1500,13311},{4129,Integer.MAX_VALUE,4129},{Integer.MIN_VALUE+4096,-1029,4160}};
        List<int[]> queries=new ArrayList<int[]>();List<TileConstructionOracle.Input> tiles=TileConstructionOracle.read(args[2]);
        for(int i=0;i<tiles.size();i++){
            TileConstructionOracle.Input p=tiles.get(i);int l=i%4;
            queries.add(new int[]{0,p.x*128+1,p.h[0],p.z*128+1,0,0,0,0,0});
            queries.add(new int[]{1,l,p.x,p.z,0,0,0,0,0});queries.add(new int[]{2,l,p.x,p.z,240,0,0,1,1});
            for(int type:new int[]{1,2,4,8,16,32,64,128})for(int seed=0;seed<3;seed++)queries.add(new int[]{3,l,p.x,p.z,type,0,0,1,seed});
            for(int span=0;span<4;span++)for(int seed=0;seed<3;seed++)queries.add(new int[]{4,l,p.x,p.x+(span&1),p.z,p.z+(span>>1),240,1,seed});
        }
        for(int x=30;x<=33;x++)for(int z=30;z<=33;z++)for(int l=0;l<4;l++)queries.add(new int[]{4,l,x,x+1,z,z+1,240,1,1});
        for(int x:new int[]{4095,4096,4097,0,13312,Integer.MIN_VALUE,Integer.MAX_VALUE})for(int z:new int[]{0,4095,4096,4097,13312})for(int y:new int[]{-901,-900,-899,-4096,4096})queries.add(new int[]{0,x,y,z,0,0,0,0,0});
        Random random=new Random(225);for(int i=0;i<256;i++)queries.add(new int[]{0,random.nextInt(),random.nextInt(),random.nextInt(),0,0,0,0,0});
        int[] cycles={225,226,0,Integer.MIN_VALUE,Integer.MAX_VALUE,-1};int groups=4*4*cameras.length;long hits=0,total=0,active=0;int[] modes=new int[6];long[] kindHits=new long[5],wallHits=new long[8];
        try(DataOutputStream out=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[3])))){
            ints(out,0x4f323235,104,104,4,4);for(boolean[][] map:visibility)for(boolean[] x:map)for(boolean v:x)out.writeBoolean(v);
            for(int[][] l:heights)for(int[] x:l)ints(out,x);ints(out,groups);
            int index=0;for(int profile=0;profile<4;profile++)for(int level=0;level<4;level++)for(int ci=0;ci<cameras.length;ci++){
                int[] c=cameras[ci];World3D.eyeX=c[0];World3D.eyeY=c[1];World3D.eyeZ=c[2];World3D.eyeTileX=ci==11?32:c[0]/128;World3D.eyeTileZ=c[2]/128;World3D.topLevel=level;World3D.visibilityMap=visibility[profile];World3D.cycle=cycles[index++%cycles.length];
                ints(out,profile,World3D.eyeX,World3D.eyeY,World3D.eyeZ,World3D.eyeTileX,World3D.eyeTileZ,World3D.cycle,base[level].length);
                for(int i=0;i<base[level].length;i++){World3D.levelOccluders[level][i]=copy(base[level][i]);ints(out,fields(base[level][i]));}
                scene.updateActiveOccluders(false);ints(out,World3D.activeOccluderCount);active+=World3D.activeOccluderCount;
                for(int i=0;i<World3D.activeOccluderCount;i++){
                    int id=-1;for(int j=0;j<base[level].length;j++)if(World3D.activeOccluders[i]==World3D.levelOccluders[level][j])id=j;
                    if(id<0)throw new AssertionError("active identity");ints(out,id);modes[World3D.activeOccluders[i].mode]++;
                }
                for(int i=0;i<base[level].length;i++)ints(out,fields(World3D.levelOccluders[level][i]));
                List<int[]> groupQueries=new ArrayList<int[]>(queries);
                // Input generation targets projected edges of ONE original active
                // occluder at a time. Only actual occluded() supplies expectations.
                for(int id=0;id<base[level].length;id++){
                    Occlude o=World3D.levelOccluders[level][id];boolean isActive=false;for(int i=0;i<World3D.activeOccluderCount;i++)if(World3D.activeOccluders[i]==o)isActive=true;
                    if(!isActive)continue;
                    for(int d:new int[]{0,1,128}){
                        int a=(o.mode<=2?o.minZ:o.minX)+((o.mode<=2?o.minDeltaZ:o.minDeltaX)*d>>8),b=(o.mode<=2?o.maxZ:o.maxX)+((o.mode<=2?o.maxDeltaZ:o.maxDeltaX)*d>>8);
                        int low=(o.mode==5?o.minZ:o.minY)+((o.mode==5?o.minDeltaZ:o.minDeltaY)*d>>8),high=(o.mode==5?o.maxZ:o.maxY)+((o.mode==5?o.maxDeltaZ:o.maxDeltaY)*d>>8);
                        for(int u:new int[]{a-1,a,b,b+1,(a+b)/2})for(int v:new int[]{low-1,low,high,high+1,(low+high)/2}){
                            int x,y,z;if(o.mode<=2){x=o.mode==1?o.minX-d:o.minX+d;y=v;z=u;}else if(o.mode<=4){x=u;y=v;z=o.mode==3?o.minZ-d:o.minZ+d;}else{x=u;y=o.minY+d;z=v;}
                            groupQueries.add(new int[]{0,x,y,z,0,0,0,1,id});
                        }
                    }
                }
                for(int[][] l:scene.levelTileOcclusionCycles)for(int[] x:l)Arrays.fill(x,0);ints(out,groupQueries.size());
                for(int[] original:groupQueries){int[] q=original.clone();if(q[7]!=0 && q[0]!=0)q[8]=q[8]==1?World3D.cycle:q[8]==2?-World3D.cycle:0;ints(out,q);boolean result=query(scene,q);ints(out,result?1:0,q[0]==0?0:scene.levelTileOcclusionCycles[q[1]][q[2]][q[q[0]==4?4:3]]);if(result){hits++;kindHits[q[0]]++;if(q[0]==3)wallHits[Integer.numberOfTrailingZeros(q[4])]++;}total++;}
                out.writeLong(cacheHash(scene));
            }
        }
        System.out.println("OcclusionOracle: "+real+" real occluders + 20 source-constructor probes; "+groups+" activations / "+active+" ordered active entries; modes="+Arrays.toString(modes)+"; "+total+" queries / "+hits+" true; 192 full cache hashes");
        System.out.println("True query kinds="+Arrays.toString(kindHits)+"; wall orientations="+Arrays.toString(wallHits));
        for(int mode=1;mode<=5;mode++)if(modes[mode]==0)throw new AssertionError("uncovered mode "+mode);
        for(long count:kindHits)if(count==0)throw new AssertionError("uncovered query kind");
        for(long count:wallHits)if(count==0)throw new AssertionError("uncovered wall orientation");
    }
}
