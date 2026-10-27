import jagex2.graphics.Pix3D;
import jagex2.io.Jagfile;
import java.io.*;
import java.lang.reflect.Field;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.Random;

/** Runs unchanged revision-225 Pix3D/Pix8 preparation against the real textures archive. */
public final class Pix3DPaletteOracle {
    private static void seedMathRandom(long seed) throws Exception {
        Class<?> holder = Class.forName("java.lang.Math$RandomNumberGeneratorHolder");
        Field field = holder.getDeclaredField("randomNumberGenerator");
        field.setAccessible(true);
        ((Random) field.get(null)).setSeed(seed);
    }

    public static void main(String[] args) throws Exception {
        seedMathRandom(225);
        Pix3D.textures = new jagex2.graphics.Pix8[50];
        Pix3D.texturePalette = new int[50][];
        Pix3D.averageTextureRGB = new int[50];
        Pix3D.activeTexels = new int[50][];
        Pix3D.colourTable = new int[65536];
        Jagfile archive = new Jagfile(Files.readAllBytes(Paths.get(args[0])), false);
        Pix3D.unpackTextures((byte) 2, archive);
        double unit = Math.random();
        double effectiveBrightness = 0.8D + (unit * 0.03D - 0.015D);
        // setBrightness consumes that same first random value.
        seedMathRandom(225);
        Pix3D.setBrightness(true, 0.8D);
        try (DataOutputStream out = new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[1])))) {
            out.writeInt(0x50335031); // P3P1
            out.writeDouble(effectiveBrightness);
            for (int value : Pix3D.colourTable) out.writeInt(value);
            int count = 0;
            for (jagex2.graphics.Pix8 image : Pix3D.textures) if (image != null) count++;
            out.writeInt(count);
            for (int id = 0; id < 50; id++) {
                jagex2.graphics.Pix8 image = Pix3D.textures[id];
                if (image == null) continue;
                out.writeInt(id);
                out.writeInt(image.width); out.writeInt(image.height);
                out.writeInt(image.palette.length);
                for (int value : image.palette) out.writeInt(value);
                out.writeInt(image.pixels.length);
                for (byte value : image.pixels) out.writeByte(value);
                out.writeInt(Pix3D.getAverageTextureRGB(1, id));
            }
        }
        System.out.println("Pix3DPaletteOracle: " + Pix3D.textureCount + " textures, brightness=" + effectiveBrightness);
    }
}
