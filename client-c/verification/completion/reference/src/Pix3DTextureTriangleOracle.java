import jagex2.graphics.Pix2D;
import jagex2.graphics.Pix3D;
import jagex2.graphics.Pix8;
import jagex2.io.Jagfile;
import java.io.*;
import java.lang.reflect.Field;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.*;

/** Calls unchanged revision-225 textureTriangle and getTexels on real textures. */
public final class Pix3DTextureTriangleOracle {
    private static void seed(long value) throws Exception {
        Class<?> holder=Class.forName("java.lang.Math$RandomNumberGeneratorHolder");
        Field field=holder.getDeclaredField("randomNumberGenerator");
        field.setAccessible(true);
        ((Random)field.get(null)).setSeed(value);
    }
    private static int[] one(int shape,int[] p,int[] shades,int mapping,int serial) {
        int[][] xs={{5,50,20},{5,50,20},{-8,72,21},{30,30,30},{50,5,32}};
        int[][] ys={{3,13,37},{3,3,37},{-8,20,50},{3,13,37},{30,5,11}};
        int[] mx={0,64,0},my={0,0,64},mz=mapping==0?
            new int[]{128,128,128}:new int[]{96,160,192};
        int[] c=new int[22];
        c[0]=65;c[1]=43;c[2]=shape==2?1:0;
        for(int j=0;j<3;j++) {
            c[3+j]=ys[shape][p[j]]; c[6+j]=xs[shape][p[j]];
            c[9+j]=shades[p[j]]; c[12+j]=mx[p[j]];
            c[15+j]=my[p[j]]; c[18+j]=mz[p[j]];
        }
        c[21]=0x136225 ^ serial*0x035D9F3B;
        return c;
    }
    public static void main(String[] args) throws Exception {
        byte[] bytes=Files.readAllBytes(Paths.get(args[0]));
        int[] ids={0,17,24,49};
        int[][] shades={{0,64,127},{64,64,64},{127,32,95}};
        int[][] perms={{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};
        int cases=0;long words=0,changed=0;
        try(DataOutputStream out=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[1])))) {
            out.writeInt(0x50335432); // P3T2
            for(int detail=0;detail<2;detail++) {
                Pix3D.lowDetail=detail==0;
                Pix3D.texelPool=null; Pix3D.poolSize=0; Pix3D.cycle=0;
                Pix3D.textures=new Pix8[50]; Pix3D.texturePalette=new int[50][];
                Pix3D.textureTranslucent=new boolean[50]; Pix3D.activeTexels=new int[50][];
                Pix3D.textureCycle=new int[50]; Pix3D.averageTextureRGB=new int[50];
                Pix3D.colourTable=new int[65536];
                Pix3D.unpackTextures((byte)2,new Jagfile(bytes,false));
                seed(4225+detail);
                Pix3D.setBrightness(true,0.8D);
                Pix3D.initPool(20,-20);
                out.writeInt(detail);out.writeInt(ids.length);
                for(int id:ids) {
                    int[] texels=Pix3D.getTexels(id).clone();
                    out.writeInt(id);out.writeInt(Pix3D.textureTranslucent[id]?0:1);
                    out.writeInt(texels.length);
                    for(int value:texels) out.writeInt(value);
                    ArrayList<int[]> set=new ArrayList<int[]>();
                    for(int shape=0;shape<5;shape++) for(int[] p:perms)
                        for(int[] shade:shades) for(int mapping=0;mapping<2;mapping++)
                            set.add(one(shape,p,shade,mapping,cases+set.size()));
                    out.writeInt(set.size());
                    for(int[] c:set) {
                        int[] data=new int[c[0]*c[1]+8];
                        for(int i=0;i<data.length;i++) data[i]=(i*0x045D9F3B ^ c[21])&0xFFFFFF;
                        Pix2D.bind(c[0],data,-1,c[1]);
                        Pix3D.init3D(c[1],c[0],0);
                        Pix3D.hclip=c[2]!=0;
                        try {
                            Pix3D.textureTriangle(c[3],c[4],c[5],c[6],c[7],c[8],
                                c[9],c[10],c[11],c[12],c[13],c[14],c[15],c[16],c[17],
                                c[18],c[19],c[20],id);
                        } catch(RuntimeException ex) {
                            throw new RuntimeException("detail="+detail+" id="+id+" case="+Arrays.toString(c),ex);
                        }
                        for(int value:c) out.writeInt(value);
                        out.writeInt(data.length);
                        for(int i=0;i<data.length;i++) {
                            out.writeInt(data[i]);
                            if(data[i]!=((i*0x045D9F3B ^ c[21])&0xFFFFFF)) changed++;
                        }
                        cases++;words+=data.length;
                    }
                }
            }
        }
        System.out.println("Pix3DTextureTriangleOracle: "+cases+" cases, "+words+" framebuffer/guard words, "+changed+" changed pixels");
    }
}
