#include "net/protocol.h"
#include "dash3d/map_streamer.h"
#include <windows.h>
#include <direct.h>
#include <process.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
static int allocation_at,allocation_count,io_failure;
static void *fault_malloc(size_t n){if(allocation_at && ++allocation_count==allocation_at)return NULL;return malloc(n);}
static void *fault_calloc(size_t n,size_t size){if(allocation_at && ++allocation_count==allocation_at)return NULL;return calloc(n,size);}
static HANDLE fault_create(LPCSTR path,DWORD access,DWORD share,LPSECURITY_ATTRIBUTES security,DWORD creation,DWORD attributes,HANDLE template){return io_failure==1?INVALID_HANDLE_VALUE:CreateFileA(path,access,share,security,creation,attributes,template);}
static BOOL fault_write(HANDLE file,LPCVOID data,DWORD size,LPDWORD written,LPOVERLAPPED overlapped){if(io_failure==2){WriteFile(file,data,size/2,written,overlapped);return FALSE;}return WriteFile(file,data,size,written,overlapped);}
static BOOL fault_flush(HANDLE file){return io_failure==3?FALSE:FlushFileBuffers(file);}
static BOOL fault_close(HANDLE file){BOOL result=CloseHandle(file);return io_failure==4?FALSE:result;}
static BOOL fault_move(LPCSTR source,LPCSTR target,DWORD flags){return io_failure==5?FALSE:MoveFileExA(source,target,flags);}
#define malloc fault_malloc
#define calloc fault_calloc
#define CreateFileA fault_create
#define WriteFile fault_write
#define FlushFileBuffers fault_flush
#define CloseHandle fault_close
#define MoveFileExA fault_move
#include "../../../src/dash3d/map_streamer.c"
#undef malloc
#undef calloc
#undef CreateFileA
#undef WriteFile
#undef FlushFileBuffers
#undef CloseHandle
#undef MoveFileExA
static void inject(int n){allocation_at=n;allocation_count=0;}
static void write_bytes(const char *path,const uint8_t *data,size_t n){FILE *f=fopen(path,"wb");assert(f && fwrite(data,1,n,f)==n);assert(!fclose(f));}
static void check_bytes(const char *path,const uint8_t *data,size_t n){FILE *f=fopen(path,"rb");assert(f);for(size_t i=0;i<n;i++)assert(fgetc(f)==data[i]);assert(fgetc(f)==EOF);fclose(f);}
static void no_temporary(const char *dir){char path[600];snprintf(path,sizeof(path),"%s/*.rs2-download-*",dir);WIN32_FIND_DATAA data;HANDLE search=FindFirstFileA(path,&data);assert(search==INVALID_HANDLE_VALUE);}
static void unchanged(MapStreamer *ms,const MapStreamer *before,int op,const uint8_t *payload,size_t length,const char *dir){assert(map_streamer_receive(ms,op,payload,length,dir)==-1);assert(!memcmp(ms,before,sizeof(*ms)));}
int main(void){
    setvbuf(stdout,NULL,_IONBF,0);char dir[400],land_path[500],loc_path[500];snprintf(dir,sizeof(dir),"verification/completion/runs/map-failure-%d",_getpid());assert(!_mkdir(dir));snprintf(land_path,sizeof(land_path),"%s/m50_50",dir);snprintf(loc_path,sizeof(loc_path),"%s/l50_50",dir);
    MapStreamer ms={0};ms.region_count=1;ms.regions[0].rx=ms.regions[0].rz=50;uint8_t chunk[]={50,50,0,1,0,4,9,8},done[]={50,50};assert(map_streamer_receive(&ms,132,chunk,sizeof(chunk),dir)==1);assert(ms.regions[0].land_length==4 && !memcmp(ms.regions[0].land_data,"\0\11\10\0",4));MapStreamer before=ms;
    int malformed=0;for(int op=20;op<=220;op++){if(op!=20 && op!=80 && op!=132 && op!=220)continue;for(size_t n=0;n<8;n++){if((op==20 || op==80)?n==2:n>=6)continue;unchanged(&ms,&before,op,chunk,n,dir);malformed++;}}
    unchanged(&ms,&before,132,NULL,6,dir);malformed++;uint8_t invalid[][8]={{50,50,0,5,0,4,1,2},{50,50,0,3,0,4,1,2},{50,50,255,255,0,0,1,2}};for(int i=0;i<3;i++){unchanged(&ms,&before,132,invalid[i],8,dir);malformed++;}unchanged(&ms,&before,20,done,2,dir);malformed++;
    assert(map_streamer_receive(&ms,999,NULL,0,NULL)==0);uint8_t unknown[]={99,99,255,255,0,0,1};assert(map_streamer_receive(&ms,132,unknown,sizeof(unknown),dir)==1 && !memcmp(&ms,&before,sizeof(ms)));assert(map_streamer_receive(&ms,80,unknown,2,dir)==1 && !memcmp(&ms,&before,sizeof(ms)));
    uint8_t resized[]={50,50,0,0,0,6,1};inject(1);unchanged(&ms,&before,132,resized,sizeof(resized),dir);inject(0);assert(!memcmp(ms.regions[0].land_data,"\0\11\10\0",4));
    const uint8_t old[]={3,4,5};write_bytes(land_path,old,sizeof(old));for(io_failure=1;io_failure<=5;io_failure++){ms.scene_state=0;assert(map_streamer_receive(&ms,80,done,2,dir)==2 && ms.scene_state==1);check_bytes(land_path,old,sizeof(old));assert(!memcmp(ms.regions[0].land_data,"\0\11\10\0",4));no_temporary(dir);}io_failure=0;assert(map_streamer_receive(&ms,80,done,2,dir)==1);check_bytes(land_path,ms.regions[0].land_data,4);no_temporary(dir);
    ms.regions[0].land_crc=packet_crc32(ms.regions[0].land_data,4);ms.regions[0].loc_crc=packet_crc32(old,sizeof(old));write_bytes(loc_path,old,sizeof(old));before=ms;uint8_t entries[96];memset(entries,0xA5,sizeof(entries));for(int fail=1;fail<=2;fail++){inject(fail);assert(map_streamer_prepare(&ms,dir,entries,sizeof(entries))==-1 && !memcmp(&ms,&before,sizeof(ms)));for(int i=0;i<96;i++)assert(entries[i]==0xA5);}inject(0);
    assert(map_streamer_prepare(&ms,dir,entries,sizeof(entries))==0 && ms.scene_state==1);ms.regions[0].loc_crc++;before=ms;memset(entries,0xA5,sizeof(entries));assert(map_streamer_prepare(&ms,dir,entries,2)==-1 && !memcmp(&ms,&before,sizeof(ms)));for(int i=0;i<96;i++)assert(entries[i]==0xA5);assert(map_streamer_prepare(&ms,dir,entries,3)==3 && !memcmp(entries,"\1\62\62",3) && ms.scene_state==0);assert(ms.regions[0].land_data && !ms.regions[0].loc_data);
    before=ms;uint8_t rebuild[175]={0};for(int n=0;n<175;n++){if(n==4 || n==14 || n==24 || n==34 || n==44 || n==54 || n==64 || n==74 || n==84 || n==94 || n==104 || n==114 || n==124 || n==134 || n==144 || n==154 || n==164)continue;Packet p=packet_wrap(rebuild,(size_t)n);assert(!map_streamer_parse_rebuild_packet(&ms,&p) && p.pos==0 && !memcmp(&ms,&before,sizeof(ms)));malformed++;}
    GameSession session;protocol_init_session(&session);session.in_game=true;session.stream.buffered=true;uint32_t seed[]={225,150,132,220};isaac_init(&session.out_cipher,seed,4);session.stream.out_len=4999;memset(session.stream.out_buf,0x6A,sizeof(session.stream.out_buf));Isaac cipher=session.out_cipher;assert(!protocol_send_map_request(&session,entries,3) && session.stream.out_overflow && session.stream.out_len==4999 && !memcmp(&cipher,&session.out_cipher,sizeof(cipher)));for(int i=0;i<5000;i++)assert(session.stream.out_buf[i]==0x6A);assert(!protocol_send_map_request(&session,NULL,0) && !memcmp(&cipher,&session.out_cipher,sizeof(cipher)));session.stream.out_overflow=false;session.stream.out_len=4995;assert(protocol_send_map_request(&session,entries,3) && session.stream.out_len==5000);cipher=session.out_cipher;session.in_game=false;assert(!protocol_send_map_request(&session,entries,3) && !memcmp(&cipher,&session.out_cipher,sizeof(cipher)));session.in_game=true;session.stream.out_len=0;assert(!protocol_send_map_request(&session,NULL,3) && !protocol_send_map_request(&session,entries,2) && !protocol_send_map_request(&session,entries,256));session.stream.buffered=false;session.stream.connected=false;assert(!protocol_send_map_request(&session,entries,3) && !memcmp(&cipher,&session.out_cipher,sizeof(cipher)));
    World *w=calloc(1,sizeof(*w)),*saved=calloc(1,sizeof(*saved));World3D *scene=calloc(1,sizeof(*scene));CollisionMap *collision=calloc(4,sizeof(*collision)),*saved_collision=calloc(4,sizeof(*collision));assert(w && saved && scene && collision && saved_collision);world_init(w,104,104);world3d_init(scene,104,104,4);for(int l=0;l<4;l++){collision_map_init(collision+l,104,104);collision[l].flags[10][10]=12345+l;w->heightmap[l][10][10]=54321+l;}memcpy(saved,w,sizeof(*w));memcpy(saved_collision,collision,4*sizeof(*collision));FloConfig floor={0};
    const uint8_t corrupt[][12]={{0,0,0,1,0},{0,1,134,161,0},{0,0,0,1,1,2,3,4,5,6,7,8},{255,255,255,255,0}};int decode_rejections=0;
    map_streamer_clear(&ms);ms.region_count=1;ms.source_buffers=true;ms.scene_state=1;for(int type=0;type<2;type++)for(int i=0;i<4;i++){uint8_t **data=type?&ms.regions[0].loc_data:&ms.regions[0].land_data;size_t *size=type?&ms.regions[0].loc_length:&ms.regions[0].land_length;*data=malloc(12);memcpy(*data,corrupt[i],12);*size=12;assert(!map_streamer_rebuild(&ms,w,scene,collision,&floor,NULL,dir) && ms.scene_state==1 && !memcmp(w,saved,sizeof(*w)) && !memcmp(collision,saved_collision,4*sizeof(*collision)));free(*data);*data=NULL;*size=0;decode_rejections++;}
    uint8_t incomplete[]={1};assert(!valid_land(incomplete,sizeof(incomplete)));assert(!valid_loc(incomplete,sizeof(incomplete),NULL));uint8_t loc_range[]={1,255,255,255,255,0};assert(!valid_loc(loc_range,sizeof(loc_range),NULL));uint8_t valid_empty[]={0};assert(valid_loc(valid_empty,1,NULL));uint8_t *land=calloc(4*64*64,1);assert(valid_land(land,4*64*64));free(land);
    FILE *real=fopen("../engine/data/pack/client/maps/m50_50","rb");assert(real);fseek(real,0,SEEK_END);long real_size=ftell(real);rewind(real);ms.regions[0].land_data=malloc((size_t)real_size);ms.regions[0].land_length=(size_t)real_size;assert(fread(ms.regions[0].land_data,1,(size_t)real_size,real)==(size_t)real_size);fclose(real);inject(1);assert(!map_streamer_rebuild(&ms,w,scene,collision,&floor,NULL,dir) && !memcmp(w,saved,sizeof(*w)));inject(0);size_t decoded_length=0;uint8_t *decoded=decode_map_bytes(ms.regions[0].land_data,ms.regions[0].land_length,&decoded_length);assert(decoded && valid_land(decoded,decoded_length));free(decoded);
    map_streamer_clear(&ms);world3d_dispose(scene);free(scene);free(w);free(saved);free(collision);free(saved_collision);printf("PASS map failure controls:4 allocation failures,5 atomic cache I/O failures,%d malformed lengths/ranges/null guards,%d corrupt decode rejections;original world/collision and previous cache bytes retained;capacity/sticky/disconnected request cipher preserved;valid decode recovery;isolated cache %s\n",malformed,decode_rejections,dir);return 0;
}
