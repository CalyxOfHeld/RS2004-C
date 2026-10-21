import java.io.*;
import jagex2.dash3d.CollisionMap;
public final class CollisionOracle {
    public static void main(String[] args) throws Exception {
        DataOutputStream out=new DataOutputStream(new FileOutputStream(args[0]));
        CollisionMap map=new CollisionMap(104,-1,104);
        for(int shape=0;shape<4;shape++) for(int r=0;r<4;r++) for(int range=0;range<2;range++) {
            map.reset((byte)74);map.addWall(true,r,50,50,range!=0,shape);
            for(int x=49;x<=51;x++) for(int z=49;z<=51;z++) out.writeInt(map.flags[x][z]);
            map.delWall(range!=0,r,50,50,1,shape);
            for(int x=49;x<=51;x++) for(int z=49;z<=51;z++) out.writeInt(map.flags[x][z]);
        }
        int[] flags={0,1,2,4,8,16,32,64,128,0x100,0x200000,0xffffff};
        for(int flag:flags) for(int shape=0;shape<10;shape++) for(int r=0;r<4;r++) for(int x=48;x<=52;x++) for(int z=48;z<=52;z++) {
            map.reset((byte)74);map.flags[x][z]=flag;
            out.writeBoolean(map.testWall(-7517,r,50,shape,z,50,x));
            out.writeBoolean(map.testWDecor(r,shape,27808,x,50,z,50));
        }
        for(int flag:flags) for(int force=0;force<16;force++) for(int x=48;x<=54;x++) for(int z=48;z<=54;z++) {
            map.reset((byte)74);map.flags[x][z]=flag;
            out.writeBoolean(map.testLoc(z,3,x,50,force,50,2,1));
        }
        for(int r=0;r<4;r++) for(int range=0;range<2;range++) {
            map.reset((byte)74);map.addLoc(r,3,2,50,9,50,range!=0);
            for(int x=49;x<=54;x++) for(int z=49;z<=54;z++) out.writeInt(map.flags[x][z]);
        }
        out.close();System.out.println("Original revision-225 CollisionMap oracle written");
    }
}
