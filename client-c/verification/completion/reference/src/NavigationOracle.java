import deob.client;
import jagex2.dash3d.CollisionMap;
import jagex2.dash3d.entity.PlayerEntity;
import jagex2.io.Isaac;
import jagex2.io.Packet;
import java.io.*;

/** Calls the bundled client's actual routing and minimap input methods. No server or game window. */
public final class NavigationOracle {
    public static void main(String[] args) throws Exception {
        DataOutputStream out = new DataOutputStream(new FileOutputStream(args[0]));
        out.writeInt(0x4e415631); out.writeInt(20);
        for (int id = 0; id < 20; id++) {
            client app = new client();
            CollisionMap map = new CollisionMap(104, -1, 104);
            app.levelCollisionMap[0] = map;
            app.sceneBaseTileX = app.sceneBaseTileZ = 3184;
            app.out = new Packet(1, new byte[5000]);
            app.out.random = new Isaac((byte)1, new int[]{17,31,57,99});
            int sx=50, sz=50, dx=55, dz=50, type=0, mx=655, my=89, yaw=0;
            boolean nearest=true, ctrl=(id&1)!=0;
            if (id==0) { dx=sx; dz=sz; }
            if (id==2) { map.flags[55][50]=0xffffff; }
            if (id==3) { dx=51; map.flags[51][50]=0xffffff; }
            if (id==4) {
                sx=10; sz=10; dx=90; dz=10;
                for(int z=1;z<=90;z++) map.flags[50][z]=0xffffff;
            }
            if (id==5) {
                sx=2; sz=2; dx=101; dz=2;
                for(int x=5;x<=98;x+=3) for(int z=1;z<=102;z++) {
                    boolean gap=((x-5)/3&1)==0 ? z==101 : z==2;
                    if(!gap) map.flags[x][z]=0xffffff;
                }
            }
            if (id==6) for(int x=54;x<=56;x++) for(int z=49;z<=51;z++) map.flags[x][z]=0xffffff;
            if (id==7) { type=2; dx=sx; dz=sz; }
            if (id>=8 && id<19) {
                type=1;
                int[] yaws={0,512,1024,1536,173,0,512,0,0,0,0}; yaw=yaws[id-8];
                mx=id==13 ? 655 : 675; my=id==13 ? 89 : 77;
                if(id==14) { mx=582; my=14; }
                if(id==15) { mx=727; my=164; }
                if(id==16) mx=581;
                if(id==17) mx=728;
                if(id==18) my=165;
            }
            if(id==19) {
                sx=1;sz=1;dx=1;dz=102;
                // A single serpentine corridor made with directional walls,
                // using every interior tile without diagonal corner cutting.
                for(int z=1;z<102;z++) for(int x=1;x<=102;x++)
                    if(x!=((z&1)!=0?102:1)) map.addWall(true,1,z,x,true,0);
            }
            app.actionKey[5]=ctrl?1:0;
            app.localPlayer=new PlayerEntity();
            app.localPlayer.pathTileX[0]=sx; app.localPlayer.pathTileZ[0]=sz;
            app.localPlayer.x=sx*128+64; app.localPlayer.z=sz*128+64;
            app.orbitCameraYaw=yaw; app.mouseClickButton=1;
            app.mouseClickX=mx; app.mouseClickY=my;
            if(type==1) app.handleMinimapInput(app.field1433);
            else app.tryMove(sx,0,nearest,dx,sz,0,type,0,dz,0,0,0);
            out.writeInt(id); out.writeInt(type); out.writeInt(sx); out.writeInt(sz);
            out.writeInt(dx); out.writeInt(dz); out.writeBoolean(nearest); out.writeBoolean(ctrl);
            out.writeInt(mx); out.writeInt(my); out.writeInt(yaw);
            for(int x=0;x<104;x++) for(int z=0;z<104;z++) out.writeInt(map.flags[x][z]);
            out.writeInt(app.flagSceneTileX); out.writeInt(app.flagSceneTileZ);
            out.writeInt(app.tryMoveNearest); out.writeInt(app.out.pos);
            out.write(app.out.data,0,app.out.pos);
            System.out.println("case="+id+" type="+type+" bytes="+app.out.pos+
                " flag="+app.flagSceneTileX+","+app.flagSceneTileZ+" nearest="+app.tryMoveNearest);
        }
        out.close();
    }
}
