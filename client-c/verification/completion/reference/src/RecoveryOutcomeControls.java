import deob.client;
import jagex2.io.ClientStream;
import java.nio.file.*;
import java.util.*;
import java.lang.reflect.*;
public class RecoveryOutcomeControls {
 public static void verify(client app,Path fixtures)throws Exception {
  Object previousChecksums=RecoveryParityOracle.get(app,"archiveChecksum");
  RecoveryParityOracle.set(app,"archiveChecksum",new int[9]); // native isolated loopback fixture uses zero CRCs
  for(String line:Files.readAllLines(fixtures.resolve("cases.txt"))) {
   String[] row=line.split(" ");int test=Integer.parseInt(row[0]),attempts=Integer.parseInt(row[1]);
   long state=Long.parseLong(row[2]);boolean accepted=row[3].equals("1");
   Field holder=RecoveryParityOracle.field(Class.forName("java.lang.Math$RandomNumberGeneratorHolder"),"randomNumberGenerator");
   ((Random)holder.get(null)).setSeed(state ^ 0x5DEECE66DL); // matched controlled initial state, once per independent case
   RecoveryParityOracle.currentFrame=2000+test;
   RecoveryParityOracle.set(app,"username","sync");RecoveryParityOracle.set(app,"password","pw");
   RecoveryParityOracle.set(app,"ingame",true);RecoveryParityOracle.set(app,"rights",true);
   RecoveryParityOracle.setInt(app,"idleTimeout",0);RecoveryParityOracle.setInt(app,"systemUpdateTimer",123);
   RecoveryParityOracle.set(app,"menuVisible",true);RecoveryParityOracle.setInt(app,"menuSize",1);
   Object local=RecoveryParityOracle.get(app,"localPlayer");RecoveryParityOracle.setInt(local,"x",6000);
   // Match native attach_old(): every independent case starts with an open
   // previous connection, including cases after a rejected/logout outcome.
   ClientStream previous=(ClientStream)RecoveryParityOracle.get(app,"stream");
   if(previous!=null)previous.close();
   ClientStream old=new ClientStream(app,(byte)2,new RecoveryTransport(new byte[0],new byte[0]));
   RecoveryParityOracle.set(app,"stream",old);
   List<RecoveryTransport> queued=new ArrayList<>();
   for(int attempt=0;attempt<attempts;attempt++) {
    byte[] data=Files.readAllBytes(fixtures.resolve("login-"+test+"-"+attempt+".bin"));
    RecoveryTransport transport=new RecoveryTransport(Arrays.copyOfRange(data,0,9),Arrays.copyOfRange(data,9,data.length));
    transport.gameSink=RecoveryParityOracle.captureOut;queued.add(transport);RecoveryParityOracle.connections.add(transport);
   }
   app.tryReconnect(false);
   if((Boolean)RecoveryParityOracle.get(app,"ingame")!=accepted)throw new AssertionError("outcome "+test);
   if(!old.closed)throw new AssertionError("old stream retained after attempt "+test);
   for(RecoveryTransport t:queued)if(!t.handshakeComplete()||t.mismatch!=null)throw new AssertionError("handshake "+test);
   if(accepted) {
    if(!(Boolean)RecoveryParityOracle.get(app,"rights") || (Boolean)RecoveryParityOracle.get(app,"menuVisible") ||
       RecoveryParityOracle.getInt(app,"menuSize")!=0 || RecoveryParityOracle.getInt(app,"systemUpdateTimer")!=0 ||
       RecoveryParityOracle.getInt(app,"idleNetCycles")!=0 || RecoveryParityOracle.getInt(local,"x")!=6000)
       throw new AssertionError("reply15 state "+test);
   } else if(!((String)RecoveryParityOracle.get(app,"username")).isEmpty() || !((String)RecoveryParityOracle.get(app,"password")).isEmpty() ||
              RecoveryParityOracle.getInt(app,"titleScreenState")!=0)throw new AssertionError("rejected credentials/title");
   System.out.println("PASS ORIGINAL RECOVERY OUTCOME case="+test+" attempts="+attempts+" accepted="+accepted+" byte-exact native login fixture");
  }
  RecoveryParityOracle.set(app,"archiveChecksum",previousChecksums);
 }
}
