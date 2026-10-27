import jagex2.graphics.*;
import jagex2.io.Jagfile;
import java.io.*;
import java.lang.reflect.Field;
import java.nio.file.*;
import java.util.*;

/** Original world draw's hover results and rendered-frame checksums. */
public final class ModelPickingOracle {
    static final int BG=0x19372b;
    static long hash(int[] pixels) {
        long h=0xcbf29ce484222325L;
        for(int v:pixels){h^=Integer.toUnsignedLong(v);h*=0x100000001b3L;}
        return h;
    }
    static List<int[]> points(Model m,int[] p) {
        List<int[]> result=new ArrayList<int[]>();
        int[][] grid={{64,64},{0,0},{-1,-1},{127,127},{128,128},{64,0},{0,64},{64,127},{127,64},{32,32},{96,32},{32,96}};
        Collections.addAll(result,grid);
        // Derive boundary INPUTS from the original model's public bounds.
        // Expected hits always come from the actual unchanged draw method.
        int sp=Model.sin[p[1]],cp=Model.cos[p[1]],sw=Model.sin[p[2]],cw=Model.cos[p[2]];
        int z=(p[5]*cw-p[3]*sw)>>16,depth=(p[4]*sp+z*cp)>>16;
        int rz=m.radius*cp>>16,far=depth+rz,near=Math.max(50,depth-rz);
        if(far<=0)return result;
        int x=(p[5]*sw+p[3]*cw)>>16,y=(p[4]*cp-z*sp)>>16,ry=m.radius*sp>>16;
        int height=ry+(m.maxY*cp>>16);
        int left=((x-m.radius)<<9)/(x>0?far:near)+64;
        int right=((x+m.radius)<<9)/(x>0?near:far)+64;
        int top=((y-height)<<9)/(y>0?far:near)+64;
        int bottom=((y+ry)<<9)/(y>0?near:far)+64;
        if(right-left>2 && bottom-top>2) {
            int mx=(left+right)/2,my=(top+bottom)/2;
            result.add(new int[]{left,my});result.add(new int[]{left+1,my});
            result.add(new int[]{right,my});result.add(new int[]{right-1,my});
            result.add(new int[]{mx,top});result.add(new int[]{mx,top+1});
            result.add(new int[]{mx,bottom});result.add(new int[]{mx,bottom-1});
        }
        return result;
    }
    public static void main(String[] args)throws Exception {
        Model.unpack(3,new Jagfile(Files.readAllBytes(Paths.get(args[0],"models")),false));
        Pix3D.lowDetail=true;Pix3D.jagged=true;
        Pix3D.unpackTextures((byte)2,new Jagfile(Files.readAllBytes(Paths.get(args[0],"textures")),false));
        Field f=Class.forName("java.lang.Math$RandomNumberGeneratorHolder").getDeclaredField("randomNumberGenerator");
        f.setAccessible(true);((Random)f.get(null)).setSeed(225);
        Pix3D.setBrightness(true,0.8);Pix3D.initPool(20,-20);
        double brightness=0.8+(new Random(225).nextDouble()*0.03-0.015);
        int[] ids={0,20,49,135,148,486,594,624};
        int[][] poses={{0,0,0,0,0,50},{256,0,0,0,40,50},{512,0,0,0,0,80},
            {768,128,0,16,40,100},{1536,128,1792,20,80,300},
            {0,0,0,0,0,500},{0,0,0,0,0,3500}};
        Map<Integer,Model> models=new HashMap<Integer,Model>();List<int[]> inputs=new ArrayList<int[]>();
        for(int id:ids) {
            Model m=new Model(false,id);m.calculateNormals(64,768,-50,-10,-50,true);models.put(id,m);
            for(int[] p:poses)for(int coarse=0;coarse<2;coarse++)for(int[] mouse:points(m,p))for(int gate=0;gate<4;gate++) {
                int n=inputs.size(),bitset=gate==2?0:gate==3?-1:0x12345000+id;
                inputs.add(new int[]{id,p[0],p[1],p[2],p[3],p[4],p[5],new int[]{0,64,192}[n%3],n&1,gate==1?0:1,coarse,mouse[0],mouse[1],bitset});
            }
        }
        int fineHits=0,coarseHits=0,disabled=0,rendered=0;
        try(DataOutputStream out=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[1])))) {
            out.writeInt(0x4d504b32);out.writeDouble(brightness);out.writeInt(inputs.size());
            for(int[] c:inputs) {
                Model m=models.get(c[0]);m.pickable=c[10]!=0;Model.checkHover=c[9]!=0;Model.mouseX=c[11];Model.mouseZ=c[12];
                Model.pickedCount=1;Model.pickedBitsets[0]=0x11223344;Model.pickedBitsets[1]=0x55667788;Model.pickedBitsets[2]=0x76543210;
                int[] pixels=new int[128*128+8];Arrays.fill(pixels,BG);
                Pix2D.bind(128,pixels,-1,128);Pix3D.init3D(128,128,0);Pix3D.trans=c[7];Pix3D.hclip=c[8]!=0;
                int[] initialX=Model.clippedX.clone(),initialY=Model.clippedY.clone(),initialC=Model.clippedColor.clone();
                Model.vertexScreenZ[0]=Integer.MIN_VALUE;
                m.draw(c[1],Model.sin[c[2]],Model.cos[c[2]],Model.sin[c[3]],Model.cos[c[3]],c[4],c[5],c[6],c[13]);
                int alpha=Pix3D.trans,clip=Pix3D.hclip?1:0;
                if(Model.vertexScreenZ[0]!=Integer.MIN_VALUE) {
                    boolean near=false;for(int v=0;v<m.vertexCount;v++)if(Model.vertexScreenX[v]==-5000)near=true;
                    int[] repeated=new int[pixels.length];Arrays.fill(repeated,BG);
                    System.arraycopy(initialX,0,Model.clippedX,0,10);System.arraycopy(initialY,0,Model.clippedY,0,10);System.arraycopy(initialC,0,Model.clippedColor,0,10);
                    Pix2D.bind(128,repeated,-1,128);Pix3D.init3D(128,128,0);Pix3D.trans=c[7];Pix3D.hclip=c[8]!=0;
                    m.draw(near,false,0);
                    if(!Arrays.equals(pixels,repeated) || alpha!=Pix3D.trans || clip!=(Pix3D.hclip?1:0))throw new AssertionError("picking render dispatch changed");
                    rendered++;
                }
                if(c[9]==0 || c[13]<=0){disabled++;if(Model.pickedCount!=1)throw new AssertionError("disabled picking appended");}
                if(Model.pickedCount==2){if(m.pickable)coarseHits++;else fineHits++;}
                if(Model.pickedCount<1 || Model.pickedCount>2 || Model.pickedBitsets[0]!=0x11223344 || Model.pickedBitsets[2]!=0x76543210)throw new AssertionError("pick list damaged");
                for(int v:c)out.writeInt(v);
                out.writeInt(m.vertexCount);out.writeInt(m.faceCount);out.writeInt(alpha);out.writeInt(clip);
                out.writeInt(Model.pickedCount);for(int j=0;j<3;j++)out.writeInt(Model.pickedBitsets[j]);out.writeLong(hash(pixels));
            }
            List<int[]> bounds=new ArrayList<int[]>();
            int[][] triangles={{10,80,20,10,20,90},{0,0,0,0,0,0},{-40,100,20,70,-10,40},{-100,-20,-80,-50,-10,-90}};
            int[][] permutations={{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};
            for(int[] t:triangles)for(int[] order:permutations) {
                int minx=Math.min(t[0],Math.min(t[1],t[2])),maxx=Math.max(t[0],Math.max(t[1],t[2]));
                int miny=Math.min(t[3],Math.min(t[4],t[5])),maxy=Math.max(t[3],Math.max(t[4],t[5]));
                int[] xs={minx-1,minx,minx+1,(minx+maxx)/2,maxx-1,maxx,maxx+1};
                int[] ys={miny-1,miny,miny+1,(miny+maxy)/2,maxy-1,maxy,maxy+1};
                for(int x:xs)for(int y:ys)bounds.add(new int[]{x,y,t[3+order[0]],t[3+order[1]],t[3+order[2]],t[order[0]],t[order[1]],t[order[2]]});
            }
            Random r=new Random(225);for(int i=0;i<1000;i++){int[] c=new int[8];for(int j=0;j<8;j++)c[j]=r.nextInt();bounds.add(c);}
            Model predicate=models.get(ids[0]);out.writeInt(bounds.size());
            for(int[] c:bounds){for(int v:c)out.writeInt(v);out.writeInt(predicate.pointWithinTriangle(c[0],c[1],c[2],c[3],c[4],c[5],c[6],c[7])?1:0);}
            System.out.println("Direct original face-bound checks: "+bounds.size());
        }
        System.out.println("ModelPickingOracle: "+inputs.size()+" cases, fine hits="+fineHits+", coarse hits="+coarseHits+", disabled="+disabled+", projected="+rendered);
        if(fineHits==0 || coarseHits==0)throw new AssertionError("missing hit coverage");
    }
}
