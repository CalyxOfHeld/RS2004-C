import java.io.*;
import java.nio.file.*;
import jagex2.io.Jagfile;
import jagex2.graphics.*;
import java.util.Arrays;

public final class CrossOracle {
    public static void main(String[] args) throws Exception {
        Jagfile archive=new Jagfile(Files.readAllBytes(Paths.get(args[0])),false);
        int[] pixels=new int[512*334];Pix2D.bind(512,pixels,-1,334);
        int[][] points={{256,167},{8,11},{519,344}};
        try(DataOutputStream out=new DataOutputStream(new FileOutputStream(args[1]))) {
            out.writeInt(0x43525331);out.writeInt(24);
            for(int frame=0;frame<8;frame++) for(int[] point:points) {
                Arrays.fill(pixels,0);
                Pix32 sprite=new Pix32(archive,"cross",frame);
                // Original drawViewport cross position in its separate viewport buffer.
                sprite.draw(point[1]-8-11,point[0]-8-8,false);
                out.writeInt(frame);out.writeInt(point[0]);out.writeInt(point[1]);
                int count=0;for(int rgb:pixels) if(rgb!=0) count++;
                out.writeInt(count);
                for(int i=0;i<pixels.length;i++) if(pixels[i]!=0) {
                    out.writeInt((i/512+11)*789+i%512+8);out.writeInt(pixels[i]);
                }
            }
        }
        System.out.println("24 original Java click-cross sprite frames; all 8 cache sprites, centre and viewport edges.");
    }
}
