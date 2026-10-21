#include "entity/npc_entity.h"
#include "entity/actor_model.h"
#include <string.h>
#include <stdlib.h>

void npc_entity_init(NpcEntity *npc) {
    if (!npc) return;
    memset(npc, 0, sizeof(NpcEntity));
    pathing_entity_init(&npc->base);
    npc->has_type = false;
}

Model* npc_entity_get_model(NpcEntity *npc, Arena *arena) {
    if (!npc || !npc->has_type) return NULL;
    return npc_get_model(&npc->type, arena);
}

Model* npc_entity_get_animated_model(NpcEntity *npc,const SeqConfig *sequences,Arena *arena) {
    if(!npc || !npc->has_type)return NULL;
    Model *rest=actor_model_cached(false,(uint64_t)npc->type.index);
    if(!rest) {
        Arena *scratch=actor_model_scratch();Model *base=npc_get_model(&npc->type,scratch);if(!base)return NULL;
        model_create_label_references(base,scratch);rest=actor_model_store(false,(uint64_t)npc->type.index,base);
    }
    Model *model=model_copy(rest,arena);if(!model)return NULL;
    if(!npc->type.anim_has_alpha)model_share_alpha(model,rest);
    else if(!model->face_alpha)model->face_alpha=arena ? arena_alloc_zero(arena,model->num_faces*sizeof(int32_t)):calloc(model->num_faces,sizeof(int32_t));
    int primary,secondary;const int *mask;actor_model_frames(&npc->base,sequences,&primary,&secondary,&mask);
    model_apply_transforms(model,primary,secondary,mask);
    if(npc->type.resizeh!=128 || npc->type.resizev!=128)for(int v=0;v<model->num_points;v++) {
        model->point_x[v]=model->point_x[v]*npc->type.resizeh/128;model->point_y[v]=model->point_y[v]*npc->type.resizev/128;model->point_z[v]=model->point_z[v]*npc->type.resizeh/128;
    }
    if(npc->base.primary_seq_id<0 || npc->base.primary_seq_delay!=0) {
        npc->base.height=0;for(int v=0;v<model->num_points;v++)if(-model->point_y[v]>npc->base.height)npc->base.height=-model->point_y[v];
    }
    return model;
}

void npc_entity_free(NpcEntity *npc) {
    if (!npc) return;
    if (npc->has_type) {
        npc_free_entry(&npc->type);
        npc->has_type = false;
    }
}
