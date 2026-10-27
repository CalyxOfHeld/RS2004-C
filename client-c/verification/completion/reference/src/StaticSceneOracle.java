import jagex2.config.*;
import jagex2.dash3d.*;
import jagex2.dash3d.type.Ground;
import jagex2.datastruct.LinkList;
import jagex2.graphics.*;
import jagex2.io.Jagfile;
import java.io.*;
import java.lang.reflect.Field;
import java.nio.file.*;
import java.util.*;
import java.awt.image.BufferedImage;
import javax.imageio.ImageIO;

/** Unchanged World/World3D render the retained real Lumbridge map chunk. */
public final class StaticSceneOracle {
    static Jagfile jag(String root,String name)throws Exception {
        return new Jagfile(Files.readAllBytes(Paths.get(root,name)),false);
    }
    static void seed()throws Exception {
        Field f=Class.forName("java.lang.Math$RandomNumberGeneratorHolder").getDeclaredField("randomNumberGenerator");
        f.setAccessible(true);((Random)f.get(null)).setSeed(225);
    }
    public static void main(String[] args)throws Exception {
        String cache=args[0],maps=args[1];
        World.lowMemory=false;World.randomHueOffset=0;World.randomLightnessOffset=0;
        Model.unpack(3,jag(cache,"models"));AnimBase.unpack(false,jag(cache,"models"));AnimFrame.unpack(false,jag(cache,"models"));
        Jagfile cfg=jag(cache,"config");SeqType.unpack(cfg,1);LocType.unpack(cfg);seed();FloType.unpack(cfg,473);
        Pix3D.lowDetail=true;Pix3D.unpackTextures((byte)2,jag(cache,"textures"));seed();Pix3D.setBrightness(true,0.8);Pix3D.initPool(20,-20);
        double brightness=0.8+(new Random(225).nextDouble()*0.03-0.015);
        byte[][][] flags=new byte[4][104][104];int[][][] heights=new int[4][105][105];
        World world=new World(104,flags,104,heights,-35388);World3D scene=new World3D(1,heights,104,4,104);
        scene.setMinLevel(0,0);CollisionMap[] collision=new CollisionMap[4];for(int l=0;l<4;l++)collision[l]=new CollisionMap(104,-1,104);
        world.loadGround(Files.readAllBytes(Paths.get(maps,"m50_50.raw")),3180,1,20,20,3180);
        seed();world.loadLocations(Files.readAllBytes(Paths.get(maps,"l50_50.raw")),scene,collision,new LinkList(0),true,20,20);
        world.build(scene,-270,collision);
        int[] distance=new int[9];for(int i=0;i<9;i++){int angle=i*32+128+15;distance[i]=(angle*3+600)*Pix3D.sinTable[angle]>>16;}
        World3D.init(distance,800,512,(byte)15,334,500);
        int under=0,over=0,walls=0,decor=0,locs=0,bridges=0,occluders=0;
        for(int l=0;l<4;l++) {
            occluders+=World3D.levelOccluderCount[l];
            for(int x=0;x<104;x++)for(int z=0;z<104;z++) {
                Ground g=scene.levelTiles[l][x][z];if(g==null)continue;
                if(g.underlay!=null)under++;if(g.overlay!=null)over++;if(g.wall!=null)walls++;if(g.decor!=null)decor++;
                if(g.bridge!=null)bridges++;
                for(int j=0;j<g.locCount;j++)if(g.locs[j].minSceneTileX==x && g.locs[j].minSceneTileZ==z)locs++;
            }
        }
        System.out.println("Scene: underlays="+under+" overlays="+over+" walls="+walls+" decor="+decor+" locs="+locs+" bridge tiles="+bridges+" occluders="+occluders);
        int[][] cameras={{42,28,256,0,0},{56,42,256,512,0},{42,56,256,1024,0},
            {28,42,256,1536,0},{42,28,320,0,1},{42,28,256,0,2}};
        int index=0;
        try(DataOutputStream out=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[2])))) {
            out.writeInt(0x53445731);out.writeInt(512);out.writeInt(334);out.writeInt(cameras.length*2);out.writeDouble(brightness);
            out.writeInt(World.randomHueOffset);out.writeInt(World.randomLightnessOffset);
            for(int mode=0;mode<2;mode++) {
                Pix3D.lowDetail=mode==0;World3D.lowMemory=mode==0;Pix3D.clearTexels(false);
                Pix3D.unpackTextures((byte)2,jag(cache,"textures"));seed();Pix3D.setBrightness(true,0.8);Pix3D.initPool(20,-20);
                for(int[] c:cameras) {
                    int x=c[0]*128+64,z=c[1]*128+64,y=heights[c[4]][c[0]][c[1]]-600;
                    int[] pixels=new int[512*334+8];Arrays.fill(pixels,0x19372b);
                    Pix2D.bind(512,pixels,-1,334);Pix3D.init3D(334,512,0);Pix3D.trans=0;Pix3D.hclip=false;Model.checkHover=false;
                    scene.draw(c[3],x,c[4],c[2],y,z,0);
                    if(World3D.tilesRemaining!=0)throw new AssertionError("unfinished tile traversal "+World3D.tilesRemaining);
                    int changed=0;for(int i=0;i<512*334;i++)if(pixels[i]!=0x19372b)changed++;
                    for(int i=512*334;i<pixels.length;i++)if(pixels[i]!=0x19372b)throw new AssertionError("scene guard changed");
                    if(changed==0)throw new AssertionError("empty scene capture");
                    for(int v:new int[]{x,y,z,c[2],c[3],c[4],mode==0?1:0,mode==0?1:0})out.writeInt(v);
                    out.writeInt(Pix3D.trans);out.writeInt(Pix3D.hclip?1:0);out.writeInt(pixels.length);for(int v:pixels)out.writeInt(v);
                    if(args.length>3) {
                        BufferedImage image=new BufferedImage(512,334,BufferedImage.TYPE_INT_RGB);image.setRGB(0,0,512,334,pixels,0,512);
                        ImageIO.write(image,"png",Paths.get(args[3],"frame-"+index+".png").toFile());
                    }
                    System.out.println("case="+index+" camera="+x+","+y+","+z+" pitch="+c[2]+" yaw="+c[3]+" plane="+c[4]+" low="+(mode==0)+" changed="+changed);index++;
                }
            }
        }
        System.out.println("StaticSceneOracle: "+index+" complete original frames; hue/light offsets="+World.randomHueOffset+"/"+World.randomLightnessOffset);
    }
}
