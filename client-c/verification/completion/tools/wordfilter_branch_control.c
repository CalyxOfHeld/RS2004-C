/* Direct controls compile the actual native source so private stages can be
 * compared with unchanged original methods without adding production hooks. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
static int fail_alloc_after=-1;
static void *controlled_malloc(size_t size) {
    if(fail_alloc_after==0)return NULL;
    if(fail_alloc_after>0)fail_alloc_after--;
    return malloc(size);
}
static void *controlled_calloc(size_t count,size_t size) {
    if(fail_alloc_after==0)return NULL;
    if(fail_alloc_after>0)fail_alloc_after--;
    return calloc(count,size);
}
#define malloc controlled_malloc
#define calloc controlled_calloc
#include "../../../src/core/wordfilter.c"
#undef malloc
#undef calloc

typedef struct {uint16_t *memory,*data;int n;} Text;
static int integer(FILE *f) {unsigned char b[4];assert(fread(b,1,4,f)==4);return wf_signed((uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3]);}
static int byte(FILE *f) {int v=fgetc(f);assert(v!=EOF);return v;}
static uint16_t unit(FILE *f) {int a=byte(f),b=byte(f);return (uint16_t)((a<<8)|b);}
static Text make_text(int n) {assert(n>=0 && n<=1048577);Text t={0};t.n=n;t.memory=malloc(((size_t)n+2)*sizeof(*t.memory));assert(t.memory);t.data=t.memory+1;for(int i=0;i<n+2;i++)t.memory[i]=0xa55a;return t;}
static Text read_text(FILE *f) {Text t=make_text(integer(f));for(int i=0;i<t.n;i++)t.data[i]=unit(f);return t;}
static void guard(Text t) {assert(t.memory[0]==0xa55a && t.memory[t.n+1]==0xa55a);}
static void release(Text t) {guard(t);free(t.memory);}
static Text clone(Text in) {Text out=make_text(in.n);memcpy(out.data,in.data,(size_t)in.n*sizeof(*in.data));return out;}
static void same(Text a,Text b,int op,uint64_t record) {
    if(a.n!=b.n || memcmp(a.data,b.data,(size_t)a.n*sizeof(*a.data)))fprintf(stderr,"Original branch mismatch op%d record%llu length%d/%d\n",op,(unsigned long long)record,a.n,b.n);
    assert(a.n==b.n && !memcmp(a.data,b.data,(size_t)a.n*sizeof(*a.data)));guard(a);guard(b);
}
static const uint8_t dot[]={'d','o','t'},slash[]={'s','l','a','s','h'},at[]={'(','a',')'};
static uint64_t table_hash(const WordFilterTables *t) {
    uint64_t h=1469598103934665603ULL;
    for(size_t i=0;i<t->fragment_count;i++)h=(h^t->fragments[i])*1099511628211ULL;
    const WordFilterWord *groups[]={t->bad_words,t->domains,t->tlds};size_t counts[]={t->bad_count,t->domain_count,t->tld_count};
    for(int group=0;group<3;group++)for(size_t i=0;i<counts[group];i++) {
        const WordFilterWord *w=&groups[group][i];h=(h^w->type^w->length^w->combination_count)*1099511628211ULL;
        for(size_t j=0;j<w->length;j++)h=(h^w->chars[j])*1099511628211ULL;
        for(size_t j=0;j<w->combination_count;j++)for(int k=0;k<2;k++)h=(h^(uint8_t)w->combinations[j][k])*1099511628211ULL;
    }
    return h;
}
static void loader_failures(WordFilterTables *tables,Jagfile *jag) {
    const char *names[]={"fragmentsenc.txt","badenc.txt","domainenc.txt","tldlist.txt"};
    uint8_t *data[4];size_t sizes[4];for(int i=0;i<4;i++){data[i]=jagfile_read(jag,names[i],NULL,&sizes[i],NULL);assert(data[i]);}
    WordFilterTables before=*tables;uint64_t hash=table_hash(tables);int failures=0;
    for(int allocation=0;allocation<10000;allocation++) {
        fail_alloc_after=allocation;
        bool ok=wordfilter_tables_decode(tables,data[0],sizes[0],data[1],sizes[1],data[2],sizes[2],data[3],sizes[3]);
        fail_alloc_after=-1;
        if(ok){assert(table_hash(tables)==hash);break;}
        failures++;assert(!memcmp(tables,&before,sizeof(*tables)) && table_hash(tables)==hash);
    }
    assert(failures>100 && failures<10000);
    for(int i=0;i<4;i++)free(data[i]);
    printf("PASS loader allocation failures=%d: every malloc/calloc failure preserves existing cache tables\n",failures);
}

static void safety(const WordFilterTables *tables,Text input,Text expected) {
    size_t size=(size_t)input.n+8,written;uint16_t *buffer=malloc(size*sizeof(*buffer)),*snapshot=malloc(size*sizeof(*snapshot));assert(buffer && snapshot);
    for(size_t i=0;i<size;i++)buffer[i]=0xa55a;
    memcpy(snapshot,buffer,size*sizeof(*buffer));
    fail_alloc_after=0;written=123;
    assert(!wordfilter_apply(tables,input.data,input.n,buffer,size,&written) && written==0);
    fail_alloc_after=-1;assert(!memcmp(buffer,snapshot,size*sizeof(*buffer)));
    assert(!wordfilter_apply(tables,input.data,SIZE_MAX,buffer,size,&written));
    assert(!wordfilter_apply(tables,input.data,(size_t)INT_MAX-1,buffer,size,&written));
    assert(!wordfilter_apply(tables,NULL,1,buffer,size,&written));
    assert(!wordfilter_apply(tables,input.data,input.n,NULL,size,&written));
    assert(!wordfilter_apply(tables,input.data,input.n,buffer,0,&written));
    assert(!wordfilter_apply(NULL,input.data,input.n,buffer,size,&written));
    WordFilterTables invalid=*tables;
    invalid.fragments=NULL;assert(!wordfilter_apply(&invalid,input.data,input.n,buffer,size,&written));
    invalid=*tables;invalid.fragment_count=SIZE_MAX;assert(!wordfilter_apply(&invalid,input.data,input.n,buffer,size,&written));
    invalid=*tables;invalid.bad_count=SIZE_MAX;assert(!wordfilter_apply(&invalid,input.data,input.n,buffer,size,&written));
    invalid=*tables;invalid.bad_words=NULL;assert(!wordfilter_apply(&invalid,input.data,input.n,buffer,size,&written));
    invalid=*tables;WordFilterWord broken=tables->bad_words[0];invalid.bad_count=1;invalid.bad_words=&broken;
    broken.length=0;assert(!wordfilter_apply(&invalid,input.data,input.n,buffer,size,&written));
    broken=tables->bad_words[0];broken.length=SIZE_MAX;assert(!wordfilter_apply(&invalid,input.data,input.n,buffer,size,&written));
    broken=tables->bad_words[0];broken.chars=NULL;assert(!wordfilter_apply(&invalid,input.data,input.n,buffer,size,&written));
    broken=tables->bad_words[0];broken.combination_count=1;broken.combinations=NULL;assert(!wordfilter_apply(&invalid,input.data,input.n,buffer,size,&written));
    assert(!memcmp(buffer,snapshot,size*sizeof(*buffer)));
    const int offsets[][2]={{1,1},{1,2},{2,1}};
    for(int test=0;test<3;test++) {
        for(size_t i=0;i<size;i++)buffer[i]=0xa55a;
        int src=offsets[test][0],dst=offsets[test][1];memcpy(buffer+src,input.data,(size_t)input.n*sizeof(*buffer));
        memcpy(snapshot,buffer,size*sizeof(*buffer));written=123;
        assert(!wordfilter_apply(tables,buffer+src,input.n,buffer+dst,expected.n,&written) && written==0);
        assert(!memcmp(buffer,snapshot,size*sizeof(*buffer)));
        assert(wordfilter_apply(tables,buffer+src,input.n,buffer+dst,(size_t)input.n+1,&written));
        assert(written==(size_t)expected.n && !memcmp(buffer+dst,expected.data,written*sizeof(*buffer)) && buffer[dst+written]==0);
        assert(buffer[0]==0xa55a && buffer[size-1]==0xa55a);
    }
    assert(wordfilter_apply(tables,NULL,0,buffer,1,&written) && written==0 && buffer[0]==0);
    assert(wordfilter_apply(tables,input.data,input.n,buffer,size,NULL));
    assert(!memcmp(buffer,expected.data,(size_t)expected.n*sizeof(*buffer)));
    free(buffer);free(snapshot);
    puts("PASS native WordFilter allocation failure/invalid metadata/NULL/size/capacity/forward-backward-in-place alias safety");
}
int main(int argc,char **argv) {
    assert(argc==3);setvbuf(stdout,NULL,_IONBF,0);
    Arena arena=arena_create(16*1024*1024);Jagfile *jag=jagfile_load_file("../engine/data/pack/client/wordenc",&arena);assert(jag);
    WordFilterTables t={0};assert(wordfilter_tables_load(&t,jag));loader_failures(&t,jag);
    FILE *f=fopen(argv[1],"rb");assert(f && integer(f)==0x57464232);
    uint64_t counts[10]={0},queries=0,record=0;int8_t (*synthetic)[2]=NULL;int synthetic_count=0;
    for(;;) {
        int op=byte(f);if(!op)break;
        if(op<1 || op>9){fprintf(stderr,"Invalid branch opcode%d\n",op);return 2;}
        record++;counts[(unsigned)op]++;
        if(op==1) {
            uint16_t pattern=unit(f),current=unit(f),next=unit(f);int bad=byte(f),domain=byte(f);
            int actual_bad=wf_emulated(next,pattern,current),actual_domain=wf_domain_emulated(next,pattern,current);
            if(bad!=actual_bad || domain!=actual_domain)fprintf(stderr,"Emulation record%llu pattern%x/current%x/next%x expected%d,%d actual%d,%d\n",(unsigned long long)record,pattern,current,next,bad,domain,actual_bad,actual_domain);
            assert(bad==actual_bad && domain==actual_domain);
        } else if(op==2) {
            Text word=read_text(f);int id=integer(f),known=byte(f);
            assert(wf_fragment_id(word.data,word.n)==id && wf_bad_fragment(&t,word.data,word.n)==(known!=0));release(word);
        } else if(op==3) {
            int id=integer(f);int8_t before=(int8_t)byte(f),after=(int8_t)byte(f);bool expected=byte(f)!=0;
            assert(id>=-1 && id<(int)t.bad_count);
            const int8_t (*pairs)[2]=id<0?synthetic:t.bad_words[id].combinations;
            int n=id<0?synthetic_count:(int)t.bad_words[id].combination_count;
            assert(wf_combo(before,pairs,n,after)==expected);
        } else if(op==4) {
            assert(!synthetic);synthetic_count=integer(f);assert(synthetic_count>0 && synthetic_count<65536);
            synthetic=malloc((size_t)synthetic_count*sizeof(*synthetic));assert(synthetic);
            for(int i=0;i<synthetic_count;i++){synthetic[i][0]=(int8_t)byte(f);synthetic[i][1]=(int8_t)byte(f);}
        } else if(op==5) {
            Text input=read_text(f),mask=read_text(f);assert(input.n==mask.n);
            for(int start=0;start<=input.n;start++) {
                int expected[4];for(int i=0;i<4;i++)expected[i]=byte(f);
                int actual[]={wf_before(input.data,mask.data,start,true),wf_after(input.data,mask.data,input.n,start-1,false),wf_before(input.data,mask.data,start,false),wf_after(input.data,mask.data,input.n,start-1,true)};
                if(memcmp(expected,actual,sizeof(expected)))fprintf(stderr,"Context record%llu length%d start%d expected%d,%d,%d,%d actual%d,%d,%d,%d\n",(unsigned long long)record,input.n,start,expected[0],expected[1],expected[2],expected[3],actual[0],actual[1],actual[2],actual[3]);
                assert(!memcmp(expected,actual,sizeof(expected)));queries++;
            }
            release(input);release(mask);
        } else if(op==6) {
            int id=integer(f);assert(id>=-3 && id<(int)t.bad_count);Text input=read_text(f),expected=read_text(f);
            const uint8_t *word=id>=0?t.bad_words[id].chars:id==-1?dot:id==-2?slash:at;
            int length=id>=0?(int)t.bad_words[id].length:id==-2?5:3;
            wf_bad_match(&t,id>=0?t.bad_words[id].combinations:NULL,id>=0?(int)t.bad_words[id].combination_count:0,input.data,input.n,word,length);
            same(input,expected,op,record);release(input);release(expected);
        } else if(op==7 || op==8) {
            int id=integer(f);assert(id>=0 && id<(int)(op==7?t.domain_count:t.tld_count));
            Text input=read_text(f),mask1=read_text(f),mask2=read_text(f),expected=read_text(f),a=clone(input),b=clone(input);
            wf_bad_match(&t,NULL,0,a.data,a.n,op==7?at:dot,3);
            wf_bad_match(&t,NULL,0,b.data,b.n,op==7?dot:slash,op==7?3:5);
            same(a,mask1,op,record);same(b,mask2,op,record);
            if(op==7)wf_domain_match(input.data,input.n,&t.domains[id],a.data,b.data);else wf_tld_match(input.data,input.n,&t.tlds[id],a.data,b.data);
            same(input,expected,op,record);release(input);release(mask1);release(mask2);release(expected);release(a);release(b);
        } else if(op==9) {
            Text input=read_text(f),expected=read_text(f);wf_numeric_fragments(input.data,input.n);same(input,expected,op,record);release(input);release(expected);
        }
    }
    assert(fgetc(f)==EOF);fclose(f);free(synthetic);
    for(int op=1;op<10;op++)printf("PASS original branch op%d cases=%llu\n",op,(unsigned long long)counts[op]);
    printf("PASS original context positions=%llu (four status methods each)\n",(unsigned long long)queries);
    f=fopen(argv[2],"rb");assert(f && integer(f)==0x57464632);int cases=integer(f);assert(cases>=3772);bool checked_safety=false;
    for(int i=0;i<cases;i++) {
        Text input=read_text(f),expected=read_text(f),output=make_text(input.n+1);size_t written=SIZE_MAX;
        assert(wordfilter_apply(&t,input.data,input.n,output.data,output.n,&written));
        if(written!=(size_t)expected.n || memcmp(output.data,expected.data,written*sizeof(*output.data)))fprintf(stderr,"Complete API mismatch case%d\n",i);
        assert(written==(size_t)expected.n && !memcmp(output.data,expected.data,written*sizeof(*output.data)) && output.data[written]==0);
        for(int j=(int)written+1;j<output.n;j++)assert(output.data[j]==0xa55a);
        if(!checked_safety && input.n>=3 && expected.n>0){safety(&t,input,expected);checked_safety=true;}
        release(input);release(expected);release(output);
    }
    assert(checked_safety && fgetc(f)==EOF);fclose(f);
    printf("PASS complete API original cases=%d; source branch controls total=%llu\n",cases,(unsigned long long)record);
    wordfilter_tables_free(&t);arena_free(&arena);return 0;
}
