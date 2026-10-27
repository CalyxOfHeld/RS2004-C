import deob.client;
import jagex2.io.Jagfile;
import jagex2.graphics.*;
import jagex2.dash3d.entity.PlayerEntity;
import java.nio.file.*;
import java.io.*;
import java.awt.image.BufferedImage;

// Diagnostic fixtures call the unchanged original rendering and hover methods.
public class ChatDisplayOracle {
 static void pixels(DataOutputStream out, int[] p) throws Exception {
  for (int v:p) out.writeInt(v);
 }
 static void menu(client c, DataOutputStream out, boolean split,int x,int y,boolean rights) throws Exception {
  c.menuSize=0;c.mouseX=x;c.mouseY=y;c.rights=rights;
  if(split) c.handlePrivateChatInput(x,27078,y);
  else if(x>22 && x<431 && y>375 && y<471) c.handleChatMouseInput(y-375,0,x-22);
  out.writeInt(split?1:0);out.writeInt(x);out.writeInt(y);out.writeInt(rights?1:0);out.writeInt(c.menuSize);
  for(int i=0;i<c.menuSize;i++) {out.writeInt(c.menuAction[i]);byte[] b=c.menuOption[i].getBytes("UTF-8");out.writeInt(b.length);out.write(b);}
 }
 public static void main(String[] a) throws Exception {
  client c=new client();
  Jagfile title=new Jagfile(Files.readAllBytes(Paths.get(a[0],"title")),false);
  Jagfile media=new Jagfile(Files.readAllBytes(Paths.get(a[0],"media")),false);
  c.fontPlain12=new PixFont(title,"p12",530);
  c.imageChatback=new Pix8(media,"chatback",0);
  c.imageScrollbar0=new Pix8(media,"scrollbar",0);c.imageScrollbar1=new Pix8(media,"scrollbar",1);
  c.areaChatback=new PixMap(c,479,1,96);c.areaViewport=new PixMap(c,512,1,334);
  c.graphics=new BufferedImage(789,532,BufferedImage.TYPE_INT_RGB).getGraphics();
  c.chatInterfaceId=-1;c.stickyChatInterfaceId=-1;c.username="self_test";
  c.localPlayer=new PlayerEntity();c.localPlayer.name="Self Test";
  c.friendCount=1;c.friendName[0]="aLiCe";
  String[] senders={"Self Test","Alice","Stranger","Bob"};
  for(int i=0;i<100;i++) {c.messageText[i]="text "+i+" @red@Abc W";c.messageType[i]=i%10;c.messageSender[i]=senders[(i/10+i)%4];}
  c.chatTyped="hello";
  try(DataOutputStream out=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(a[1])))) {
   out.writeInt(144);int n=0;
   for(int pub=0;pub<4;pub++)for(int priv=0;priv<3;priv++)for(int trade=0;trade<3;trade++)for(int split=0;split<2;split++)for(int timer=0;timer<2;timer++) {
    c.username=new String[]{"self_test","SELF_test","sElF_test","other_name"}[n%4];
    c.publicChatSetting=pub;c.privateChatSetting=priv;c.tradeChatSetting=trade;c.splitPrivateChat=split;c.systemUpdateTimer=timer;
    c.chatScrollOffset=new int[]{0,42,448,1000}[n%4];
    out.writeInt(pub);out.writeInt(priv);out.writeInt(trade);out.writeInt(split);out.writeInt(timer);out.writeInt(c.chatScrollOffset);
    c.drawChatback(false);pixels(out,c.areaChatback.pixels);
    c.areaViewport.bind((byte)62);for(int i=0;i<c.areaViewport.pixels.length;i++)c.areaViewport.pixels[i]=(i*37+12345)&0xffffff;
    c.drawPrivateMessages(222);pixels(out,c.areaViewport.pixels);
    out.writeInt(95+46+8);
    for(int y=376;y<=470;y++) menu(c,out,false,23,y,(y&1)!=0);
    for(int y=300;y<=345;y++) menu(c,out,true,9,y,(y&1)!=0);
    for(int x:new int[]{22,23,430,431})menu(c,out,false,x,449,true);
    for(int x:new int[]{8,9,519,520})menu(c,out,true,x,340,true);
    n++;
   }
  }
  System.out.println("144 original chatback/viewport images and 21456 hover queries captured");
  System.exit(0);
 }
}
