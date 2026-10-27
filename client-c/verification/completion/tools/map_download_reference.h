#ifndef MAP_DOWNLOAD_REFERENCE_H
#define MAP_DOWNLOAD_REFERENCE_H
#include "net/protocol.h"
#include "gfx/pix3d.h"
#include "gfx/anim.h"
#include <direct.h>
#include <process.h>
#include <limits.h>
static FILE *md_file;
static int md_case;
static void md_check(bool ok,const char *what){if(!ok){fprintf(stderr,"map reference case %d: %s\n",md_case,what);exit(3);}}
static uint32_t md_u32(void){uint8_t b[4];md_check(fread(b,1,4,md_file)==4,"fixture truncated");return (uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3];}
static int md_byte(void){int b=fgetc(md_file);md_check(b!=EOF,"fixture byte");return b;}
static uint8_t *md_data(int *length){*length=(int32_t)md_u32();md_check(*length>=-1 && *length<2000001,"fixture size");if(*length<0)return NULL;uint8_t *p=malloc(*length? (size_t)*length:1);md_check(p!=NULL,"fixture allocation");md_check(fread(p,1,(size_t)*length,md_file)==(size_t)*length,"fixture bytes");return p;}
static void md_equal_data(const uint8_t *actual,size_t length){int n;uint8_t *expected=md_data(&n);md_check((actual==NULL)==(expected==NULL),"nullable map buffer");if(expected)md_check(length==(size_t)n && !memcmp(actual,expected,length),"map/cache bytes");free(expected);}
static void md_equal(int actual,const char *label){int expected=(int32_t)md_u32();if(actual!=expected){fprintf(stderr,"map reference case%d %s: actual%d expected%d\n",md_case,label,actual,expected);exit(3);}}
static void md_cipher(const Isaac *cipher){md_equal(cipher->count,"cipher count");md_equal((int32_t)cipher->a,"cipher a");md_equal((int32_t)cipher->b,"cipher b");md_equal((int32_t)cipher->c,"cipher c");for(int i=0;i<256;i++)md_equal((int32_t)cipher->rsl[i],"cipher rsl");for(int i=0;i<256;i++)md_equal((int32_t)cipher->mem[i],"cipher mem");}
static void md_write(const char *path,const uint8_t *data,size_t length){FILE *f=fopen(path,"wb");md_check(f!=NULL,"isolated cache open");md_check(fwrite(data,1,length,f)==length,"isolated cache write");md_check(!fclose(f),"isolated cache close");}
static void md_cache(const char *dir){int count=(int)md_u32();md_check(count>=0 && count<100,"cache count");for(int i=0;i<count;i++){int n;uint8_t *name=md_data(&n);md_check(n>0 && n<30,"cache name");char path[600];snprintf(path,sizeof(path),"%s/%.*s",dir,n,name);free(name);FILE *f=fopen(path,"rb");uint8_t *data=NULL;size_t size=0;if(f){md_check(!fseek(f,0,SEEK_END),"cache seek");long end=ftell(f);md_check(end>=0 && end<=2000000,"cache size");size=(size_t)end;rewind(f);data=malloc(size?size:1);md_check(data!=NULL && fread(data,1,size,f)==size,"cache read");fclose(f);}md_equal_data(data,size);free(data);}}
static void md_reset_cache(const uint8_t *payload,size_t length,const char *dir){Packet p=packet_wrap((uint8_t*)payload,length);int count=g4(&p);md_check(count>=0 && count<100,"reset count");for(int i=0;i<count;i++){int n=g2(&p);md_check(n>0 && n<30 && p.pos+(size_t)n+4<=length,"reset name");char path[600];snprintf(path,sizeof(path),"%s/%.*s",dir,n,p.data+p.pos);p.pos+=(size_t)n;int size=g4(&p);md_check(size>=0 && p.pos+(size_t)size<=length,"reset size");md_write(path,p.data+p.pos,(size_t)size);p.pos+=(size_t)size;}md_check(!p.error && p.pos==length,"reset framing");}
static void (*md_wire_reader)(GameSession*,int);
static void md_snapshot(MapStreamer *ms,World *world,CollisionMap *collision,GameSession *session,bool cutscene,bool built,const char *dir){
        md_equal(ms->center_zone_x,"center x");md_equal(ms->center_zone_z,"center z");md_equal(ms->base_tile_x,"base x");md_equal(ms->base_tile_z,"base z");md_equal(ms->scene_state,"scene state");md_check(md_byte()==cutscene,"cutscene");md_equal(ms->region_count,"region count");for(int i=0;i<ms->region_count;i++){MapRegionDescriptor *r=&ms->regions[i];md_equal(r->rx*256+r->rz,"map index");md_equal_data(r->land_data,r->land_length);md_equal_data(r->loc_data,r->loc_length);}
        int wire=(int)md_u32();md_check(wire>=0 && wire<5000,"wire length");if(md_wire_reader)md_wire_reader(session,wire);uint8_t expected[5000];md_check(fread(expected,1,(size_t)wire,md_file)==(size_t)wire,"wire bytes");md_check(session->stream.out_len==(size_t)wire && !memcmp(expected,session->stream.out_buf,(size_t)wire),"encrypted outgoing wire");md_cipher(&session->out_cipher);md_cache(dir);md_check(md_byte()==built,"built gate");if(built){for(int l=0;l<4;l++)for(int x=0;x<105;x++)for(int z=0;z<105;z++)md_equal(world->heightmap[l][x][z],"height");for(int l=0;l<4;l++)for(int x=0;x<104;x++)for(int z=0;z<104;z++){md_check(world->tile_flags[l][x][z]==md_byte(),"tile flags");md_equal(collision[l].flags[x][z],"collision");}}
}
static int map_download_reference(const char *fixture){
    md_file=fopen(fixture,"rb");md_check(md_file!=NULL,"open original map fixture");md_check(md_u32()==0x4d443235,"fixture magic");int count=(int)md_u32();md_check(count>0 && count<10000,"case count");uint32_t seed[4];for(int i=0;i<4;i++)seed[i]=md_u32();
    char root[400],dir[500];static unsigned run;snprintf(root,sizeof(root),"verification/completion/runs/map-native-%d-%u",_getpid(),run++);md_check(!_mkdir(root),"fresh isolated directory");
    Arena arena=arena_create(64*1024*1024);Jagfile *config=jagfile_load_file("../engine/data/pack/client/config",&arena),*models=jagfile_load_file("../engine/data/pack/client/models",&arena);md_check(config && models,"actual cache");FloConfig floors;flo_unpack(&floors,config);LocConfig locations;loc_unpack(&locations,config);model_init_repo(models,&arena);anim_unpack(models,&arena);pix3d_init();
    World *world=calloc(1,sizeof(*world));World3D *scene=calloc(1,sizeof(*scene));CollisionMap *collision=calloc(4,sizeof(*collision));GameSession *session=calloc(1,sizeof(*session));md_check(world && scene && collision && session,"world allocations");world_init(world,104,104);world->build225=true;world3d_init(scene,104,104,4);for(int i=0;i<4;i++)collision_map_init(collision+i,104,104);MapStreamer ms={0};bool cutscene=true,writes=true;int reset=0,builds=0,packets=0;
    for(md_case=0;md_case<count;md_case++){
        int op=(int32_t)md_u32();int n=(int)md_u32();md_check(n>=0 && n<=65535,"payload length");uint8_t *payload=malloc(n?(size_t)n:1);md_check(payload && fread(payload,1,(size_t)n,md_file)==(size_t)n,"payload");session->stream.out_len=0;bool built=false;
        if(op==-1){snprintf(dir,sizeof(dir),"%s/cache-%d",root,reset++);md_check(!_mkdir(dir),"fresh scenario cache");md_reset_cache(payload,(size_t)n,dir);map_streamer_clear(&ms);cutscene=true;writes=true;world_reset(world);world3d_reset(scene);protocol_init_session(session);session->in_game=true;session->stream.buffered=true;isaac_init(&session->out_cipher,seed,4);}
        else if(op==-2)writes=payload[0]!=0;
        else if(op==237){Packet p=packet_wrap(payload,(size_t)n);int x=g2(&p),z=g2(&p);if(x!=ms.center_zone_x || z!=ms.center_zone_z || ms.scene_state==0){p.pos=0;md_check(map_streamer_parse_rebuild_packet(&ms,&p),"rebuild parse");uint8_t entries[96];int length=map_streamer_prepare(&ms,dir,entries,sizeof(entries));md_check(length>=0 && protocol_send_map_request(session,entries,length),"cache/request");cutscene=false;}packets++;}
        else if(op==184){if(ms.scene_state==1){md_check(map_streamer_rebuild(&ms,world,scene,collision,&floors,&locations,dir),"actual scene rebuild");for(int i=0;i<4;i++)md_check(protocol_send_keepalive(session),"build keepalive");built=true;builds++;}packets++;}
        else {char failed[600];snprintf(failed,sizeof(failed),"%s/absent/subdirectory",dir);md_check(map_streamer_receive(&ms,op,payload,(size_t)n,writes?dir:failed)>0,"map receive");packets++;}
        free(payload);md_snapshot(&ms,world,collision,session,cutscene,built,dir);
    }
    md_check(fgetc(md_file)==EOF,"fixture tail");fclose(md_file);map_streamer_clear(&ms);world3d_dispose(scene);free(scene);free(world);free(collision);free(session);loc_free(&locations);flo_free(&floors);arena_free(&arena);printf("PASS map download: %d original states,%d packets,%d actual scene rebuilds;full buffers/cache/wire/ISAAC/heights/flags/collision;isolated cache %s\n",count,packets,builds,root);return 0;
}
#endif
