import java.io.*;
import java.nio.file.*;
import jagex2.io.Jagfile;
import jagex2.io.Packet;
import jagex2.sound.Tone;
import jagex2.sound.Wave;

/** Runs unchanged bundled synthesis methods with a shared noise-table input.
 * Original Tone.init uses Math.random, so arbitrary launches cannot have equal
 * noise samples. Setting its public input table here isolates synthesis parity;
 * it does not establish parity between the two random-number generators.
 */
public final class WaveOracle {
    public static void main(String[] args) throws Exception {
        Jagfile archive=new Jagfile(Files.readAllBytes(Paths.get(args[0])),false);
        Wave.unpack(new Packet(363,archive.read("sounds.dat",null,(byte)0)),1);
        int seed=0x12345678;
        for(int i=0;i<Tone.noise.length;i++) {
            seed=seed*1103515245+12345;
            Tone.noise[i]=(seed&0x80000000)!=0?1:-1;
        }
        int count=0;
        try(DataOutputStream out=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[1])))) {
            out.writeInt(0x57323235); // W225
            for(int id=0;id<Wave.tracks.length;id++) if(Wave.tracks[id]!=null) {
                for(int loops=1;loops<=2;loops++) {
                    Packet wav=Wave.generate((byte)-16,loops,id);
                    out.writeShort(id);out.writeByte(loops);out.writeInt(wav.pos);
                    out.write(wav.data,0,wav.pos);count++;
                }
            }
            out.writeShort(65535);
        }
        System.out.println("Original Java Wave/Tone: "+count+" cache WAV records; shared deterministic noise input, loops 1 and 2.");
    }
}
