import deob.client;
import jagex2.io.*;
import jagex2.graphics.*;
import jagex2.config.Component;
import java.io.*;
import java.nio.file.*;
import java.awt.image.BufferedImage;
public class PrivatePromptOracle {
 static void text(DataOutputStream out,String s)throws Exception{out.writeInt(s.length());for(char ch:s.toCharArray())out.writeChar(ch);}
 public static void main(String[] args)throws Exception {
  client c=new client();client.nodeId=10;
  for(int i=0;i<100;i++){c.friendName[i]="Friend "+i;c.friendWorld[i]=i%3==0?0:i%3==1?10:12;}
  Jagfile title=new Jagfile(Files.readAllBytes(Paths.get(args[0],"title")),false),media=new Jagfile(Files.readAllBytes(Paths.get(args[0],"media")),false);
  c.fontBold12=new PixFont(title,"b12",530);c.imageChatback=new Pix8(media,"chatback",0);c.areaChatback=new PixMap(c,479,1,96);c.areaViewport=new PixMap(c,512,1,334);c.graphics=new BufferedImage(789,532,BufferedImage.TYPE_INT_RGB).getGraphics();c.menuVisible=false;
  try(DataOutputStream out=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[1])))) {
   out.writeInt(0x50503235);out.writeInt(804);int options=0;
   for(int count:new int[]{0,1,3,100})for(int index=0;index<201;index++) {
    int code=index<200?index+1:203;c.friendCount=count;Component row=new Component();row.clientCode=code;row.height=160;row.text="before";row.buttonType=0;row.scroll=0;c.updateInterfaceContent(0,row);
    out.writeInt(count);out.writeInt(code);text(out,row.text);out.writeInt(row.buttonType);out.writeInt(row.scroll);c.menuSize=0;
    if(row.buttonType==1)c.handleSocialMenuOption(row,(byte)8);
    out.writeInt(c.menuSize);for(int i=0;i<c.menuSize;i++){out.writeInt(c.menuAction[i]);text(out,c.menuOption[i]);options++;}
   }
   String[] inputs={"","Hello WORLD!","@red@word","ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz 0123456789","                                                                                ","WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW"};
   out.writeInt(24);
   for(String name:new String[]{"Alice","Self Test","aBc_123","Long Name123"})for(String input:inputs) {
    c.showSocialInput=true;c.socialInput=input;c.socialMessage="Enter message to send to "+name;c.chatbackInputOpen=true;c.modalMessage="hidden";c.drawChatback(false);
    text(out,c.socialMessage);text(out,input);for(int pixel:c.areaChatback.pixels)out.writeInt(pixel);
   }
   System.out.println("Original private UI:804 dynamic friend rows,menu options="+options+",24 prompt images/1103616 pixels; actual updateInterfaceContent/handleSocialMenuOption/drawChatback");
  }
  System.exit(0);
 }
}
