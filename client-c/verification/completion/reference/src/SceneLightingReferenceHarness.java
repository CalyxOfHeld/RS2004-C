import deob.client;
import sign.signlink;
import java.net.*;
import java.io.*;
import java.awt.image.BufferedImage;
import javax.imageio.ImageIO;
import jagex2.graphics.*;
import java.util.concurrent.atomic.AtomicBoolean;

/** Actual original live scene, drawn at the native capture's exact camera. */
public class SceneLightingReferenceHarness {
 public static void main(String[] args) {
  try {run(args);}catch(Throwable t){t.printStackTrace();System.exit(1);}
 }
 static void run(String[] args)throws Exception {
  client.nodeId=10;client.portOffset=1;client.members=true;client.setLowMemory(false);
  signlink.startpriv(InetAddress.getByName("127.0.0.1"));final client app=new client();app.initApplication(532,789,0);
  if(app.frame!=null)app.frame.setVisible(false);
  long deadline=System.currentTimeMillis()+60000;
  while(client.loopCycle<3 && System.currentTimeMillis()<deadline && !app.errorLoading && !app.errorHost)Thread.sleep(100);
  if(app.errorLoading || app.errorHost || client.loopCycle<3)throw new IOException("Original client did not load");
  AtomicBoolean logged=new AtomicBoolean();client.referenceTask=()->{app.login(System.getenv("RS2_USERNAME"),System.getenv("RS2_PASSWORD"),false);logged.set(true);};
  deadline=System.currentTimeMillis()+30000;while(!logged.get() && System.currentTimeMillis()<deadline)Thread.sleep(50);
  if(!logged.get() || !app.ingame)throw new IOException("Original login failed: "+app.loginMessage0+" "+app.loginMessage1);
  int start=client.loopCycle;deadline=System.currentTimeMillis()+40000;
  while((app.sceneState!=2 || client.loopCycle<start+100) && System.currentTimeMillis()<deadline)Thread.sleep(100);
  if(app.sceneState!=2)throw new IOException("Original region did not become active");
  final int x=Integer.parseInt(args[1]),y=Integer.parseInt(args[2]),z=Integer.parseInt(args[3]),pitch=Integer.parseInt(args[4]),yaw=Integer.parseInt(args[5]);
  AtomicBoolean drawn=new AtomicBoolean();client.referenceTask=()->{
   try {
    app.areaViewport.bind((byte)62);Pix3D.lineOffset=app.areaViewportOffsets;Pix2D.clear(1);
    app.scene.clearTemporaryLocs(0);app.scene.draw(yaw,x,app.currentLevel,pitch,y,z,0);
    BufferedImage image=new BufferedImage(app.areaViewport.width,app.areaViewport.height,BufferedImage.TYPE_INT_RGB);
    image.setRGB(0,0,app.areaViewport.width,app.areaViewport.height,app.areaViewport.pixels,0,app.areaViewport.width);
    ImageIO.write(image,"png",new File(args[0]));
    System.out.println("[REFERENCE] original static scene base="+app.sceneBaseTileX+","+app.sceneBaseTileZ+" player="+(app.localPlayer.x/128+app.sceneBaseTileX)+","+(app.localPlayer.z/128+app.sceneBaseTileZ)+" plane="+app.currentLevel+" camera="+x+","+y+","+z+" pitch="+pitch+" yaw="+yaw+" high-detail");
    drawn.set(true);
   }catch(Exception e){throw new RuntimeException(e);}
  };
  deadline=System.currentTimeMillis()+10000;while(!drawn.get() && System.currentTimeMillis()<deadline)Thread.sleep(50);
  if(!drawn.get())throw new IOException("Original capture did not finish");System.exit(0);
 }
}
