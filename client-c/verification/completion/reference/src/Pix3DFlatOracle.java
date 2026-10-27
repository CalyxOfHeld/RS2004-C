import jagex2.graphics.Pix2D;
import jagex2.graphics.Pix3D;
import java.io.*;
import java.util.*;

/** Calls the unchanged bundled revision-225 methods; no rasterizer is copied here. */
public final class Pix3DFlatOracle {
    static final List<int[]> cases = new ArrayList<int[]>();
    // mode,w,h,left,top,right,bottom,trans,hclip,rgb,x0,x1,x2,y0,y1,y2,base,seed
    static void add(int mode, int w, int h, int left, int top, int right, int bottom,
                    int trans, boolean clip, int rgb, int[] x, int[] y, int base) {
        cases.add(new int[]{mode,w,h,left,top,right,bottom,trans,clip?1:0,rgb,
                x[0],x[1],x[2],y[0],y[1],y[2],base,0x12345678 ^ cases.size()});
    }
    public static void main(String[] args) throws Exception {
        int[][] xs = {{8,2,26},{2,26,8},{26,8,2},{2,26,8},{8,8,8},
                {-20,9,55},{3,26,17},{3,26,17},{0,32,16},{32760,-32760,17}};
        int[][] ys = {{2,19,19},{2,2,19},{2,10,19},{7,7,7},{2,10,19},
                {-9,12,45},{-8,-3,-1},{30,40,50},{0,0,24},{-8,10,30}};
        int[][] permutations = {{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};
        int[] alphas = {0,1,64,128,255,256};
        for (int shape=0; shape<xs.length; shape++) for (int[] p: permutations)
            for (int alpha: alphas) for (int clipping=0; clipping<2; clipping++) {
                boolean safe = shape != 5 && shape != 9;
                if (clipping == 0 && !safe) continue;
                int[] x = {xs[shape][p[0]],xs[shape][p[1]],xs[shape][p[2]]};
                int[] y = {ys[shape][p[0]],ys[shape][p[1]],ys[shape][p[2]]};
                add(1,33,25,4,3,31,22,alpha,clipping!=0,0xE357A9,x,y,0);
            }
        // Every four-pixel unrolling remainder, empty/reversed spans and clip boundaries.
        for (int alpha: alphas) for (int len=-2;len<=20;len++) for(int clipping=0;clipping<2;clipping++)
            add(0,33,5,4,2,29,4,alpha,clipping!=0,0x1973DF,
                new int[]{clipping!=0?-3:3,(clipping!=0?-3:3)+len,0},new int[3],33);
        Random random = new Random(225);
        for (int i=0;i<4096;i++) {
            int w=9+random.nextInt(72), h=9+random.nextInt(48);
            boolean clip=(i&1)!=0;
            int[] x=new int[3],y=new int[3];
            for(int j=0;j<3;j++) {
                x[j]=clip?random.nextInt(w*3)-w:random.nextInt(w);
                y[j]=random.nextInt(h*3)-h;
            }
            add(1,w,h,random.nextInt(4),random.nextInt(4),w-random.nextInt(4),
                h-random.nextInt(4),alphas[i%alphas.length],clip,random.nextInt(0x1000000),x,y,0);
        }
        for(int[] size: new int[][]{{512,334},{765,503}}) for(int alpha: alphas)
            add(1,size[0],size[1],0,0,size[0],size[1],alpha,true,0xC47127,
                new int[]{-40,size[0]+71,size[0]/2},new int[]{-17,size[1]/2,size[1]+42},0);
        try(DataOutputStream out=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[0])))) {
            out.writeInt(0x50334631); // P3F1, big endian
            out.writeInt(cases.size());
            long pixels=0;
            for(int index=0;index<cases.size();index++) {
                int[] c=cases.get(index);
                int[] data=new int[c[1]*c[2]+8]; // neighbor read and untouched guard words
                for(int i=0;i<data.length;i++) data[i]=(i*0x045D9F3B ^ c[17]) & 0xFFFFFF;
                Pix2D.bind(c[1],data,-1,c[2]);
                Pix2D.setClipping(c[6],c[4],c[5],789,c[3]);
                Pix3D.init2D(-1);
                Pix3D.trans=c[7]; Pix3D.hclip=c[8]!=0;
                if(c[0]==0) Pix3D.flatRaster(data,c[16],c[9],0,c[10],c[11]);
                else Pix3D.flatTriangle(c[13],c[14],c[15],c[10],c[11],c[12],c[9]);
                for(int v:c) out.writeInt(v);
                out.writeInt(data.length);
                for(int v:data) out.writeInt(v);
                pixels+=data.length;
            }
            System.out.println("Pix3DFlatOracle: "+cases.size()+" cases, "+pixels+" framebuffer/guard words");
        }
    }
}
