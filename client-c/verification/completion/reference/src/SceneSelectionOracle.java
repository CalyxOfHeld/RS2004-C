import jagex2.config.*;
import jagex2.dash3d.*;
import jagex2.dash3d.type.Ground;
import jagex2.datastruct.LinkList;
import jagex2.graphics.*;
import java.io.*;
import java.nio.file.*;
import java.util.*;

/** Calls unchanged draw; virtual drawTile stops before traversal mutates selection. */
public final class SceneSelectionOracle {
    static final RuntimeException STOP=new RuntimeException("selection captured");
    static final class Probe extends World3D {
        boolean stopped;
        Probe(int[][][] h){super(1,h,104,4,104);}
        @Override public void drawTile(Ground g,boolean first){stopped=true;throw STOP;}
    }
    static void ints(DataOutputStream out,int... v)throws Exception {for(int n:v)out.writeInt(n);}
    static void flags(DataOutputStream out,Ground g)throws Exception {out.writeBoolean(g.visible);out.writeBoolean(g.update);out.writeBoolean(g.containsLocs);out.writeInt(g.checkLocSpans);}
    static void reset(Probe scene){
        int id=0;for(int l=0;l<4;l++)for(int x=0;x<104;x++)for(int z=0;z<104;z++,id++){
            Ground g=scene.levelTiles[l][x][z];if(g==null)continue;
            g.visible=(id&1)!=0;g.update=(id&2)!=0;g.containsLocs=(id&4)!=0;g.checkLocSpans=id%31+1;
        }
    }
    public static void main(String[] args)throws Exception {
        String cache=args[0],maps=args[1];World.lowMemory=false;World.randomHueOffset=0;World.randomLightnessOffset=0;
        Model.unpack(3,StaticSceneOracle.jag(cache,"models"));AnimBase.unpack(false,StaticSceneOracle.jag(cache,"models"));AnimFrame.unpack(false,StaticSceneOracle.jag(cache,"models"));
        SeqType.unpack(StaticSceneOracle.jag(cache,"config"),1);LocType.unpack(StaticSceneOracle.jag(cache,"config"));StaticSceneOracle.seed();FloType.unpack(StaticSceneOracle.jag(cache,"config"),473);
        Pix3D.lowDetail=true;Pix3D.unpackTextures((byte)2,StaticSceneOracle.jag(cache,"textures"));StaticSceneOracle.seed();Pix3D.setBrightness(true,0.8);Pix3D.initPool(20,-20);
        int[][][] heights=new int[4][105][105];World world=new World(104,new byte[4][104][104],104,heights,-35388);Probe scene=new Probe(heights);
        scene.setMinLevel(0,0);CollisionMap[] collision=new CollisionMap[4];for(int l=0;l<4;l++)collision[l]=new CollisionMap(104,-1,104);
        world.loadGround(Files.readAllBytes(Paths.get(maps,"m50_50.raw")),3180,1,20,20,3180);StaticSceneOracle.seed();world.loadLocations(Files.readAllBytes(Paths.get(maps,"l50_50.raw")),scene,collision,new LinkList(0),true,20,20);world.build(scene,-270,collision);
        int[] distance=new int[9];for(int i=0;i<9;i++){int angle=i*32+143;distance[i]=(angle*3+600)*Pix3D.sinTable[angle]>>16;}World3D.init(distance,800,512,(byte)15,334,500);
        boolean[][][][][] matrices=new boolean[4][][][][];matrices[0]=World3D.visibilityMatrix;
        for(int k=1;k<4;k++){
            matrices[k]=new boolean[8][32][51][51];
            for(int p=0;p<8;p++)for(int yaw=0;yaw<32;yaw++)for(int x=0;x<51;x++)for(int z=0;z<51;z++)matrices[k][p][yaw][x][z]=k==2 || (k==3 && (p*11+yaw*7+x*3+z)%5==0);
        }
        List<int[]> cases=new ArrayList<int[]>();
        int[][] positions={{42*128+64,28*128+64},{-1,-1},{104*128,104*128},{0,103*128+127},{103*128+127,0},{24*128,25*128},{25*128,24*128},{79*128,78*128}};
        int[] pitches={97,127,128,159,160,255,256,319,320,351,352,383},yaws={0,63,64,511,512,1023,1024,1535,1536,1983,1984,2047};
        int n=0;
        for(int profile=0;profile<4;profile++)for(int[] position:positions)for(int j=0;j<3;j++){
            cases.add(new int[]{position[0],j==0?-600:j==1?-2600:Integer.MAX_VALUE,position[1],pitches[n%12],yaws[n%12],n%4,n%5,profile,n%2==0?Integer.MAX_VALUE:-1});n++;
        }
        for(int delta:new int[]{1999,2000,2001})cases.add(new int[]{42*128+64,heights[0][42][28]-delta,28*128+64,256,0,3,0,1,225});
        long selected=0;int stopped=0,present=0;
        try(DataOutputStream out=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[2])))) {
            ints(out,0x53323235,104,104,4,4);
            for(boolean[][][][] matrix:matrices)for(boolean[][][] p:matrix)for(boolean[][] yaw:p)for(boolean[] x:yaw)for(boolean value:x)out.writeBoolean(value);
            reset(scene);
            for(int l=0;l<4;l++)for(int x=0;x<104;x++)for(int z=0;z<104;z++){
                Ground g=scene.levelTiles[l][x][z];out.writeBoolean(g!=null);if(g!=null){present++;ints(out,g.drawLevel,heights[l][x][z],g.locCount);flags(out,g);}
            }
            ints(out,cases.size());
            for(int[] c:cases){
                reset(scene);scene.minLevel=c[6];World3D.visibilityMatrix=matrices[c[7]];World3D.cycle=c[8];scene.stopped=false;
                try{scene.draw(c[4],c[0],c[5],c[3],c[1],c[2],0);}catch(RuntimeException e){if(e!=STOP)throw e;}
                if(scene.stopped!=(World3D.tilesRemaining>0))throw new AssertionError("capture boundary");
                if(scene.stopped)stopped++;selected+=World3D.tilesRemaining;
                int pi=-1,yi=-1;for(int p=0;p<8;p++)for(int yaw=0;yaw<32;yaw++)if(World3D.visibilityMap==matrices[c[7]][p][yaw]){pi=p;yi=yaw;}
                ints(out,c);ints(out,World3D.cycle,World3D.sinEyePitch,World3D.cosEyePitch,World3D.sinEyeYaw,World3D.cosEyeYaw,World3D.eyeX,World3D.eyeY,World3D.eyeZ,World3D.eyeTileX,World3D.eyeTileZ,World3D.topLevel,World3D.minDrawTileX,World3D.minDrawTileZ,World3D.maxDrawTileX,World3D.maxDrawTileZ,World3D.tilesRemaining,pi,yi);
                for(int l=0;l<4;l++)for(int x=0;x<104;x++)for(int z=0;z<104;z++){Ground g=scene.levelTiles[l][x][z];if(g!=null)flags(out,g);}
            }
        }
        System.out.println("SceneSelectionOracle: "+cases.size()+" cameras; "+present+" real scene tiles; "+(cases.size()*(long)present)+" tile states; "+selected+" eligible across "+stopped+" intercepted draws; four visibility profiles");
    }
}
