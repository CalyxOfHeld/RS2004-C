#include "dash3d/scene_selection225.h"
#include "gfx/pix3d.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t word(FILE *f){unsigned char b[4];if(fread(b,1,4,f)!=4){fputs("truncated selection fixture\n",stderr);exit(2);}return (uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3];}
static int32_t integer(FILE *f){uint32_t v=word(f);return v<=INT32_MAX?(int32_t)v:-1-(int32_t)(UINT32_MAX-v);}
static bool boolean(FILE *f){int b=fgetc(f);if(b<0 || b>1){fputs("invalid selection boolean\n",stderr);exit(2);}return b!=0;}
static void read_flags(FILE *f,SceneTileState225 *t){t->visible=boolean(f);t->update=boolean(f);t->contains_locs=boolean(f);t->check_loc_spans=integer(f);}
int main(int argc,char **argv){
    if(argc!=2)return 2;
    FILE *f=fopen(argv[1],"rb");if(!f){perror(argv[1]);return 2;}
    if(word(f)!=0x53323235)return 2;
    SceneGrid225 grid={0};grid.max_x=integer(f);grid.max_z=integer(f);grid.max_level=integer(f);unsigned profiles=word(f);
    if(grid.max_x<1 || grid.max_x>104 || grid.max_z<1 || grid.max_z>104 || grid.max_level<1 || grid.max_level>4 || profiles!=4)return 2;
    Visibility225 *visibility=calloc(profiles,sizeof(*visibility));if(!visibility)return 2;
    for(unsigned k=0;k<profiles;k++)for(int p=0;p<8;p++)for(int yaw=0;yaw<32;yaw++)for(int x=0;x<51;x++)for(int z=0;z<51;z++)visibility[k].matrix[p][yaw][x][z]=boolean(f);
    size_t count=(size_t)grid.max_x*grid.max_z*grid.max_level;
    struct Guarded {uint32_t before[8];SceneTileState225 tiles[4*104*104];uint32_t after[8];};
    struct Guarded *guard=calloc(1,sizeof(*guard));SceneTileState225 *original=calloc(count,sizeof(*original));if(!guard || !original)return 2;
    grid.tiles=guard->tiles;for(int j=0;j<8;j++)guard->before[j]=guard->after[j]=UINT32_C(0x19a3b7e5);
    unsigned present=0;
    for(size_t i=0;i<count;i++){
        SceneTileState225 *t=&original[i];t->present=boolean(f);
        if(t->present){present++;t->draw_level=integer(f);t->height=integer(f);t->loc_count=integer(f);read_flags(f,t);}
    }
    unsigned cases=word(f);if(!cases || cases>10000)return 2;
    unsigned failed=0,view_bad=0;uint64_t state_bad=0,checked=0;pix3d_init();
    for(unsigned n=0;n<cases;n++){
        int c[9],expected[18];for(int j=0;j<9;j++)c[j]=integer(f);for(int j=0;j<18;j++)expected[j]=integer(f);
        if(c[7]<0 || (unsigned)c[7]>=profiles)return 2;
        grid.min_level=c[6];memcpy(grid.tiles,original,count*sizeof(*original));SceneSelection225 view={0};view.cycle=c[8];
        if(!world3d_select_tiles225(&grid,&visibility[c[7]],&view,c[0],c[1],c[2],c[3],c[4],c[5])){fputs("selection rejected valid input\n",stderr);return 2;}
        int actual[]={view.cycle,view.sin_pitch,view.cos_pitch,view.sin_yaw,view.cos_yaw,view.eye_x,view.eye_y,view.eye_z,view.eye_tile_x,view.eye_tile_z,view.top_level,view.min_x,view.min_z,view.max_x,view.max_z,view.tiles_remaining,view.map_pitch,view.map_yaw};
        unsigned camera_bad=0;for(int j=0;j<18;j++)if(actual[j]!=expected[j]){if(view_bad<8)printf("camera %u field %d C=%d Java=%d\n",n,j,actual[j],expected[j]);view_bad++;camera_bad++;}
        uint64_t before=state_bad;
        for(size_t i=0;i<count;i++){
            SceneTileState225 *t=&grid.tiles[i],*base=&original[i];
            if(t->present!=base->present || t->height!=base->height || t->draw_level!=base->draw_level || t->loc_count!=base->loc_count){fputs("selection changed input metadata\n",stderr);return 1;}
            SceneTileState225 want=*base;if(base->present){read_flags(f,&want);checked++;}
            if(t->visible!=want.visible || t->update!=want.update || t->contains_locs!=want.contains_locs || t->check_loc_spans!=want.check_loc_spans){
                if(state_bad<8)printf("camera %u tile %zu C=%d/%d/%d/%d Java=%d/%d/%d/%d\n",n,i,t->visible,t->update,t->contains_locs,t->check_loc_spans,want.visible,want.update,want.contains_locs,want.check_loc_spans);
                state_bad++;
            }
        }
        for(int j=0;j<8;j++)if(guard->before[j]!=UINT32_C(0x19a3b7e5) || guard->after[j]!=UINT32_C(0x19a3b7e5)){fputs("selection guard damaged\n",stderr);return 1;}
        if(camera_bad || state_bad!=before)failed++;
    }
    if(fgetc(f)!=EOF){fputs("trailing selection fixture data\n",stderr);return 2;}
    fclose(f);free(visibility);free(original);free(guard);
    printf("Scene selection: %u/%u cameras match; %u source tiles; %llu tile states; %u camera-field and %llu tile-state differences; guards intact\n",cases-failed,cases,present,(unsigned long long)checked,view_bad,(unsigned long long)state_bad);
    return failed?1:0;
}
