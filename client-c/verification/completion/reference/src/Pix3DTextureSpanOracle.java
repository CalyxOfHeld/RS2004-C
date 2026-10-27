import jagex2.graphics.Pix2D;
import jagex2.graphics.Pix3D;
import jagex2.graphics.Pix8;
import jagex2.io.Jagfile;
import java.io.*;
import java.lang.reflect.Field;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.*;

/** Calls unchanged revision-225 getTexels and textureRaster on real cache textures. */
public final class Pix3DTextureSpanOracle {
    private static void seed(long value) throws Exception {
        Class<?> holder=Class.forName("java.lang.Math$RandomNumberGeneratorHolder");
        Field field=holder.getDeclaredField("randomNumberGenerator");
        field.setAccessible(true);
        ((Random)field.get(null)).setSeed(value);
    }
    private static int[] caseArgs(int w,int clip,int shade,int variant,int serial) {
        int left=clip!=0?-3:1, right=clip!=0?w+7:w-1;
        int den=variant==0?4:7;
        int scale=Pix3D.lowDetail?4096:16384;
        int a=den*(variant==0?1000:2700);
        int b=den*(variant==0?1300:3300);
        int c=den*scale;
        int da=variant==0?128:-160, db=variant==0?72:112, dc=variant==0?0:64;
        int seed=0x12345678 ^ (serial*0x31531);
        // w,h,left,right,hclip,shadeA,shadeB,a,b,c,da,db,dc,offset,seed
        return new int[]{w,3,left,right,clip,shade,shade+(variant==0?1000:-1300),
            a,b,c,da,db,dc,w,seed};
    }
    public static void main(String[] args) throws Exception {
        byte[] bytes=Files.readAllBytes(Paths.get(args[0]));
        int[] ids={0,17,24,49};
        int[] shades={0,2048,4096,8192,12288,16384,32768,49152};
        long words=0; int cases=0;
        try(DataOutputStream out=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[1])))) {
            out.writeInt(0x50335431); // P3T1
            for(int detail=0;detail<2;detail++) {
                Pix3D.lowDetail=detail==0;
                Pix3D.texelPool=null;
                Pix3D.poolSize=0;
                Pix3D.textures=new Pix8[50];
                Pix3D.texturePalette=new int[50][];
                Pix3D.textureTranslucent=new boolean[50];
                Pix3D.activeTexels=new int[50][];
                Pix3D.textureCycle=new int[50];
                Pix3D.averageTextureRGB=new int[50];
                Pix3D.colourTable=new int[65536];
                Pix3D.cycle=0;
                Pix3D.unpackTextures((byte)2,new Jagfile(bytes,false));
                seed(225+detail);
                Pix3D.setBrightness(true,0.8D);
                Pix3D.initPool(20,-20);
                out.writeInt(detail);
                out.writeInt(ids.length);
                for(int id:ids) {
                    int[] texels=Pix3D.getTexels(id).clone();
                    boolean opaque=!Pix3D.textureTranslucent[id];
                    out.writeInt(id); out.writeInt(opaque?1:0);
                    out.writeInt(texels.length);
                    for(int texel:texels) out.writeInt(texel);
                    ArrayList<int[]> set=new ArrayList<int[]>();
                    for(int w:new int[]{17,33,127}) for(int clip=0;clip<2;clip++)
                        for(int shade:shades) for(int variant=0;variant<2;variant++)
                            set.add(caseArgs(w,clip,shade,variant,cases+set.size()));
                    out.writeInt(set.size());
                    for(int[] c:set) {
                        int[] data=new int[c[0]*c[1]+8];
                        for(int i=0;i<data.length;i++) data[i]=(i*0x045D9F3B ^ c[14])&0xFFFFFF;
                        Pix2D.bind(c[0],data,-1,c[1]);
                        Pix3D.init3D(c[1],c[0],0);
                        Pix3D.hclip=c[4]!=0; Pix3D.opaque=opaque;
                        try {
                            Pix3D.textureRaster(data,texels,0,0,c[13],c[2],c[3],c[5],c[6],
                                c[7],c[8],c[9],c[10],c[11],c[12]);
                        } catch(RuntimeException ex) {
                            throw new RuntimeException("detail="+detail+" texture="+id+" case="+Arrays.toString(c),ex);
                        }
                        for(int value:c) out.writeInt(value);
                        out.writeInt(data.length);
                        for(int value:data) out.writeInt(value);
                        words+=data.length; cases++;
                    }
                }
            }
        }
        System.out.println("Pix3DTextureSpanOracle: "+cases+" real-cache span cases, "+words+" framebuffer/guard words");
    }
}
