import jagex2.dash3d.World;
import java.io.*;
public class BridgeLevelOracle {
 public static void main(String[] args)throws Exception {
  byte[][][] flags=new byte[4][3][3];World w=new World(3,flags,3,new int[4][4][4],-35388);
  try(DataOutputStream out=new DataOutputStream(new FileOutputStream(args[0]))) {
   for(int l=0;l<4;l++)for(int b=0;b<256;b++)for(int f=0;f<256;f++) {
    flags[1][1][1]=(byte)b;flags[l][1][1]=(byte)f;
    out.writeByte(w.getDrawLevel(l,(byte)10,1,1));
   }
  }
  System.out.println("Original World.getDrawLevel: 262144 source-plane/bridge/visibility-flag cases");
 }
}
