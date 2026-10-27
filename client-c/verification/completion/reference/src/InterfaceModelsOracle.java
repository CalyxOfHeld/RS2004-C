import deob.client;
import jagex2.io.*;
import jagex2.config.*;
import jagex2.graphics.*;
import jagex2.datastruct.LruCache;
import java.io.*;
import java.net.*;
import java.nio.file.*;
import java.lang.reflect.*;
import java.util.*;
public class InterfaceModelsOracle {
 static client c=new client();static DataOutputStream out;static int[] ids=new int[3];static int cases,pictures;static final int SIZE=512,HEIGHT=334;
 static Jagfile jag(String d,String n)throws Exception{return new Jagfile(Files.readAllBytes(Paths.get(d,n)),false);}
 static void array(int[] a)throws Exception{out.writeInt(a==null?-1:a.length);if(a!=null)for(int v:a)out.writeInt(v);}
 static void model(Model m)throws Exception{out.writeBoolean(m!=null);if(m==null)return;out.writeInt(m.vertexCount);out.writeInt(m.faceCount);out.writeInt(m.texturedFaceCount);out.writeInt(m.priority);for(int[] a:new int[][]{m.vertexX,m.vertexY,m.vertexZ,m.faceVertexA,m.faceVertexB,m.faceVertexC,m.faceColor,m.faceColorA,m.faceColorB,m.faceColorC,m.faceInfo,m.facePriority,m.faceAlpha,m.texturedVertexA,m.texturedVertexB,m.texturedVertexC})array(a);}
 static byte[] bytes(int... values)throws Exception{ByteArrayOutputStream b=new ByteArrayOutputStream();DataOutputStream d=new DataOutputStream(b);for(int v:values)d.writeShort(v);return b.toByteArray();}
 static void one(int op,int slot,int a,int b)throws Exception{
  byte[] payload=op==87?bytes(ids[slot],a):bytes(ids[slot],a,b);out.writeInt(op);out.writeInt(payload.length);out.write(payload);byte[] frame=new byte[payload.length+1];frame[0]=(byte)op;System.arraycopy(payload,0,frame,1,payload.length);final ByteArrayInputStream input=new ByteArrayInputStream(frame);Socket socket=new Socket(){public void setSoTimeout(int n){}public void setTcpNoDelay(boolean v){}public InputStream getInputStream(){return input;}public OutputStream getOutputStream(){return new ByteArrayOutputStream();}};c.stream=new ClientStream(c,(byte)2,socket);c.randomIn=null;c.packetType=-1;if(!c.read(false)||input.available()!=0||c.packetType!=-1)throw new Exception("read failed");
  for(int i:ids){Component v=Component.instances[i];out.writeInt(v.xan);out.writeInt(v.yan);out.writeInt(v.zoom);model(v.model);}for(int i:ids)for(int j:ids)out.writeBoolean(Component.instances[i].model==Component.instances[j].model);
  Component v=Component.instances[ids[slot]];Model m=v.getModel(-1,-1,false);boolean picture=cases%9==0 && m!=null && m.faceCount<1000 && v.zoom>=200 && v.zoom<=5000;
  if(m!=null && m.faceInfo!=null)for(int info:m.faceInfo)if((info&2)!=0)picture=false;
  out.writeBoolean(picture);if(picture){out.writeInt(slot);int[] pixels=new int[SIZE*HEIGHT];Arrays.fill(pixels,0);Pix2D.bind(SIZE,pixels,-657,HEIGHT);Pix3D.init2D(-1);Pix3D.centerW3D=v.width/2;Pix3D.centerH3D=v.height/2;int ey=Pix3D.sinTable[v.xan]*v.zoom>>16,ez=Pix3D.cosTable[v.xan]*v.zoom>>16;m.drawSimple(0,v.yan,0,v.xan,0,ey,ez);for(int pixel:pixels)out.writeInt(pixel);pictures++;}cases++;
 }
 public static void main(String[] args)throws Exception {
  Model.unpack(3,jag(args[0],"models"));ObjType.unpack(jag(args[0],"config"));Jagfile title=jag(args[0],"title");PixFont[] fonts={new PixFont(title,"p11",530),new PixFont(title,"p12",530),new PixFont(title,"b12",530),new PixFont(title,"q8",530)};Component.unpack(jag(args[0],"media"),fonts,1,jag(args[0],"interface"));int found=0;for(Component v:Component.instances)if(v!=null && v.type==6 && found<3){ids[found++]=v.id;v.model=null;v.anim=v.activeAnim=-1;v.xan=256;v.yan=0;v.zoom=1000;}if(found!=3)throw new Exception("widgets");
  Field holder=Class.forName("java.lang.Math$RandomNumberGeneratorHolder").getDeclaredField("randomNumberGenerator");holder.setAccessible(true);((Random)holder.get(null)).setSeed(225);Pix3D.setBrightness(true,0.8);
  List<Integer> objects=new ArrayList<>(),raw=new ArrayList<>();for(int i=0;i<ObjType.count && objects.size()<100;i++){try{ObjType t=ObjType.get(i);Model m=t.getInterfaceModel(50);if(m.vertexCount>0){objects.add(i);if(!raw.contains(t.model))raw.add(t.model);}}catch(RuntimeException ignored){}}ObjType.modelCache=new LruCache((byte)0,50);
  ByteArrayOutputStream bytes=new ByteArrayOutputStream();out=new DataOutputStream(new BufferedOutputStream(bytes));one(103,0,0,65535);
  for(int i=0;i<Math.min(40,raw.size());i++){one(87,i%3,raw.get(i),0);Model m=Component.instances[ids[i%3]].model;if(m.faceCount>0){int colour=m.faceColor[0];one(103,i%3,colour,(colour+123)&65535);one(103,i%3,(colour+123)&65535,colour);}}
  for(int divisor:new int[]{1,10,50,100,200,1000,65535})for(int i=0;i<5;i++)one(46,i%3,objects.get(i),divisor);
  one(46,0,objects.get(0),100);one(46,1,objects.get(0),100);for(int i:objects)one(46,2,i,100);one(46,2,objects.get(0),100);one(87,1,raw.get(0),0);
  for(int i=0;i<50;i++){one(46,i%3,objects.get(i%objects.size()),100);one(87,(i+1)%3,raw.get(i%raw.size()),0);}
  for(int id:new int[]{995,882,892,314,561,996}){one(46,0,id,100);one(46,1,id,100);one(87,2,ObjType.get(id).model,0);}
  out.flush();try(DataOutputStream f=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[1])))){f.writeInt(0x494d3235);f.writeInt(cases);for(int id:ids)f.writeInt(id);for(int value:Pix3D.colourTable)f.writeInt(value);f.write(bytes.toByteArray());}System.out.println("Original interface models:"+cases+" actual packet46/87/103 states,3 cached widgets "+Arrays.toString(ids)+",full meshes/nullable colours/shading/aliases,Lru50 eviction/count50;"+pictures+" complete512x334 previews");
 }
}
