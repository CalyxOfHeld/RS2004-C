#include "dash3d/visibility225.h"
#include "gfx/pix3d.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t word(FILE *f){unsigned char b[4];if(fread(b,1,4,f)!=4){fputs("truncated visibility fixture\n",stderr);exit(2);}return (uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3];}
static int32_t integer(FILE *f){uint32_t v=word(f);return v<=INT32_MAX?(int32_t)v:-1-(int32_t)(UINT32_MAX-v);}
static void read_projection(FILE *f,VisibilityProjection225 *p){
    p->left=integer(f);p->top=integer(f);p->right=integer(f);p->bottom=integer(f);p->center_x=integer(f);p->center_y=integer(f);
    p->sin_yaw=integer(f);p->cos_yaw=integer(f);p->sin_pitch=integer(f);p->cos_pitch=integer(f);
}
static bool same_projection(const VisibilityProjection225 *a,const VisibilityProjection225 *b){
    return a->left==b->left && a->top==b->top && a->right==b->right && a->bottom==b->bottom && a->center_x==b->center_x && a->center_y==b->center_y &&
        a->sin_yaw==b->sin_yaw && a->cos_yaw==b->cos_yaw && a->sin_pitch==b->sin_pitch && a->cos_pitch==b->cos_pitch;
}
int main(int argc,char **argv){
    if(argc!=2)return 2;
    FILE *f=fopen(argv[1],"rb");if(!f){perror(argv[1]);return 2;}
    if(word(f)!=0x56323235)return 2;
    unsigned matrices=word(f);if(!matrices || matrices>100)return 2;
    struct Guarded {uint32_t before[8];Visibility225 v;uint32_t after[8];};
    struct Guarded *guard=calloc(1,sizeof(*guard));if(!guard)return 2;
    for(int j=0;j<8;j++)guard->before[j]=guard->after[j]=UINT32_C(0x19a3b7e5);
    Visibility225 *v=&guard->v;pix3d_init();unsigned failed=0,state_bad=0;uint64_t total=0,bad=0;
    for(unsigned n=0;n<matrices;n++){
        int initial=integer(f),above=integer(f),width=integer(f),height=integer(f),below=integer(f);int32_t distance[9];
        if(initial<0 || initial>2)return 2;
        for(int j=0;j<9;j++)distance[j]=integer(f);
        if(initial!=2)memset(v->matrix,initial?1:0,sizeof(v->matrix));
        VisibilityProjection225 expected;read_projection(f,&expected);
        if(!world3d_visibility_init225(v,distance,above,width,height,below)){fputs("visibility init rejected valid input\n",stderr);return 2;}
        uint64_t before=bad;
        for(int p=0;p<8;p++)for(int yaw=0;yaw<32;yaw++)for(int x=0;x<51;x++)for(int z=0;z<51;z++){
            int value=fgetc(f);if(value<0 || value>1)return 2;
            if(v->matrix[p][yaw][x][z]!=(value!=0)){if(bad<8)printf("matrix %u [%d][%d][%d][%d] C=%d Java=%d\n",n,p,yaw,x,z,v->matrix[p][yaw][x][z],value);bad++;}total++;
        }
        bool state=!same_projection(&v->projection,&expected);state_bad+=state;if(bad!=before || state)failed++;
        for(int j=0;j<8;j++)if(guard->before[j]!=UINT32_C(0x19a3b7e5) || guard->after[j]!=UINT32_C(0x19a3b7e5)){fputs("visibility guard damaged\n",stderr);return 1;}
    }
    unsigned points=word(f),point_bad=0,hits=0;if(!points || points>100000)return 2;
    for(unsigned n=0;n<points;n++){
        VisibilityProjection225 p;read_projection(f,&p);int x=integer(f),z=integer(f),y=integer(f),expected=integer(f);if(expected<0 || expected>1)return 2;
        bool actual=world3d_visibility_point225(&p,x,z,y);hits+=expected!=0;
        if(actual!=(expected!=0)){if(point_bad<8)printf("point %u C=%d Java=%d\n",n,actual,expected);point_bad++;}
    }
    if(fgetc(f)!=EOF){fputs("trailing visibility fixture data\n",stderr);return 2;}
    fclose(f);free(guard);
    printf("Visibility matrices: %u/%u match; %llu booleans, %llu differences, %u state differences; guards intact\n",matrices-failed,matrices,(unsigned long long)total,(unsigned long long)bad,state_bad);
    printf("Visibility points: %u/%u match; %u accepted\n",points-point_bad,points,hits);
    return failed || point_bad?1:0;
}
