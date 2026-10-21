import deob.client;
import jagex2.graphics.*;
import jagex2.io.Jagfile;
import java.io.*;
import java.nio.file.*;
import java.util.Arrays;

/** Records sparse complete framebuffers from original drawOnMinimap and cache sprites. */
public final class MinimapOracle {
    public static void main(String[] args) throws Exception {
        Jagfile media=new Jagfile(Files.readAllBytes(Paths.get(args[0])),false);
        client app=new client(); app.imageMapback=new Pix8(media,"mapback",0);
        int w=app.imageMapback.width,h=app.imageMapback.height;
        int[] pixels=new int[w*h]; Pix2D.bind(w,pixels,-1,h);
        int[][] offsets={{0,0},{48,0},{50,1},{70,0},{50,50},{0,75},{-60,55},{81,0}};
        int[] yaws={0,173,512,1024,1536};
        try(DataOutputStream out=new DataOutputStream(new FileOutputStream(args[1]))) {
            out.writeInt(0x4d415032); out.writeInt(w); out.writeInt(h); out.writeInt(160);
            for(int i=0;i<2048;i++) {out.writeInt(Pix3D.sinTable[i]);out.writeInt(Pix3D.cosTable[i]);}
            for(int kind=0;kind<4;kind++) {
                Pix32 sprite=new Pix32(media,kind==3?"mapflag":"mapdots",kind==3?0:kind);
                for(int yaw:yaws) for(int[] offset:offsets) {
                    // Deliberately straddle /32 quantization boundaries.
                    int px=6400+31,pz=6400+31,tx=px+offset[0]*32+1,tz=pz+offset[1]*32+1;
                    Arrays.fill(pixels,0); app.orbitCameraYaw=yaw;
                    app.drawOnMinimap(tz/32-pz/32,4,sprite,tx/32-px/32);
                    out.writeInt(kind); out.writeInt(yaw); out.writeInt(px); out.writeInt(pz);
                    out.writeInt(tx); out.writeInt(tz);
                    int count=0; for(int rgb:pixels) if(rgb!=0) count++;
                    out.writeInt(count);
                    for(int i=0;i<pixels.length;i++) if(pixels[i]!=0) {out.writeInt(i);out.writeInt(pixels[i]);}
                }
            }
        }
        System.out.println("160 original Java minimap sprite framebuffers: "+w+"x"+h+", real mapdots/mapflag, 5 yaws, 8 offsets.");
    }
}
