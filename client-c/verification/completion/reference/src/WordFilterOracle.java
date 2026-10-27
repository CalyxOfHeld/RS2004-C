import jagex2.io.Jagfile;
import jagex2.wordenc.WordFilter;
import java.io.*;
import java.nio.file.*;
import java.util.*;

// Unchanged revision225 tables, primitives and complete filter expectations.
// Expanded source-derived families/branches are captured by WordFilterCases.
public class WordFilterOracle {
 static void word(DataOutputStream out,char[] w)throws IOException{out.writeInt(w.length);for(char c:w)out.writeByte(c);}
 static void text(DataOutputStream out,String s)throws IOException{out.writeInt(s.length());for(int i=0;i<s.length();i++)out.writeChar(s.charAt(i));}
 public static void main(String[] args)throws Exception {
  Locale.setDefault(Locale.ENGLISH);
  WordFilter.unpack(new Jagfile(Files.readAllBytes(Paths.get(args[0])),false));
  try(DataOutputStream out=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[1])))){
   out.writeInt(0x57463232);out.writeInt(WordFilter.fragments.length);
   for(int v:WordFilter.fragments)out.writeInt(v);
   out.writeInt(WordFilter.badWords.length);
   for(int i=0;i<WordFilter.badWords.length;i++){
    word(out,WordFilter.badWords[i]);byte[][] pairs=WordFilter.badCombinations[i];out.writeInt(pairs==null?0:pairs.length);
    if(pairs!=null)for(byte[] pair:pairs){out.writeByte(pair[0]);out.writeByte(pair[1]);}
   }
   out.writeInt(WordFilter.domains.length);for(char[] w:WordFilter.domains)word(out,w);
   out.writeInt(WordFilter.tlds.length);for(int i=0;i<WordFilter.tlds.length;i++){out.writeInt(WordFilter.tldType[i]);word(out,WordFilter.tlds[i]);}
  }
  List<String> inputs=new ArrayList<>(Arrays.asList(""," ","hello WORLD!","cook cook's cooks seeks sheet","normal\t text\n ","127.0.0.1","visit example.com","www.example.net/path","name@example.com","cost \u00a3 10 or \u20ac 20"));
  for(int i=0;i<256;i++)inputs.add("a"+(char)i+"b");
  for(char[] w:WordFilter.badWords){String s=new String(w);inputs.add(s);inputs.add("before "+s+" after");inputs.add(s.toUpperCase(Locale.ENGLISH));inputs.add(String.join(".",s.split("")));}
  for(char[] w:WordFilter.domains){String s=new String(w);inputs.add(s+".com");inputs.add("name@"+s+".net");}
  for(char[] w:WordFilter.tlds)inputs.add("www.example."+new String(w)+"/path");
  Random random=new Random(225);for(int i=0;i<1000;i++){char[] s=new char[random.nextInt(101)];for(int j=0;j<s.length;j++)s[j]=(char)random.nextInt(256);inputs.add(new String(s));}
  WordFilterCases.appendFull(inputs);
  try(DataOutputStream out=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[2])))){
   out.writeInt(0x57464632);out.writeInt(inputs.size());
   for(String s:inputs){text(out,s);text(out,WordFilter.filter(s,0));}
  }
  List<char[]> helperInputs=new ArrayList<>();
  for(int i=0;i<65536;i++)helperInputs.add(new char[]{' ',' ',(char)i,' ',' ','A','b','C','!',(char)i,' '});
  for(String s:new String[]{"","ALL UPPER","MiXeD CaSe","ABcDE FGhIJ","*ABC DEfg!","ABCD...","  a\t\t b\n ","\u20ac\u00a3"})helperInputs.add(s.toCharArray());
  Random hr=new Random(225);for(int i=0;i<2000;i++){char[] s=new char[hr.nextInt(129)];for(int j=0;j<s.length;j++)s[j]=(char)hr.nextInt(65536);helperInputs.add(s);}
  try(DataOutputStream out=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(args[3])))){
   out.writeInt(0x57464832);out.writeInt(helperInputs.size());
   for(char[] original:helperInputs){
    text(out,new String(original));char[] normalized=original.clone();WordFilter.filterCharacters(normalized,0);text(out,new String(normalized));
    char[] masked=original.clone();for(int j=0;j<masked.length;j++){if(j%5==0)masked[j]='*';else if(masked[j]>='A' && masked[j]<='Z')masked[j]+=32;}
    text(out,new String(masked));WordFilter.replaceUpperCases(masked,135,original);text(out,new String(masked));
    WordFilter.formatUpperCases((byte)6,masked);text(out,new String(masked));
   }
  }
  if(args.length>4)WordFilterCases.writeBranches(args[4]);
  System.out.println("Original UTF-16 normalization/case helper cases="+helperInputs.size()+" (all65536 code units plus strings/random).");
  System.out.println("Original tables: fragments="+WordFilter.fragments.length+" bad="+WordFilter.badWords.length+" domains="+WordFilter.domains.length+" tlds="+WordFilter.tlds.length);
  System.out.println("Original full-filter UTF-16 expectations="+inputs.size()+"; English locale; compare with native full pipeline separately.");
 }
}
