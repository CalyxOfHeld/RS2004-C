#include "config/spotanim.h"
#include "gfx/pix3d.h"
#include <stdlib.h>
#include <string.h>
void spotanim_free(SpotAnimConfig *c) {
    for(int i=0;i<c->count;i++) model_free_unshared(c->instances[i].base);
    free(c->instances);memset(c,0,sizeof(*c));
}
bool spotanim_unpack(SpotAnimConfig *c,Jagfile *j,const SeqConfig *seqs) {
    memset(c,0,sizeof(*c));c->sequences=seqs;
    size_t len=0;uint8_t *data=jagfile_read(j,"spotanim.dat",NULL,&len,NULL);
    if(!data || len<2) {free(data);return false;}
    Packet p=packet_wrap(data,len);c->count=g2(&p);c->instances=calloc(c->count,sizeof(*c->instances));
    if(!c->instances) {free(data);c->count=0;return false;}
    for(int i=0;i<c->count;i++) {
        SpotAnimType *s=&c->instances[i];s->anim=-1;s->resizeh=s->resizev=128;
        for(;;) {
            if(p.pos>=len) goto bad;
            int op=g1(&p);if(!op) break;
            if(op==3) {s->anim_has_alpha=true;continue;}
            if(op==7 || op==8) {if(p.pos>=len) goto bad;int v=g1(&p);if(op==7)s->ambient=v;else s->contrast=v;continue;}
            if(p.pos+2>len) goto bad;
            int v=g2(&p);
            if(op==1)s->model=v;else if(op==2)s->anim=v;else if(op==4)s->resizeh=v;
            else if(op==5)s->resizev=v;else if(op==6)s->orientation=v;
            else if(op>=40 && op<46)s->recol_s[op-40]=v;
            else if(op>=50 && op<56)s->recol_d[op-50]=v;else goto bad;
        }
        if(s->anim>=0 && (!seqs || s->anim>=seqs->count)) goto bad;
    }
    if(p.pos!=len) goto bad;
    free(data);return true;
bad:free(data);spotanim_free(c);return false;
}
const SeqType *spotanim_seq(const SpotAnimConfig *c,int id) {
    if(!c || id<0 || id>=c->count || !c->sequences) return NULL;
    int anim=c->instances[id].anim;
    return anim>=0 && anim<c->sequences->count?&c->sequences->instances[anim]:NULL;
}
Model *spotanim_model(SpotAnimConfig *c,int id,int frame,int pitch,int offset,bool oriented,Arena *arena) {
    if(!c || id<0 || id>=c->count) return NULL;
    SpotAnimType *s=&c->instances[id];
    if(!s->base) {
        s->base=model_load(s->model,NULL);if(!s->base)return NULL;
        // Preserve the original's recol_s[0] gate for all six recolour entries.
        if(s->recol_s[0]) for(int i=0;i<6;i++)model_recolor(s->base,s->recol_s[i],s->recol_d[i]);
    }
    // Model copies allocate a number of arrays; reserve a conservative bound
    // before entering the older arena-backed copy/label routines.
    size_t need=65536u+(size_t)s->base->num_points*128u+(size_t)s->base->num_faces*128u;
    if(!arena || arena->offset>arena->capacity || need>arena->capacity-arena->offset)return NULL;
    Model *m=model_copy(s->base,arena);if(!m)return NULL;
    if(s->anim_has_alpha && !m->face_alpha)m->face_alpha=arena_alloc_zero(arena,m->num_faces*sizeof(int32_t));
    if(offset)model_translate(m,-offset,0,0);
    const SeqType *seq=spotanim_seq(c,id);
    if(seq && frame>=0 && frame<seq->frame_count) {
        model_create_label_references(m,arena);model_apply_transform(m,seq->frames[frame]);
    }
    int sin=pix3d_sin_table[pitch&2047],cos=pix3d_cos_table[pitch&2047];
    for(int i=0;i<m->num_points;i++) {
        m->point_x[i]=m->point_x[i]*s->resizeh/128;
        m->point_y[i]=m->point_y[i]*s->resizev/128;
        m->point_z[i]=m->point_z[i]*s->resizeh/128;
        if(pitch) {int y=(m->point_y[i]*cos-m->point_z[i]*sin)>>16;m->point_z[i]=(m->point_y[i]*sin+m->point_z[i]*cos)>>16;m->point_y[i]=y;}
        if(oriented) for(int n=0;n<s->orientation/90 && n<3;n++) {int x=m->point_x[i];m->point_x[i]=m->point_z[i];m->point_z[i]=-x;}
    }
    model_calculate_normals(m,s->ambient+64,s->contrast+850,-30,-50,-30);
    return m;
}
