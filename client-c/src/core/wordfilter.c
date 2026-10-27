/* revision225 WordFilter.unpack/readBadWords/readDomains/readFragments/readTld. */
#include "core/wordfilter.h"
#include "core/packet.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>

static void free_words(WordFilterWord *words, size_t count) {
    if (!words) return;
    for (size_t i=0;i<count;i++) {free(words[i].chars);free(words[i].combinations);}
    free(words);
}
void wordfilter_tables_free(WordFilterTables *tables) {
    if (!tables) return;
    free(tables->fragments);
    free_words(tables->bad_words,tables->bad_count);
    free_words(tables->domains,tables->domain_count);
    free_words(tables->tlds,tables->tld_count);
    memset(tables,0,sizeof(*tables));
}
static bool read_words(const uint8_t *data,size_t size,WordFilterWord **out,size_t *count,int kind) {
    if (!data) return false;
    Packet p=packet_wrap((uint8_t*)data,size);
    int32_t n=g4(&p);
    size_t minimum=kind==0?1u:2u;
    if (p.error || n<0 || (size_t)n>(size-p.pos)/minimum || (size_t)n>SIZE_MAX/sizeof(WordFilterWord)) return false;
    WordFilterWord *words=n?calloc((size_t)n,sizeof(*words)):NULL;
    if (n && !words) return false;
    *out=words;*count=(size_t)n;
    for (int32_t i=0;i<n;i++) {
        WordFilterWord *w=&words[i];
        if (kind==2) w->type=g1(&p);
        int length=g1(&p);
        if (p.error || (size_t)length>size-p.pos) return false;
        w->length=(size_t)length;
        w->chars=length?malloc((size_t)length):NULL;
        if (length && !w->chars) return false;
        if (length) gdata(&p,w->chars,(size_t)length);
        if (kind==1) {
            int combinations=g1(&p);
            if (p.error || (size_t)combinations>(size-p.pos)/2) return false;
            w->combination_count=(size_t)combinations;
            w->combinations=combinations?malloc((size_t)combinations*sizeof(*w->combinations)):NULL;
            if (combinations && !w->combinations) return false;
            for (int j=0;j<combinations;j++) {w->combinations[j][0]=g1b(&p);w->combinations[j][1]=g1b(&p);}
        }
        if (p.error) return false;
    }
    return true;
}
bool wordfilter_tables_decode(WordFilterTables *tables,const uint8_t *fragments,size_t fragments_size,
    const uint8_t *bad,size_t bad_size,const uint8_t *domains,size_t domains_size,const uint8_t *tlds,size_t tlds_size) {
    if (!tables || !fragments) return false;
    WordFilterTables next={0};
    Packet p=packet_wrap((uint8_t*)fragments,fragments_size);
    int32_t n=g4(&p);
    if (p.error || n<0 || (size_t)n>(fragments_size-p.pos)/2 || (size_t)n>SIZE_MAX/sizeof(*next.fragments)) return false;
    next.fragment_count=(size_t)n;next.fragments=n?malloc((size_t)n*sizeof(*next.fragments)):NULL;
    if (n && !next.fragments) return false;
    for (int32_t i=0;i<n;i++)next.fragments[i]=(uint16_t)g2(&p);
    if (p.error || !read_words(bad,bad_size,&next.bad_words,&next.bad_count,1) ||
        !read_words(domains,domains_size,&next.domains,&next.domain_count,0) ||
        !read_words(tlds,tlds_size,&next.tlds,&next.tld_count,2)) {
        wordfilter_tables_free(&next);return false;
    }
    wordfilter_tables_free(tables);*tables=next;return true;
}
bool wordfilter_tables_load(WordFilterTables *tables,Jagfile *archive) {
    if (!tables || !archive) return false;
    const char *names[]={"fragmentsenc.txt","badenc.txt","domainenc.txt","tldlist.txt"};
    uint8_t *data[4]={0};size_t sizes[4]={0};
    for (int i=0;i<4;i++)data[i]=jagfile_read(archive,names[i],NULL,&sizes[i],NULL);
    bool ok=wordfilter_tables_decode(tables,data[0],sizes[0],data[1],sizes[1],data[2],sizes[2],data[3],sizes[3]);
    for (int i=0;i<4;i++)free(data[i]);
    return ok;
}

/* WordFilter.filterCharacters/replaceUpperCases/formatUpperCases. */
bool wordfilter_normalize(uint16_t *text,size_t length) {
    if (!text && length) return false;
    size_t count=0;
    for (size_t i=0;i<length;i++) {
        uint16_t ch=text[i];
        text[count]=((ch>=32 && ch<=127) || ch=='\n' || ch=='\t' || ch==163 || ch==8364)?ch:' ';
        if (!count || text[count]!=' ' || text[count-1]!=' ')count++;
    }
    while (count<length)text[count++]=' ';
    return true;
}
bool wordfilter_restore_case(uint16_t *filtered,const uint16_t *original,size_t length) {
    if (length && (!filtered || !original)) return false;
    for (size_t i=0;i<length;i++)if(filtered[i]!='*' && original[i]>='A' && original[i]<='Z')filtered[i]=original[i];
    return true;
}
bool wordfilter_format_case(uint16_t *text,size_t length) {
    if (!text && length) return false;
    bool initial=true;
    for (size_t i=0;i<length;i++) {
        uint16_t ch=text[i];bool lower=ch>='a' && ch<='z',upper=ch>='A' && ch<='Z';
        if (!lower && !upper)initial=true;
        else if(initial) {if(lower)initial=false;}
        else if(upper)text[i]=(uint16_t)(ch+'a'-'A');
    }
    return true;
}

/* Complete revision225 filtering stages. All indices mirror Java int indices;
 * char values remain UTF-16. Only obfuscation guards with fixed call constants
 * and unused locals are omitted. */
static bool wf_lower(uint16_t c) {return c>='a' && c<='z';}
static bool wf_upper(uint16_t c) {return c>='A' && c<='Z';}
static bool wf_alpha(uint16_t c) {return wf_lower(c)||wf_upper(c);}
static bool wf_digit(uint16_t c) {return c>='0' && c<='9';}
static bool wf_symbol(uint16_t c) {return !wf_alpha(c) && !wf_digit(c);}
/* Original isLowerCaseAlpha means an ignorable lower-case letter or nonletter. */
static bool wf_weak(uint16_t c) {return !wf_lower(c)||c=='v'||c=='x'||c=='j'||c=='q'||c=='z';}
static int32_t wf_signed(uint32_t v) {return v<=INT32_MAX?(int32_t)v:-(int32_t)(UINT32_MAX-v)-1;}
static int32_t wf_fragment_id(const uint16_t *word,int length) {
    if(length>6)return 0;
    uint32_t value=0;
    for(int i=length-1;i>=0;i--) {
        uint16_t c=word[i];
        if(wf_lower(c))value=value*38u+c-'a'+1u;
        else if(c=='\'')value=value*38u+27u;
        else if(wf_digit(c))value=value*38u+c-'0'+28u;
        else if(c!=0)return 0;
    }
    return wf_signed(value);
}
static bool wf_bad_fragment(const WordFilterTables *t,const uint16_t *word,int length) {
    bool numeric=true;
    for(int i=0;i<length;i++)if(!wf_digit(word[i]) && word[i]!=0)numeric=false;
    if(numeric)return true;
    int32_t id=wf_fragment_id(word,length);
    int low=0,high=(int)t->fragment_count-1;
    if(high<0)return false;
    if(id==t->fragments[low] || id==t->fragments[high])return true;
    do {
        int mid=low+(high-low)/2;
        if(id==t->fragments[mid])return true;
        if(id<t->fragments[mid])high=mid;else low=mid;
    }while(low!=high && low+1!=high);
    return false;
}
static int8_t wf_index(uint16_t c) {
    if(wf_lower(c))return (int8_t)(c-'a'+1);
    if(c=='\'')return 28;
    if(wf_digit(c))return (int8_t)(c-'0'+29);
    return 27;
}
static bool wf_combo(int8_t before,const int8_t pairs[][2],int count,int8_t after) {
    if(!pairs || count<=0)return false;
    int low=0,high=count-1;
    if((pairs[low][0]==before && pairs[low][1]==after)||(pairs[high][0]==before && pairs[high][1]==after))return true;
    do {
        int mid=low+(high-low)/2;
        if(pairs[mid][0]==before && pairs[mid][1]==after)return true;
        if(before<pairs[mid][0] || (before==pairs[mid][0] && after<pairs[mid][1]))high=mid;
        else low=mid;
    }while(low!=high && low+1!=high);
    return false;
}
/* Java getEmulatedSize(next,pattern,current,7326). */
static int wf_emulated(uint16_t next,uint16_t pattern,uint16_t current) {
    if(pattern==current)return 1;
    switch(pattern) {
    case 'a':if(current=='4'||current=='@'||current=='^')return 1;return current=='/'&&next=='\\'?2:0;
    case 'b':if(current=='6'||current=='8')return 1;return current=='1'&&next=='3'?2:0;
    case 'c':return current=='('||current=='<'||current=='{'||current=='['?1:0;
    case 'd':return current=='['&&next==')'?2:0;
    case 'e':return current=='3'||current==8364?1:0;
    case 'f':if(current=='p'&&next=='h')return 2;return current==163?1:0;
    case 'g':return current=='9'||current=='6'?1:0;
    case 'h':return current=='#'?1:0;
    case 'i':return current=='y'||current=='l'||current=='j'||current=='1'||current=='!'||current==':'||current==';'||current=='|'?1:0;
    case 'l':return current=='1'||current=='|'||current=='i'?1:0;
    case 'o':if(current=='0'||current=='*')return 1;
        return (current=='('&&next==')')||(current=='['&&next==']')||(current=='{'&&next=='}')||(current=='<'&&next=='>')?2:0;
    case 's':return current=='5'||current=='z'||current=='$'||current=='2'?1:0;
    case 't':return current=='7'||current=='+'?1:0;
    case 'u':if(current=='v')return 1;
        return (current=='\\'&&next=='/')||(current=='\\'&&next=='|')||(current=='|'&&next=='/')?2:0;
    case 'v':return (current=='\\'&&next=='/')||(current=='\\'&&next=='|')||(current=='|'&&next=='/')?2:0;
    case 'w':return current=='v'&&next=='v'?2:0;
    case 'x':return (current==')'&&next=='(')||(current=='}'&&next=='{')||(current==']'&&next=='[')||(current=='>'&&next=='<')?2:0;
    case '0':if(current=='o'||current=='O')return 1;
        return (current=='('&&next==')')||(current=='{'&&next=='}')||(current=='['&&next==']')?2:0;
    case '1':return current=='l'?1:0;
    case ',':return current=='.'?1:0;
    case '.':return current==','?1:0;
    case '!':return current=='i'?1:0;
    default:return 0;
    }
}
static int wf_domain_emulated(uint16_t next,uint16_t pattern,uint16_t current) {
    if(pattern==current)return 1;
    if(pattern=='o' && current=='0')return 1;
    if(pattern=='o' && current=='(' && next==')')return 2;
    if(pattern=='c' && (current=='('||current=='<'||current=='['))return 1;
    if(pattern=='e' && current==8364)return 1;
    if(pattern=='s' && current=='$')return 1;
    if(pattern=='l' && current=='i')return 1;
    return 0;
}
/* Java filter(byte,byte[][],char[],char[]). Repeated matches, ignored digits,
 * punctuation and skip ratio are deliberately handled in original order. */
static void wf_bad_match(const WordFilterTables *t,const int8_t pairs[][2],int pair_count,
    uint16_t *text,int n,const uint8_t *word,int length) {
    if(length>n)return;
    int step=1;
    for(int start=0;start<=n-length;start+=step) {
        int end=start,matched=0,skipped=0;step=1;
        bool punctuation=false,digit_match=false,digit_skip=false;
        while(end<n && !(digit_match && digit_skip)) {
            uint16_t current=text[end],next=end+1<n?text[end+1]:0;
            int advance=matched<length?wf_emulated(next,word[matched],current):0;
            if(advance>0) {
                if((advance==1 && wf_digit(current))||(advance==2 && (wf_digit(current)||wf_digit(next))))digit_match=true;
                end+=advance;matched++;
            } else {
                if(matched==0)break;
                advance=wf_emulated(next,word[matched-1],current);
                if(advance>0) {end+=advance;if(matched==1)step++;}
                else {
                    if(matched>=length || !wf_weak(current))break;
                    if(wf_symbol(current) && current!='\'')punctuation=true;
                    if(wf_digit(current))digit_skip=true;
                    end++;skipped++;
                    if(wf_signed((uint32_t)skipped*100u)/(end-start)>90)break;
                }
            }
        }
        if(matched>=length && (!digit_match || !digit_skip)) {
            bool censor=true;
            if(punctuation) {
                bool left=start==0 || (wf_symbol(text[start-1]) && text[start-1]!='\'');
                bool right=end>=n || (wf_symbol(text[end]) && text[end]!='\'');
                if(!left || !right) {
                    bool unknown=false;
                    for(int at=left?start:start-2;!unknown && at<end;at++) {
                        if(at<0 || (wf_symbol(text[at]) && text[at]!='\''))continue;
                        uint16_t fragment[3]={0};int used=0;
                        for(;used<3 && at+used<n && (!wf_symbol(text[at+used])||text[at+used]=='\'');used++)fragment[used]=text[at+used];
                        bool candidate=used!=0;
                        if(used<3 && at>0 && (!wf_symbol(text[at-1])||text[at-1]=='\''))candidate=false;
                        if(candidate && !wf_bad_fragment(t,fragment,3))unknown=true;
                    }
                    if(!unknown)censor=false;
                }
            } else {
                uint16_t before=start>0?text[start-1]:' ',after=end<n?text[end]:' ';
                if(pairs && wf_combo(wf_index(before),pairs,pair_count,wf_index(after)))censor=false;
            }
            if(censor) {
                int digits=0,letters=0;
                for(int i=start;i<end;i++){if(wf_digit(text[i]))digits++;else if(wf_alpha(text[i]))letters++;}
                if(digits<=letters)for(int i=start;i<end;i++)text[i]='*';
            }
        }
    }
}
/* Four original context routines share their direction-dependent mechanics. */
static int wf_before(const uint16_t *text,const uint16_t *masked,int start,bool at_sign) {
    if(start==0)return 2;
    for(int i=start-1;i>=0 && wf_symbol(text[i]);i--)
        if(at_sign?text[i]=='@':(text[i]=='.'||text[i]==','))return 3;
    int stars=0;
    for(int i=start-1;i>=0 && wf_symbol(masked[i]);i--)if(masked[i]=='*')stars++;
    if(stars>=3)return 4;
    return wf_symbol(text[start-1])?1:0;
}
static int wf_after(const uint16_t *text,const uint16_t *masked,int n,int last,bool slash) {
    if(last+1==n)return 2;
    for(int i=last+1;i<n && wf_symbol(text[i]);i++)
        if(slash?(text[i]=='\\'||text[i]=='/'):(text[i]=='.'||text[i]==','))return 3;
    int stars=0;
    for(int i=last+1;i<n && wf_symbol(masked[i]);i++)if(masked[i]=='*')stars++;
    if(stars>=(slash?5:3))return 4;
    return wf_symbol(text[last+1])?1:0;
}
static void wf_domain_match(uint16_t *text,int n,const WordFilterWord *word,
    const uint16_t *at,const uint16_t *dots) {
    int length=(int)word->length,step=1;
    if(length>n)return;
    for(int start=0;start<=n-length;start+=step) {
        int end=start,matched=0;step=1;
        while(end<n) {
            uint16_t current=text[end],next=end+1<n?text[end+1]:0;
            int advance=matched<length?wf_domain_emulated(next,word->chars[matched],current):0;
            if(advance>0){end+=advance;matched++;}
            else {
                if(matched==0)break;
                advance=wf_domain_emulated(next,word->chars[matched-1],current);
                if(advance>0){end+=advance;if(matched==1)step++;}
                else {if(matched>=length || !wf_symbol(current))break;end++;}
            }
        }
        if(matched>=length && (wf_before(text,at,start,true)>2 || wf_after(text,dots,n,end-1,false)>2))
            for(int i=start;i<end;i++)text[i]='*';
    }
}
static void wf_tld_match(uint16_t *text,int n,const WordFilterWord *word,
    const uint16_t *dots,const uint16_t *slashes) {
    int length=(int)word->length,step=1;
    if(length>n)return;
    for(int start=0;start<=n-length;start+=step) {
        int end=start,matched=0;step=1;
        while(end<n) {
            uint16_t current=text[end],next=end+1<n?text[end+1]:0;
            int advance=matched<length?wf_domain_emulated(next,word->chars[matched],current):0;
            if(advance>0){end+=advance;matched++;}
            else {
                if(matched==0)break;
                advance=wf_domain_emulated(next,word->chars[matched-1],current);
                if(advance>0){end+=advance;if(matched==1)step++;}
                else {if(matched>=length || !wf_symbol(current))break;end++;}
            }
        }
        if(matched>=length) {
            int left=wf_before(text,dots,start,false),right=wf_after(text,slashes,n,end-1,true);
            bool censor=(word->type==1 && left>0 && right>0) ||
                (word->type==2 && ((left>2 && right>0)||(left>0 && right>2))) ||
                (word->type==3 && left>0 && right>2);
            /* Java computes an additional type3 left>2/right>0 boolean but
             * never uses it. It must not be added to the censor condition. */
            if(!censor)continue;
            int first=start,last=end-1;
            if(left>2) {
                if(left==4) {
                    bool found=false;
                    for(int i=start-1;i>=0;i--) {
                        if(found){if(dots[i]!='*')break;first=i;}
                        else if(dots[i]=='*'){first=i;found=true;}
                    }
                }
                bool found=false;
                for(int i=first-1;i>=0;i--) {
                    if(found){if(wf_symbol(text[i]))break;first=i;}
                    else if(!wf_symbol(text[i])){found=true;first=i;}
                }
            }
            if(right>2) {
                if(right==4) {
                    bool found=false;
                    for(int i=last+1;i<n;i++) {
                        if(found){if(slashes[i]!='*')break;last=i;}
                        else if(slashes[i]=='*'){last=i;found=true;}
                    }
                }
                bool found=false;
                for(int i=last+1;i<n;i++) {
                    if(found){if(wf_symbol(text[i]))break;last=i;}
                    else if(!wf_symbol(text[i])){found=true;last=i;}
                }
            }
            for(int i=first;i<=last;i++)text[i]='*';
        }
    }
}
static void wf_numeric_fragments(uint16_t *text,int n) {
    int offset=0,groups=0,first=0;
    for(;;) {
        int start=offset;
        while(start<n && !wf_digit(text[start]))start++;
        if(start==n)return;
        bool separator=false;
        for(int i=offset;i<start && !separator;i++)if(!wf_symbol(text[i]) && !wf_weak(text[i]))separator=true;
        if(separator)groups=0;
        if(groups==0)first=start;
        offset=start;while(offset<n && wf_digit(text[offset]))offset++;
        uint32_t value=0;
        for(int i=start;i<offset;i++)value=value*10u+text[i]-'0';
        if(wf_signed(value)<=255 && offset-start<=8)groups++;else groups=0;
        if(groups==4){for(int i=first;i<offset;i++)text[i]='*';groups=0;}
    }
}
static bool wf_tables_valid(const WordFilterTables *t) {
    if(!t || !t->fragment_count || t->fragment_count>INT_MAX || !t->fragments)return false;
    const WordFilterWord *groups[]={t->bad_words,t->domains,t->tlds};
    size_t counts[]={t->bad_count,t->domain_count,t->tld_count};
    for(int group=0;group<3;group++) {
        if(counts[group]>INT_MAX || (counts[group] && !groups[group]))return false;
        for(size_t i=0;i<counts[group];i++) {
            const WordFilterWord *w=&groups[group][i];
            if(!w->length || w->length>INT_MAX || !w->chars || w->combination_count>INT_MAX ||
                (w->combination_count && !w->combinations))return false;
        }
    }
    return true;
}
bool wordfilter_apply(const WordFilterTables *tables,const uint16_t *input,size_t length,
    uint16_t *output,size_t capacity,size_t *out_length) {
    if(out_length)*out_length=0;
    if(!wf_tables_valid(tables) || (!input && length) || !output || !capacity ||
        length>(size_t)INT_MAX-2 || length>SIZE_MAX/(5*sizeof(uint16_t)))return false;
    /* Five equal buffers: normalized original, lower original, filtered,
     * and two stage clones. The result remains internal until success. */
    size_t span=length?length:1;
    uint16_t *memory=malloc(span*5*sizeof(*memory));if(!memory)return false;
    uint16_t *original=memory,*lower=memory+span,*filtered=memory+2*span,*clone1=memory+3*span,*clone2=memory+4*span;
    if(length)memcpy(original,input,length*sizeof(*input));
    wordfilter_normalize(original,length);
    size_t first=0,last=length;
    while(first<last && original[first]<=32)first++;
    while(last>first && original[last-1]<=32)last--;
    size_t count=last-first;
    if(count>length || count>INT_MAX){free(memory);return false;}
    if(count)memmove(original,original+first,count*sizeof(*original));
    int n=(int)count;
    for(int i=0;i<n;i++)lower[i]=wf_upper(original[i])?(uint16_t)(original[i]+'a'-'A'):original[i];
    if(count)memcpy(filtered,lower,count*sizeof(*filtered));
    static const uint8_t dot[]={'d','o','t'},slash[]={'s','l','a','s','h'},at[]={'(','a',')'};
    if(count){memcpy(clone1,filtered,count*sizeof(*filtered));memcpy(clone2,filtered,count*sizeof(*filtered));}
    wf_bad_match(tables,NULL,0,clone1,n,dot,3);wf_bad_match(tables,NULL,0,clone2,n,slash,5);
    for(size_t i=0;i<tables->tld_count;i++)wf_tld_match(filtered,n,&tables->tlds[i],clone1,clone2);
    for(int pass=0;pass<2;pass++)for(size_t i=tables->bad_count;i>0;i--) {
        const WordFilterWord *w=&tables->bad_words[i-1];
        wf_bad_match(tables,w->combinations,(int)w->combination_count,filtered,n,w->chars,(int)w->length);
    }
    if(count){memcpy(clone1,filtered,count*sizeof(*filtered));memcpy(clone2,filtered,count*sizeof(*filtered));}
    wf_bad_match(tables,NULL,0,clone1,n,at,3);wf_bad_match(tables,NULL,0,clone2,n,dot,3);
    for(size_t i=tables->domain_count;i>0;i--)wf_domain_match(filtered,n,&tables->domains[i-1],clone1,clone2);
    wf_numeric_fragments(filtered,n);
    static const char *allowlist[]={"cook","cook's","cooks","seeks","sheet"};
    for(size_t w=0;w<sizeof(allowlist)/sizeof(*allowlist);w++) {
        size_t size=strlen(allowlist[w]);
        for(size_t start=0;size<=count && start<=count-size;start++) {
            size_t i=0;while(i<size && lower[start+i]==(uint8_t)allowlist[w][i])i++;
            if(i==size)for(i=0;i<size;i++)filtered[start+i]=(uint8_t)allowlist[w][i];
        }
    }
    wordfilter_restore_case(filtered,original,count);wordfilter_format_case(filtered,count);
    first=0;last=count;
    while(first<last && filtered[first]<=32)first++;
    while(last>first && filtered[last-1]<=32)last--;
    count=last-first;
    if(count>=capacity){free(memory);return false;}
    if(count)memcpy(output,filtered+first,count*sizeof(*output));
    output[count]=0;
    if(out_length)*out_length=count;
    free(memory);return true;
}
