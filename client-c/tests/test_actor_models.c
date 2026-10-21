#include "entity/actor_model.h"
#include "entity/player_entity.h"
#include "entity/npc_entity.h"
#include "gfx/anim.h"
#include "gfx/pix3d.h"
#include "dash3d/effects.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int rd(FILE *f){uint8_t b[4];assert(fread(b,1,4,f)==4);return (int32_t)((uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3]);}
static void compare(FILE *f,const Model *m,int id,int variant) {
    int vertices=rd(f),faces=rd(f),textures=rd(f);
    if(!m || vertices!=m->num_points || faces!=m->num_faces || textures!=m->num_t){fprintf(stderr,"counts actor=%d variant=%d expected=%d/%d/%d actual=%d/%d/%d\n",id,variant,vertices,faces,textures,m?m->num_points:-1,m?m->num_faces:-1,m?m->num_t:-1);abort();}
    for(int i=0;i<vertices;i++) {
        int x=rd(f),y=rd(f),z=rd(f);
        if(x!=m->point_x[i] || y!=m->point_y[i] || z!=m->point_z[i]){fprintf(stderr,"vertex actor=%d variant=%d v=%d expected=%d,%d,%d actual=%d,%d,%d\n",id,variant,i,x,y,z,m->point_x[i],m->point_y[i],m->point_z[i]);abort();}
    }
    for(int i=0;i<faces;i++) {
        int a=rd(f),b=rd(f),c=rd(f),colour=rd(f),sa=rd(f),sb=rd(f),sc=rd(f),type=rd(f),priority=rd(f),alpha=rd(f);
        if(a!=m->face_vertex_a[i] || b!=m->face_vertex_b[i] || c!=m->face_vertex_c[i] || type!=(m->face_render_type?m->face_render_type[i]:0) || priority!=(m->face_priority?m->face_priority[i]:m->priority) || alpha!=(m->face_alpha?m->face_alpha[i]:0) || ((type&2) && colour!=m->face_colour[i]) || sa!=m->face_colour_a[i] || (!(type&1) && (sb!=m->face_colour_b[i] || sc!=m->face_colour_c[i]))) {
            fprintf(stderr,"face actor=%d variant=%d f=%d alpha=%d/%d shade=%d,%d,%d/%d,%d,%d type=%d/%d\n",id,variant,i,alpha,m->face_alpha?m->face_alpha[i]:0,sa,sb,sc,m->face_colour_a[i],m->face_colour_b[i],m->face_colour_c[i],type,m->face_render_type?m->face_render_type[i]:0);abort();
        }
    }
    for(int i=0;i<textures;i++){int a=rd(f),b=rd(f),c=rd(f);assert(a==m->texture_a[i] && b==m->texture_b[i] && c==m->texture_c[i]);}
}
static void selection(FILE *f,PathingEntity *a) {a->seq_stand_id=rd(f);a->primary_seq_id=rd(f);a->primary_seq_frame=rd(f);a->primary_seq_delay=rd(f);a->secondary_seq_id=rd(f);a->secondary_seq_frame=rd(f);}
int main(void) {
    setbuf(stdout,NULL);Arena perm=arena_create(64*1024*1024),frame=arena_create(8*1024*1024),retained=arena_create(4*1024*1024);pix3d_init();
    Jagfile *config=jagfile_load_file("../engine/data/pack/client/config",&perm),*models=jagfile_load_file("../engine/data/pack/client/models",&perm);assert(config&&models);model_init_repo(models,&perm);assert(anim_unpack(models,&perm));
    SeqConfig seq;seq_unpack(&seq,config);NpcConfig npcs;npc_unpack(&npcs,config);ObjConfig objects;obj_unpack(&objects,config,true);IdkConfig identities;idk_unpack(&identities,config,&perm);SpotAnimConfig spots;assert(spotanim_unpack(&spots,config,&seq));
    FILE *f=fopen("tests/fixtures/actor-reference.bin","rb");assert(f && rd(f)==0x41435431);int npc_cases=0;
    for(int id;(id=rd(f))!=-1;npc_cases++) {
        NpcEntity npc;npc_entity_init(&npc);assert(npc_get(&npcs,id,&npc.type));npc.has_type=true;selection(f,&npc.base);
        npc.base.height=rd(f);arena_reset(&frame);Model *model=npc_entity_get_animated_model(&npc,&seq,&frame);compare(f,model,id,npc_cases%3);assert(npc.base.height==rd(f));npc_entity_free(&npc);
    }
    int nonrenderable=rd(f);assert(nonrenderable==0);
    printf("PASS: all %d real NPC definitions, %d original ready/walk/blended/scaled meshes, face values and retained rest-pose lighting\n",npcs.count,npc_cases);
    int player_cases=0;
    for(int length;(length=rd(f))!=-1;player_cases++) {
        assert(length>0 && length<256);uint8_t bytes[256];assert(fread(bytes,1,length,f)==(size_t)length);
        uint64_t hash=(uint64_t)(uint32_t)rd(f)<<32;hash|=(uint32_t)rd(f);PlayerEntity player;player_entity_init(&player);Packet packet=packet_wrap(bytes,length);player_entity_read_appearance(&player,&packet);assert(!packet.error && packet.pos==(size_t)length);
        if(hash!=player.appearance_hashcode){fprintf(stderr,"appearance=%d hash expected=%llx actual=%llx\n",player_cases,(unsigned long long)hash,(unsigned long long)player.appearance_hashcode);abort();}
        selection(f,&player.base);arena_reset(&frame);Model *model=player_entity_get_model(&player,&identities,&objects,&seq,&frame);compare(f,model,-1,player_cases);assert(player.base.height==rd(f));
    }
    printf("PASS: %d original decoded player appearance/sequence meshes across both genders, equipment, all %d sequences, hand overrides, frame blending, delays and fixed rest-pose lighting\n",player_cases,seq.count);
    int combined=rd(f);
    PlayerEntity effects_player;player_entity_init(&effects_player);effects_player.visible=true;
    int slots[]={8,11,4,6,9,7,10};for(int part=0;part<7;part++)for(int id=0;id<identities.count;id++)if(identities.instances[id].type==part && !identities.instances[id].disable){effects_player.appearances[slots[part]]=256+id;break;}
    effects_player.appearances[5]=1205+512;effects_player.appearances[3]=1171+512;int counts[]={12,16,16,6,8};for(int i=0;i<5;i++)effects_player.colors[i]=(i+3)%counts[i];
    for(int n=0;n<combined;n++) {
        int actor=rd(f),spot=rd(f),spot_frame=rd(f),height=rd(f);arena_reset(&frame);
        if(actor==0) {
            NpcEntity npc;npc_entity_init(&npc);assert(npc_get(&npcs,0,&npc.type));npc.has_type=true;selection(f,&npc.base);npc.base.spotanim_id=spot;npc.base.spotanim_frame=spot_frame;npc.base.spotanim_height=height;
            Model *body=npc_entity_get_animated_model(&npc,&seq,&frame);Model *model=actor_with_spotanim(body,&npc.base,&spots,&frame);compare(f,model,0,n);assert(npc.base.height==rd(f));npc_entity_free(&npc);
        } else {
            selection(f,&effects_player.base);effects_player.base.spotanim_id=spot;effects_player.base.spotanim_frame=spot_frame;effects_player.base.spotanim_height=height;
            Model *body=player_entity_get_model(&effects_player,&identities,&objects,&seq,&frame);Model *model=actor_with_spotanim(body,&effects_player.base,&spots,&frame);compare(f,model,-1,n);assert(effects_player.base.height==rd(f));
        }
    }
    printf("PASS: %d complete original NPC/player draw composites with all animated spot effects, first/last frames, original lighting/alpha/texture fields and body height\n",combined);
    int retained_id=rd(f);
    if(retained_id>=0) {
        actor_models_clear();NpcEntity npc;npc_entity_init(&npc);assert(npc_get(&npcs,retained_id,&npc.type));npc.has_type=true;
        Model *model=npc_entity_get_animated_model(&npc,NULL,&retained);assert(model && model->alpha_storage && retained.shared_count>0);compare(f,model,retained_id,0);npc_entity_free(&npc);
        for(int id=0;id<40;id++)if(id!=retained_id) {
            npc_entity_init(&npc);assert(npc_get(&npcs,id,&npc.type));npc.has_type=true;arena_reset(&frame);assert(npc_entity_get_animated_model(&npc,NULL,&frame));npc_entity_free(&npc);
        }
        assert(actor_model_cached(false,retained_id)==NULL);compare(f,model,retained_id,1);actor_models_clear();compare(f,model,retained_id,2);
        assert(model->alpha_storage[0]==1);arena_reset(&retained);assert(retained.shared_count==0);
        puts("PASS: original retained shared-alpha mesh survives NPC LRU eviction/clear; frame-arena reset releases its final heap reference");
    }
    assert(fgetc(f)==EOF);fclose(f);
    // Original Model.applyTransform returns without vertex groups, even for alpha frames.
    int face_frame=-1,face_group=-1;
    for(int id=0;id<g_anim_cfg.frame_count && face_frame<0;id++){AnimFrame *af=anim_get_frame(id);if(!af || !af->base)continue;for(int i=0;i<af->length;i++){int group=af->groups[i];if(af->base->types[group]==5 && af->base->label_counts[group]>0 && af->x[i]!=0){face_frame=id;face_group=af->base->labels[group][0];break;}}}
    assert(face_frame>=0);Model face_only={.num_faces=1};int label=face_group,alpha=100;face_only.face_label=&label;face_only.face_alpha=&alpha;model_apply_transform(&face_only,face_frame);assert(alpha==100);
    actor_models_clear();spotanim_free(&spots);seq_free(&seq);npc_free(&npcs);obj_free(&objects);idk_free(&identities);arena_free(&retained);arena_free(&frame);arena_free(&perm);return 0;
}
