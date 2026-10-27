import jagex2.io.Packet;
import jagex2.wordenc.WordPack;
import jagex2.wordenc.WordFilter;
import jagex2.io.Jagfile;
import java.nio.file.*;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.*;

// Invokes unchanged bundled revision225 WordPack, English/default byte repertoire.
public class WordPackOracle {
 static void bytes(DataOutputStream out, byte[] b) throws IOException {out.writeInt(b.length);out.write(b);}
 static byte[] latin(String s) {return s.getBytes(StandardCharsets.ISO_8859_1);}
 static String text(byte[] b) {return new String(b,StandardCharsets.ISO_8859_1);}
 static byte[] decode(byte[] b) {return latin(WordPack.unpack(new Packet(363,b),118,b.length));}
 public static void main(String[] args) throws Exception {
  Locale.setDefault(Locale.ENGLISH);
  List<byte[]> inputs=new ArrayList<>();
  inputs.add(new byte[0]);
  for(int i=0;i<256;i++)inputs.add(new byte[]{(byte)i});
  for(int i=0;i<65536;i++)inputs.add(new byte[]{(byte)(i>>8),(byte)i});
  for(String s:new String[]{"Hello World!","hello? world. AGAIN! yes","   hello...WORLD!a?b", "@red@Hello", "e", "ee", "x", "ex", "xe", "xex", "Pounds: \u00a3 $%", "Mixed_Unsupported^{}|~<>"}) inputs.add(latin(s));
  for(int length=0;length<=101;length++)for(byte ch:new byte[]{'e','x','!',' ',(byte)0xa3}) {byte[] b=new byte[length];Arrays.fill(b,ch);inputs.add(b);}
  // Actual revision225 cache strings exercise the codec's real data repertoire.
  // This only supplies text inputs; native WordFilter is not implemented here.
  WordFilter.unpack(new Jagfile(Files.readAllBytes(Paths.get(args[1])),false));
  int cacheWords=0;
  for(char[][] group:new char[][][]{WordFilter.badWords,WordFilter.domains,WordFilter.tlds})for(char[] word:group){
   for(char ch:word)if(ch>255)throw new IllegalStateException("Cache text outside native Latin-1 repertoire");
   inputs.add(latin(new String(word)));cacheWords++;
  }
  Random random=new Random(225);
  for(int i=0;i<3000;i++){byte[] b=new byte[random.nextInt(161)];random.nextBytes(b);inputs.add(b);}
  List<byte[]> raw=new ArrayList<>();raw.add(new byte[0]);
  for(int i=0;i<256;i++)raw.add(new byte[]{(byte)i});
  for(int i=0;i<65536;i++)raw.add(new byte[]{(byte)(i>>8),(byte)i});
  for(int length=0;length<=101;length++)for(int v:new int[]{0,255,221,18,222,13}){byte[] b=new byte[length];Arrays.fill(b,(byte)v);raw.add(b);}
  for(int i=0;i<5000;i++){byte[] b=new byte[random.nextInt(102)];random.nextBytes(b);raw.add(b);}
  int rejected=0;
  try(DataOutputStream out=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[0])))) {
   out.writeInt(0x57503232);out.writeInt(1);out.writeInt(inputs.size()*2);
   for(byte[] input:inputs)for(boolean flush:new boolean[]{false,true}){
    out.writeInt(flush?1:0);bytes(out,input);
    Packet p=new Packet(363,new byte[256]);WordPack.pack(p,flush,text(input));
    byte[] packed=Arrays.copyOf(p.data,p.pos);bytes(out,packed);bytes(out,decode(packed));
   }
   out.writeInt(raw.size());
   for(byte[] b:raw){bytes(out,b);byte[] decoded;
    try{decoded=decode(b);}catch(ArrayIndexOutOfBoundsException e){out.writeInt(0);rejected++;continue;}
    out.writeInt(1);bytes(out,decoded);
   }
  }
  System.out.println("Actual revision225 wordenc cache strings: "+cacheWords+" (badWords/domains/tlds), both flush modes.");
  System.out.println("PASS original WordPack capture: "+inputs.size()*2+" pack cases; "+raw.size()+" raw decode cases; "+rejected+" original100-character overflows; locale English; ISO-8859-1 text.");
 }
}
