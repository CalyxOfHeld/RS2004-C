import jagex2.config.*;
import jagex2.dash3d.*;
import jagex2.dash3d.type.*;
import jagex2.datastruct.LinkList;
import jagex2.graphics.*;
import jagex2.io.Jagfile;
import java.io.*;
import java.lang.reflect.Field;
import java.nio.file.*;
import java.util.*;

/** Direct unchanged World3D ground calls on real-cache tile inputs. */
public final class GroundDrawOracle {
    static Jagfile jag(String dir,String name)throws Exception{return new Jagfile(Files.readAllBytes(Paths.get(dir,name)),false);}
    static void seed()throws Exception {
        Field f=Class.forName("java.lang.Math$RandomNumberGeneratorHolder").getDeclaredField("randomNumberGenerator");
        f.setAccessible(true);((Random)f.get(null)).setSeed(225);
    }
    static class Tile {
        int x,z;int[] h;TileUnderlay u;TileOverlay o;
        Tile(int x,int z,int[] h,TileUnderlay u,TileOverlay o){this.x=x;this.z=z;this.h=h.clone();this.u=u;this.o=o;}
    }
    static int[] colours(TileUnderlay u){return new int[]{u.southwestColor,u.southeastColor,u.northeastColor,u.northwestColor};}
    static boolean valid(TileUnderlay u,int max){for(int c:colours(u))if(c<0||c>max)return false;return true;}
    static boolean flat(int[] h){return h[0]==h[1]&&h[0]==h[2]&&h[0]==h[3];}
    static void writeTile(DataOutputStream out,Tile t)throws Exception {
        out.writeInt(t.u!=null?0:1);out.writeInt(t.x);out.writeInt(t.z);for(int v:t.h)out.writeInt(v);
        if(t.u!=null){for(int v:colours(t.u))out.writeInt(v);out.writeInt(t.u.textureId);out.writeInt(t.u.rgb);out.writeInt(t.u.flat?1:0);}
        else {
            TileOverlay o=t.o;for(int v:new int[]{o.flat?1:0,o.shape,o.rotation,o.backgroundRgb,o.foregroundRgb,o.triangleTextureIds==null?0:1,o.vertexX.length,o.triangleVertexA.length})out.writeInt(v);
            for(int v=0;v<o.vertexX.length;v++){out.writeInt(o.vertexX[v]);out.writeInt(o.vertexY[v]);out.writeInt(o.vertexZ[v]);}
            for(int f=0;f<o.triangleVertexA.length;f++)for(int v:new int[]{o.triangleVertexA[f],o.triangleVertexB[f],o.triangleVertexC[f],o.triangleColorA[f],o.triangleColorB[f],o.triangleColorC[f],o.triangleTextureIds==null?-1:o.triangleTextureIds[f]})out.writeInt(v);
        }
    }
    public static void main(String[] args)throws Exception {
        String cache=args[0],maps=args[1];World.lowMemory=false;World.randomHueOffset=0;World.randomLightnessOffset=0;
        Jagfile cfg=jag(cache,"config"),models=jag(cache,"models");Model.unpack(3,models);AnimBase.unpack(false,models);AnimFrame.unpack(false,models);
        SeqType.unpack(cfg,1);LocType.unpack(cfg);seed();FloType.unpack(cfg,473);
        Pix3D.lowDetail=true;Pix3D.unpackTextures((byte)2,jag(cache,"textures"));seed();Pix3D.setBrightness(true,0.8);Pix3D.initPool(20,-20);
        double brightness=0.8+(new Random(225).nextDouble()*0.03-0.015);
        int[][][] heights=new int[4][105][105];World world=new World(104,new byte[4][104][104],104,heights,-35388);
        World3D scene=new World3D(1,heights,104,4,104);CollisionMap[] collision=new CollisionMap[4];for(int l=0;l<4;l++)collision[l]=new CollisionMap(104,-1,104);
        world.loadGround(Files.readAllBytes(Paths.get(maps,"m50_50.raw")),3180,1,20,20,3180);
        seed();world.loadLocations(Files.readAllBytes(Paths.get(maps,"l50_50.raw")),scene,collision,new LinkList(0),true,20,20);world.build(scene,-270,collision);
        List<Tile> tiles=new ArrayList<Tile>();Set<String> seen=new HashSet<String>();Tile plain=null,textured=null,flatHeights=null,slopeHeights=null;
        for(int l=0;l<4;l++)for(int x=1;x<103;x++)for(int z=1;z<103;z++) {
            Ground g=scene.levelTiles[l][x][z];if(g==null || (g.underlay==null && g.overlay==null))continue;
            int level=g.occludeLevel;int[] h={heights[level][x][z],heights[level][x+1][z],heights[level][x+1][z+1],heights[level][x][z+1]};
            Tile t=new Tile(x,z,h,g.underlay,g.overlay);
            if(flat(h)&&flatHeights==null)flatHeights=t;if(!flat(h)&&slopeHeights==null)slopeHeights=t;
            if(g.underlay!=null){if(g.underlay.textureId==-1 && valid(g.underlay,65535) && plain==null)plain=t;if(g.underlay.textureId>=0 && valid(g.underlay,127) && textured==null)textured=t;}
            String key=g.underlay!=null?"u/"+g.underlay.textureId+"/"+g.underlay.flat+"/"+flat(h):"o/"+g.overlay.shape+"/"+g.overlay.rotation+"/"+g.overlay.flat+"/"+(g.overlay.triangleTextureIds!=null);
            if(seen.add(key) && tiles.size()<80)tiles.add(t);
        }
        if(plain==null || textured==null || flatHeights==null || slopeHeights==null)throw new AssertionError("missing actual tile prototypes");
        int real=tiles.size();int[] base=colours(plain.u);
        int[][][] temporaryHeights=new int[1][105][105];World3D constructors=new World3D(1,temporaryHeights,104,1,104);
        for(int shape=0;shape<13;shape++)for(int rotation=0;rotation<4;rotation++)for(Tile ht:new Tile[]{flatHeights,slopeHeights})for(int texture=0;texture<2;texture++) {
            int[] over=texture==0?base:colours(textured.u),h=ht.h;int x=ht.x,z=ht.z;
            constructors.levelTiles[0][x][z]=null;
            constructors.setTile(0,x,z,shape,rotation,texture==0?-1:textured.u.textureId,h[0],h[1],h[2],h[3],base[0],base[1],base[2],base[3],over[0],over[1],over[2],over[3],plain.u.rgb,textured.u.rgb);
            Ground g=constructors.levelTiles[0][x][z];tiles.add(new Tile(x,z,h,g.underlay,g.overlay));
        }
        // The source's 12345678 sentinel is absent from the selected map
        // families. Exercise its explicit draw-skip contract while retaining
        // real heights/geometry. Picking occurs before this raster skip.
        for(int mask=1;mask<=3;mask++) {
            TileUnderlay u=new TileUnderlay((mask&1)!=0?12345678:base[0],base[1],(mask&2)!=0?12345678:base[2],base[3],-1,plain.u.rgb,plain.u.flat);
            tiles.add(new Tile(flatHeights.x,flatHeights.z,flatHeights.h,u,null));
        }
        int hx=flatHeights.x,hz=flatHeights.z;int[] hh=flatHeights.h;
        constructors.levelTiles[0][hx][hz]=null;
        constructors.setTile(0,hx,hz,2,0,-1,hh[0],hh[1],hh[2],hh[3],12345678,12345678,12345678,12345678,12345678,12345678,12345678,12345678,plain.u.rgb,plain.u.rgb);
        tiles.add(new Tile(hx,hz,hh,null,constructors.levelTiles[0][hx][hz].overlay));
        int[][][] drawHeights=new int[1][105][105];World3D renderer=new World3D(1,drawHeights,104,1,104);
        int cases=tiles.size()*6*3,n=0,changedCases=0,clicks=0,early=0;
        try(DataOutputStream out=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[2])))) {
            out.writeInt(0x47323235);out.writeDouble(brightness);out.writeInt(tiles.size());for(Tile t:tiles)writeTile(out,t);out.writeInt(cases);
            int detail=1;
            for(int mode=0;mode<3;mode++) {
                int low=mode==2?0:1;
                if(low!=detail){detail=low;Pix3D.lowDetail=low!=0;Pix3D.clearTexels(false);Pix3D.unpackTextures((byte)2,jag(cache,"textures"));seed();Pix3D.setBrightness(true,0.8);Pix3D.initPool(20,-20);}
                World3D.lowMemory=mode==0;
                for(int index=0;index<tiles.size();index++)for(int pose=0;pose<6;pose++) {
                    Tile t=tiles.get(index);int x=t.x*128+64,z=t.z*128+64,y=(t.h[0]+t.h[1]+t.h[2]+t.h[3])/4;
                    int ex=x,ez=z-512,ey=y-512,pitch=256,yaw=0;
                    if(pose==1){ex=x+512;ez=z;yaw=512;}
                    if(pose==2){ez=z-1536;ey=y-1536;}
                    if(pose==3 || pose==4){ez=t.z*128-(pose==3?50:49);ey=y-3;pitch=0;}
                    if(pose==5){ey=y+512;pitch=1792;}
                    int[][] mouse={{32,32},{25,39},{48,16}};boolean draw=!(t.o!=null && pose==2 && mode==0);
                    int[] c={index,ex,ey,ez,pitch,yaw,mode==0?1:0,low,n&1,draw?1:0,n%4==0?0:1,mouse[n%3][0],mouse[n%3][1],new int[]{0,64,192}[n%3],n&1,-7,-9};
                    int[] pixels=new int[64*64+8];Arrays.fill(pixels,0x19372b);Pix2D.bind(64,pixels,-1,64);Pix3D.init3D(64,64,0);Pix3D.jagged=c[8]!=0;Pix3D.trans=c[13];Pix3D.hclip=c[14]!=0;
                    World3D.eyeX=ex;World3D.eyeY=ey;World3D.eyeZ=ez;World3D.takingInput=c[10]!=0;World3D.mouseX=c[11];World3D.mouseY=c[12];World3D.clickTileX=c[15];World3D.clickTileZ=c[16];
                    drawHeights[0][t.x][t.z]=t.h[0];drawHeights[0][t.x+1][t.z]=t.h[1];drawHeights[0][t.x+1][t.z+1]=t.h[2];drawHeights[0][t.x][t.z+1]=t.h[3];
                    if(t.u!=null)renderer.drawTileUnderlay(t.u,0,Model.sin[pitch],Model.cos[pitch],Model.sin[yaw],Model.cos[yaw],t.x,t.z);
                    else renderer.drawTileOverlay(Model.sin[yaw],t.z,t.o,t.x,Model.cos[pitch],Model.sin[pitch],Model.cos[yaw],draw);
                    boolean changed=false;for(int v:pixels)if(v!=0x19372b)changed=true;if(changed)changedCases++;
                    if(World3D.clickTileX==t.x && World3D.clickTileZ==t.z)clicks++;if(Pix3D.trans==c[13] && c[13]!=0)early++;
                    for(int v:c)out.writeInt(v);out.writeInt(Pix3D.trans);out.writeInt(Pix3D.hclip?1:0);out.writeInt(World3D.clickTileX);out.writeInt(World3D.clickTileZ);out.writeInt(pixels.length);for(int v:pixels)out.writeInt(v);n++;
                }
            }
            out.writeInt(World3D.TEXTURE_HSL.length);for(int hsl:World3D.TEXTURE_HSL)out.writeInt(hsl);
            int[] shades={-128,0,1,31,64,96,126,127,128,255,Integer.MIN_VALUE,Integer.MAX_VALUE};out.writeInt(World3D.TEXTURE_HSL.length*shades.length);
            for(int hsl:World3D.TEXTURE_HSL)for(int shade:shades){out.writeInt(shade);out.writeInt(hsl);out.writeInt(renderer.mulLightness(shade,hsl,9));}
            List<int[]> picks=new ArrayList<int[]>();int[][] tris={{10,80,20,10,20,90},{0,0,0,0,0,0},{-40,100,20,70,-10,40},{-100,-20,-80,-50,-10,-90}};
            int[][] perms={{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};
            for(int[] t:tris)for(int[] p:perms){int minx=Math.min(t[0],Math.min(t[1],t[2])),maxx=Math.max(t[0],Math.max(t[1],t[2])),miny=Math.min(t[3],Math.min(t[4],t[5])),maxy=Math.max(t[3],Math.max(t[4],t[5]));
                int[] xs={minx-1,minx,minx+1,(minx+maxx)/2,maxx-1,maxx,maxx+1},ys={miny-1,miny,miny+1,(miny+maxy)/2,maxy-1,maxy,maxy+1};
                for(int xx:xs)for(int yy:ys)picks.add(new int[]{xx,yy,t[3+p[0]],t[3+p[1]],t[3+p[2]],t[p[0]],t[p[1]],t[p[2]]});}
            Random r=new Random(225);for(int i=0;i<1000;i++){int[] c=new int[8];for(int j=0;j<8;j++)c[j]=r.nextInt();picks.add(c);}
            out.writeInt(picks.size());for(int[] c:picks){for(int v:c)out.writeInt(v);out.writeInt(renderer.pointInsideTriangle(c[0],c[1],c[2],c[3],c[4],c[5],c[6],c[7])?1:0);}
            System.out.println("GroundDrawOracle: "+real+" real map tile families + 208 original constructor combinations + 4 source-sentinel cases; "+n+" draws, "+changedCases+" nonempty, "+clicks+" clicks, "+early+" alpha-preserving cases; 600 lightness / "+picks.size()+" pick checks");
            if(changedCases==0 || clicks==0)throw new AssertionError("missing visible/input coverage");
        }
    }
}
