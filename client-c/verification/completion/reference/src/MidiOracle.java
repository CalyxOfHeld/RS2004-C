import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.util.zip.CRC32;
import jagex2.io.BZip2;
import jagex2.io.Packet;

/** Uses the bundled original decoder for every real song and jingle. */
public final class MidiOracle {
    public static void main(String[] args) throws Exception {
        Path root=Paths.get(args[0]); List<Path> files=new ArrayList<>();
        for(String dir:new String[]{"songs","jingles"}) try(java.util.stream.Stream<Path> paths=Files.list(root.resolve(dir))) {
            paths.filter(p->p.toString().endsWith(".mid")).sorted().forEach(files::add);
        }
        long bytes=0;
        try(DataOutputStream out=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[1])))) {
            out.writeInt(0x4d494431);out.writeInt(files.size());
            for(Path p:files) {
                byte[] packed=Files.readAllBytes(p); int size=new Packet(363,packed).g4();
                if(size<4 || size>2097152) throw new IOException("Bad cache length");
                byte[] decoded=new byte[size];
                if(BZip2.read(decoded,size,packed,packed.length-4,4)!=size) throw new IOException("Short decode");
                byte[] name=root.relativize(p).toString().replace('\\','/').getBytes("UTF-8");
                CRC32 crc=new CRC32();crc.update(packed);
                out.writeShort(name.length);out.write(name);out.writeInt(packed.length);
                out.writeInt((int)crc.getValue());out.writeInt(size);out.write(decoded);bytes+=size;
            }
        }
        System.out.println(files.size()+" actual cache MIDI assets; "+bytes+" original Java decoded bytes.");
    }
}
