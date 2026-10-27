#include "dash3d/map_streamer.h"
#include "core/bzip2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#define MAP_CACHE_LIMIT 2000000u
#define MAP_DECODE_LIMIT 100000u

void map_streamer_init(MapStreamer *ms){if(ms)memset(ms,0,sizeof(*ms));}
void map_streamer_clear(MapStreamer *ms){
    if(!ms)return;
    for(int i=0;i<MAX_STREAMED_REGIONS;i++){free(ms->regions[i].land_data);free(ms->regions[i].loc_data);}
    map_streamer_init(ms);
}
bool map_streamer_parse_rebuild_packet(MapStreamer *ms,Packet *pkt){
    if(!ms || !pkt || pkt->error || pkt->pos>pkt->length || pkt->length-pkt->pos<4)return false;
    size_t descriptors=pkt->length-pkt->pos-4;
    if(descriptors%10 || descriptors/10>MAX_STREAMED_REGIONS)return false;
    Packet p=*pkt;MapStreamer next={0};next.current_level=ms->current_level;
    next.center_zone_x=g2(&p);next.center_zone_z=g2(&p);
    next.base_tile_x=(next.center_zone_x-6)*8;next.base_tile_z=(next.center_zone_z-6)*8;
    next.region_count=(int)(descriptors/10);next.scene_state=1;
    for(int i=0;i<next.region_count;i++){MapRegionDescriptor *r=&next.regions[i];r->rx=g1(&p);r->rz=g1(&p);r->land_crc=(uint32_t)g4(&p);r->loc_crc=(uint32_t)g4(&p);}
    if(p.error)return false;
    map_streamer_clear(ms);*ms=next;*pkt=p;return true;
}
static bool map_path(char *path,size_t capacity,const char *dir,int type,int x,int z){
    if(!dir || !*dir || x<0 || x>255 || z<0 || z>255)return false;
    int n=snprintf(path,capacity,"%s/%c%d_%d",dir,type?'l':'m',x,z);return n>=0 && (size_t)n<capacity;
}
/* 1 readable,0 absent/invalid I/O,-1 allocation failure. */
static int map_read_file(const char *path,uint8_t **data,size_t *length){
    FILE *f=fopen(path,"rb");if(!f)return 0;
    if(fseek(f,0,SEEK_END)){fclose(f);return 0;}long n=ftell(f);
    if(n<0 || (unsigned long)n>MAP_CACHE_LIMIT || fseek(f,0,SEEK_SET)){fclose(f);return 0;}
    uint8_t *bytes=malloc(n?(size_t)n:1);if(!bytes){fclose(f);return -1;}
    bool ok=fread(bytes,1,(size_t)n,f)==(size_t)n;if(fclose(f))ok=false;
    if(!ok){free(bytes);return 0;}*data=bytes;*length=(size_t)n;return 1;
}
static int map_cached_data(const char *dir,int type,const MapRegionDescriptor *r,uint8_t **data,size_t *length){
    uint32_t crc=type?r->loc_crc:r->land_crc;if(!crc)return 0;
    char path[512];if(!map_path(path,sizeof(path),dir,type,r->rx,r->rz))return 0;
    int result=map_read_file(path,data,length);
    if(result==1 && packet_crc32(*data,*length)!=crc){free(*data);*data=NULL;*length=0;return 0;}return result;
}
int map_streamer_missing_files(const MapStreamer *ms,const char *dir,uint8_t *entries,int capacity){
    if(!ms || ms->region_count<0 || ms->region_count>MAX_STREAMED_REGIONS || capacity<0 || (!entries && capacity))return -1;
    uint8_t ordered[MAX_STREAMED_REGIONS*6];int length=0;
    for(int i=0;i<ms->region_count;i++)for(int type=0;type<2;type++){
        const MapRegionDescriptor *r=&ms->regions[i];uint32_t crc=type?r->loc_crc:r->land_crc;if(!crc)continue;
        uint8_t *data=NULL;size_t size=0;int found=map_cached_data(dir,type,r,&data,&size);free(data);if(found<0)return -1;
        if(!found){ordered[length++]=(uint8_t)type;ordered[length++]=(uint8_t)r->rx;ordered[length++]=(uint8_t)r->rz;}
    }
    if(length>capacity || (length && !entries))return -1;
    if(length)memcpy(entries,ordered,(size_t)length);
    return length;
}
int map_streamer_prepare(MapStreamer *ms,const char *dir,uint8_t *entries,int capacity){
    if(!ms || ms->region_count<0 || ms->region_count>MAX_STREAMED_REGIONS || capacity<0 || (!entries && capacity))return -1;
    MapStreamer next=*ms;uint8_t ordered[MAX_STREAMED_REGIONS*6];int length=0;next.source_buffers=true;
    for(int i=0;i<MAX_STREAMED_REGIONS;i++){next.regions[i].land_data=next.regions[i].loc_data=NULL;next.regions[i].land_length=next.regions[i].loc_length=0;}
    for(int i=0;i<next.region_count;i++)for(int type=0;type<2;type++){
        MapRegionDescriptor *r=&next.regions[i];uint32_t crc=type?r->loc_crc:r->land_crc;if(!crc)continue;
        int found=map_cached_data(dir,type,r,type?&r->loc_data:&r->land_data,type?&r->loc_length:&r->land_length);
        if(found<0){map_streamer_clear(&next);return -1;}
        if(!found){ordered[length++]=(uint8_t)type;ordered[length++]=(uint8_t)r->rx;ordered[length++]=(uint8_t)r->rz;}
    }
    if(length>capacity || (length && !entries)){map_streamer_clear(&next);return -1;}
    next.scene_state=length?0:1;map_streamer_clear(ms);*ms=next;if(length)memcpy(entries,ordered,(size_t)length);return length;
}
/* Same-directory atomic publication. A failed write never replaces a previous file. */
static bool map_cache_write(const char *dir,int type,int x,int z,const uint8_t *bytes,size_t length){
    char path[512],temporary[600];if(!map_path(path,sizeof(path),dir,type,x,z))return false;
#ifdef _WIN32
    static unsigned counter;
    int n=snprintf(temporary,sizeof(temporary),"%s.rs2-download-%lu-%u",path,(unsigned long)GetCurrentProcessId(),counter++);
    if(n<0 || (size_t)n>=sizeof(temporary))return false;
    HANDLE file=CreateFileA(temporary,GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);if(file==INVALID_HANDLE_VALUE)return false;
    DWORD written=0;bool ok=length<=UINT32_MAX && WriteFile(file,bytes,(DWORD)length,&written,NULL) && written==length;
    if(ok && !FlushFileBuffers(file))ok=false;
    if(!CloseHandle(file))ok=false;
    if(ok && !MoveFileExA(temporary,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))ok=false;
    if(!ok)DeleteFileA(temporary);
    return ok;
#else
    (void)bytes;(void)length;(void)temporary;return false;
#endif
}
int map_streamer_receive(MapStreamer *ms,int opcode,const uint8_t *payload,size_t length,const char *dir){
    bool chunk=opcode==132 || opcode==220,done=opcode==80 || opcode==20;
    if(!chunk && !done)return 0;
    if(!ms || !payload || ms->region_count<0 || ms->region_count>MAX_STREAMED_REGIONS || (chunk?(length<6 || length>65535):length!=2))return -1;
    int at=-1;for(int i=0;i<ms->region_count;i++)if(ms->regions[i].rx==payload[0] && ms->regions[i].rz==payload[1])at=i;
    if(at<0)return 1;
    MapRegionDescriptor *r=&ms->regions[at];int type=opcode==220 || opcode==20;uint8_t **data=type?&r->loc_data:&r->land_data;size_t *size=type?&r->loc_length:&r->land_length;
    if(chunk){size_t offset=payload[2]*256u+payload[3],total=payload[4]*256u+payload[5],n=length-6;
        if(offset>total || n>total-offset)return -1;
        if(!*data || *size!=total){uint8_t *next=calloc(total?total:1,1);if(!next)return -1;if(n)memcpy(next+offset,payload+6,n);free(*data);*data=next;*size=total;}
        else if(n)memcpy(*data+offset,payload+6,n);
        ms->source_buffers=true;return 1;
    }
    if(!*data)return -1; /* Active source cache would dereference null; reject safely. */
    bool saved=map_cache_write(dir,type,r->rx,r->rz,*data,*size);ms->scene_state=1;return saved?1:2;
}
static uint8_t *decode_map_bytes(const uint8_t *compressed,size_t size,size_t *out_length){
    if(!compressed || size<5)return NULL;
    uint32_t n=(uint32_t)compressed[0]<<24|(uint32_t)compressed[1]<<16|(uint32_t)compressed[2]<<8|compressed[3];
    if(n>MAP_DECODE_LIMIT)return NULL;
    uint8_t *data=malloc(n?n:1);if(!data)return NULL;
    if(!bzip_decompress_exact(data,n,compressed+4,size-4)){free(data);return NULL;}*out_length=n;return data;
}
static uint8_t *decode_map_file(const char *dir,int type,const MapRegionDescriptor *r,size_t *length){
    char path[512];if(!map_path(path,sizeof(path),dir,type,r->rx,r->rz))return NULL;
    uint8_t *compressed=NULL;size_t size=0;if(map_read_file(path,&compressed,&size)!=1)return NULL;
    uint8_t *decoded=decode_map_bytes(compressed,size,length);free(compressed);return decoded;
}
static bool valid_land(const uint8_t *data,size_t size){
    size_t at=0;for(int tile=0;tile<4*64*64;tile++)for(;;){if(at>=size)return false;int code=data[at++];if(code==0)break;if(code<=49){if(at>=size)return false;at++;if(code==1)break;}}
    return true;
}
static int map_smart(const uint8_t *data,size_t size,size_t *at){if(*at>=size)return -1;int n=data[(*at)++];if(n<128)return n;if(*at>=size)return -1;return n*256+data[(*at)++]-32768;}
static bool valid_loc(const uint8_t *data,size_t size,const LocConfig *cfg){
    size_t at=0;int id=-1;
    for(;;){int delta=map_smart(data,size,&at);if(delta<0 || (int64_t)id+delta>INT_MAX)return false;if(!delta)return true;id+=delta;if(cfg && id>=cfg->count)return false;int packed=0;
        for(;;){delta=map_smart(data,size,&at);if(delta<0)return false;if(!delta)break;if(delta-1>16383-packed || at>=size)return false;packed+=delta-1;at++;}}
}
static void clear_landscape(MapStreamer *ms,World *w,FloConfig *flo,int x,int z){
    if(ms->center_zone_z>=800)return;
    uint8_t water=0;
    for(int i=0;i<flo->count;i++)if(!strcasecmp(flo->instances[i].name,"water")){water=(uint8_t)(i+1);break;}
    for(int dz=z;dz<z+64;dz++)for(int dx=x;dx<x+64;dx++)if(dx>=0 && dx<w->max_tile_x && dz>=0 && dz<w->max_tile_z){w->tile_overlays[0][dx][dz]=water;for(int l=0;l<4;l++){w->heightmap[l][dx][dz]=0;w->tile_flags[l][dx][dz]=0;}}
}
bool map_streamer_rebuild(MapStreamer *ms,World *world,World3D *world3d,CollisionMap collision_maps[4],FloConfig *flo_cfg,LocConfig *loc_cfg,const char *maps_dir){
    if(!ms || !world || !world3d || !collision_maps || !flo_cfg || ms->region_count<0 || ms->region_count>MAX_STREAMED_REGIONS)return false;
    if(ms->region_count==0 && !ms->source_buffers){int n=0;for(int x=ms->base_tile_x/64;x<=(ms->base_tile_x+103)/64 && n<MAX_STREAMED_REGIONS;x++)for(int z=ms->base_tile_z/64;z<=(ms->base_tile_z+103)/64 && n<MAX_STREAMED_REGIONS;z++){ms->regions[n].rx=x;ms->regions[n++].rz=z;}ms->region_count=n;}
    uint8_t *land[MAX_STREAMED_REGIONS]={0},*loc[MAX_STREAMED_REGIONS]={0};size_t land_length[MAX_STREAMED_REGIONS]={0},loc_length[MAX_STREAMED_REGIONS]={0};bool ok=true;
    for(int i=0;i<ms->region_count;i++){MapRegionDescriptor *r=&ms->regions[i];
        if(ms->source_buffers){if(r->land_data)land[i]=decode_map_bytes(r->land_data,r->land_length,&land_length[i]);if(r->loc_data)loc[i]=decode_map_bytes(r->loc_data,r->loc_length,&loc_length[i]);if((r->land_data && !land[i]) || (r->loc_data && !loc[i]))ok=false;}
        else {land[i]=decode_map_file(maps_dir,0,r,&land_length[i]);loc[i]=decode_map_file(maps_dir,1,r,&loc_length[i]);}
        if((land[i] && !valid_land(land[i],land_length[i])) || (loc[i] && !valid_loc(loc[i],loc_length[i],loc_cfg)))ok=false;
    }
    if(ok){world_reset(world);world3d_reset(world3d);for(int l=0;l<4;l++)collision_map_reset(&collision_maps[l]);
        for(int i=0;i<ms->region_count;i++){MapRegionDescriptor *r=&ms->regions[i];int x=r->rx*64-ms->base_tile_x,z=r->rz*64-ms->base_tile_z;if(land[i])world_load_ground(world,land[i],land_length[i],x,z,ms->base_tile_x,ms->base_tile_z);else if(ms->source_buffers)clear_landscape(ms,world,flo_cfg,x,z);}
        for(int i=0;i<ms->region_count;i++){MapRegionDescriptor *r=&ms->regions[i];if(loc[i])world_load_locations(world,loc[i],loc_length[i],loc_cfg,world3d,collision_maps,r->rx*64-ms->base_tile_x,r->rz*64-ms->base_tile_z);}
        world_build(world,world3d,flo_cfg,collision_maps);ms->scene_state=2;
    }
    for(int i=0;i<MAX_STREAMED_REGIONS;i++){free(land[i]);free(loc[i]);}return ok;
}
bool map_streamer_needs_rebuild(const MapStreamer *ms,int x,int z){return ms && ms->scene_state==2 && (x<16 || x>=88 || z<16 || z>=88);}
