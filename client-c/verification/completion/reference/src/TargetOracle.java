import deob.client;
import jagex2.config.Component;
import jagex2.dash3d.*;
import jagex2.dash3d.type.*;
import jagex2.dash3d.entity.*;
import jagex2.io.*;
import java.io.*;
import java.nio.file.*;

/** Actual original menu dispatch and routing against explicit state fixtures. */
public final class TargetOracle {
    public static void main(String[] args) throws Exception {
        Component.unpack(null,null,1,new Jagfile(Files.readAllBytes(Paths.get(args[0])),false));
        int[] actions={900,450,217,367,265,55,965,651,391};
        int[] flags={2,4,1,8};
        try(DataOutputStream out=new DataOutputStream(new FileOutputStream(args[1]))) {
            out.writeInt(0x54415231);out.writeInt(actions.length);
            for(int i=0;i<actions.length;i++) {
                client app=new client();
                int kind=i%4,flag=i==8?16:flags[kind],spell=-1;
                for(Component c:Component.instances) if(c!=null && c.buttonType==2 && (c.actionTarget&flag)!=0) {spell=c.id;break;}
                if(spell<0) throw new IOException("No actual cache spell for flag "+flag);
                app.sceneBaseTileX=app.sceneBaseTileZ=3184;
                app.levelCollisionMap[0]=new CollisionMap(104,-1,104);
                app.localPlayer=new PlayerEntity();app.localPlayer.pathTileX[0]=50;app.localPlayer.pathTileZ[0]=50;
                NpcEntity npc=new NpcEntity();npc.pathTileX[0]=npc.pathTileZ[0]=50;app.npcs[42]=npc;
                PlayerEntity player=new PlayerEntity();player.pathTileX[0]=player.pathTileZ[0]=50;app.players[17]=player;
                app.scene=new World3D(1,new int[4][105][105],104,4,104);
                Ground ground=new Ground(0,50,50);Wall wall=new Wall();
                wall.bitset=(2<<29)|(1530<<14)|(50<<7)|50;ground.wall=wall;app.scene.levelTiles[0][50][50]=ground;
                app.objInterface=1205;app.objSelectedSlot=4;app.objSelectedInterface=3214;
                app.activeSpellId=spell;app.objSelected=i<4?1:0;app.spellSelected=i>=4?1:0;
                app.mouseClickX=256;app.mouseClickY=167;
                app.menuAction[0]=actions[i];app.menuParamB[0]=i==8?3:50;app.menuParamC[0]=i==8?3214:50;
                app.menuParamA[0]=i==8?995:kind==0?42:kind==1?wall.bitset:kind==2?995:17;
                app.out=new Packet(1,new byte[5000]);app.out.random=new Isaac((byte)1,new int[]{17,31,57,99});
                app.useMenuOption(6412,0);
                out.writeInt(i);out.writeInt(spell);out.writeInt(app.crossMode);out.writeInt(app.out.pos);
                out.write(app.out.data,0,app.out.pos);
                if(app.objSelected!=0 || app.spellSelected!=0) throw new IOException("Selection did not clear");
                System.out.println("case="+i+" action="+actions[i]+" spell="+spell+" bytes="+app.out.pos+" cross="+app.crossMode);
            }
        }
    }
}
