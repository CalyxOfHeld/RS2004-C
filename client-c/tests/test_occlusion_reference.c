#include "dash3d/occlusion225.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OCC_FIELDS(X) X(min_tile_x) X(max_tile_x) X(min_tile_z) X(max_tile_z) X(type) X(min_x) X(max_x) X(min_z) X(max_z) X(min_y) X(max_y) X(mode) X(min_delta_x) X(max_delta_x) X(min_delta_z) X(max_delta_z) X(min_delta_y) X(max_delta_y)
static uint32_t word(FILE *f){unsigned char b[4];if(fread(b,1,4,f)!=4){fputs("truncated occlusion fixture\n",stderr);exit(2);}return (uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3];}
static int32_t integer(FILE *f){uint32_t v=word(f);return v<=INT32_MAX?(int32_t)v:-1-(int32_t)(UINT32_MAX-v);}
static bool boolean(FILE *f){int v=fgetc(f);if(v<0 || v>1)exit(2);return v!=0;}
static int32_t negative(int32_t a){uint32_t v=0u-(uint32_t)a;return v<=INT32_MAX?(int32_t)v:-1-(int32_t)(UINT32_MAX-v);}
static size_t cell(const Occlusion225 *s,int l,int x,int z){return ((size_t)l*(s->max_x+1)+x)*(s->max_z+1)+z;}
static uint64_t cache_hash(const int32_t *data,size_t count){uint64_t h=UINT64_C(0xcbf29ce484222325);for(size_t i=0;i<count;i++){h^=(uint32_t)data[i];h*=UINT64_C(0x100000001b3);}return h;}
int main(int argc,char **argv){
    if(argc!=2)return 2;
    FILE *f=fopen(argv[1],"rb");if(!f){perror(argv[1]);return 2;}
    if(word(f)!=0x4f323235)return 2;
    Occlusion225 s={0};s.max_x=integer(f);s.max_z=integer(f);s.levels=integer(f);unsigned maps=word(f);
    if(s.max_x!=104 || s.max_z!=104 || s.levels!=4 || maps!=4)return 2;
    bool visibility[4][51][51];for(unsigned m=0;m<maps;m++)for(int x=0;x<51;x++)for(int z=0;z<51;z++)visibility[m][x][z]=boolean(f);
    size_t cells=(size_t)s.levels*(s.max_x+1)*(s.max_z+1);int32_t *heights=malloc(cells*sizeof(*heights)),*cache=calloc(cells+16,sizeof(*cache));if(!heights || !cache)return 2;
    for(size_t i=0;i<cells;i++)heights[i]=integer(f);
    s.heights=heights;s.tile_cycles=cache+8;
    for(int i=0;i<8;i++)cache[i]=cache[cells+8+i]=INT32_C(0x19a3b7e5);
    struct Guarded {uint32_t before[8];Occluder225 pool[500];uint32_t after[8];} guarded={0};s.pool=guarded.pool;
    for(int i=0;i<8;i++)guarded.before[i]=guarded.after[i]=UINT32_C(0x19a3b7e5);
    unsigned groups=word(f);if(!groups || groups>10000)return 2;
    unsigned failed=0;uint64_t active_bad=0,field_bad=0,query_bad=0,queries=0,hash_bad=0,active_total=0,hits[5]={0},kinds[5]={0};
    for(unsigned n=0;n<groups;n++){
        unsigned profile=word(f);s.eye_x=integer(f);s.eye_y=integer(f);s.eye_z=integer(f);s.eye_tile_x=integer(f);s.eye_tile_z=integer(f);s.cycle=integer(f);s.count=word(f);
        if(profile>=maps || s.count>500)return 2;
        for(size_t i=0;i<s.count;i++){
            #define READ(name) s.pool[i].name=integer(f);
            OCC_FIELDS(READ)
            #undef READ
        }
        if(!world3d_occlusion_activate225(&s,visibility[profile]))return 2;
        uint64_t before=active_bad+field_bad+query_bad+hash_bad;
        unsigned expected_count=word(f);if(expected_count>s.count)return 2;
        if(expected_count!=s.active_count){if(active_bad<8)printf("group %u active count C=%zu Java=%u\n",n,s.active_count,expected_count);active_bad++;}
        for(unsigned i=0;i<expected_count;i++){unsigned id=word(f);if(id>=s.count)return 2;if(i>=s.active_count || id!=s.active[i])active_bad++;}active_total+=expected_count;
        for(size_t i=0;i<s.count;i++){
            #define CHECK(name) {int expected=integer(f);if(expected!=s.pool[i].name){if(field_bad<8)printf("group %u occluder %zu " #name " C=%d Java=%d\n",n,i,s.pool[i].name,expected);field_bad++;}}
            OCC_FIELDS(CHECK)
            #undef CHECK
        }
        memset(s.tile_cycles,0,cells*sizeof(*s.tile_cycles));unsigned count=word(f);if(count>100000)return 2;
        for(unsigned i=0;i<count;i++){
            int q[9];for(int j=0;j<9;j++)q[j]=integer(f);int expected=integer(f),expected_cache=integer(f);if(q[0]<0 || q[0]>4 || expected<0 || expected>1)return 2;
            size_t anchor=0;
            if(q[0]){
                int l=q[1],x=q[2],z=q[q[0]==4?4:3];if(l<0 || l>=s.levels || x<0 || x>=s.max_x || z<0 || z>=s.max_z)return 2;
                if(q[0]==4 && (q[3]<x || q[3]>=s.max_x || q[5]<z || q[5]>=s.max_z))return 2;
                anchor=cell(&s,l,x,z);if(q[7])s.tile_cycles[anchor]=q[8];
            }
            bool actual=false;
            if(q[0]==0){
                size_t active_count=s.active_count;uint16_t first=s.active[0];
                if(q[7]){if(q[8]<0 || (size_t)q[8]>=s.count)return 2;s.active_count=1;s.active[0]=(uint16_t)q[8];}
                actual=world3d_occluded225(&s,q[1],q[2],q[3]);s.active_count=active_count;s.active[0]=first;
            }
            if(q[0]==1)actual=world3d_tile_occluded225(&s,q[1],q[2],q[3]);
            if(q[0]==2)actual=world3d_object_occluded225(&s,q[1],q[2],q[3],q[4]);
            if(q[0]==3)actual=world3d_wall_occluded225(&s,q[1],q[2],q[3],q[4]);
            if(q[0]==4)actual=world3d_loc_occluded225(&s,q[1],q[2],q[3],q[4],q[5],q[6]);
            int tag=q[0]?s.tile_cycles[anchor]:0;
            if(actual!=(expected!=0) || tag!=expected_cache){if(query_bad<8)printf("group %u query %u kind=%d C=%d/%d Java=%d/%d cycle=%d negative=%d\n",n,i,q[0],actual,tag,expected,expected_cache,s.cycle,negative(s.cycle));query_bad++;}
            kinds[q[0]]++;hits[q[0]]+=expected!=0;queries++;
        }
        uint64_t expected_hash=(uint64_t)word(f)<<32;expected_hash|=word(f);if(cache_hash(s.tile_cycles,cells)!=expected_hash)hash_bad++;
        for(int i=0;i<8;i++)if(cache[i]!=INT32_C(0x19a3b7e5) || cache[cells+8+i]!=INT32_C(0x19a3b7e5) || guarded.before[i]!=UINT32_C(0x19a3b7e5) || guarded.after[i]!=UINT32_C(0x19a3b7e5)){fputs("occlusion guard damaged\n",stderr);return 1;}
        if(before!=active_bad+field_bad+query_bad+hash_bad)failed++;
    }
    if(fgetc(f)!=EOF){fputs("trailing occlusion fixture data\n",stderr);return 2;}
    fclose(f);free(heights);free(cache);
    printf("Occlusion: %u/%u groups; %llu active entries; %llu queries; active=%llu fields=%llu query/cache=%llu full-cache-hash=%llu differences; guards intact\n",groups-failed,groups,(unsigned long long)active_total,(unsigned long long)queries,(unsigned long long)active_bad,(unsigned long long)field_bad,(unsigned long long)query_bad,(unsigned long long)hash_bad);
    for(int i=0;i<5;i++)printf("  kind %d: %llu/%llu true\n",i,(unsigned long long)hits[i],(unsigned long long)kinds[i]);
    return failed?1:0;
}
