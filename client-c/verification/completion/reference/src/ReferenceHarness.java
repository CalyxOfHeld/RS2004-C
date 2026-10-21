import deob.client;
import sign.signlink;
import java.net.*;
import java.io.*;
import java.awt.image.BufferedImage;
import javax.imageio.ImageIO;
import jagex2.graphics.PixMap;

public final class ReferenceHarness {
    private static void save(PixMap map,File file) throws Exception {
        if(map==null) throw new IllegalStateException("Reference buffer missing: "+file.getName());
        BufferedImage image=new BufferedImage(map.width,map.height,BufferedImage.TYPE_INT_RGB);
        image.setRGB(0,0,map.width,map.height,map.pixels.clone(),0,map.width);ImageIO.write(image,"png",file);
    }
    private static void blit(BufferedImage image,PixMap map,int x,int y) {
        if(map!=null) image.setRGB(x,y,map.width,map.height,map.pixels.clone(),0,map.width);
    }
    public static void main(String[] args) {
        try { run(args); } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
    private static void run(String[] args) throws Exception {
        File out=new File(args[0]).getCanonicalFile();
        client.nodeId=10;client.portOffset=1;client.members=true;client.setLowMemory(true);
        signlink.startpriv(InetAddress.getByName("127.0.0.1"));
        final client app=new client();app.initApplication(532,789,0);
        long deadline=System.currentTimeMillis()+60000;
        while(client.loopCycle<3 && System.currentTimeMillis()<deadline && !app.errorLoading && !app.errorHost) Thread.sleep(100);
        if(app.errorLoading || app.errorHost || client.loopCycle<3) throw new IllegalStateException("Reference load failed or timed out");
        System.out.println("[REFERENCE] Loaded original client/cache with isolated cache directory");
        final java.util.concurrent.atomic.AtomicBoolean loginDone=new java.util.concurrent.atomic.AtomicBoolean(false);
        client.referenceTask=new Runnable() { public void run() {
            app.login(System.getenv("RS2_USERNAME"),System.getenv("RS2_PASSWORD"),false);loginDone.set(true);
        }};
        deadline=System.currentTimeMillis()+30000;
        while(!loginDone.get() && System.currentTimeMillis()<deadline) Thread.sleep(50);
        if(!loginDone.get()) throw new IllegalStateException("Reference login timed out");
        if(!app.ingame) throw new IllegalStateException("Reference login failed: "+app.loginMessage0+" "+app.loginMessage1);
        app.orbitCameraYaw=0;app.orbitCameraPitch=128;app.cameraAnticheatOffsetX=0;app.cameraAnticheatOffsetZ=0;app.cameraAnticheatAngle=0;
        int start=client.loopCycle;deadline=System.currentTimeMillis()+40000;
        while((app.sceneState!=2 || client.loopCycle<start+150) && System.currentTimeMillis()<deadline) Thread.sleep(100);
        if(app.sceneState!=2 || !app.ingame) throw new IllegalStateException("Reference scene did not become ready");
        save(app.areaViewport,new File(out,"java-viewport.png"));
        save(app.areaMapback,new File(out,"java-mapback.png"));
        save(app.areaSidebar,new File(out,"java-sidebar.png"));
        BufferedImage frame=new BufferedImage(789,532,BufferedImage.TYPE_INT_RGB);
        blit(frame,app.areaBackleft1,0,11);blit(frame,app.areaBackleft2,0,375);
        blit(frame,app.areaBackright1,729,5);blit(frame,app.areaBackright2,752,231);
        blit(frame,app.areaBacktop1,0,0);blit(frame,app.areaBacktop2,561,0);
        blit(frame,app.areaBackvmid1,520,11);blit(frame,app.areaBackvmid2,520,231);blit(frame,app.areaBackvmid3,501,375);
        blit(frame,app.areaBackhmid2,0,345);blit(frame,app.areaBackhmid1,520,165);
        blit(frame,app.areaBackbase1,0,471);blit(frame,app.areaBackbase2,501,492);
        blit(frame,app.areaViewport,8,11);blit(frame,app.areaMapback,561,5);blit(frame,app.areaSidebar,562,231);blit(frame,app.areaChatback,22,375);
        ImageIO.write(frame,"png",new File(out,"java-composed-client.png"));
        System.out.println("[REFERENCE] base="+app.sceneBaseTileX+","+app.sceneBaseTileZ+" local="+app.localPlayer.x+","+app.localPlayer.z+" camera="+app.cameraX+","+app.cameraY+","+app.cameraZ+" pitch="+app.cameraPitch+" yaw="+app.cameraYaw+" npcs="+app.npcCount);
        System.out.println("[REFERENCE] Saved actual viewport/map/sidebar buffers and a composition at original client offsets; not a desktop screenshot.");
        System.exit(0);
    }
}
