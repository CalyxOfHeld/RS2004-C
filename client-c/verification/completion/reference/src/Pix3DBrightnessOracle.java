import jagex2.graphics.Pix3D;
import jagex2.io.Jagfile;
import java.io.*;
import java.lang.reflect.Field;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.Random;

/** Checks revision-225 startup and later brightness change on a real texture. */
public final class Pix3DBrightnessOracle {
    private static void seed(long value) throws Exception {
        Class<?> holder=Class.forName("java.lang.Math$RandomNumberGeneratorHolder");
        Field field=holder.getDeclaredField("randomNumberGenerator");
        field.setAccessible(true);
        ((Random)field.get(null)).setSeed(value);
    }
    private static void record(DataOutputStream out,double brightness,int id) throws Exception {
        out.writeDouble(brightness);
        for(int value:Pix3D.colourTable) out.writeInt(value);
        int[] texels=Pix3D.getTexels(id);
        out.writeInt(texels.length);
        for(int value:texels) out.writeInt(value);
        out.writeInt(Pix3D.getAverageTextureRGB(1,id));
        out.writeInt(Pix3D.textureTranslucent[id]?1:0);
    }
    public static void main(String[] args) throws Exception {
        long randomSeed=225;
        seed(randomSeed);
        double first=0.8D+(Math.random()*0.03D-0.015D);
        double second=0.6D+(Math.random()*0.03D-0.015D);
        seed(randomSeed);
        Pix3D.lowDetail=true;
        Pix3D.unpackTextures((byte)2,new Jagfile(Files.readAllBytes(Paths.get(args[0])),false));
        Pix3D.setBrightness(true,0.8D);
        Pix3D.initPool(20,-20);
        try(DataOutputStream out=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[1])))) {
            out.writeInt(0x50334231); // P3B1
            out.writeInt(17);
            record(out,first,17);
            Pix3D.setBrightness(true,0.6D);
            record(out,second,17);
        }
        System.out.println("Pix3DBrightnessOracle: texture 17, startup="+first+", change="+second);
    }
}
