import jagex2.dash3d.World3D;
import jagex2.dash3d.type.*;
import java.io.*;
import java.util.*;

/** Calls unchanged revision-225 setTile; inputs retain real-map provenance. */
public final class TileConstructionOracle {
    static final class Input { int x,z; int[] h,c; int rgb,texture; }
    static List<Input> read(String path)throws Exception {
        List<Input> result=new ArrayList<Input>();
        try(DataInputStream in=new DataInputStream(new BufferedInputStream(new FileInputStream(path)))) {
            if(in.readInt()!=0x47323235)throw new IOException("expected G225");
            in.readDouble();if(in.readInt()<80)throw new IOException("missing real tile families");
            for(int n=0;n<80;n++) {
                int kind=in.readInt();Input p=new Input();p.x=in.readInt();p.z=in.readInt();p.h=new int[4];
                for(int j=0;j<4;j++)p.h[j]=in.readInt();
                if(kind==0){p.c=new int[4];for(int j=0;j<4;j++)p.c[j]=in.readInt();p.texture=in.readInt();p.rgb=in.readInt();in.readInt();}
                else if(kind==1){
                    int[] header=new int[8];for(int j=0;j<8;j++)header[j]=in.readInt();
                    for(int j=0;j<header[6]*3+header[7]*7;j++)in.readInt();
                }else throw new IOException("bad tile kind");
                result.add(p);
            }
        }
        return result;
    }
    static void ints(DataOutputStream out,int... a)throws Exception {for(int v:a)out.writeInt(v);}
    static void emit(DataOutputStream out,World3D scene,boolean reset,int[] a)throws Exception {
        if(reset)scene.levelTiles[a[0]][a[1]][a[2]]=null;
        scene.setTile(a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8],a[9],a[10],a[11],a[12],a[13],a[14],a[15],a[16],a[17],a[18],a[19]);
        out.writeInt(reset?1:0);ints(out,a);
        Ground g=scene.levelTiles[a[0]][a[1]][a[2]];TileUnderlay u=g.underlay;TileOverlay o=g.overlay;
        ints(out,u==null?0:1,o==null?0:1);
        if(u!=null)ints(out,u.southwestColor,u.southeastColor,u.northeastColor,u.northwestColor,u.textureId,u.rgb,u.flat?1:0);
        if(o!=null){
            ints(out,o.shape,o.rotation,o.backgroundRgb,o.foregroundRgb,o.flat?1:0,o.triangleTextureIds==null?0:1,o.vertexX.length,o.triangleVertexA.length);
            for(int j=0;j<o.vertexX.length;j++)ints(out,o.vertexX[j],o.vertexY[j],o.vertexZ[j]);
            for(int j=0;j<o.triangleVertexA.length;j++)ints(out,o.triangleVertexA[j],o.triangleVertexB[j],o.triangleVertexC[j],o.triangleColorA[j],o.triangleColorB[j],o.triangleColorC[j],o.triangleTextureIds==null?-1:o.triangleTextureIds[j]);
        }
    }
    public static void main(String[] args)throws Exception {
        List<Input> inputs=read(args[0]);Input plain=null,textured=null;
        for(Input p:inputs)if(p.c!=null){if(p.texture==-1&&plain==null)plain=p;if(p.texture>=0&&textured==null)textured=p;}
        if(plain==null||textured==null)throw new AssertionError("missing cache prototypes");
        World3D scene=new World3D(1,new int[4][105][105],104,4,104);
        int count=80*13*4*2+8*13*4+4*13;
        try(DataOutputStream out=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[1])))) {
            ints(out,0x54323235,count);int n=0;
            for(Input p:inputs)for(int shape=0;shape<13;shape++)for(int rot=0;rot<4;rot++)for(int tex=0;tex<2;tex++) {
                int[] c=plain.c,d=tex==0?plain.c:textured.c,h=p.h;
                emit(out,scene,true,new int[]{n++%4,p.x,p.z,shape,rot,tex==0?-1:textured.texture,h[0],h[1],h[2],h[3],c[0],c[1],c[2],c[3],d[0],d[1],d[2],d[3],plain.rgb,textured.rgb});
            }
            // Defined Java signed overflow and odd negative midpoint rounding.
            int[] edges={Integer.MIN_VALUE,Integer.MAX_VALUE,-12345679,-3,-1,0,1,12345678};
            for(int v:edges)for(int shape=0;shape<13;shape++)for(int rot=0;rot<4;rot++) {
                emit(out,scene,true,new int[]{n++%4,0,103,shape,rot,46,v,v+1,v+2,v+3,v,v+1,v+2,v+3,v+3,v+2,v+1,v,0x123456,0xabcdef});
            }
            // Both references survive an update of the other tile layer.
            for(int level=0;level<4;level++)for(int step=0;step<13;step++) {
                int shape=new int[]{0,2,1,3,0,4,1,5,6,7,8,9,12}[step];
                int[] h=inputs.get(0).h,c=plain.c,d=textured.c;
                emit(out,scene,step==0,new int[]{level,103,0,shape,shape%4,shape%2==0?-1:textured.texture,h[0],h[1],h[2],h[3],c[0],c[1],c[2],c[3],d[0],d[1],d[2],d[3],plain.rgb,textured.rgb});n++;
            }
            if(n!=count)throw new AssertionError("count");
            System.out.println("TileConstructionOracle: "+count+" setTile calls: 8,320 real-map combinations, 416 signed boundary cases, 52 retained-layer updates");
        }
    }
}
