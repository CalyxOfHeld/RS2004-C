import jagex2.dash3d.World3D;
import jagex2.dash3d.type.*;
import jagex2.graphics.*;
import java.io.*;
import java.security.MessageDigest;
import java.util.*;

/** Observation only: serialize inputs before actual original drawing continues. */
public final class PainterTraceHooks {
    static DataOutputStream out;static boolean recording;static int frames,models,underlays,overlays,graphFrame=-1;static int[] camera;
    static final Map<String,Integer> assets=new HashMap<String,Integer>();
    static void ints(DataOutputStream output,int... values)throws IOException {for(int v:values)output.writeInt(v);}
    static void array(DataOutputStream output,int[] values,int length)throws IOException {
        if(values==null){output.writeInt(-1);return;}if(values.length<length)throw new IOException("short original model array");output.writeInt(length);for(int i=0;i<length;i++)output.writeInt(values[i]);
    }
    static void finish(){
        try{if(out!=null){if(recording)throw new IOException("unfinished original draw");ints(out,9,frames,models,underlays,overlays,assets.size());out.close();out=null;String graph=System.getProperty("rs2.scene.graph");if(graph!=null)SceneGraphOracle.close(graph);}}
        catch(IOException e){throw new UncheckedIOException(e);}
    }
    public static void begin(World3D scene,int yaw,int x,int plane,int pitch,int y,int z,int unused){
        try{
            if(recording)throw new IOException("nested scene draw");
            if((Model.checkHover || World3D.takingInput) && !Boolean.getBoolean("rs2.painter.allowPicking"))throw new IOException("trace requires inactive picking");
            if(out==null){
                String path=System.getProperty("rs2.painter.trace");if(path==null)throw new IOException("missing trace path");
                out=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(path)));
                ints(out,0x50323235);out.writeDouble(0.8+(new Random(225).nextDouble()*0.03-0.015));
                Runtime.getRuntime().addShutdownHook(new Thread(new Runnable(){public void run(){finish();}}));
            }
            recording=true;
            graphFrame=-1;
            camera=new int[]{yaw,x,plane,pitch,y,z,unused};
            ints(out,0,frames,Pix2D.width2d,Pix2D.height2d,yaw,x,plane,pitch,y,z,unused,World3D.lowMemory?1:0,Pix3D.lowDetail?1:0,Pix3D.jagged?1:0,Pix3D.trans,Pix3D.hclip?1:0);
        }catch(IOException e){throw new UncheckedIOException(e);}
    }
    public static void end(){
        try{
            if(!recording)throw new IOException("unpaired original draw return");
            ints(out,5,frames,Pix3D.trans,Pix3D.hclip?1:0,World3D.tilesRemaining,Pix2D.data.length);for(int pixel:Pix2D.data)out.writeInt(pixel);
            recording=false;frames++;
        }catch(IOException e){throw new UncheckedIOException(e);}
    }
    public static void tileStart(World3D scene,Ground root){
        if(!recording||graphFrame==frames)return;
        graphFrame=frames;
        try{String path=System.getProperty("rs2.scene.graph");if(path!=null)SceneGraphOracle.capture(scene,root,camera,path);}
        catch(Exception e){throw new IllegalStateException("original scene graph capture failed",e);}
    }
    static byte[] snapshot(Model m)throws IOException {
        ByteArrayOutputStream bytes=new ByteArrayOutputStream();DataOutputStream data=new DataOutputStream(bytes);
        ints(data,m.vertexCount,m.faceCount,m.texturedFaceCount,m.priority,m.radius,m.maxY,m.minY,m.minDepth,m.maxDepth,m.pickable?1:0);
        array(data,m.vertexX,m.vertexCount);array(data,m.vertexY,m.vertexCount);array(data,m.vertexZ,m.vertexCount);
        array(data,m.faceVertexA,m.faceCount);array(data,m.faceVertexB,m.faceCount);array(data,m.faceVertexC,m.faceCount);
        array(data,m.faceColor,m.faceCount);array(data,m.faceColorA,m.faceCount);array(data,m.faceColorB,m.faceCount);array(data,m.faceColorC,m.faceCount);
        array(data,m.faceInfo,m.faceCount);array(data,m.facePriority,m.faceCount);array(data,m.faceAlpha,m.faceCount);
        array(data,m.texturedVertexA,m.texturedFaceCount);array(data,m.texturedVertexB,m.texturedFaceCount);array(data,m.texturedVertexC,m.texturedFaceCount);data.flush();return bytes.toByteArray();
    }
    public static void model(Model m,int yaw,int sp,int cp,int sw,int cw,int dx,int dy,int dz,int bitset){
        if(!recording)return;
        try{
            byte[] data=snapshot(m),digest=MessageDigest.getInstance("SHA-256").digest(data);String key=Base64.getEncoder().encodeToString(digest);Integer id=assets.get(key);
            if(id==null){id=assets.size();assets.put(key,id);ints(out,1,id,data.length);out.write(data);}
            ints(out,2,id,yaw,sp,cp,sw,cw,dx,dy,dz,bitset,Pix3D.trans,Pix3D.hclip?1:0);models++;
        }catch(Exception e){throw new IllegalStateException("original model trace failed",e);}
    }
    public static void underlay(World3D scene,TileUnderlay u,int level,int sp,int cp,int sw,int cw,int x,int z){
        if(!recording)return;
        try{
            ints(out,3,Pix3D.trans,Pix3D.hclip?1:0,level,sp,cp,sw,cw);
            int[] h={scene.levelHeightmaps[level][x][z],scene.levelHeightmaps[level][x+1][z],scene.levelHeightmaps[level][x+1][z+1],scene.levelHeightmaps[level][x][z+1]};
            GroundDrawOracle.writeTile(out,new GroundDrawOracle.Tile(x,z,h,u,null));underlays++;
        }catch(Exception e){throw new IllegalStateException("original underlay trace failed",e);}
    }
    public static void overlay(World3D scene,int sw,int z,TileOverlay o,int x,int cp,int sp,int cw,boolean draw){
        if(!recording)return;
        try{ints(out,4,Pix3D.trans,Pix3D.hclip?1:0,sw,cp,sp,cw,draw?1:0);GroundDrawOracle.writeTile(out,new GroundDrawOracle.Tile(x,z,new int[4],null,o));overlays++;}
        catch(Exception e){throw new IllegalStateException("original overlay trace failed",e);}
    }
}
