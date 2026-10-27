import jagex2.dash3d.World3D;
import jagex2.graphics.Model;
import java.io.*;
import java.util.*;

/** Original init/testPoint calls, no copied projection or matrix algorithm. */
public final class VisibilityOracle {
    static void ints(DataOutputStream out,int... values)throws Exception {for(int v:values)out.writeInt(v);}
    static int[] state(){return new int[]{World3D.viewportLeft,World3D.viewportTop,World3D.viewportRight,World3D.viewportBottom,World3D.viewportCenterX,World3D.viewportCenterY,World3D.sinEyeYaw,World3D.cosEyeYaw,World3D.sinEyePitch,World3D.cosEyePitch};}
    static void point(DataOutputStream out,int[] viewport,int yaw,int pitch,int x,int z,int y)throws Exception {
        World3D.viewportLeft=viewport[0];World3D.viewportTop=viewport[1];World3D.viewportRight=viewport[2];World3D.viewportBottom=viewport[3];World3D.viewportCenterX=viewport[4];World3D.viewportCenterY=viewport[5];
        World3D.sinEyeYaw=Model.sin[yaw];World3D.cosEyeYaw=Model.cos[yaw];World3D.sinEyePitch=Model.sin[pitch];World3D.cosEyePitch=Model.cos[pitch];
        ints(out,state());ints(out,x,z,y,World3D.testPoint(x,z,y,-268)?1:0);
    }
    public static void main(String[] args)throws Exception {
        List<TileConstructionOracle.Input> real=TileConstructionOracle.read(args[0]);
        int[][] configs={{800,512,334,500,0,0},{800,765,503,500,1,0},{128,64,64,128,2,1},{0,512,334,0,0,2},{800,512,334,500,1,3},{500,0,0,800,2,0}};
        int pointCount=80*8*4+6*5*5+7*3+4096;
        try(DataOutputStream out=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[1])))) {
            ints(out,0x56323235,configs.length);long ones=0;
            for(int[] config:configs){
                int above=config[0],width=config[1],height=config[2],below=config[3],initial=config[4],profile=config[5];
                if(initial!=2)for(boolean[][][] p:World3D.visibilityMatrix)for(boolean[][] yaw:p)for(boolean[] x:yaw)Arrays.fill(x,initial==1);
                int[] distance=new int[9];
                for(int i=0;i<9;i++){
                    int angle=i*32+128+15;distance[i]=(angle*3+600)*Model.sin[angle]>>16;
                    if(profile==1)distance[i]=real.get(i).h[i%4];
                    if(profile==2)distance[i]=(i%2==0?Integer.MIN_VALUE:Integer.MAX_VALUE);
                    if(profile==3)distance[i]=0;
                }
                ints(out,initial,above,width,height,below);ints(out,distance);
                World3D.init(distance,above,width,(byte)15,height,below);ints(out,state());
                for(boolean[][][] p:World3D.visibilityMatrix)for(boolean[][] yaw:p)for(boolean[] x:yaw)for(boolean value:x){out.writeByte(value?1:0);if(value)ones++;}
            }
            ints(out,pointCount);int written=0;int[] viewport={0,0,512,334,256,167};
            for(TileConstructionOracle.Input p:real)for(int direction=0;direction<8;direction++)for(int step=0;step<4;step++){
                point(out,viewport,direction*256,128+step*64,(p.x-50)*128,(p.z-50)*128+step*128,p.h[step]+600);written++;
            }
            int[] square={0,0,512,512,256,256};
            for(int depth:new int[]{49,50,51,3499,3500,3501})for(int x:new int[]{-26,-25,0,25,26})for(int y:new int[]{-26,-25,0,25,26}){point(out,square,0,0,x,depth,y);written++;}
            for(int x:new int[]{65536,65537,-65536,131072,-131072,Integer.MIN_VALUE,Integer.MAX_VALUE})for(int depth:new int[]{50,1000,3500}){point(out,square,0,0,x,depth,0);written++;}
            Random random=new Random(225);
            int[][] views={viewport,square,{-100,-80,400,300,150,110},{0,0,0,0,0,0},{Integer.MIN_VALUE,Integer.MIN_VALUE,Integer.MAX_VALUE,Integer.MAX_VALUE,Integer.MAX_VALUE,Integer.MIN_VALUE}};
            for(int i=0;i<4096;i++){
                int x=i<2048?random.nextInt(16001)-8000:random.nextInt(),z=i<2048?random.nextInt(16001)-8000:random.nextInt(),y=i<2048?random.nextInt(16001)-8000:random.nextInt();
                point(out,views[i%views.length],random.nextInt(2048),random.nextInt(2048),x,z,y);written++;
            }
            if(written!=pointCount)throw new AssertionError("point count");
            System.out.println("VisibilityOracle: "+configs.length+" matrices / "+(configs.length*8*32*51*51)+" booleans ("+ones+" true), "+written+" point queries; real-map, viewport/depth boundaries, signed random inputs and retained edges");
        }
    }
}
