import jagex2.graphics.Model;
import jagex2.graphics.Pix2D;
import jagex2.graphics.Pix3D;
import jagex2.io.Jagfile;
import java.io.*;
import java.lang.reflect.Field;
import java.nio.file.*;
import java.util.*;

/** Calls the unchanged revision-225 world-space Model.draw. No picking. */
public final class ModelWorldOracle {
    private static void seed() throws Exception {
        Field f=Class.forName("java.lang.Math$RandomNumberGeneratorHolder").getDeclaredField("randomNumberGenerator");
        f.setAccessible(true);((Random)f.get(null)).setSeed(225);
    }
    public static void main(String[] args) throws Exception {
        byte[] models=Files.readAllBytes(Paths.get(args[0],"models"));
        byte[] textures=Files.readAllBytes(Paths.get(args[0],"textures"));
        Model.unpack(3,new Jagfile(models,false));Model.checkHover=false;
        int[] ids={0,20,44,49,54,135,148,486,570,594,624};
        // model yaw, eye pitch/yaw, dx/dy/dz; close views cross z=50.
        int[][] poses={{0,0,0,0,0,50},{256,0,0,0,40,50},
            {512,0,0,0,0,80},{768,128,0,16,40,100},
            {1024,256,256,-20,90,100},{1536,128,1792,20,80,300},
            {256,0,512,-500,20,0},{0,0,0,0,0,500},
            {0,0,0,0,0,3500},{0,0,0,0,0,-500},
            {0,0,0,1500,0,500},{0,0,0,0,-1500,500}};
        int caseCount=ids.length*poses.length*3,detail=-1,n=0,empty=0,culled=0;
        long changed=0;int[] nearTriangles=new int[4],nearQuads=new int[4];
        double brightness=0.8+(new Random(225).nextDouble()*0.03-0.015);
        try(DataOutputStream out=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[1])))) {
            out.writeInt(0x4d575231);out.writeDouble(brightness);out.writeInt(caseCount);
            for(int mode=0;mode<3;mode++) for(int id:ids) for(int[] p:poses) {
                int low=mode==1?0:1,jagged=mode==2?0:1;
                if(detail!=low) {
                    detail=low;Pix3D.lowDetail=low!=0;Pix3D.clearTexels(false);
                    Pix3D.unpackTextures((byte)2,new Jagfile(textures,false));
                    seed();Pix3D.setBrightness(true,0.8);Pix3D.initPool(20,-20);
                }
                Pix3D.jagged=jagged!=0;
                Model m=new Model(false,id);m.calculateNormals(64,768,-50,-10,-50,true);
                int initialAlpha=new int[]{0,64,192}[n%3];boolean initialClip=(n&1)!=0;
                Pix3D.trans=initialAlpha;Pix3D.hclip=initialClip;
                int[] clipX=Model.clippedX.clone(),clipY=Model.clippedY.clone(),clipC=Model.clippedColor.clone();
                int[] pixels=new int[128*128+8];Arrays.fill(pixels,0x19372b);
                Pix2D.bind(128,pixels,-1,128);Pix3D.init3D(128,128,0);
                Model.vertexScreenZ[0]=Integer.MIN_VALUE;
                m.draw(p[0],Model.sin[p[1]],Model.cos[p[1]],Model.sin[p[2]],Model.cos[p[2]],p[3],p[4],p[5],0);
                int finalAlpha=Pix3D.trans;boolean finalClip=Pix3D.hclip;
                boolean projected=Model.vertexScreenZ[0]!=Integer.MIN_VALUE;
                boolean near=false;
                if(projected) {
                    for(int v=0;v<m.vertexCount;v++) if(Model.vertexScreenX[v]==-5000) near=true;
                    for(int d=0;d<m.maxDepth;d++) for(int j=0;j<Model.tmpDepthFaceCount[d];j++) {
                        int f=Model.tmpDepthFaces[d][j];if(!Model.faceNearClipped[f]) continue;
                        int front=(Model.vertexViewSpaceZ[m.faceVertexA[f]]>=50?1:0)+
                            (Model.vertexViewSpaceZ[m.faceVertexB[f]]>=50?1:0)+(Model.vertexViewSpaceZ[m.faceVertexC[f]]>=50?1:0);
                        int type=m.faceInfo==null?0:m.faceInfo[f]&3;
                        if(front==1)nearTriangles[type]++;if(front==2)nearQuads[type]++;
                    }
                    // World draw also swallows dispatch errors. Repeat its
                    // final draw outside that catch from the same initial
                    // alpha/clip/scratch state, and require identical output.
                    int[] endX=Model.clippedX.clone(),endY=Model.clippedY.clone(),endC=Model.clippedColor.clone();
                    System.arraycopy(clipX,0,Model.clippedX,0,10);System.arraycopy(clipY,0,Model.clippedY,0,10);System.arraycopy(clipC,0,Model.clippedColor,0,10);
                    Pix3D.trans=initialAlpha;Pix3D.hclip=initialClip;
                    int[] repeated=new int[pixels.length];Arrays.fill(repeated,0x19372b);
                    Pix2D.bind(128,repeated,-1,128);Pix3D.init3D(128,128,0);
                    m.draw(near,false,0);
                    if(!Arrays.equals(pixels,repeated) || finalAlpha!=Pix3D.trans || finalClip!=Pix3D.hclip ||
                       !Arrays.equals(endX,Model.clippedX) || !Arrays.equals(endY,Model.clippedY) || !Arrays.equals(endC,Model.clippedColor))
                        throw new AssertionError("world dispatch changed at case "+n);
                } else culled++;
                int[] input={id,p[0],p[1],p[2],p[3],p[4],p[5],low,jagged,initialAlpha,initialClip?1:0};
                for(int value:input)out.writeInt(value);
                out.writeInt(m.vertexCount);out.writeInt(m.faceCount);
                out.writeInt(finalAlpha);out.writeInt(finalClip?1:0);out.writeInt(pixels.length);
                long before=changed;for(int value:pixels){out.writeInt(value);if(value!=0x19372b)changed++;}
                if(before==changed)empty++;
                System.out.println("case="+n+" model="+id+" projected="+projected+" near="+near+" alpha="+initialAlpha+"->"+finalAlpha);
                n++;
            }
        }
        System.out.println("ModelWorldOracle: "+n+" cases, "+changed+" changed words, "+empty+" empty views, "+culled+" culled");
        System.out.println("Near candidates by type 0/1/2/3: triangles="+Arrays.toString(nearTriangles)+" quads="+Arrays.toString(nearQuads));
        for(int t=0;t<4;t++)if(nearTriangles[t]==0 || nearQuads[t]==0)throw new AssertionError("missing near face type "+t);
    }
}
