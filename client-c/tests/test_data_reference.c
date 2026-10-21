// Native counterpart of reference/src/DataOracle.java: dumps every object the
// native decoders produce from the bundled cache in the same
// "type id field=value" format. Fields the native structs do not store are
// simply not emitted; compare_data_reference.py reports them separately.
//   test_data_reference <cache dir> <out.txt> <model count>
#include "core/arena.h"
#include "core/jagfile.h"
#include "core/jrandom.h"
#include "config/flo.h"
#include "config/idk.h"
#include "config/loc.h"
#include "config/npc.h"
#include "config/obj.h"
#include "config/seq.h"
#include "config/spotanim.h"
#include "config/varp_type.h"
#include "gfx/anim.h"
#include "gfx/model.h"
#include "gfx/pixfont.h"
#include "ui/component.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE *out;
static const char *type_name;
static int cur_id;

/* SHA-256 (FIPS 180-4), used only to fingerprint sprite pixels as DataOracle does. */
typedef struct { uint32_t h[8]; uint8_t buf[64]; size_t len; uint64_t total; } Sha;
static const uint32_t K[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
#define ROR(x,n) (((x)>>(n))|((x)<<(32-(n))))
static void sha_block(Sha *s, const uint8_t *p) {
    uint32_t w[64], a,b,c,d,e,f,g,h;
    for (int i=0;i<16;i++) w[i]=(uint32_t)p[4*i]<<24|(uint32_t)p[4*i+1]<<16|(uint32_t)p[4*i+2]<<8|p[4*i+3];
    for (int i=16;i<64;i++) {
        uint32_t s0=ROR(w[i-15],7)^ROR(w[i-15],18)^(w[i-15]>>3), s1=ROR(w[i-2],17)^ROR(w[i-2],19)^(w[i-2]>>10);
        w[i]=w[i-16]+s0+w[i-7]+s1;
    }
    a=s->h[0];b=s->h[1];c=s->h[2];d=s->h[3];e=s->h[4];f=s->h[5];g=s->h[6];h=s->h[7];
    for (int i=0;i<64;i++) {
        uint32_t t1=h+(ROR(e,6)^ROR(e,11)^ROR(e,25))+((e&f)^(~e&g))+K[i]+w[i];
        uint32_t t2=(ROR(a,2)^ROR(a,13)^ROR(a,22))+((a&b)^(a&c)^(b&c));
        h=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;
    }
    s->h[0]+=a;s->h[1]+=b;s->h[2]+=c;s->h[3]+=d;s->h[4]+=e;s->h[5]+=f;s->h[6]+=g;s->h[7]+=h;
}
static void sha_init(Sha *s) {
    static const uint32_t iv[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    memcpy(s->h,iv,sizeof(iv)); s->len=0; s->total=0;
}
static void sha_byte(Sha *s, uint8_t v) {
    s->buf[s->len++]=v; s->total++;
    if (s->len==64) { sha_block(s,s->buf); s->len=0; }
}
static void sha_hex(Sha *s, char out16[17]) {
    uint64_t bits=s->total*8;
    sha_byte(s,0x80);
    while (s->len!=56) sha_byte(s,0);
    for (int i=7;i>=0;i--) sha_byte(s,(uint8_t)(bits>>(8*i)));
    for (int i=0;i<8;i++) sprintf(out16+2*i,"%02x",(unsigned)(s->h[i/4]>>(24-8*(i%4)))&255);
    out16[16]=0;
}
static void sha_ints(const uint32_t *p, int n, char hex[17]) {
    Sha s; sha_init(&s);
    for (int i=0;i<n;i++) { sha_byte(&s,(uint8_t)(p[i]>>24)); sha_byte(&s,(uint8_t)(p[i]>>16)); sha_byte(&s,(uint8_t)(p[i]>>8)); sha_byte(&s,(uint8_t)p[i]); }
    sha_hex(&s,hex);
}

static void key(const char *field) { fprintf(out, "%s %d %s=", type_name, cur_id, field); }
static void f_int(const char *field, long v) { key(field); fprintf(out, "%ld\n", v); }
static void f_null(const char *field) { key(field); fputs("null\n", out); }
static void put_str(const char *s) {
    fputc('"', out);
    for (const unsigned char *p=(const unsigned char *)s; *p; p++) {
        if (*p=='"'||*p=='\\') fprintf(out,"\\%c",*p);
        else if (*p<32||*p>126) fprintf(out,"\\u%04x",*p);
        else fputc(*p,out);
    }
    fputc('"', out);
}
static void f_str(const char *field, const char *s) { key(field); if (s) put_str(s); else fputs("null", out); fputc('\n', out); }
static void f_ints(const char *field, const int *v, int n) {
    key(field);
    if (!v) { fputs("null\n", out); return; }
    fputc('[', out);
    for (int i=0;i<n;i++) fprintf(out, i?",%d":"%d", v[i]);
    fputs("]\n", out);
}
static void f_bytes_of_string(const char *field, const char *s) {
    key(field);
    if (!s || !*s) { fputs("null\n", out); return; }
    fputc('[', out);
    for (int i=0;s[i];i++) fprintf(out, i?",%d":"%d", (int)(signed char)s[i]);
    fputs("]\n", out);
}
static void f_ops(const char *field, char ops[5][32], const bool valid[5]) {
    bool any=false;
    for (int i=0;i<5;i++) any|=valid[i];
    key(field);
    if (!any) { fputs("null\n", out); return; }
    fputc('[', out);
    for (int i=0;i<5;i++) { if (i) fputc(',', out); if (valid[i]) put_str(ops[i]); else fputs("null", out); }
    fputs("]\n", out);
}
static void put_pix32(const Pix32 *p) {
    if (!p) { fputs("null", out); return; }
    char hex[17]; sha_ints(p->pixels, p->width*p->height, hex);
    fprintf(out, "Pix32(%dx%d crop=%d,%d full=%dx%d px=%s)", p->width, p->height, p->crop_x, p->crop_y, p->crop_w, p->crop_h, hex);
}

static void dump_model(int id, Model *m) {
    type_name="model"; cur_id=id;
    if (!m) { fprintf(out,"model %d null\n",id); return; }
    f_ints("faceAlpha", m->face_alpha, m->num_faces);
    f_ints("faceColor", m->face_colour, m->num_faces);
    f_ints("faceColorA", m->face_colour_a, m->num_faces);
    f_ints("faceColorB", m->face_colour_b, m->num_faces);
    f_ints("faceColorC", m->face_colour_c, m->num_faces);
    f_int("faceCount", m->num_faces);
    f_ints("faceInfo", m->face_render_type, m->num_faces);
    f_ints("faceLabel", m->face_label, m->num_faces);
    f_ints("facePriority", m->face_priority, m->num_faces);
    f_ints("faceVertexA", m->face_vertex_a, m->num_faces);
    f_ints("faceVertexB", m->face_vertex_b, m->num_faces);
    f_ints("faceVertexC", m->face_vertex_c, m->num_faces);
    f_int("maxDepth", m->max_depth);
    f_int("minDepth", m->min_depth);
    f_int("objRaise", m->obj_raise);
    f_int("priority", m->priority);
    f_int("texturedFaceCount", m->num_t);
    f_ints("texturedVertexA", m->texture_a ? m->texture_a : (int[]){0}, m->num_t);
    f_ints("texturedVertexB", m->texture_b ? m->texture_b : (int[]){0}, m->num_t);
    f_ints("texturedVertexC", m->texture_c ? m->texture_c : (int[]){0}, m->num_t);
    f_int("vertexCount", m->num_points);
    f_ints("vertexLabel", m->vertex_label, m->num_points);
    f_ints("vertexX", m->point_x, m->num_points);
    f_ints("vertexY", m->point_y, m->num_points);
    f_ints("vertexZ", m->point_z, m->num_points);
}

static void dump_component(ComponentConfig *cfg, PixFont *fonts[4], int id) {
    Component *c=&cfg->instances[id];
    type_name="component"; cur_id=id;
    if (c->id!=id) { fprintf(out,"component %d null\n",id); return; }
    f_str("action", c->action);
    f_int("actionTarget", c->action_target);
    f_str("actionVerb", c->action_verb);
    f_int("activeAnim", c->active_anim_id);
    f_int("activeColour", (int32_t)c->active_colour);
    key("activeGraphic"); put_pix32(c->active_graphic); fputc('\n', out);
    // Java holds getModel(id) (never null for a listed id); compare presence.
    bool model3d=c->type==COMPONENT_TYPE_MODEL_3D;
    key("activeModel"); fputs(model3d && c->active_model_id>=0 && model_load(c->active_model_id,NULL) ? "<Model>\n":"null\n", out);
    f_str("activeText", c->active_text);
    f_int("anim", c->anim_id);
    f_int("buttonType", c->button_type);
    f_int("center", c->center);
    f_ints("childId", c->child_id, c->child_count);
    f_ints("childX", c->child_x, c->child_count);
    f_ints("childY", c->child_y, c->child_count);
    f_int("clientCode", c->client_code);
    f_int("colour", (int32_t)c->colour);
    f_int("draggable", c->draggable);
    f_int("fill", c->fill);
    key("font");
    if (!c->font) fputs("null\n", out);
    else { int f=-1; for (int i=0;i<4;i++) if (fonts[i]==c->font) f=i; fprintf(out, "font%d\n", f); }
    key("graphic"); put_pix32(c->graphic); fputc('\n', out);
    f_int("height", c->height);
    f_int("hide", c->hide);
    f_int("id", c->id);
    f_int("interactable", c->interactable);
    f_ints("invSlotObjCount", c->inv_slot_obj_count, c->width*c->height);
    f_ints("invSlotObjId", c->inv_slot_obj_id, c->width*c->height);
    if (c->type==COMPONENT_TYPE_INV_GRID) {
        f_ints("invSlotOffsetX", c->inv_slot_offset_x, 20);
        f_ints("invSlotOffsetY", c->inv_slot_offset_y, 20);
        key("invSlotSprite"); fputc('[', out);
        for (int i=0;i<20;i++) { if (i) fputc(',', out); put_pix32(c->inv_slot_sprite[i]); }
        fputs("]\n", out);
    } else {
        f_null("invSlotOffsetX"); f_null("invSlotOffsetY"); f_null("invSlotSprite");
    }
    // Component.decode allocates iops for type 2 (inventory) and type 7 (inventory text).
    if (c->type==COMPONENT_TYPE_INV_GRID || c->type==COMPONENT_TYPE_TEXT_LIST) {
        key("iops"); fputc('[', out);
        for (int i=0;i<5;i++) { if (i) fputc(',', out); if (c->iops[i]) put_str(c->iops[i]); else fputs("null", out); }
        fputs("]\n", out);
    } else {
        f_null("iops");
    }
    f_int("layer", c->layer);
    f_int("marginX", c->margin_x);
    f_int("marginY", c->margin_y);
    key("model"); fputs(model3d && c->model_id>=0 && model_load(c->model_id,NULL) ? "<Model>\n":"null\n", out);
    f_str("option", c->option);
    f_int("overColour", (int32_t)c->over_colour);
    f_int("overLayer", c->over_layer);
    f_ints("scriptComparator", c->script_comparator, c->script_comparator_count);
    f_ints("scriptOperand", c->script_operand, c->script_comparator_count);
    key("scripts");
    if (!c->scripts) fputs("null\n", out);
    else {
        fputc('[', out);
        for (int i=0;i<c->script_count;i++) {
            if (i) fputc(',', out);
            fputc('[', out);
            for (int j=0;j<c->script_lengths[i];j++) fprintf(out, j?",%d":"%d", c->scripts[i][j]);
            fputc(']', out);
        }
        fputs("]\n", out);
    }
    f_int("scroll", c->scroll);
    f_int("scrollPosition", c->scroll_position);
    f_int("shadowed", c->shadowed);
    f_str("text", c->text);
    f_int("type", c->type);
    f_int("unusedBoolean1", c->unused_bool1);
    f_int("unusedShort1", c->unused_short1);
    f_int("usable", c->usable);
    f_int("width", c->width);
    f_int("x", c->x);
    f_int("xan", c->xan);
    f_int("y", c->y);
    f_int("yan", c->yan);
    f_int("zoom", c->zoom);
}

static void dump_font(int id, PixFont *f) {
    type_name="font"; cur_id=id;
    f_ints("charAdvance", f->char_advance, 95);
    key("charMask"); fputc('[', out);
    for (int i=0;i<94;i++) {
        if (i) fputc(',', out);
        int n=f->char_mask_width[i]*f->char_mask_height[i];
        fputc('[', out);
        for (int j=0;j<n;j++) fprintf(out, j?",%d":"%d", (int)(int8_t)f->char_mask[i][j]);
        fputc(']', out);
    }
    fputs("]\n", out);
    f_ints("charMaskHeight", f->char_mask_height, 94);
    f_ints("charMaskWidth", f->char_mask_width, 94);
    f_ints("charOffsetX", f->char_offset_x, 94);
    f_ints("charOffsetY", f->char_offset_y, 94);
    f_ints("drawWidth", f->draw_width, 256);
    f_int("height", f->height);
}

static int dump_all(const char *cache, const char *out_path, int model_count) {
    char path[512];
    out=fopen(out_path,"wb");
    if (!out) return 1;
    Arena arena=arena_create(512u*1024*1024);
#define JAG(var,name) snprintf(path,sizeof(path),"%s/%s",cache,name); Jagfile *var=jagfile_load_file(path,&arena); if(!var){fprintf(stderr,"missing %s\n",path);return 1;}
    JAG(config,"config"); JAG(models,"models"); JAG(media,"media"); JAG(title,"title"); JAG(iface,"interface");

    math_random_seed(225); // DataOracle seeds Math.random() the same way
    FloConfig flo={0}; flo_unpack(&flo,config);
    fprintf(out,"flo count=%d\n",flo.count);
    for (int i=0;i<flo.count;i++) {
        FloType *f=&flo.instances[i]; type_name="flo"; cur_id=i;
        f_int("chroma",f->chroma); f_int("hsl",f->hsl); f_int("hue",f->hue); f_int("lightness",f->lightness);
        f_int("luminance",f->luminance); f_str("name",f->name[0]?f->name:NULL); f_int("occlude",f->occlude);
        f_int("overlay",f->overlay); f_int("rgb",f->rgb); f_int("saturation",f->saturation); f_int("texture",f->texture);
    }
    IdkConfig idk={0}; idk_unpack(&idk,config,&arena);
    fprintf(out,"idk count=%d\n",idk.count);
    for (int i=0;i<idk.count;i++) {
        IdkType *k=&idk.instances[i]; type_name="idk"; cur_id=i;
        f_int("disable",k->disable); f_ints("heads",k->heads,5); f_ints("models",k->models,k->model_count);
        f_ints("recol_d",k->recol_d,6); f_ints("recol_s",k->recol_s,6); f_int("type",k->type);
    }
    anim_unpack(models,&arena); // client.load order: AnimFrame before SeqType
    SeqConfig seq={0}; seq_unpack(&seq,config);
    SpotAnimConfig spot={0}; spotanim_unpack(&spot,config,&seq);
    fprintf(out,"spotanim count=%d\n",spot.count);
    for (int i=0;i<spot.count;i++) {
        SpotAnimType *s=&spot.instances[i]; type_name="spotanim"; cur_id=i;
        f_int("ambient",s->ambient); f_int("anim",s->anim); f_int("animHasAlpha",s->anim_has_alpha); f_int("contrast",s->contrast);
        f_int("model",s->model); f_int("orientation",s->orientation); f_ints("recol_d",s->recol_d,6); f_ints("recol_s",s->recol_s,6);
        f_int("resizeh",s->resizeh); f_int("resizev",s->resizev);
    }
    VarpTypeConfig varp={0}; varp_type_unpack(&varp,config);
    fprintf(out,"varp count=%d\n",varp.count);
    for (int i=0;i<varp.count;i++) {
        VarpType *v=&varp.instances[i]; type_name="varp"; cur_id=i;
        f_int("clientcode",v->clientcode); f_int("code1",v->code1); f_str("code10",v->has_code10?v->code10:NULL);
        f_int("code2",v->code2); f_int("code4",v->code4); f_int("code6",v->code6); f_int("code7",v->code7);
        f_int("code8",v->code8); f_int("hasCode3",v->has_code3);
    }
    ObjConfig obj={0}; obj_unpack(&obj,config,true);
    fprintf(out,"obj count=%d\n",obj.count);
    for (int i=0;i<obj.count;i++) {
        ObjType o; type_name="obj"; cur_id=i;
        if (!obj_get(&obj,i,&o)) { fprintf(out,"obj %d null\n",i); continue; }
        f_int("certlink",o.certlink); f_int("certtemplate",o.certtemplate); f_int("code10",o.code10); f_int("code9",o.code9);
        f_int("cost",o.cost);
        bool cnt=false; for (int k=0;k<10;k++) cnt|=o.countobj[k]!=0||o.countco[k]!=0;
        f_ints("countco",cnt?o.countco:NULL,10); f_ints("countobj",cnt?o.countobj:NULL,10);
        f_bytes_of_string("desc",o.desc); f_int("index",o.index); f_ops("iop",o.iop,o.iop_valid);
        f_int("manhead",o.manhead); f_int("manhead2",o.manhead2); f_int("manwear",o.manwear); f_int("manwear2",o.manwear2);
        f_int("manwear3",o.manwear3); f_int("manwearOffsetY",o.manwear_offset_y); f_int("members",o.members); f_int("model",o.model);
        f_str("name",o.name[0]?o.name:NULL); f_ops("op",o.op,o.op_valid);
        f_ints("recol_d",o.recol_d,o.recol_count); f_ints("recol_s",o.recol_s,o.recol_count); f_int("stackable",o.stackable);
        f_int("womanhead",o.womanhead); f_int("womanhead2",o.womanhead2); f_int("womanwear",o.womanwear); f_int("womanwear2",o.womanwear2);
        f_int("womanwear3",o.womanwear3); f_int("womanwearOffsetY",o.womanwear_offset_y); f_int("xan2d",o.xan2d); f_int("xof2d",o.xof2d);
        f_int("yan2d",o.yan2d); f_int("yof2d",o.yof2d); f_int("zan2d",o.zan2d); f_int("zoom2d",o.zoom2d);
    }
    LocConfig loc={0}; loc_unpack(&loc,config);
    fprintf(out,"loc count=%d\n",loc.count);
    for (int i=0;i<loc.count;i++) {
        LocType l; type_name="loc"; cur_id=i;
        if (!loc_get(&loc,i,&l)) { fprintf(out,"loc %d null\n",i); continue; }
        f_int("active",l.active); f_int("ambient",l.ambient); f_int("anim",l.anim); f_int("animHasAlpha",l.anim_has_alpha);
        f_int("blockrange",l.blockrange); f_int("blockwalk",l.blockwalk); f_int("contrast",l.contrast); f_bytes_of_string("desc",l.desc);
        f_int("forceapproach",l.forceapproach); f_int("forcedecor",l.forcedecor); f_int("hillskew",l.hillskew); f_int("index",l.index);
        f_int("length",l.length); f_int("mapfunction",l.mapfunction); f_int("mapscene",l.mapscene); f_int("mirror",l.mirror);
        f_ints("models",l.models,l.model_count); f_str("name",l.name[0]?l.name:NULL); f_int("occlude",l.occlude);
        f_int("offsetx",l.offsetx); f_int("offsety",l.offsety); f_int("offsetz",l.offsetz); f_ops("op",l.op,l.op_valid);
        f_ints("recol_d",l.recol_d,l.recol_count); f_ints("recol_s",l.recol_s,l.recol_count);
        f_int("resizex",l.resizex); f_int("resizey",l.resizey); f_int("resizez",l.resizez); f_int("shadow",l.shadow);
        f_ints("shapes",l.shapes,l.model_count); f_int("sharelight",l.sharelight); f_int("wallwidth",l.wallwidth); f_int("width",l.width);
    }
    NpcConfig npc={0}; npc_unpack(&npc,config);
    fprintf(out,"npc count=%d\n",npc.count);
    for (int i=0;i<npc.count;i++) {
        NpcType n; type_name="npc"; cur_id=i;
        if (!npc_get(&npc,i,&n)) { fprintf(out,"npc %d null\n",i); continue; }
        f_int("animHasAlpha",n.anim_has_alpha); f_bytes_of_string("desc",n.desc); f_ints("heads",n.heads,n.head_count);
        f_int("index",n.index); f_int("minimap",n.minimap); f_ints("models",n.models,n.model_count); f_str("name",n.name[0]?n.name:NULL);
        f_ops("op",n.op,n.op_valid); f_int("readyanim",n.readyanim); f_ints("recol_d",n.recol_d,n.recol_count); f_ints("recol_s",n.recol_s,n.recol_count);
        f_int("resizeh",n.resizeh); f_int("resizev",n.resizev); f_int("resizex",n.resizex); f_int("resizey",n.resizey); f_int("resizez",n.resizez);
        f_int("size",n.size); f_int("vislevel",n.vislevel); f_int("walkanim",n.walkanim); f_int("walkanim_b",n.walkanim_b);
        f_int("walkanim_l",n.walkanim_l); f_int("walkanim_r",n.walkanim_r);
    }

    fprintf(out,"animbase count=%d\n",g_anim_cfg.base_count);
    for (int i=0;i<g_anim_cfg.base_count;i++) {
        AnimBase *b=&g_anim_cfg.bases[i]; type_name="animbase"; cur_id=i;
        if (!b->types) { fprintf(out,"animbase %d null\n",i); continue; }
        key("labels"); fputc('[', out);
        for (int j=0;j<b->length;j++) {
            if (j) fputc(',', out);
            fputc('[', out);
            for (int k=0;k<b->label_counts[j];k++) fprintf(out, k?",%d":"%d", b->labels[j][k]);
            fputc(']', out);
        }
        fputs("]\n", out);
        f_int("length",b->length); f_ints("types",b->types,b->length);
    }
    fprintf(out,"animframe count=%d\n",g_anim_cfg.frame_count);
    for (int i=0;i<g_anim_cfg.frame_count;i++) {
        AnimFrame *f=&g_anim_cfg.frames[i]; type_name="animframe"; cur_id=i;
        if (!f->base) { fprintf(out,"animframe %d null\n",i); continue; }
        key("base"); fprintf(out,"base%d\n",(int)(f->base-g_anim_cfg.bases));
        f_int("delay",f->delay); f_ints("groups",f->groups,f->length); f_int("length",f->length);
        f_ints("x",f->x,f->length); f_ints("y",f->y,f->length); f_ints("z",f->z,f->length);
    }
    fprintf(out,"seq count=%d\n",seq.count);
    for (int i=0;i<seq.count;i++) {
        SeqType *s=&seq.instances[i]; type_name="seq"; cur_id=i;
        f_ints("delay",s->delay,s->frame_count); f_int("frameCount",s->frame_count); f_ints("frames",s->frames,s->frame_count);
        f_ints("iframes",s->iframes,s->frame_count); f_int("lefthand",s->lefthand); f_int("priority",s->priority);
        f_int("replaycount",s->replaycount); f_int("replayoff",s->replayoff); f_int("righthand",s->righthand);
        f_int("stretches",s->stretches); f_ints("walkmerge",s->walkmerge,s->walkmerge_count+1);
    }

    model_init_repo(models,&arena);
    fprintf(out,"model count=%d\n",model_count);
    for (int i=0;i<model_count;i++) dump_model(i,model_load(i,&arena));

    static PixFont p11,p12,b12,q8;
    pixfont_init_lookup();
    pixfont_load(&p11,title,"p11",&arena); pixfont_load(&p12,title,"p12",&arena);
    pixfont_load(&b12,title,"b12",&arena); pixfont_load(&q8,title,"q8",&arena);
    PixFont *fonts[4]={&p11,&p12,&b12,&q8};
    for (int i=0;i<4;i++) dump_font(i,fonts[i]);
    ComponentConfig comp={0}; component_unpack(&comp,media,fonts,iface,&arena);
    fprintf(out,"component count=%d\n",comp.count);
    for (int i=0;i<comp.count;i++) dump_component(&comp,fonts,i);
    fclose(out);
    return 0;
}

/* --check: digest every dumped object exactly as tools/data_reference_digest.py
   digests the DataOracle dump (an object's "field=value" lines sorted by field
   name, each ending in '\n', or "null\n" for a null object; SHA-256, first 16
   hex) and compare with the hashed fixture. */
typedef struct { char *line; size_t name_len; } FieldLine;
static int cmp_field(const void *a, const void *b) {
    const FieldLine *x=a, *y=b;
    size_t n=x->name_len<y->name_len?x->name_len:y->name_len;
    int c=memcmp(x->line,y->line,n);
    if (c) return c;
    return x->name_len<y->name_len ? -1 : x->name_len>y->name_len;
}
static char *read_line(FILE *f) {
    size_t cap=256,len=0; char *b=malloc(cap); int ch;
    while ((ch=fgetc(f))!=EOF && ch!='\n') { if (len+1>=cap) b=realloc(b,cap*=2); b[len++]=(char)ch; }
    if (ch==EOF && len==0) { free(b); return NULL; }
    b[len]=0;
    return b;
}
typedef struct { char type[16]; int id; char hex[17]; } Digest;
static Digest *digests;
static size_t digest_count, digest_cap;
static void add_digest(const char *type, int id, const char *hex) {
    if (digest_count==digest_cap) digests=realloc(digests,(digest_cap=digest_cap?digest_cap*2:4096)*sizeof(Digest));
    Digest *d=&digests[digest_count++];
    snprintf(d->type,sizeof(d->type),"%s",type); d->id=id; snprintf(d->hex,sizeof(d->hex),"%s",hex);
}
static void finish_object(const char *type, int id, FieldLine *fl, size_t n, bool null_obj) {
    Sha s; sha_init(&s); char hex[17];
    if (null_obj) { for (const char *p="null\n";*p;p++) sha_byte(&s,(uint8_t)*p); }
    else {
        qsort(fl,n,sizeof(*fl),cmp_field);
        for (size_t i=0;i<n;i++) { for (const char *p=fl[i].line;*p;p++) sha_byte(&s,(uint8_t)*p); sha_byte(&s,'\n'); }
    }
    sha_hex(&s,hex);
    add_digest(type,id,hex);
}
// Counts are stored as id -1 with the decimal count in place of a digest.
static int digest_dump(const char *path) {
    FILE *f=fopen(path,"rb");
    if (!f) return 1;
    char type[16]="", cur_type[16]=""; int cur=-1; bool cur_null=false;
    FieldLine *fl=NULL; size_t n=0, cap=0; char *line;
    while ((line=read_line(f))) {
        char *sp=strchr(line,' ');
        if (!sp) { free(line); continue; }
        size_t tl=(size_t)(sp-line); if (tl>=sizeof(type)) tl=sizeof(type)-1;
        memcpy(type,line,tl); type[tl]=0;
        char *rest=sp+1;
        if (!strncmp(rest,"count=",6)) { add_digest(type,-1,rest+6); free(line); continue; }
        int id=atoi(rest);
        char *kv=strchr(rest,' '); kv=kv?kv+1:rest+strlen(rest);
        if (cur>=0 && (id!=cur || strcmp(type,cur_type))) {
            finish_object(cur_type,cur,fl,n,cur_null);
            for (size_t i=0;i<n;i++) free(fl[i].line);
            n=0; cur_null=false;
        }
        cur=id; memcpy(cur_type,type,sizeof(type));
        if (!strcmp(kv,"null")) { cur_null=true; free(line); continue; }
        if (n==cap) fl=realloc(fl,(cap=cap?cap*2:64)*sizeof(*fl));
        char *eq=strchr(kv,'=');
        fl[n].name_len=eq?(size_t)(eq-kv):strlen(kv);
        fl[n].line=malloc(strlen(kv)+1); strcpy(fl[n].line,kv); n++;
        free(line);
    }
    if (cur>=0) { finish_object(cur_type,cur,fl,n,cur_null); for (size_t i=0;i<n;i++) free(fl[i].line); }
    free(fl); fclose(f);
    return 0;
}

static int check(const char *cache, const char *fixture) {
    FILE *fx=fopen(fixture,"rb");
    if (!fx) { printf("FAIL DATA-REF: fixture %s missing (regenerate with verification/completion/tools/data_reference_digest.py)\n",fixture); return 1; }
    int model_count=-1; char *line;
    Digest *want=NULL; size_t wn=0, wc=0;
    while ((line=read_line(fx))) {
        Digest w; memset(&w,0,sizeof(w));
        if (sscanf(line,"%15s %d %16s",w.type,&w.id,w.hex)==3) {
            if (w.id==-1 && !strcmp(w.type,"model")) model_count=atoi(w.hex);
            if (wn==wc) want=realloc(want,(wc=wc?wc*2:4096)*sizeof(Digest));
            want[wn++]=w;
        }
        free(line);
    }
    fclose(fx);
    if (model_count<0) { printf("FAIL DATA-REF: fixture has no model count\n"); return 1; }
    const char *tmp="build/data-reference-native.txt";
    if (dump_all(cache,tmp,model_count)) { printf("FAIL DATA-REF: native dump failed\n"); return 1; }
    if (digest_dump(tmp)) { printf("FAIL DATA-REF: cannot read %s\n",tmp); return 1; }
    size_t bad=0, shown=0, j=0;
    for (size_t i=0;i<wn;i++) {
        const Digest *got=NULL;
        // Both lists are in dump order; scan forward from the last match.
        for (size_t k=0;k<digest_count && !got;k++) {
            const Digest *d=&digests[(j+k)%digest_count];
            if (d->id==want[i].id && !strcmp(d->type,want[i].type)) { got=d; j=(size_t)(d-digests)+1; }
        }
        if (!got || strcmp(got->hex,want[i].hex)) {
            bad++;
            if (shown++<20) printf("  mismatch %s %d: java=%s native=%s\n",want[i].type,want[i].id,want[i].hex,got?got->hex:"missing");
        }
    }
    if (digest_count!=wn) { printf("  entries: java=%zu native=%zu\n",wn,digest_count); bad++; }
    if (bad) { printf("FAIL DATA-REF: %zu entries differ from DataOracle; run tools/compare_data_reference.py for fields\n",bad); return 1; }
    printf("PASS DATA-REF: %zu decoded entries (configs, varps, models, anims, fonts, interfaces, sprites) equal DataOracle\n",wn);
    return 0;
}

int main(int argc, char **argv) {
    setbuf(stdout,NULL);
    if (argc==4 && !strcmp(argv[1],"--check")) return check(argv[2],argv[3]);
    if (argc==4) { int r=dump_all(argv[1],argv[2],atoi(argv[3])); if (!r) puts("native data dump written"); return r; }
    fprintf(stderr,"usage: %s <cache dir> <out.txt> <model count>\n       %s --check <cache dir> <digest fixture>\n",argv[0],argv[0]);
    return 2;
}
