#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "core/wordfilter.h"
#include "core/arena.h"
#include "core/jagfile.h"

static int wf_int(FILE *f) {
    unsigned char b[4];assert(fread(b,1,4,f)==4);
    return (int)((uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3]);
}
static void wf_words(FILE *f,WordFilterWord *words,size_t count,int kind) {
    assert(wf_int(f)==(int)count);
    for(size_t i=0;i<count;i++) {
        if(kind==2)assert(wf_int(f)==words[i].type);
        assert(wf_int(f)==(int)words[i].length);
        for(size_t j=0;j<words[i].length;j++)assert(fgetc(f)==words[i].chars[j]);
        if(kind==1) {
            assert(wf_int(f)==(int)words[i].combination_count);
            for(size_t j=0;j<words[i].combination_count;j++)for(int k=0;k<2;k++) {
                int value=fgetc(f);assert(value>=0);assert((int8_t)value==words[i].combinations[j][k]);
            }
        }
    }
}
static size_t wf_text(FILE *f,uint16_t *text,size_t capacity) {
    int n=wf_int(f);assert(n>=0 && (size_t)n<=capacity);
    for(int i=0;i<n;i++){int high=fgetc(f),low=fgetc(f);assert(high>=0 && low>=0);text[i]=(uint16_t)((high<<8)|low);}
    return (size_t)n;
}
static void wf_helper_guards(const uint16_t *text,size_t length) {
    assert(text[0]==0xa5a5);for(size_t i=length+1;i<130;i++)assert(text[i]==0xa5a5);
}
static void wordfilter_helpers(void) {
    const char *override=getenv("RS2_WORDFILTER_HELPERS_ORACLE");
    FILE *f=fopen(override && *override?override:"tests/fixtures/wordfilter-helpers-reference.bin","rb");assert(f);
    assert(wf_int(f)==0x57464832);int count=wf_int(f);assert(count==67544);
    for(int i=0;i<count;i++) {
        uint16_t original[128],expected[128],actual[130],masked[128];
        size_t n=wf_text(f,original,128);assert(wf_text(f,expected,128)==n);
        for(int j=0;j<130;j++)actual[j]=0xa5a5;
        memcpy(actual+1,original,n*sizeof(uint16_t));assert(wordfilter_normalize(actual+1,n));
        assert(!memcmp(actual+1,expected,n*sizeof(uint16_t)));wf_helper_guards(actual,n);
        assert(wf_text(f,masked,128)==n && wf_text(f,expected,128)==n);
        memcpy(actual+1,masked,n*sizeof(uint16_t));assert(wordfilter_restore_case(actual+1,original,n));
        assert(!memcmp(actual+1,expected,n*sizeof(uint16_t)));wf_helper_guards(actual,n);
        assert(wf_text(f,expected,128)==n);assert(wordfilter_format_case(actual+1,n));
        assert(!memcmp(actual+1,expected,n*sizeof(uint16_t)));wf_helper_guards(actual,n);
    }
    assert(fgetc(f)==EOF);fclose(f);
    assert(wordfilter_normalize(NULL,0) && !wordfilter_normalize(NULL,1));
    assert(wordfilter_restore_case(NULL,NULL,0) && !wordfilter_restore_case(NULL,NULL,1));
    assert(wordfilter_format_case(NULL,0) && !wordfilter_format_case(NULL,1));
    printf("PASS original WordFilter UTF-16 helpers: %d cases/all65536 code units,normalization/restore/format case exact; guards/NULL boundaries pass\n",count);
}

static uint16_t *wf_alloc_text(FILE *f,size_t *length) {
    int n=wf_int(f);assert(n>=0 && n<=1048576);*length=(size_t)n;
    uint16_t *text=malloc(((size_t)n+1)*sizeof(*text));assert(text);
    for(int i=0;i<n;i++){int high=fgetc(f),low=fgetc(f);assert(high>=0 && low>=0);text[i]=(uint16_t)((high<<8)|low);}
    text[n]=0;return text;
}
static void wf_print_text(const uint16_t *text,size_t n) {
    for(size_t i=0;i<n;i++) {unsigned c=text[i];if(c>=32 && c<127)fputc((int)c,stderr);else fprintf(stderr,"<%04x>",c);}
    fputc('\n',stderr);
}
static void wordfilter_full(WordFilterTables *tables) {
    const char *override=getenv("RS2_WORDFILTER_FULL_ORACLE");
    FILE *f=fopen(override && *override?override:"tests/fixtures/wordfilter-reference.bin","rb");assert(f);
    assert(wf_int(f)==0x57464632);int count=wf_int(f);assert(count>=3772);
    for(int i=0;i<count;i++) {
        size_t n,want,written=999;
        uint16_t *input=wf_alloc_text(f,&n),*expected=wf_alloc_text(f,&want);
        assert(want<=n);uint16_t *actual=malloc((n+3)*sizeof(*actual));assert(actual);
        for(size_t j=0;j<n+3;j++)actual[j]=0xa5a5;
        bool ok=wordfilter_apply(tables,input,n,actual+1,want+1,&written);
        if(!ok || written!=want || memcmp(actual+1,expected,want*sizeof(*expected))) {
            fprintf(stderr,"FULL WORDFILTER original mismatch case=%d input_units=%zu expected_units=%zu actual_units=%zu success=%d\n",i,n,want,written,ok);
            wf_print_text(input,n);wf_print_text(expected,want);if(ok)wf_print_text(actual+1,written);
        }
        assert(ok && written==want && !memcmp(actual+1,expected,want*sizeof(*expected)) && actual[1+written]==0);
        assert(actual[0]==0xa5a5);for(size_t j=written+2;j<n+3;j++)assert(actual[j]==0xa5a5);
        if(i%53==0) {
            for(size_t j=0;j<n+3;j++)actual[j]=0xa5a5;
            written=999;assert(!wordfilter_apply(tables,input,n,actual+1,want,&written) && written==0);
            for(size_t j=0;j<n+3;j++)assert(actual[j]==0xa5a5);
            memcpy(actual+1,input,n*sizeof(*input));written=999;
            assert(wordfilter_apply(tables,actual+1,n,actual+1,n+1,&written) && written==want && !memcmp(actual+1,expected,want*sizeof(*expected)));
        }
        free(input);free(expected);free(actual);
    }
    assert(fgetc(f)==EOF);fclose(f);
    printf("PASS complete WordFilter original pipeline: %d full outputs exact; capacity/guards/in-place controls pass\n",count);
}

static void wordfilter_cache(Arena *arena) {
    Jagfile *jag=jagfile_load_file("../engine/data/pack/client/wordenc",arena);assert(jag);
    WordFilterTables tables={0};assert(wordfilter_tables_load(&tables,jag));
    const char *override=getenv("RS2_WORDFILTER_TABLES_ORACLE");
    FILE *f=fopen(override && *override?override:"tests/fixtures/wordfilter-tables-reference.bin","rb");assert(f);
    assert(wf_int(f)==0x57463232 && wf_int(f)==(int)tables.fragment_count);
    for(size_t i=0;i<tables.fragment_count;i++)assert(wf_int(f)==tables.fragments[i]);
    wf_words(f,tables.bad_words,tables.bad_count,1);wf_words(f,tables.domains,tables.domain_count,0);wf_words(f,tables.tlds,tables.tld_count,2);
    assert(fgetc(f)==EOF);fclose(f);
    wordfilter_full(&tables);
    printf("PASS original WordFilter tables: %zu fragments,%zu bad,%zu domains,%zu tlds; all chars/signed pairs/types exact\n",tables.fragment_count,tables.bad_count,tables.domain_count,tables.tld_count);
    const char *names[]={"fragmentsenc.txt","badenc.txt","domainenc.txt","tldlist.txt"};
    uint8_t *data[4];size_t sizes[4];for(int i=0;i<4;i++){data[i]=jagfile_read(jag,names[i],NULL,&sizes[i],NULL);assert(data[i]);}
    size_t required[4]={4+tables.fragment_count*2,4,4,4};
    for(size_t i=0;i<tables.bad_count;i++)required[1]+=2+tables.bad_words[i].length+2*tables.bad_words[i].combination_count;
    for(size_t i=0;i<tables.domain_count;i++)required[2]+=1+tables.domains[i].length;
    for(size_t i=0;i<tables.tld_count;i++)required[3]+=2+tables.tlds[i].length;
    size_t rejected=0;WordFilterTables retained=tables;
    for(int i=0;i<4;i++) {
        assert(required[i]<=sizes[i]);
        for(size_t n=0;n<required[i];n++) {
            size_t lengths[4]={sizes[0],sizes[1],sizes[2],sizes[3]};lengths[i]=n;
            assert(!wordfilter_tables_decode(&tables,data[0],lengths[0],data[1],lengths[1],data[2],lengths[2],data[3],lengths[3]));
            assert(!memcmp(&tables,&retained,sizeof(tables)));rejected++;
        }
    }
    assert(!wordfilter_tables_load(&tables,NULL) && !memcmp(&tables,&retained,sizeof(tables)));
    uint8_t hostile[]={0x7f,255,255,255};
    assert(!wordfilter_tables_decode(&tables,hostile,4,data[1],sizes[1],data[2],sizes[2],data[3],sizes[3]));
    assert(!memcmp(&tables,&retained,sizeof(tables)));
    printf("PASS WordFilter table bounds: %zu truncated prefixes rejected transactionally; malformed huge count and NULL archive preserve existing tables\n",rejected);
    for(int i=0;i<4;i++)free(data[i]);
    wordfilter_tables_free(&tables);wordfilter_tables_free(&tables);assert(!tables.fragments && !tables.bad_words && !tables.bad_count);
    uint8_t empty[]={0,0,0,0},bad[]={0,0,0,1,1,'a',1,128,255},tld[]={0,0,0,1,255,0};
    assert(wordfilter_tables_decode(&tables,empty,4,bad,sizeof(bad),empty,4,tld,sizeof(tld)));
    assert(tables.bad_count==1 && tables.bad_words[0].combinations[0][0]==-128 && tables.bad_words[0].combinations[0][1]==-1);
    assert(tables.tld_count==1 && tables.tlds[0].type==255 && tables.tlds[0].length==0);
    assert(wordfilter_tables_decode(&tables,empty,4,empty,4,empty,4,empty,4));
    assert(!tables.bad_count && !tables.bad_words && !tables.tld_count);wordfilter_tables_free(&tables);
    puts("PASS WordFilter signed pair bytes,empty strings/counts,replacement and idempotent free");
}

int main(void) {
    printf("[Test Jagfile] Loading 'title' archive from real 2004scape cache...\n");

    Arena arena = arena_create(16 * 1024 * 1024);

    const char *archive_path = "../engine/data/pack/client/title";
    Jagfile *jag = jagfile_load_file(archive_path, &arena);
    assert(jag != NULL);
    printf("  Successfully loaded Jagfile! File count = %d (expected 10)\n", jag->file_count);
    assert(jag->file_count == 10);

    size_t out_size = 0;
    uint8_t *title_dat = jagfile_read(jag, "title.dat", NULL, &out_size, &arena);
    assert(title_dat != NULL);
    printf("  Successfully extracted 'title.dat'! Size = %zu (expected 17803)\n", out_size);
    assert(out_size == 17803);

    // Verify JPEG header: 0xFF, 0xD8, 0xFF, 0xE0
    printf("  First 4 bytes: %d, %d, %d, %d (expected 255, 216, 255, 224)\n",
           title_dat[0], title_dat[1], title_dat[2], title_dat[3]);
    assert(title_dat[0] == 255);
    assert(title_dat[1] == 216);
    assert(title_dat[2] == 255);
    assert(title_dat[3] == 224);

    wordfilter_cache(&arena);
    wordfilter_helpers();
    arena_free(&arena);
    printf("[Test Jagfile] ALL TESTS PASSED! 100%% parity with live 2004scape cache.\n");
    return 0;
}
