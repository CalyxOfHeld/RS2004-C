import jagex2.graphics.Pix2D;
import jagex2.graphics.Pix3D;
import java.io.*;
import java.util.*;

/** Executes the unchanged bundled revision-225 Gouraud methods. */
public final class Pix3DGouraudOracle {
    static final List<int[]> cases = new ArrayList<int[]>();
    // mode,w,h,left,top,right,bottom,trans,hclip,jagged,x0..2,y0..2,c0..2,base,seed
    static void add(int mode,int w,int h,int alpha,boolean clip,boolean jagged,
                    int[] x,int[] y,int[] colour,int base) {
        cases.add(new int[]{mode,w,h,2,3,w-2,h-2,alpha,clip?1:0,jagged?1:0,
            x[0],x[1],x[2],y[0],y[1],y[2],colour[0],colour[1],colour[2],base,
            0x12345678 ^ cases.size()});
    }
    public static void main(String[] args) throws Exception {
        int[][] xs={{8,2,26},{2,26,8},{26,8,2},{2,26,8},{8,8,8},
            {-20,9,55},{3,26,17},{3,26,17},{0,32,16},{32760,-32760,17}};
        int[][] ys={{2,19,19},{2,2,19},{2,10,19},{7,7,7},{2,10,19},
            {-9,12,45},{-8,-3,-1},{30,40,50},{0,0,24},{-8,10,30}};
        int[][] permutations={{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};
        int[] alphas={0,1,64,128,255,256};
        int[] colours={4096,32768,61440};
        for(int shape=0;shape<xs.length;shape++) for(int[] p:permutations)
            for(int alpha:alphas) for(int clip=0;clip<2;clip++) for(int jagged=0;jagged<2;jagged++) {
                if(clip==0 && (shape==5 || shape==9)) continue;
                int[] x=new int[3],y=new int[3],c=new int[3];
                for(int j=0;j<3;j++) { x[j]=xs[shape][p[j]]; y[j]=ys[shape][p[j]];
                    c[j]=shape==9?32768:colours[p[j]]; }
                add(1,33,25,alpha,clip!=0,jagged!=0,x,y,c,0);
            }
        for(int alpha:alphas) for(int len=-2;len<=25;len++)
            for(int clip=0;clip<2;clip++) for(int jagged=0;jagged<2;jagged++) for(int direction=0;direction<2;direction++) {
                int left=clip!=0?-3:3;
                int a=(direction==0?12000:52000)*256+127;
                int b=(direction==0?52000:12000)*256+63;
                add(0,35,6,alpha,clip!=0,jagged!=0,new int[]{left,left+len,0},
                    new int[3],new int[]{a,b,0},35);
            }
        Random random=new Random(225);
        for(int i=0;i<1536;i++) {
            int w=9+random.nextInt(72),h=9+random.nextInt(48);
            boolean clip=(i&1)!=0;
            int[] x=new int[3],y=new int[3],c=new int[3];
            for(int j=0;j<3;j++) { x[j]=clip?random.nextInt(w*3)-w:random.nextInt(w);
                y[j]=random.nextInt(h*3)-h; c[j]=4096+random.nextInt(57344); }
            for(int jagged=0;jagged<2;jagged++)
                add(1,w,h,alphas[i%alphas.length],clip,jagged!=0,x,y,c,0);
        }
        for(int alpha:alphas) for(int jagged=0;jagged<2;jagged++) for(int shade:new int[]{0,65535})
            add(1,33,25,alpha,true,jagged!=0,new int[]{-4,39,17},new int[]{-8,10,29},
                new int[]{shade,shade,shade},0);
        // Reciprocal-table last valid group indices, and real viewport dimensions.
        for(int w:new int[]{512,765,2047}) for(int alpha:alphas) for(int jagged=0;jagged<2;jagged++) {
            add(0,w,6,alpha,false,jagged!=0,new int[]{0,w-1,0},new int[3],
                new int[]{32000*256+255,34000*256,0},w);
            if(w<2047) add(1,w,334,alpha,true,jagged!=0,new int[]{-40,w+71,w/2},
                new int[]{-17,167,376},new int[]{8000,56000,28000},0);
        }
        try(DataOutputStream out=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[0])))) {
            out.writeInt(0x50334731); // P3G1, big endian
            for(int i=0;i<65536;i++) {
                Pix3D.colourTable[i]=(i*0x001F123B ^ (i>>>3)) & 0xFFFFFF;
                out.writeInt(Pix3D.colourTable[i]);
            }
            out.writeInt(cases.size()); long words=0;
            for(int index=0;index<cases.size();index++) {
                int[] c=cases.get(index),data=new int[c[1]*c[2]+8];
                for(int j=0;j<data.length;j++) data[j]=(j*0x045D9F3B ^ c[20]) & 0xFFFFFF;
                Pix2D.bind(c[1],data,-1,c[2]);
                Pix2D.setClipping(c[6],c[4],c[5],789,c[3]); Pix3D.init2D(-1);
                Pix3D.trans=c[7]; Pix3D.hclip=c[8]!=0; Pix3D.jagged=c[9]!=0;
                try {
                    if(c[0]==0) Pix3D.gouraudRaster(data,c[19],0,0,c[10],c[11],c[16],c[17]);
                    else Pix3D.gouraudTriangle(c[13],c[14],c[15],c[10],c[11],c[12],c[16],c[17],c[18]);
                } catch(RuntimeException e) { throw new RuntimeException("case "+index+" "+Arrays.toString(c),e); }
                for(int v:c) out.writeInt(v); out.writeInt(data.length);
                for(int v:data) out.writeInt(v); words+=data.length;
            }
            System.out.println("Pix3DGouraudOracle: "+cases.size()+" cases, "+words+" framebuffer/guard words");
        }
    }
}
