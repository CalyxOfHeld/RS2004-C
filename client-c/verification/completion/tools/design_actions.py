from pathlib import Path
p=Path('include/ui/client_actions.h');s=p.read_text().replace('#include "net/protocol.h"','#include "net/protocol.h"\n#include "config/idk.h"\n#include "entity/player_entity.h"');s=s.replace('typedef struct {\n    GameSession', '''typedef struct {int gender,parts[7],colors[5];bool initialized;} ClientDesign;
typedef struct {
    GameSession''').replace('    bool live,trace;','    bool live,trace;\n    IdkConfig *idkits;\n    ClientDesign *design;');s=s.replace('ActionResult client_action_dispatch', 'void client_design_init(ClientDesign *design,IdkConfig *kits);\nvoid client_design_prepare(ClientActions *ctx,Component *root,Arena *frame,int cycle);\nActionResult client_action_dispatch');p.write_text(s)
p=Path('include/net/protocol.h');s=p.read_text().replace('bool protocol_send_item_on_item(', 'bool protocol_send_design(GameSession *session,int gender,const int parts[7],const int colors[5]);\nbool protocol_send_item_on_item(');p.write_text(s)
p=Path('src/net/protocol.c');s=p.read_text();idx=s.index('bool protocol_send_item_on_item(');s=s[:idx]+'''bool protocol_send_design(GameSession *session,int gender,const int parts[7],const int colors[5]) {
    if(!session || !session->in_game || !parts || !colors || gender<0 || gender>1) return false;
    uint8_t bytes[14];Packet p=packet_wrap(bytes,sizeof(bytes));
    p1(&p,52+isaac_next(&session->out_cipher));p1(&p,gender);
    for(int i=0;i<7;i++) p1(&p,parts[i]);
    for(int i=0;i<5;i++) p1(&p,colors[i]);
    return client_stream_write(&session->stream,bytes,p.pos);
}

'''+s[idx:];p.write_text(s)
p=Path('src/ui/client_actions.c');s=p.read_text().replace('#include <string.h>','#include <string.h>\n#include <math.h>\n#include "gfx/pix3d.h"');idx=s.index('ActionResult client_action_dispatch');s=s[:idx]+'''static void design_validate(ClientDesign *d,IdkConfig *kits) {
    for(int part=0;part<7;part++) {
        d->parts[part]=-1;
        for(int kit=0;kit<kits->count;kit++) if(!kits->instances[kit].disable && kits->instances[kit].type==part+d->gender*7) {
            d->parts[part]=kit;break;
        }
    }
}
void client_design_init(ClientDesign *design,IdkConfig *kits) {
    memset(design,0,sizeof(*design));design_validate(design,kits);design->initialized=true;
}
static ActionResult design_button(ClientActions *ctx,int code) {
    ClientDesign *d=ctx->design;IdkConfig *kits=ctx->idkits;
    if(!d || !kits || !kits->count) return ACTION_FAILED;
    if(!d->initialized) client_design_init(d,kits);
    if(code>=300 && code<=313) {
        int part=(code-300)/2,kit=d->parts[part];
        if(kit>=0) for(int attempt=0;attempt<kits->count;attempt++) {
            kit=(kit+((code&1)?1:kits->count-1))%kits->count;
            if(!kits->instances[kit].disable && kits->instances[kit].type==part+d->gender*7) {d->parts[part]=kit;break;}
        }
    } else if(code>=314 && code<=323) {
        const int counts[5]={12,16,16,6,8};int part=(code-314)/2;
        d->colors[part]=(d->colors[part]+((code&1)?1:counts[part]-1))%counts[part];
    } else if(code==324 || code==325) {
        int gender=code-324;if(d->gender!=gender) {d->gender=gender;design_validate(d,kits);}
    } else if(code==326) {
        if(ctx->live && !protocol_send_design(ctx->session,d->gender,d->parts,d->colors)) return ACTION_FAILED;
        return ctx->live?ACTION_SENT:ACTION_LOCAL;
    }
    return ACTION_LOCAL;
}
void client_design_prepare(ClientActions *ctx,Component *root,Arena *frame,int cycle) {
    if(!ctx || !ctx->design || !root) return;
    if(root->client_code==327) {
        if(!ctx->design->initialized) client_design_init(ctx->design,ctx->idkits);
        PlayerEntity preview;player_entity_init(&preview);preview.visible=true;preview.gender=ctx->design->gender;
        const int slots[7]={8,11,4,6,9,7,10};
        for(int i=0;i<7;i++) if(ctx->design->parts[i]>=0) preview.appearances[slots[i]]=256+ctx->design->parts[i];
        memcpy(preview.colors,ctx->design->colors,sizeof(preview.colors));
        root->model=player_entity_get_model(&preview,ctx->idkits,ctx->objects,NULL,frame);
        root->xan=150;root->yan=((int)(sin(cycle/40.0)*256))&2047;
    }
    if(root->child_id) for(int i=0;i<root->child_count;i++) {
        Component *child=component_get(ctx->components,root->child_id[i]);
        if(child) client_design_prepare(ctx,child,frame,cycle);
    }
}

'''+s[idx:];s=s.replace('''        if(ctx->live) sent=protocol_send_button(ctx->session,action.component);''','''        if(button->client_code>=300 && button->client_code<=326) {
            ActionResult design=design_button(ctx,button->client_code);
            if(design==ACTION_FAILED || button->client_code!=326) return design;
        } else if(button->client_code!=0 && button->client_code!=205) return ACTION_UNAVAILABLE;
        if(ctx->live) sent=protocol_send_button(ctx->session,action.component);''');p.write_text(s)
p=Path('platform/sdl2_main.c');s=p.read_text().replace('''    ClientActions actions={&session,&comp_cfg,&varp_ctx,&obj_cfg,&selection,&chatbox,
        &viewport_interface_id,&chat_interface_id,live_server,trace};''','''    ClientDesign design={0};
    ClientActions actions={.session=&session,.components=&comp_cfg,.varps=&varp_ctx,.objects=&obj_cfg,
        .selection=&selection,.chatbox=&chatbox,.viewport_interface=&viewport_interface_id,
        .chat_interface=&chat_interface_id,.live=live_server,.trace=trace,.idkits=&idk_cfg,.design=&design};''');s=s.replace('''                    component_draw_with_varps(&comp_cfg, modal_com, mx_pos, my_pos, 0, &varp_ctx);''','''                    client_design_prepare(&actions,modal_com,&arena_frame,rendered_frames);
                    component_draw_with_varps(&comp_cfg, modal_com, mx_pos, my_pos, 0, &varp_ctx);''');s=s.replace('if ((forward != 0 || strafe != 0) && player.base.path_length == 0)', 'if (offline && (forward != 0 || strafe != 0) && player.base.path_length == 0)');p.write_text(s)
# Do not overwrite frame-arena lighting arrays with fresh per-frame heap allocations.
p=Path('src/gfx/model.c');s=p.read_text().replace('    m->face_colour_a = (int32_t *)malloc(m->num_faces * sizeof(int32_t));', '    if(!m->face_colour_a) m->face_colour_a = (int32_t *)malloc(m->num_faces * sizeof(int32_t));').replace('    m->face_colour_b = (int32_t *)malloc(m->num_faces * sizeof(int32_t));','    if(!m->face_colour_b) m->face_colour_b = (int32_t *)malloc(m->num_faces * sizeof(int32_t));').replace('    m->face_colour_c = (int32_t *)malloc(m->num_faces * sizeof(int32_t));','    if(!m->face_colour_c) m->face_colour_c = (int32_t *)malloc(m->num_faces * sizeof(int32_t));');p.write_text(s)
