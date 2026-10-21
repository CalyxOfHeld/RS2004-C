import jagex2.config.Component;
import jagex2.io.Jagfile;
import java.nio.file.*;
public class SpellLayout {
 public static void main(String[] a) throws Exception {
  Component.unpack(null,null,1,new Jagfile(Files.readAllBytes(Paths.get(a[0])),false));
  int id=1152;
  for(int n=0;n<10;n++) {
   Component c=Component.instances[id];System.out.println("id="+id+" layer="+c.layer+" action="+c.action+" size="+c.width+","+c.height);
   boolean found=false;
   for(Component p:Component.instances) if(p!=null && p.childId!=null) for(int i=0;i<p.childId.length;i++) if(p.childId[i]==id) {
    System.out.println("parent="+p.id+" offset="+p.childX[i]+","+p.childY[i]);id=p.id;found=true;break;
   }
   if(!found) break;
  }
 }
}
