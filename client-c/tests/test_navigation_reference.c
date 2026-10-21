#include "dash3d/minimap.h"
#include "dash3d/pathfinder.h"
#include "gfx/pix3d.h"
#include "net/protocol.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int read_int(FILE *f) {
    uint8_t bytes[4]; assert(fread(bytes,1,4,f)==4);
    return (int)((uint32_t)bytes[0]<<24 | (uint32_t)bytes[1]<<16 | (uint32_t)bytes[2]<<8 | bytes[3]);
}
static bool read_bool(FILE *f) { int c=fgetc(f); assert(c==0 || c==1); return c!=0; }

int main(void) {
    setbuf(stdout,NULL);
    FILE *f=fopen("tests/fixtures/navigation-reference.bin","rb"); assert(f);
    assert(read_int(f)==0x4e415631); int cases=read_int(f); assert(cases==20);
    assert(client_stream_global_init()); pix3d_init();
    socket_t listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP); assert(listener!=INVALID_SOCKET_VAL);
    struct sockaddr_in address={0}; address.sin_family=AF_INET; address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    assert(bind(listener,(struct sockaddr *)&address,sizeof(address))==0); assert(listen(listener,1)==0);
#ifdef _WIN32
    int address_size=sizeof(address);
#else
    socklen_t address_size=sizeof(address);
#endif
    assert(getsockname(listener,(struct sockaddr *)&address,&address_size)==0);
    GameSession session; protocol_init_session(&session);
    assert(client_stream_connect(&session.stream,"127.0.0.1",ntohs(address.sin_port)));
    socket_t peer=accept(listener,NULL,NULL); assert(peer!=INVALID_SOCKET_VAL); session.in_game=true;
    static Pathfinder pf; static PathResult path;
    for(int n=0;n<cases;n++) {
        int id=read_int(f),type=read_int(f),sx=read_int(f),sz=read_int(f),dx=read_int(f),dz=read_int(f);
        bool nearest=read_bool(f),ctrl=read_bool(f);
        int mx=read_int(f),my=read_int(f),yaw=read_int(f);
        CollisionMap map; collision_map_init(&map,104,104);
        for(int x=0;x<104;x++) for(int z=0;z<104;z++) map.flags[x][z]=read_int(f);
        int flag_x=read_int(f),flag_z=read_int(f),fallback=read_int(f),length=read_int(f);
        uint8_t expected[128],actual[128]; assert(length>=0 && length<=(int)sizeof(expected));
        assert(fread(expected,1,(size_t)length,f)==(size_t)length);
        uint32_t seed[]={17,31,57,99}; isaac_init(&session.out_cipher,seed,4);
        MinimapClick click={0}; bool eligible=true;
        if(type==1) {
            eligible=minimap_click_target(mx,my,sx*128+64,sz*128+64,yaw,0,&click);
            dx=click.tile_x; dz=click.tile_z;
        }
        bool found=eligible && pathfinder_find_path(&pf,&map,sx,sz,dx,dz,nearest,&path);
        bool sent=false;
        if(found) {
            assert(path.step_count>0);
            assert(path.steps_x[path.step_count-1]==flag_x && path.steps_z[path.step_count-1]==flag_z);
            assert(path.reached_nearest==(fallback!=0));
            int px=sx,pz=sz;
            for(int i=0;i<path.step_count;i++) {
                assert(abs(path.steps_x[i]-px)<=1 && abs(path.steps_z[i]-pz)<=1);
                px=path.steps_x[i]; pz=path.steps_z[i];
            }
            if(type==1) sent=protocol_send_minimap_route(&session,3184,3184,sx,sz,path.steps_x,path.steps_z,
                path.step_count,ctrl,click.click_x,click.click_y,yaw,0,0,sx*128+64,sz*128+64,path.reached_nearest);
            else if(type==2) sent=protocol_send_operation_route(&session,3184,3184,sx,sz,path.steps_x,path.steps_z,path.step_count,ctrl);
            else sent=protocol_send_route(&session,3184,3184,sx,sz,path.steps_x,path.steps_z,path.step_count,ctrl);
        }
        assert(sent==(length>0));
        if(sent) {
            int got=0;
            while(got<length) { int bytes=recv(peer,(char *)actual+got,length-got,0); assert(bytes>0); got+=bytes; }
            if(memcmp(expected,actual,(size_t)length)) {
                fprintf(stderr,"Original Java wire mismatch case %d\n",id); return 1;
            }
        }
        printf("PASS Java navigation case=%d bytes=%d steps=%d\n",id,length,found?path.step_count:0);
    }
    assert(fgetc(f)==EOF); fclose(f); protocol_close_session(&session);
#ifdef _WIN32
    closesocket(peer); closesocket(listener);
#else
    close(peer); close(listener);
#endif
    client_stream_global_cleanup();
    puts("PASS: 20 original Java cases, encrypted route/trailer bytes, endpoints, nearest choice, same-tile cancellation, long paths and minimap rotations/bounds");
    return 0;
}
