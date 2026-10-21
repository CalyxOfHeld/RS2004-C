#include "config/loc.h"
#include "gfx/model.h"
#include "gfx/pix3d.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <strings.h>

typedef struct {uint64_t key,age;Model *model;} LocModelCache;
static LocModelCache base_cache[500],frame_cache[30];
static uint64_t cache_age;
static Model *cache_get(LocModelCache *cache,int count,uint64_t key) {
    for(int i=0;i<count;i++) if(cache[i].model && cache[i].key==key) {cache[i].age=++cache_age;return cache[i].model;}
    return NULL;
}
static void cache_put(LocModelCache *cache,int count,uint64_t key,Model *model) {
    int oldest=0;
    for(int i=0;i<count;i++) {
        if(!cache[i].model) {oldest=i;break;}
        if(cache[i].age<cache[oldest].age) oldest=i;
    }
    model_free_unshared(cache[oldest].model);
    cache[oldest]=(LocModelCache){key,++cache_age,model};
}
void loc_clear_model_cache(bool all) {
    for(int i=0;i<500;i++){model_free_unshared(base_cache[i].model);base_cache[i]=(LocModelCache){0};}
    if(all) for(int i=0;i<30;i++){model_free_unshared(frame_cache[i].model);frame_cache[i]=(LocModelCache){0};}
}

void loc_reset(LocType* loc) {
    memset(loc, 0, sizeof(LocType));
    loc->index = -1;
    loc->width = 1;
    loc->length = 1;
    loc->blockwalk = true;
    loc->blockrange = true;
    loc->active = false;
    loc->hillskew = false;
    loc->sharelight = false;
    loc->occlude = false;
    loc->anim = -1;
    loc->wallwidth = 16;
    loc->ambient = 0;
    loc->contrast = 0;
    loc->anim_has_alpha = false;
    loc->mapfunction = -1;
    loc->mapscene = -1;
    loc->mirror = false;
    loc->shadow = true;
    loc->resizex = 128;
    loc->resizey = 128;
    loc->resizez = 128;
    loc->forceapproach = 0;
    loc->offsetx = 0;
    loc->offsety = 0;
    loc->offsetz = 0;
    loc->forcedecor = false;
}

void loc_unpack(LocConfig* config, Jagfile* jag) {
    loc_clear_model_cache(true);
    size_t idx_len = 0;
    uint8_t* idx_data = jagfile_read(jag, "loc.idx", NULL, &idx_len, NULL);
    if (!idx_data) {
        config->count = 0;
        config->offsets = NULL;
        config->dat = NULL;
        config->dat_len = 0;
        return;
    }

    Packet idx_pkt = packet_wrap(idx_data, idx_len);
    config->count = g2(&idx_pkt);
    config->offsets = (int*)malloc(config->count * sizeof(int));

    int offset = 2;
    for (int i = 0; i < config->count; i++) {
        config->offsets[i] = offset;
        offset += g2(&idx_pkt);
    }
    free(idx_data);

    size_t dat_sz = 0;
    config->dat = jagfile_read(jag, "loc.dat", NULL, &dat_sz, NULL);
    config->dat_len = (int32_t)dat_sz;
}

void loc_decode(LocType* loc, Packet* pkt) {
    int active_override = -1;

    while (1) {
        int code = g1(pkt);
        if (code == 0) {
            if (active_override == -1) {
                loc->active = false;
                if (loc->model_count > 0 && loc->shapes && loc->shapes[0] == 10) {
                    loc->active = true;
                }
                for (int i = 0; i < 5; i++) {
                    if (loc->op_valid[i]) {
                        loc->active = true;
                        break;
                    }
                }
            }
            return;
        }

        if (code == 1) {
            int count = g1(pkt);
            loc->model_count = count;
            loc->models = (int*)malloc(count * sizeof(int));
            loc->shapes = (int*)malloc(count * sizeof(int));
            for (int i = 0; i < count; i++) {
                loc->models[i] = g2(pkt);
                loc->shapes[i] = g1(pkt);
            }
        } else if (code == 2) {
            gjstr(pkt, loc->name, sizeof(loc->name));
        } else if (code == 3) {
            gjstr(pkt, loc->desc, sizeof(loc->desc));
        } else if (code == 14) {
            loc->width = g1(pkt);
        } else if (code == 15) {
            loc->length = g1(pkt);
        } else if (code == 17) {
            loc->blockwalk = false;
        } else if (code == 18) {
            loc->blockrange = false;
        } else if (code == 19) {
            active_override = g1(pkt);
            if (active_override == 1) {
                loc->active = true;
            }
        } else if (code == 21) {
            loc->hillskew = true;
        } else if (code == 22) {
            loc->sharelight = true;
        } else if (code == 23) {
            loc->occlude = true;
        } else if (code == 24) {
            loc->anim = g2(pkt);
            if (loc->anim == 65535) {
                loc->anim = -1;
            }
        } else if (code == 25) {
            loc->anim_has_alpha = true;
        } else if (code == 28) {
            loc->wallwidth = g1(pkt);
        } else if (code == 29) {
            loc->ambient = g1b(pkt);
        } else if (code == 39) {
            loc->contrast = g1b(pkt);
        } else if (code >= 30 && code < 39) {
            int op_idx = code - 30;
            if (op_idx < 5) {
                char temp[32];
                gjstr(pkt, temp, sizeof(temp));
                if (strcasecmp(temp, "hidden") != 0) {
                    snprintf(loc->op[op_idx], sizeof(loc->op[op_idx]), "%s", temp);
                    loc->op_valid[op_idx] = true;
                }
            }
        } else if (code == 40) {
            int count = g1(pkt);
            loc->recol_count = count;
            loc->recol_s = (int*)malloc(count * sizeof(int));
            loc->recol_d = (int*)malloc(count * sizeof(int));
            for (int i = 0; i < count; i++) {
                loc->recol_s[i] = g2(pkt);
                loc->recol_d[i] = g2(pkt);
            }
        } else if (code == 60) {
            loc->mapfunction = g2(pkt);
        } else if (code == 62) {
            loc->mirror = true;
        } else if (code == 64) {
            loc->shadow = false;
        } else if (code == 65) {
            loc->resizex = g2(pkt);
        } else if (code == 66) {
            loc->resizey = g2(pkt);
        } else if (code == 67) {
            loc->resizez = g2(pkt);
        } else if (code == 68) {
            loc->mapscene = g2(pkt);
        } else if (code == 69) {
            loc->forceapproach = g1(pkt);
        } else if (code == 70) {
            loc->offsetx = g2b(pkt);
        } else if (code == 71) {
            loc->offsety = g2b(pkt);
        } else if (code == 72) {
            loc->offsetz = g2b(pkt);
        } else if (code == 73) {
            loc->forcedecor = true;
        }
    }
}

bool loc_get(LocConfig* config, int id, LocType* out_loc) {
    if (!config || !config->dat || id < 0 || id >= config->count) {
        return false;
    }

    loc_reset(out_loc);
    out_loc->index = id;

    Packet pkt = packet_wrap(config->dat, config->dat_len);
    pkt.pos = (size_t)config->offsets[id];

    loc_decode(out_loc, &pkt);
    return true;
}

void loc_free_entry(LocType* loc) {
    if (!loc) return;
    if (loc->models) free(loc->models);
    if (loc->shapes) free(loc->shapes);
    if (loc->recol_s) free(loc->recol_s);
    if (loc->recol_d) free(loc->recol_d);
    loc->models = NULL;
    loc->shapes = NULL;
    loc->recol_s = NULL;
    loc->recol_d = NULL;
}

void loc_free(LocConfig* config) {
    loc_clear_model_cache(true);
    if (!config) return;
    if (config->offsets) free(config->offsets);
    if (config->dat) free(config->dat);
    config->offsets = NULL;
    config->dat = NULL;
    config->count = 0;
}

Model* loc_get_model(const LocType* loc, int shape, int rotation, Arena* arena) {
    return loc_get_model_frame(loc,shape,rotation,-1,arena);
}

Model* loc_get_model_frame(const LocType* loc, int shape, int rotation, int frame, Arena* arena) {
    if (!loc || !loc->models || !loc->shapes || loc->model_count <= 0) return NULL;
    int model_idx = -1;
    if (loc->shapes) {
        for (int i = 0; i < loc->model_count; i++) {
            if (loc->shapes[i] == shape) {
                model_idx = i;
                break;
            }
        }
    } else {
        model_idx = 0;
    }
    if(model_idx<0) return NULL; // original LocType never substitutes another shape
    int m_id = loc->models[model_idx];
    if (m_id < 0) return NULL;

    uint64_t key=(uint64_t)((loc->index<<6)+(model_idx<<3)+rotation)+((uint64_t)(frame+1)<<32);
    Model *cached=cache_get(frame_cache,30,key);
    if(cached) {
        Model *result=model_copy(cached,arena);
        if(result && !arena && !loc->anim_has_alpha) model_share_alpha(result,cached);
        return result;
    }
    bool mirror=loc->mirror != (rotation>3);
    uint64_t base_key=(uint64_t)(m_id+(mirror?65536:0));
    Model *base=cache_get(base_cache,500,base_key);
    if(!base) {
        base=model_load(m_id,NULL);if(!base) return NULL;
        if(mirror) {
            for(int i=0;i<base->num_points;i++) base->point_z[i]=-base->point_z[i];
            for(int i=0;i<base->num_faces;i++){int v=base->face_vertex_a[i];base->face_vertex_a[i]=base->face_vertex_c[i];base->face_vertex_c[i]=v;}
        }
        cache_put(base_cache,500,base_key,base);
    }
    Model *m=model_copy(base,NULL);if(!m) return NULL;
    if(!loc->anim_has_alpha) model_share_alpha(m,base);
    if(loc->anim_has_alpha && !m->face_alpha) m->face_alpha=calloc(m->num_faces,sizeof(int32_t));
    // LocType.getModel animates the mirrored base before rotating/scaling it.
    if(frame>=0) {
        model_apply_transform(m,frame);
    }
    for(int turn=0;turn<(rotation&3);turn++) for(int i=0;i<m->num_points;i++) {
        int x=m->point_x[i];m->point_x[i]=m->point_z[i];m->point_z[i]=-x;
    }
    if(loc->recol_s && loc->recol_d) for(int i=0;i<loc->recol_count;i++) model_recolor(m,loc->recol_s[i],loc->recol_d[i]);
    for(int i=0;i<m->num_points;i++) {
        m->point_x[i]=m->point_x[i]*loc->resizex/128+loc->offsetx;
        m->point_y[i]=m->point_y[i]*loc->resizey/128+loc->offsety;
        m->point_z[i]=m->point_z[i]*loc->resizez/128+loc->offsetz;
    }
    model_calculate_normals(m, 64 + loc->ambient, 768 + loc->contrast * 5, -50, -10, -50);
    // Original sharelight models defer Gouraud face shades to scene lighting.
    if(loc->sharelight) for(int f=0;f<m->num_faces;f++) if(!m->face_render_type || !(m->face_render_type[f]&1))
        m->face_colour_a[f]=m->face_colour_b[f]=m->face_colour_c[f]=0;
    if(loc->blockwalk) for(int i=0;i<m->num_points;i++)
        if(-m->point_y[i]>m->obj_raise) m->obj_raise=-m->point_y[i];
    // Cached geometry no longer needs animation groups. Returned models own
    // their arrays; shared alpha keeps the original cache aliasing semantics.
    if(m->label_vertices) for(int i=0;i<m->num_label_vertices;i++) free(m->label_vertices[i]);
    free(m->label_vertices);free(m->label_vertex_counts);
    m->label_vertices=NULL;m->label_vertex_counts=NULL;m->num_label_vertices=0;
    cache_put(frame_cache,30,key,m);
    Model *result=model_copy(m,arena);
    if(result && !arena && !loc->anim_has_alpha) model_share_alpha(result,m);
    return result;
}

void loc_skew_model(Model *m,const LocType *loc,int sw,int se,int ne,int nw) {
    if(!m || !loc->hillskew) return;
    int average=(sw+se+ne+nw)/4;
    for(int v=0;v<m->num_points;v++) {
        int south=sw+(se-sw)*(m->point_x[v]+64)/128;
        int north=nw+(ne-nw)*(m->point_x[v]+64)/128;
        m->point_y[v]+=south+(north-south)*(m->point_z[v]+64)/128-average;
    }
}
