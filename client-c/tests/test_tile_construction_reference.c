#include "dash3d/world3d.h"
#include <stdio.h>
#include <stdlib.h>

static uint32_t word(FILE *f){
    unsigned char b[4];if(fread(b,1,4,f)!=4){fputs("truncated tile fixture\n",stderr);exit(2);}
    return (uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3];
}
static int32_t integer(FILE *f){uint32_t v=word(f);return v<=INT32_MAX?(int32_t)v:-1-(int32_t)(UINT32_MAX-v);}
static unsigned differences,fields,failed;
static void check(FILE *f,int actual,unsigned n,const char *label){
    int expected=integer(f);fields++;
    if(actual!=expected){if(differences<12)printf("case %u %s: C=%d Java=%d\n",n,label,actual,expected);differences++;}
}
int main(int argc,char **argv){
    if(argc!=2)return 2;
    FILE *f=fopen(argv[1],"rb");if(!f){perror(argv[1]);return 2;}
    if(word(f)!=0x54323235)return 2;
    unsigned count=word(f);if(!count||count>100000)return 2;
    World3D *w=calloc(1,sizeof(*w));if(!w)return 2;
    world3d_init(w,104,104,4);
    for(unsigned n=0;n<count;n++){
        unsigned reset=word(f);int a[20];for(int j=0;j<20;j++)a[j]=integer(f);
        if(reset>1||a[0]<0||a[0]>3||a[1]<0||a[1]>103||a[2]<0||a[2]>103||a[3]<0||a[3]>12||a[4]<0||a[4]>3)return 2;
        GroundTile *t=&w->tiles[a[0]][a[1]][a[2]];
        if(reset){t->has_underlay=false;t->has_overlay=false;}
        world3d_set_tile(w,a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8],a[9],a[10],a[11],a[12],a[13],a[14],a[15],a[16],a[17],a[18],a[19]);
        unsigned before=differences;
        int has_u=integer(f),has_o=integer(f);if((has_u!=0&&has_u!=1)||(has_o!=0&&has_o!=1))return 2;
        fields+=2;if(t->has_underlay!=(has_u!=0)||t->has_overlay!=(has_o!=0))differences++;
        if(has_u){
            TileUnderlay *u=&t->underlay;int values[]={u->southwest_color,u->southeast_color,u->northeast_color,u->northwest_color,u->texture_id,u->rgb,u->flat};
            for(unsigned j=0;j<sizeof(values)/sizeof(*values);j++)check(f,values[j],n,"underlay");
        }
        if(has_o){
            TileOverlay *o=&t->overlay;
            int values[]={o->shape,o->rotation,o->background_rgb,o->foreground_rgb,o->flat,o->triangle_texture_id!=-1};
            for(unsigned j=0;j<sizeof(values)/sizeof(*values);j++)check(f,values[j],n,"overlay");
            int vertices=integer(f),triangles=integer(f);if(vertices<4||vertices>6||triangles<2||triangles>6)return 2;
            fields+=2;if(vertices!=o->vertex_count||triangles!=o->triangle_count)differences++;
            for(int j=0;j<vertices;j++){check(f,o->vertex_x[j],n,"vertex x");check(f,o->vertex_y[j],n,"vertex y");check(f,o->vertex_z[j],n,"vertex z");}
            for(int j=0;j<triangles;j++){
                int tri[]={o->triangle_vertex_a[j],o->triangle_vertex_b[j],o->triangle_vertex_c[j],o->triangle_color_a[j],o->triangle_color_b[j],o->triangle_color_c[j],o->triangle_texture[j]};
                for(unsigned k=0;k<sizeof(tri)/sizeof(*tri);k++)check(f,tri[k],n,"triangle");
            }
        }
        if(differences!=before)failed++;
    }
    if(fgetc(f)!=EOF){fputs("trailing tile fixture data\n",stderr);return 2;}
    fclose(f);free(w);
    printf("Tile construction: %u/%u calls match; %u fields, %u differences\n",count-failed,count,fields,differences);
    return differences?1:0;
}
