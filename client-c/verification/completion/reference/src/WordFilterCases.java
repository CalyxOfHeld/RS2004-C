import jagex2.wordenc.WordFilter;
import java.io.*;
import java.util.*;

// Source-driven diagnostic inputs, always evaluated by unchanged revision225
// methods. These are verification cases, not invented game rules.
public class WordFilterCases {
 static final Map<String,Integer> families=new LinkedHashMap<>();
 static final List<Match> badMatches=new ArrayList<>(),domainMatches=new ArrayList<>(),tldMatches=new ArrayList<>();
 static final List<String> numeric=new ArrayList<>();
 static final class Match {final int id;final String text;Match(int i,String s){id=i;text=s;}}
 static void add(List<String> out,String family,String s){out.add(s);families.put(family,families.getOrDefault(family,0)+1);}
 static String repeat(String s,int n){StringBuilder b=new StringBuilder();for(int i=0;i<n;i++)b.append(s);return b.toString();}
 static final char[] repertoire;
 static {repertoire=new char[130];for(int i=0;i<128;i++)repertoire[i]=(char)i;repertoire[128]=163;repertoire[129]=8364;}
 static char fromIndex(byte value){int v=value;if(v>=1&&v<=26)return (char)('a'+v-1);if(v==28)return '\'';if(v>=29&&v<=38)return (char)('0'+v-29);return ' ';}
 static void bad(List<String> out,int id,String s,String family){add(out,family,s);badMatches.add(new Match(id,s));}
 static void domain(List<String> out,int id,String s){add(out,"domain-context",s);domainMatches.add(new Match(id,s));}
 static void tld(List<String> out,int id,String s){add(out,"tld-context",s);tldMatches.add(new Match(id,s));}
 static void numeric(List<String> out,String s){add(out,"numeric",s);numeric.add(s);}
 static void appendFull(List<String> out) {
  String[] contexts={""," ","a","ab","x","'","1","0","_",".","@"};
  for(int id=0;id<WordFilter.badWords.length;id++) {
   String word=new String(WordFilter.badWords[id]);
   for(String before:contexts)for(String after:contexts)bad(out,id,before+word+after,"bad-boundaries");
   for(String separator:new String[]{".","*","'","-"," ",",","q","v","1"}) {
    StringBuilder b=new StringBuilder();for(int i=0;i<word.length();i++){if(i>0)b.append(separator);b.append(word.charAt(i));}bad(out,id,b.toString(),"bad-skips");
   }
   bad(out,id,repeat(word.substring(0,1),4)+word.substring(1),"bad-repeat");
   bad(out,id,word+repeat(word.substring(word.length()-1),4),"bad-repeat");
   for(int gap:new int[]{1,9,10,20,50})bad(out,id,word.substring(0,1)+repeat(".",gap)+word.substring(1),"bad-skip-ratio");
   for(int at=0;at<word.length();at++)for(char current:repertoire) {
    int size=WordFilter.getEmulatedSize((char)0,word.charAt(at),current,7326);
    if(size==1 && current!=word.charAt(at)) {
     String s=word.substring(0,at)+current+word.substring(at+1);
     bad(out,id,s,"bad-single-substitution");bad(out,id,"xx"+s+"yy","bad-single-substitution");
    } else if(size!=1)for(char next:repertoire)if(WordFilter.getEmulatedSize(next,word.charAt(at),current,7326)==2) {
     String s=word.substring(0,at)+current+next+word.substring(at+1);
     bad(out,id,s,"bad-pair-substitution");bad(out,id,"xx"+s+"yy","bad-pair-substitution");
    }
   }
   byte[][] pairs=WordFilter.badCombinations[id];
   if(pairs!=null)for(byte[] pair:pairs){bad(out,id,""+fromIndex(pair[0])+word+fromIndex(pair[1]),"bad-cache-exemptions");}
  }
  for(int id=0;id<WordFilter.domains.length;id++) {
   String word=new String(WordFilter.domains[id]);
   for(String s:new String[]{word,"name@"+word+".net","name (a) "+word+" dot net",word+" dot com",word+",com","a"+word+"b",word+"/com","("+word+")",word+".c","x@"+word,"x "+word+" x"})domain(out,id,s);
   for(int at=0;at<word.length();at++)for(char current:repertoire) {
    int size=WordFilter.getEmulatedDomainCharSize(-81,(char)0,word.charAt(at),current);
    if(size==1 && current!=word.charAt(at))domain(out,id,"name@"+word.substring(0,at)+current+word.substring(at+1)+".com");
    else if(size!=1)for(char next:repertoire)if(WordFilter.getEmulatedDomainCharSize(-81,next,word.charAt(at),current)==2)domain(out,id,"name@"+word.substring(0,at)+current+next+word.substring(at+1)+".com");
   }
  }
  String[] prefixes={""," ","x","x.","x,","x dot ","x d0t ","x.d.o.t.","@"};
  String[] suffixes={""," ","x","/path","\\path"," slash path","5lash path","."," dot path"};
  for(int id=0;id<WordFilter.tlds.length;id++)for(String before:prefixes)for(String after:suffixes)tld(out,id,before+new String(WordFilter.tlds[id])+after);
  String[] values={"0","1","255","256"},separators={"."," ","v","a","'","-","@@","A"};
  for(String a:values)for(String b:values)for(String c:values)for(String d:values)for(String sep:separators)numeric(out,a+sep+b+sep+c+sep+d);
  for(int place=0;place<4;place++)for(String v:new String[]{"9","10","99","999","00000000","000000001","4294967295","2147483648","9999999999999999999999999999"})for(String sep:separators){String[] group={"1","2","3","4"};group[place]=v;numeric(out,String.join(sep,group));}
  for(int count=1;count<=8;count++)for(String sep:separators)numeric(out,repeat("127"+sep,count));
  for(int length=1;length<=96;length++)numeric(out,"1.2."+repeat("9",length)+".4");
  for(int c=0;c<65536;c++)add(out,"utf16-unit-context","A"+(char)c+"B");
  for(char c:repertoire){add(out,"trim-boundary",""+c+"AbC");add(out,"trim-boundary","AbC"+c);}
  for(String allowed:WordFilter.ALLOWLIST)for(String before:contexts)for(String after:contexts)add(out,"allowlist",before+allowed.toUpperCase(Locale.ENGLISH)+after);
  Random r=new Random(225225);
  String alphabet="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .,!@#$%&*()-_/'\\|[]{}<>\t\n";
  for(int i=0;i<5000;i++){char[] s=new char[r.nextInt(161)];for(int j=0;j<s.length;j++)s[j]=alphabet.charAt(r.nextInt(alphabet.length()));add(out,"random-ascii",new String(s));}
  for(int length:new int[]{0,1,79,80,81,99,100,101,255,256,1024,4096,65536}){String s=repeat("ALL cook 127.0.0.1 x.com / ",1+length/26);add(out,"long-input",s.substring(0,Math.min(length,s.length())));}
  System.out.println("Expanded full-output families: "+families);
 }
 static void array(DataOutputStream out,char[] s)throws IOException{out.writeInt(s.length);for(char c:s)out.writeChar(c);}
 static char[] literal(int id){return (id==-1?"dot":id==-2?"slash":"(a)").toCharArray();}
 static void match(DataOutputStream out,Match m)throws IOException {
  out.writeByte(6);out.writeInt(m.id);char[] input=m.text.toCharArray(),result=input.clone();
  WordFilter.filter((byte)-102,m.id>=0?WordFilter.badCombinations[m.id]:null,result,m.id>=0?WordFilter.badWords[m.id]:literal(m.id));
  array(out,input);array(out,result);
 }
 static void context(DataOutputStream out,char[] input,char[] masked)throws IOException {
  out.writeByte(5);array(out,input);array(out,masked);
  for(int start=0;start<=input.length;start++){
   out.writeByte(WordFilter.getDomainAtFilterStatus(start,input,(byte)6,masked));
   out.writeByte(WordFilter.getDomainDotFilterStatus(masked,input,start-1,-808));
   out.writeByte(WordFilter.getTldDotFilterStatus(input,false,masked,start));
   out.writeByte(WordFilter.getTldSlashFilterStatus(masked,-678,start-1,input));
  }
 }
 static void contexts(DataOutputStream out,String s)throws IOException {
  char[] input=s.toCharArray();for(int variant=0;variant<4;variant++){
   char[] mask=input.clone();for(int i=0;i<mask.length;i++){
    if(variant==1 && Character.isLetterOrDigit(mask[i]))mask[i]='*';
    else if(variant==2)mask[i]='*';else if(variant==3)mask[i]=(i%2==0?'*':' ');
   }context(out,input,mask);
  }
 }
 static void fragment(DataOutputStream out,char[] s)throws IOException {
  out.writeByte(2);array(out,s);out.writeInt(WordFilter.firstFragmentId(5,s));out.writeBoolean(WordFilter.isBadFragment(s,6));
 }
 static void writeBranches(String path)throws IOException {
  TreeSet<Character> patterns=new TreeSet<>();for(int c=32;c<127;c++)patterns.add((char)c);
  patterns.add((char)0);patterns.add((char)163);patterns.add((char)8364);patterns.add((char)65535);
  for(char[][] group:new char[][][]{WordFilter.badWords,WordFilter.domains,WordFilter.tlds})for(char[] w:group)for(char c:w)patterns.add(c);
  long emulations=0,combos=0,fragments=0;
  try(DataOutputStream out=new DataOutputStream(new BufferedOutputStream(new FileOutputStream(path)))) {
   out.writeInt(0x57464232);
   for(char pattern:patterns) {
    for(int current=0;current<65536;current++){out.writeByte(1);out.writeChar(pattern);out.writeChar(current);out.writeChar(0);out.writeByte(WordFilter.getEmulatedSize((char)0,pattern,(char)current,7326));out.writeByte(WordFilter.getEmulatedDomainCharSize(-81,(char)0,pattern,(char)current));emulations++;}
    for(char current:repertoire)for(char next:repertoire){out.writeByte(1);out.writeChar(pattern);out.writeChar(current);out.writeChar(next);out.writeByte(WordFilter.getEmulatedSize(next,pattern,current,7326));out.writeByte(WordFilter.getEmulatedDomainCharSize(-81,next,pattern,current));emulations++;}
   }
   char[] fa="abcdefghijklmnopqrstuvwxyz0123456789'\0".toCharArray();
   fragment(out,new char[0]);fragments++;
   for(char a:fa){fragment(out,new char[]{a});fragments++;for(char b:fa){fragment(out,new char[]{a,b});fragments++;for(char c:fa){fragment(out,new char[]{a,b,c});fragments++;}}}
   Random random=new Random(225);
   for(int i=0;i<10000;i++){char[] s=new char[random.nextInt(9)];for(int j=0;j<s.length;j++)s[j]=fa[random.nextInt(fa.length)];fragment(out,s);fragments++;}
   for(String s:new String[]{"999999","zzzzzz","abcdef","ABC","a?b","\u20ac","a\0b"}){fragment(out,s.toCharArray());fragments++;}
   byte[][] synthetic=new byte[256][2];for(int i=0;i<256;i++){synthetic[i][0]=(byte)(i-128);synthetic[i][1]=(byte)(i*73+13);}
   out.writeByte(4);out.writeInt(synthetic.length);for(byte[] pair:synthetic){out.writeByte(pair[0]);out.writeByte(pair[1]);}
   for(int id=-1;id<WordFilter.badWords.length;id++){
    byte[][] pairs=id<0?synthetic:WordFilter.badCombinations[id];if(pairs==null)continue;
    int first=id<0?-128:1,last=id<0?127:38;
    for(int before=first;before<=last;before++)for(int after=first;after<=last;after++){
     out.writeByte(3);out.writeInt(id);out.writeByte(before);out.writeByte(after);out.writeBoolean(WordFilter.comboMatches(841,(byte)before,pairs,(byte)after));combos++;
    }
   }
   String ca="a0@.,/\\* ";contexts(out,"");
   for(int length=1;length<=4;length++){int cases=(int)Math.pow(ca.length(),length);for(int i=0;i<cases;i++){int value=i;char[] chars=new char[length];for(int j=0;j<length;j++){chars[j]=ca.charAt(value%ca.length());value/=ca.length();}contexts(out,new String(chars));}}
   for(int i=0;i<2000;i++){char[] chars=new char[random.nextInt(25)];for(int j=0;j<chars.length;j++)chars[j]=ca.charAt(random.nextInt(ca.length()));contexts(out,new String(chars));}
   for(String s:new String[]{"dot com slash path","x dot coM/path","  *** dot * /path","prefix.com","prefix.com/path","@example,com","example(a)domain"})contexts(out,s);
   for(Match m:badMatches)match(out,m);
   for(int id=-1;id>=-3;id--)for(String s:new String[]{"dot","d0t","d.o.t","doooooot","slash","sl4sh","5l45h","s.l.a.s.h","(a)","( a )","x(a)y","a dot com slash path"})match(out,new Match(id,s));
   for(Match m:domainMatches){char[] input=m.text.toCharArray(),result=input.clone(),at=input.clone(),dots=input.clone();
    WordFilter.filter((byte)-102,null,at,"(a)".toCharArray());WordFilter.filter((byte)-102,null,dots,"dot".toCharArray());
    WordFilter.filterDomain(dots,-706,at,WordFilter.domains[m.id],result);
    out.writeByte(7);out.writeInt(m.id);array(out,input);array(out,at);array(out,dots);array(out,result);
   }
   for(Match m:tldMatches){char[] input=m.text.toCharArray(),result=input.clone(),dots=input.clone(),slashes=input.clone();
    WordFilter.filter((byte)-102,null,dots,"dot".toCharArray());WordFilter.filter((byte)-102,null,slashes,"slash".toCharArray());
    WordFilter.filterTld(slashes,WordFilter.tldType[m.id],true,result,WordFilter.tlds[m.id],dots);
    out.writeByte(8);out.writeInt(m.id);array(out,input);array(out,dots);array(out,slashes);array(out,result);
   }
   for(String s:numeric){char[] input=s.toCharArray(),result=input.clone();WordFilter.filterFragments(8,result);out.writeByte(9);array(out,input);array(out,result);}
   out.writeByte(0);
  }
  System.out.println("Direct original branches: emulations="+emulations+" fragments="+fragments+" combos="+combos+" badMatches="+badMatches.size()+" domains="+domainMatches.size()+" tlds="+tldMatches.size()+" numeric="+numeric.size());
 }
}
