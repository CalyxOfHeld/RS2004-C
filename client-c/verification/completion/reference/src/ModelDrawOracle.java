import jagex2.graphics.Model;
import jagex2.graphics.Pix2D;
import jagex2.graphics.Pix3D;
import jagex2.io.Jagfile;
import java.io.*;
import java.lang.reflect.Field;
import java.nio.file.*;
import java.util.*;

/** Real-cache revision-225 Model.drawSimple framebuffer fixture. */
public final class ModelDrawOracle {
    private static void seed(long value) throws Exception {
        Class<?> holder=Class.forName("java.lang.Math$RandomNumberGeneratorHolder");
        Field field=holder.getDeclaredField("randomNumberGenerator");
        field.setAccessible(true);
        ((Random)field.get(null)).setSeed(value);
    }
    public static void main(String[] args) throws Exception {
        byte[] models=Files.readAllBytes(Paths.get(args[0],"models"));
        byte[] textures=Files.readAllBytes(Paths.get(args[0],"textures"));
        Model.unpack(3,new Jagfile(models,false));
        Pix3D.lowDetail=true;
        Pix3D.unpackTextures((byte)2,new Jagfile(textures,false));
        seed(225);
        Pix3D.setBrightness(true,0.8D);
        double brightness=0.8D+(new Random(225).nextDouble()*0.03D-0.015D);
        Pix3D.initPool(20,-20);
        int[][] cases={{0,0,0,0,500},{0,0,256,0,500},{1,0,0,0,500},
            {1,0,256,0,500},{2,0,0,0,500},{2,0,256,0,500}};
        boolean extended=args.length>2 && args[2].equals("--extended");
        List<int[]> inputs=new ArrayList<int[]>();
        for(int[] c:cases) inputs.add(new int[]{c[0],c[1],c[2],c[3],0,0,0,c[4],1,1});
        if(extended) {
            // IDs selected from this cache's actual face types, alpha and
            // priority-10/11 arrays; see the native checkpoint inventory.
            int[] ids={0,1,2,20,44,49,54,135,147,148,294,304,486,517,570,594,624};
            int[][] poses={{0,0,0,0,0,0,500},{0,256,0,0,0,0,500},
                {128,512,64,0,16,20,850},{0,1024,0,128,-20,350,850},
                {256,1536,128,256,0,1100,1100},{64,1792,32,0,-32,-16,700}};
            for(int mode=0;mode<3;mode++) for(int id:ids) for(int[] p:poses)
                inputs.add(new int[]{id,p[0],p[1],p[2],p[3],p[4],p[5],p[6],mode==1?0:1,mode==2?0:1});
        }
        long changed=0;int minimumZ=Integer.MAX_VALUE,empty=0;
        try(DataOutputStream out=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[1])))) {
            out.writeInt(extended?0x4D445233:0x4D445232); // MDR3: full poses/detail/jagged
            out.writeDouble(brightness);
            out.writeInt(inputs.size());
            int detail=1;
            for(int[] c:inputs) {
                if(detail!=c[8]) {
                    detail=c[8];Pix3D.lowDetail=detail!=0;
                    Pix3D.clearTexels(false);
                    Pix3D.unpackTextures((byte)2,new Jagfile(textures,false));
                    seed(225);Pix3D.setBrightness(true,0.8D);Pix3D.initPool(20,-20);
                }
                Pix3D.jagged=c[9]!=0;
                Model m=new Model(false,c[0]);
                m.calculateNormals(64,768,-50,-10,-50,true);
                System.out.println("model="+c[0]+" vertices="+m.vertexCount+
                    " faces="+m.faceCount+" textured="+m.texturedFaceCount+
                    " priorityArray="+(m.facePriority!=null)+" alphaArray="+(m.faceAlpha!=null));
                int[] pixels=new int[128*128+8];
                Arrays.fill(pixels,0x19372B);
                Pix2D.bind(128,pixels,-1,128);
                Pix3D.init3D(128,128,0);
                m.drawSimple(c[1],c[2],c[3],c[4],c[5],c[6],c[7]);
                int origin=(c[6]*Model.sin[c[4]]+c[7]*Model.cos[c[4]])>>16;
                for(int v=0;v<m.vertexCount;v++) {
                    int z=Model.vertexScreenZ[v]+origin;
                    minimumZ=Math.min(minimumZ,z);
                    if(z<50) throw new AssertionError("simple corpus crosses near plane");
                }
                // drawSimple swallows dispatch exceptions. Repeat its final
                // draw on a fresh identical surface, outside that catch, so a
                // broken oracle cannot silently emit a partial framebuffer.
                int[] repeated=new int[pixels.length];Arrays.fill(repeated,0x19372B);
                Pix2D.bind(128,repeated,-1,128);Pix3D.init3D(128,128,0);
                m.draw(false,false,0);
                if(!Arrays.equals(pixels,repeated)) throw new AssertionError("drawSimple dispatch changed");
                if(extended) for(int value:c) out.writeInt(value);
                else for(int value:new int[]{c[0],c[1],c[2],c[3],c[7]}) out.writeInt(value);
                out.writeInt(m.vertexCount);out.writeInt(m.faceCount);
                out.writeInt(pixels.length);
                long before=changed;
                for(int value:pixels) {out.writeInt(value);if(value!=0x19372B)changed++;}
                if(before==changed) empty++;
            }
        }
        System.out.println("ModelDrawOracle: "+inputs.size()+" real-cache draws, "+changed+" changed words, brightness="+brightness+", minimum z="+minimumZ+", empty="+empty);
    }
}
