import jagex2.config.*;
import jagex2.dash3d.*;
import jagex2.graphics.*;
import jagex2.io.*;
import java.nio.file.*;
public class BridgeLightingProbe {
 static Jagfile jag(String name)throws Exception{return new Jagfile(Files.readAllBytes(Paths.get("engine/data/pack/client",name)),false);}
 static Model mesh(int rotation){return LocType.get(3007).getModel(0,rotation,0,0,0,0,-1);}
 public static void main(String[] args)throws Exception {
  Model.unpack(3,jag("models"));LocType.unpack(jag("config"));
  int scale=768*(int)Math.sqrt(5100)>>8;
  for(int rotation:new int[]{1,3}) {
   LocType.modelCacheStatic.clear();LocType.modelCacheDynamic.clear();
   Model a=mesh(rotation),b=mesh(rotation);int vertices=a.vertexCount,faces=a.faceCount;
   World3D scene=new World3D(1,new int[4][105][105],104,4,104);
   scene.addWall(0,0,1,1,8,a,null,40,0,25,(byte)0);
   scene.addWall(0,0,1,1,8,b,null,41,0,25,(byte)0);
   scene.buildModels(-10,64,-50,768,-50,false);
   LocType.modelCacheStatic.clear();LocType.modelCacheDynamic.clear();
   Model isolated=mesh(rotation);isolated.applyLighting(64,scale,-50,-10,-50);
   int changed=0,hidden=0;
   for(int f=0;f<a.faceCount;f++){
    if(a.faceColorA[f]!=isolated.faceColorA[f]||a.faceColorB[f]!=isolated.faceColorB[f]||a.faceColorC[f]!=isolated.faceColorC[f])changed++;
    if(a.faceInfo!=null&&a.faceInfo[f]==-1)hidden++;
   }
   System.out.println("Original adjacent loc3007, rotation="+rotation+", vertices="+vertices+", faces="+faces+", first railing faces with changed shades="+changed+", faces hidden after seam merge="+hidden);
  }
 }
}
