package sign;
import java.io.*;
import java.util.concurrent.*;
public final class MixedAudioBoundary {
 public static Semaphore reached=new Semaphore(0),gate=new Semaphore(0);
 public static byte[][] waves=new byte[5][],midis=new byte[5][];
 public static FileOutputStream open(final String name)throws IOException{return new FileOutputStream(FileDescriptor.out){public void write(byte[] b,int off,int len){boolean wave=name.startsWith("sound");int slot=name.charAt(wave?5:6)-'0';(wave?waves:midis)[slot]=java.util.Arrays.copyOfRange(b,off,off+len);}public void close(){}};}
 public static void step()throws InterruptedException{reached.release();gate.acquire();}
 public static void ready()throws Exception{if(!reached.tryAcquire(3,TimeUnit.SECONDS))throw new Exception("worker timeout");}
 public static void service()throws Exception{gate.release();ready();}
}
