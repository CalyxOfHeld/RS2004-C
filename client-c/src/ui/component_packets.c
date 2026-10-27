#include "ui/component_packets.h"
#include "config/obj.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include "ui/chatbox.h"

int last_login_receive225(LastLoginInfo225 *info,InterfaceState *interfaces,Chatbox *chat,
    ComponentConfig *components,GameSession *session,const uint8_t *data,size_t length){
    if(!info || !interfaces || !chat || !components || components->count<0 ||
       (components->count && !components->instances) || !session || !data || length!=9)return -1;
    info->last_address=(uint32_t)data[0]<<24|(uint32_t)data[1]<<16|(uint32_t)data[2]<<8|data[3];
    info->days_since_login=data[4]*256+data[5];info->days_since_recovery=data[6];info->unread_messages=data[7]*256+data[8];
    if(!info->last_address || interfaces->viewport!=-1)return 1;
    snprintf(info->dns_request,sizeof(info->dns_request),"%u.%u.%u.%u",(unsigned)data[0],(unsigned)data[1],(unsigned)data[2],(unsigned)data[3]);
    strcpy(info->dns,info->dns_request);info->dns_generation++;
    if(!protocol_send_report(session,false,0,0,false))return -2;
    component_continue_close225(components,interfaces->sidebar,interfaces->chat);
    interfaces->viewport=interfaces->sidebar=interfaces->chat=-1;
    chat->report_input[0]=0;chat->report_mute=false;
    int code=info->days_since_recovery==201?650:655;
    for(int i=0;i<components->count;i++)if(components->instances[i].client_code==code){interfaces->viewport=components->instances[i].layer;break;}
    return 2;
}
void last_login_prepare225(LastLoginInfo225 *info,Component *component){
    if(!info || !component || component->client_code<650 || component->client_code>655)return;
    int code=component->client_code;char *text=info->content[code-650];
    if(code==650 || code==655){
        char when[40];if(info->days_since_login==0)strcpy(when,"earlier today");else if(info->days_since_login==1)strcpy(when,"yesterday");else snprintf(when,sizeof(when),"%d days ago",info->days_since_login);
        if(!info->last_address)text[0]=0;else snprintf(text,384,"You last logged in %s from: %s",when,info->dns[0]?info->dns:"null");
    }else if(code==651){
        snprintf(text,384,"%d unread message%s",info->unread_messages,info->unread_messages==1?"":"s");component->colour=info->unread_messages?65280:16776960;
    }else if(info->days_since_recovery==201)text[0]=0;
    else if(code==652){
        if(info->days_since_recovery==200)strcpy(text,"You have not yet set any password recovery questions.");
        else {char when[40];if(info->days_since_recovery==0)strcpy(when,"Earlier today");else if(info->days_since_recovery==1)strcpy(when,"Yesterday");else snprintf(when,sizeof(when),"%d days ago",info->days_since_recovery);snprintf(text,384,"%s you changed your recovery questions",when);}
    }else if(code==653)strcpy(text,info->days_since_recovery==200?"We strongly recommend you do so now to secure your account.":"If you do not remember making this change then cancel it immediately");
    else strcpy(text,"Do this from the 'account management' area on our front webpage");
    component->text=text;
}

typedef struct InterfaceModel225 {Model *model;int object;bool cached,colourless;uint64_t stamp;struct InterfaceModel225 *next;} InterfaceModel225;
typedef struct {InterfaceModel225 *head;uint64_t clock;} InterfaceModels225;
static InterfaceModel225 *interface_model_node225(const ComponentConfig *cfg,const Model *model){InterfaceModels225 *store=cfg?cfg->packet_models225:NULL;for(InterfaceModel225 *n=store?store->head:NULL;n;n=n->next)if(n->model==model)return n;return NULL;}
bool component_model_colourless225(const ComponentConfig *cfg,const Model *model){if(model && model->colourless225)return true;InterfaceModel225 *n=interface_model_node225(cfg,model);return n && n->colourless;}
void component_models_stats225(const ComponentConfig *cfg,int *owned,int *cached){*owned=*cached=0;InterfaceModels225 *store=cfg?cfg->packet_models225:NULL;for(InterfaceModel225 *n=store?store->head:NULL;n;n=n->next){(*owned)++;if(n->cached)(*cached)++;}}
static void interface_models_collect225(ComponentConfig *cfg){InterfaceModels225 *s=cfg->packet_models225;if(!s)return;int cached=0;InterfaceModel225 *oldest=NULL;
    for(InterfaceModel225 *n=s->head;n;n=n->next)if(n->cached){cached++;if(!oldest || n->stamp<oldest->stamp)oldest=n;}
    if(cached>50)oldest->cached=false;
    InterfaceModel225 **link=&s->head;while(*link){InterfaceModel225 *n=*link;bool used=n->cached;for(int i=0;!used && i<cfg->count;i++)used=cfg->instances[i].model==n->model;
        if(used)link=&n->next;else {*link=n->next;model_free_unshared(n->model);free(n);}}
}
void component_models_clear225(ComponentConfig *cfg){if(!cfg)return;InterfaceModels225 *s=cfg->packet_models225;if(!s)return;
    for(int i=0;i<cfg->count;i++)if(interface_model_node225(cfg,cfg->instances[i].model))cfg->instances[i].model=NULL;
    InterfaceModel225 *n=s->head;while(n){InterfaceModel225 *next=n->next;model_free_unshared(n->model);free(n);n=next;}free(s);cfg->packet_models225=NULL;
}
static int interface_model_packet225(ComponentConfig *cfg,int opcode,const uint8_t *data,size_t length){
    size_t expected=opcode==87?4:6;if(length!=expected || !data)return -1;
    int id=data[0]*256+data[1];Component *c=component_get(cfg,id);if(!c)return -1;
    int a=data[2]*256+data[3],b=length==6?data[4]*256+data[5]:0;
    if(opcode==103){if(!c->model)return 1;if(c->model->num_faces>0 && component_model_colourless225(cfg,c->model))return -1;model_recolor(c->model,a,b);return 1;}
    ObjType base;bool object=opcode==46;if(object && (!b || !cfg->obj_config || !obj_get(cfg->obj_config,a,&base)))return -1;
    InterfaceModels225 *store=cfg->packet_models225;
    if(!store){store=calloc(1,sizeof(*store));if(!store){if(object)obj_free_entry(&base);return -1;}cfg->packet_models225=store;}
    int object_id=a,model_id=a;ObjType variant;bool have_variant=false;InterfaceModel225 *node=NULL;
    if(object){int variant_id=-1;for(int i=0;i<10;i++)if(base.countco[i] && 50>=base.countco[i])variant_id=base.countobj[i];
        if(variant_id!=-1){if(!obj_get(cfg->obj_config,variant_id,&variant)){obj_free_entry(&base);return -1;}have_variant=true;object_id=variant_id;model_id=variant.model;}else model_id=base.model;
        for(InterfaceModel225 *n=store->head;n;n=n->next)if(n->cached && n->object==object_id){node=n;break;}
    }
    if(!node){Model *model=model_load(model_id,NULL);if(!model){if(have_variant)obj_free_entry(&variant);if(object)obj_free_entry(&base);return -1;}
        node=calloc(1,sizeof(*node));if(!node){model_free_unshared(model);if(have_variant)obj_free_entry(&variant);if(object)obj_free_entry(&base);return -1;}
        node->model=model;node->object=object_id;node->cached=object;
        if(object){ObjType *type=have_variant?&variant:&base;for(int i=0;i<type->recol_count;i++)model_recolor(model,type->recol_s[i],type->recol_d[i]);model_calculate_normals(model,64,768,-50,-10,-50);node->colourless=true;for(int i=0;i<model->num_faces;i++)if(model->face_render_type && (model->face_render_type[i]&2)){node->colourless=false;break;}}
        node->next=store->head;store->head=node;
    }
    node->stamp=++store->clock;c->model=node->model;
    if(object){c->xan=base.xan2d;c->yan=base.yan2d;c->zoom=base.zoom2d*100/b;obj_free_entry(&base);}
    if(have_variant)obj_free_entry(&variant);
    interface_models_collect225(cfg);return 1;
}

void interface_state_init(InterfaceState *state) {
    state->viewport=state->sidebar=state->chat=-1;
}

int interface_state_apply(InterfaceState *state,int opcode,const uint8_t *data,size_t length) {
    // client.java read(): modal sidebar overlays are separate from tab mappings.
    size_t expected;
    switch(opcode) {
        case 28: expected=4;break;
        case 14: case 168: case 195: expected=2;break;
        case 129: expected=0;break;
        default: return 0;
    }
    if(!state || length!=expected || (expected && !data)) return -1;
    int id=expected ? ((data[0]<<8)|data[1]):-1;
    if(id==65535) id=-1;
    if(opcode==28) {
        state->viewport=id;state->sidebar=(data[2]<<8)|data[3];
        if(state->sidebar==65535) state->sidebar=-1;
        state->chat=-1;
    } else if(opcode==14) {
        state->chat=id;state->sidebar=state->viewport=-1;
    } else if(opcode==168) {
        state->viewport=id;state->sidebar=state->chat=-1;
    } else if(opcode==195) {
        state->sidebar=id;state->viewport=state->chat=-1;
    } else interface_state_init(state);
    return 1;
}

typedef struct { const uint8_t *data; size_t size,pos; bool error; } Reader;
static uint32_t read_be(Reader *r,size_t bytes) {
    if(r->pos>r->size || bytes>r->size-r->pos) {r->error=true;return 0;}
    uint32_t value=0; while(bytes--) value=(value<<8)|r->data[r->pos++]; return value;
}
static int read_quantity(Reader *r) {
    uint32_t value=read_be(r,1); if(value==255) value=read_be(r,4);
    if(value>INT_MAX) r->error=true;
    return (int)value;
}
static int inventory_packet(ComponentConfig *cfg,Arena *arena,int opcode,Reader *r) {
    int id=read_be(r,2); Component *c=component_get(cfg,id);
    if(r->error || !c || (c->type!=COMPONENT_TYPE_INV_GRID && c->type!=COMPONENT_TYPE_TEXT_LIST)) return -1;
    if(c->width<=0 || c->height<=0 || c->width>65536/c->height) return -1;
    int capacity=c->width*c->height;
    if(opcode==15) {
        if(r->pos!=r->size) return -1;
        if(c->inv_slot_obj_id) memset(c->inv_slot_obj_id,0,capacity*sizeof(int));
        if(c->inv_slot_obj_count) memset(c->inv_slot_obj_count,0,capacity*sizeof(int));
        return 1;
    }
    int *ids=calloc(capacity,sizeof(int)),*counts=calloc(capacity,sizeof(int));
    if(!ids||!counts) {free(ids);free(counts);return -1;}
    if(opcode==213) {
        if(c->inv_slot_obj_id) memcpy(ids,c->inv_slot_obj_id,capacity*sizeof(int));
        if(c->inv_slot_obj_count) memcpy(counts,c->inv_slot_obj_count,capacity*sizeof(int));
    }
    int total=opcode==98 ? (int)read_be(r,1) : 0;
    if(total>capacity) r->error=true;
    int index=0;
    while(!r->error && (opcode==98 ? index<total : r->pos<r->size)) {
        int slot=opcode==98 ? index++ : (int)read_be(r,1);
        int object=read_be(r,2), count=read_quantity(r);
        if(slot>=capacity) {r->error=true;break;}
        ids[slot]=object;counts[slot]=object ? count : 0;
    }
    if(r->error || r->pos!=r->size) {free(ids);free(counts);return -1;}
    if(!c->inv_slot_obj_id) c->inv_slot_obj_id=arena_alloc_zero(arena,capacity*sizeof(int));
    if(!c->inv_slot_obj_count) c->inv_slot_obj_count=arena_alloc_zero(arena,capacity*sizeof(int));
    if(!c->inv_slot_obj_id || !c->inv_slot_obj_count) {free(ids);free(counts);return -1;}
    memcpy(c->inv_slot_obj_id,ids,capacity*sizeof(int));
    memcpy(c->inv_slot_obj_count,counts,capacity*sizeof(int));
    free(ids);free(counts);return 1;
}
int component_apply_packet(ComponentConfig *cfg,VarpContext *varps,Arena *arena,
                           int opcode,const uint8_t *data,size_t length) {
    if(!cfg || !varps || !arena || (!data && length)) return -1;
    if(opcode==46 || opcode==87 || opcode==103)return interface_model_packet225(cfg,opcode,data,length);
    Reader r={data,length,0,false};
    if(opcode==98 || opcode==213 || opcode==15) return inventory_packet(cfg,arena,opcode,&r);
    if(opcode==193) {
        if(length) return -1;
        for(int id=0;id<MAX_VARPS;id++)varp_set(varps,id,varps->server_varps[id]);
        return 1;
    }
    if(opcode==150 || opcode==175) {
        if(length!=(opcode==150?3u:6u)) return -1;
        int id=read_be(&r,2);int value=opcode==150?(int8_t)read_be(&r,1):(int32_t)read_be(&r,4);
        if(id>=MAX_VARPS) return -1;
        varps->server_varps[id]=value;
        varp_set(varps,id,value);return 1;
    }
    if(opcode==44) {
        if(length!=6) return -1;
        int skill=read_be(&r,1);uint32_t xp=read_be(&r,4);int level=read_be(&r,1);
        if(skill>=25 || xp>INT_MAX/10) return -1;
        varps->skill_levels[skill]=level;varps->skill_exp[skill]=(int)xp*10;
        int points=0,base=1;
        for(int i=1;i<=99;i++) {
            if(xp>=(uint32_t)(points/4)) base=i;
            points+=(int)(i+300.0*pow(2.0,i/7.0));
        }
        varps->skill_base_levels[skill]=base;return 1;
    }
    if(opcode==68) {if(length!=1) return -1;varps->energy=data[0];return 1;}
    if(opcode==22) {if(length!=2) return -1;varps->weight=(int16_t)read_be(&r,2);return 1;}
    size_t expected=opcode==2?4:opcode==26?3:opcode==209?6:0;
    if(!expected && opcode!=201) return 0;
    if((expected && length!=expected) || length<2) return -1;
    int id=read_be(&r,2);Component *c=component_get(cfg,id);if(!c) return -1;
    if(opcode==2) {
        uint32_t color=read_be(&r,2);
        c->colour=((color>>10)&31)<<19 | ((color>>5)&31)<<11 | (color&31)<<3;
    } else if(opcode==26) c->hide=read_be(&r,1)==1;
    else if(opcode==209) {c->x=(int16_t)read_be(&r,2);c->y=(int16_t)read_be(&r,2);}
    else if(opcode==201) {
        if(length<3 || data[length-1]!=10 || memchr(data+2,10,length-3)) return -1;
        char *text=arena_alloc(arena,length-1);if(!text) return -1;
        memcpy(text,data+2,length-3);text[length-3]=0;c->text=text;
    }
    return 1;
}
