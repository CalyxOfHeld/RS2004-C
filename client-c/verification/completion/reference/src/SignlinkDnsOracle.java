package sign;
import java.io.*;
import java.util.concurrent.*;
public class SignlinkDnsOracle {
 public static volatile CountDownLatch entered=new CountDownLatch(1),release=new CountDownLatch(1);
 public static volatile int calls;public static volatile boolean fail;public static volatile String address;
 public static String resolve(String input)throws Exception{address=input;calls++;entered.countDown();if(!release.await(3,TimeUnit.SECONDS))throw new Exception("controlled DNS timeout");if(fail)throw new IOException("controlled resolver failure");return "resolved.example";}
 static void text(DataOutputStream out,String value)throws Exception{if(value==null){out.writeInt(-1);return;}byte[] bytes=value.getBytes("US-ASCII");out.writeInt(bytes.length);out.write(bytes);}
 static void snapshot(DataOutputStream out)throws Exception{text(out,signlink.dns);text(out,signlink.dnsreq);out.writeInt(calls);text(out,address);}
 static void awaitClear()throws Exception{long end=System.nanoTime()+TimeUnit.SECONDS.toNanos(3);while(signlink.dnsreq!=null && System.nanoTime()<end)Thread.sleep(1);if(signlink.dnsreq!=null)throw new Exception("source worker did not clear DNS request");}
 static void request(String value)throws Exception{entered=new CountDownLatch(1);release=new CountDownLatch(1);signlink.dnslookup(value);if(!entered.await(3,TimeUnit.SECONDS))throw new Exception("source resolver not entered");}
 public static void main(String[] args)throws Exception{
  ByteArrayOutputStream code=new ByteArrayOutputStream();try(InputStream input=signlink.class.getResourceAsStream("signlink.class")){byte[] buffer=new byte[4096];int n;while((n=input.read(buffer))!=-1)code.write(buffer,0,n);}if(!new String(code.toByteArray(),"ISO-8859-1").contains("SignlinkDnsOracle"))throw new Exception("Use the verifier's instrumented signlink class;original worker execution is forbidden here");
  signlink.threadliveid=225;signlink.socketreq=0;signlink.threadreq=null;signlink.loadreq=signlink.savereq=signlink.urlreq=signlink.dnsreq=null;Thread worker=new Thread(new signlink());worker.setDaemon(true);worker.start();long end=System.nanoTime()+TimeUnit.SECONDS.toNanos(3);while(!signlink.active && System.nanoTime()<end)Thread.sleep(1);if(!signlink.active)throw new Exception("source worker startup");
  try(DataOutputStream out=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[0])))){
   out.writeInt(0x444e3235);out.writeInt(5);
   request("192.0.2.1");snapshot(out);signlink.dnslookup("198.51.100.2");snapshot(out);release.countDown();awaitClear();snapshot(out);
   request("203.0.113.3");release.countDown();awaitClear();snapshot(out);
   fail=true;request("203.0.113.4");release.countDown();awaitClear();snapshot(out);
  }finally{signlink.threadliveid++;worker.join(3000);if(worker.isAlive())throw new Exception("source worker still alive");}
  if(calls!=3 || !"unknown".equals(signlink.dns) || signlink.dnsreq!=null)throw new Exception("source DNS outcome");
  System.out.println("Original signlink DNS:5 states,3 resolver calls;numeric publication,old completion overwrites newer numeric andclears newer request,subsequent success,unknown failure;actual copied run/dnslookup with only cache-startup/resolver boundaries replaced;no disk cache or external DNS query");
 }
}
