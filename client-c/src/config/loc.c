#include "config/loc.h"
#include "gfx/model.h"
#include "gfx/pix3d.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <strings.h>

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
    if (!config) return;
    if (config->offsets) free(config->offsets);
    if (config->dat) free(config->dat);
    config->offsets = NULL;
    config->dat = NULL;
    config->count = 0;
}

Model* loc_get_model(const LocType* loc, int shape, int rotation, Arena* arena) {
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

    // model_load returns a fresh, unshared model in this port; no cached
    // source model needs copying. The scene owns this returned allocation.
    Model *m=model_load(m_id,arena);
    if(!m) return NULL;

    if(loc->mirror != (rotation>3)) {
        for(int i=0;i<m->num_points;i++) m->point_z[i]=-m->point_z[i];
        for(int i=0;i<m->num_faces;i++) {int v=m->face_vertex_a[i];m->face_vertex_a[i]=m->face_vertex_c[i];m->face_vertex_c[i]=v;}
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
    return m;
}
