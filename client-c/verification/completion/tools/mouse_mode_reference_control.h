#ifndef MOUSE_MODE_REFERENCE_CONTROL_H
#define MOUSE_MODE_REFERENCE_CONTROL_H
#include "ui/client_actions.h"
#include "ui/component_packets.h"
#include "config/varp_type.h"
#include "gfx/pix2d.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
static int mm_record;
static void mm_check(bool yes,const char *label){if(!yes){fprintf(stderr,"mouse mode record%d: %s\n",mm_record,label);exit(3);}}
static int mm_int(FILE *f){uint8_t b[4];mm_check(fread(b,1,4,f)==4,"fixture integer");return (int32_t)((uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3]);}
static bool mm_bool(FILE *f){int n=fgetc(f);mm_check(n==0 || n==1,"fixture boolean");return n!=0;}
static void mm_eq(FILE *f,int value,const char *label){int expected=mm_int(f);if(expected!=value){fprintf(stderr,"mouse mode record%d %s expected%d actual%d\n",mm_record,label,expected,value);exit(3);}}
static void mm_cipher(FILE *f,Isaac *a){mm_eq(f,a->count,"cipher count");mm_eq(f,(int32_t)a->a,"cipher a");mm_eq(f,(int32_t)a->b,"cipher b");mm_eq(f,(int32_t)a->c,"cipher c");for(int i=0;i<256;i++)mm_eq(f,(int32_t)a->rsl[i],"cipher rsl");for(int i=0;i<256;i++)mm_eq(f,(int32_t)a->mem[i],"cipher mem");}
typedef struct {void (*setting)(int,int,int);void (*input)(const int*);void (*wire)(GameSession*,int);} MouseModeHooks;
static void mm_menu(ContextMenu *menu,int count,int action){context_menu_init(menu);for(int i=0;i<count;i++){MenuAction a={.type=MENU_CANCEL};if(i==count-1){if(action==44 || action==2044)a=(MenuAction){.type=MENU_IF_CONTINUE,.component=0};else a=(MenuAction){.type=MENU_SOCIAL,.operation=action==436?4:1};}context_menu_add_action(menu,i==count-1?"Option @yel@Test":"Cancel",a);}}
static void mm_native_setting(int op,int id,int value,ComponentConfig *cfg,VarpContext *v,GameSession *s,Arena *arena){
 if(op==0){v->varps[id]=value;varp_update225(v,id);return;}
 if(op==6){Component *b=component_get(cfg,value);mm_check(b!=NULL,"cached setting button");ClientActions a={.components=cfg,.varps=v,.session=s,.live=true};mm_check(client_action_dispatch(&a,(MenuAction){.type=MENU_IF_BUTTON,.component=value,.operation=b->button_type})==ACTION_SENT,"cached setting dispatch");return;}
 if(op==4 || op==5){int script[]={5,id,0},*scripts[]={script},length=3;Component b={.id=0,.type=4,.button_type=op,.script_count=1,.scripts=scripts,.script_lengths=&length,.script_comparator_count=1,.script_operand=&value};Component saved=cfg->instances[0];cfg->instances[0]=b;ClientActions a={.components=cfg,.varps=v,.session=s,.live=true};mm_check(client_action_dispatch(&a,(MenuAction){.type=MENU_IF_BUTTON,.component=0,.operation=op})==ACTION_SENT,"setting button");cfg->instances[0]=saved;return;}
 uint8_t data[6]={(uint8_t)(id>>8),(uint8_t)id};size_t n=op==193?0:op==150?3:6;if(op==193)v->server_varps[id]=value;else if(op==150)data[2]=(uint8_t)value;else for(int i=0;i<4;i++)data[i+2]=(uint8_t)((uint32_t)value>>(24-8*i));mm_check(component_apply_packet(cfg,v,arena,op,data,n)==1,"native varp packet");
}
static void mouse_mode_reference_run(const char *path,ComponentConfig *cfg,VarpContext *v,GameSession *s,ContextMenu *menu,PixFont *font,const MouseModeHooks *hooks){
 FILE *f=fopen(path,"rb");mm_check(f && mm_int(f)==0x4d4d3235,"source fixture");int settings=mm_int(f),inputs=mm_int(f),releases=mm_int(f),id=mm_int(f),neutral=mm_int(f);mm_check(settings==1744 && inputs==5120 && releases==320,"source counts");mm_check(varp_type_clientcode(v->types,id)==5 && varp_type_clientcode(v->types,neutral)==0,"actual cache metadata");uint32_t seed[4];for(int i=0;i<4;i++)seed[i]=(uint32_t)mm_int(f);Arena arena=arena_create(1024);
 for(mm_record=0;mm_record<settings;mm_record++){int op=mm_int(f),target=mm_int(f),prior=mm_int(f),option=mm_int(f),value=mm_int(f);memset(v->varps,0,sizeof(v->varps));memset(v->server_varps,0,sizeof(v->server_varps));v->varps[target]=prior;v->server_varps[target]=77;v->mouse_buttons_option=option;isaac_init(&s->out_cipher,seed,4);s->in_game=true;s->stream.out_len=0;s->stream.out_overflow=false;
  if(hooks)hooks->setting(op,target,value);else mm_native_setting(op,target,value,cfg,v,s,&arena);
  mm_eq(f,v->varps[target],"local varp");mm_eq(f,v->server_varps[target],"server cache");mm_eq(f,v->mouse_buttons_option,"mouse option");int n=mm_int(f);uint8_t bytes[32];mm_check(n>=0 && n<32 && fread(bytes,1,(size_t)n,f)==(size_t)n,"expected setting wire");if(hooks)hooks->wire(s,n);mm_check(s->stream.out_len==(size_t)n && !memcmp(bytes,s->stream.out_buf,(size_t)n),"full setting wire");mm_cipher(f,&s->out_cipher);
 }
 for(int row=0;row<inputs;row++,mm_record++){int a[9];for(int i=0;i<9;i++)a[i]=mm_int(f);int release=a[0],option=a[1],count=a[2],code=a[3],button=a[4],x=a[5],y=a[6],spell=a[7],drag=a[8];mm_menu(menu,count,code);v->mouse_buttons_option=option;cfg->pressed_continue=false;cfg->drag.active=drag!=0;cfg->drag.threshold=false;cfg->drag.cycles=0;cfg->drag.grab_x=x;cfg->drag.grab_y=y;isaac_init(&s->out_cipher,seed,4);s->stream.out_len=0;s->stream.out_overflow=false;
  if(hooks)hooks->input(a);else {
   bool allowed=release || (!drag && !(spell && x>=520 && x<=788 && y>=165 && y<=230));
   if(allowed){if((release || button==1) && context_menu_primary_opens225(menu,option))context_menu_open225(menu,x,y,font);else if(button==2 && count>0)context_menu_open225(menu,x,y,font);else if(count>0){if(code==44 || code==2044){mm_check(protocol_send_resume_pausebutton(s,0),"native default continue");cfg->pressed_continue=true;}}}
   if(release)cfg->drag.active=false;
  }
  bool visible=mm_bool(f);mm_check(menu->visible==visible,"menu visibility");if(visible){int area=mm_int(f),ox=area==0?8:area==1?562:22,oy=area==0?11:area==1?231:375;mm_eq(f,menu->x-ox,"menu x");mm_eq(f,menu->y-oy,"menu y");mm_eq(f,menu->width,"menu width");mm_eq(f,menu->height,"menu height");}
  mm_check(cfg->pressed_continue==mm_bool(f),"default action waiting");mm_eq(f,cfg->drag.active?1:0,"drag defer/release");int n=mm_int(f);uint8_t bytes[32];mm_check(n>=0 && n<32 && fread(bytes,1,(size_t)n,f)==(size_t)n,"expected input wire");if(hooks)hooks->wire(s,n);mm_check(s->stream.out_len==(size_t)n && !memcmp(bytes,s->stream.out_buf,(size_t)n),"full input wire");mm_cipher(f,&s->out_cipher);
 }
 mm_check(fgetc(f)==EOF,"fixture EOF");fclose(f);arena_free(&arena);cfg->drag.active=false;cfg->pressed_continue=false;puts("PASS original mouse mode:1744 setting states,5120 input states/320 original release slices;cache metadata,local/server/option state,menu visibility/rectangles,waiting/drag,full wire/ISAAC");
}
static void mouse_mode_faults(ComponentConfig *cfg,VarpContext *v){
 ContextMenu m={0};mm_check(!context_menu_primary_opens225(NULL,1) && !context_menu_primary_opens225(&m,1),"null/empty menu");m.size=MAX_MENU_OPTIONS+1;mm_check(!context_menu_primary_opens225(&m,1),"menu bound");
 GameSession *s=calloc(1,sizeof(*s));mm_check(s!=NULL,"fault session");s->in_game=true;s->stream.buffered=true;uint32_t seed[]={225,5,406,235};isaac_init(&s->out_cipher,seed,4);Isaac before=s->out_cipher;s->stream.out_len=4998;
 int id=170,value=1,script[]={5,id,0},*scripts[]={script},length=3;Component saved=cfg->instances[0];cfg->instances[0]=(Component){.id=0,.type=4,.button_type=5,.script_count=1,.scripts=scripts,.script_lengths=&length,.script_comparator_count=1,.script_operand=&value};ClientActions a={.components=cfg,.varps=v,.session=s,.live=true};v->varps[id]=0;v->mouse_buttons_option=77;
 mm_check(client_action_dispatch(&a,(MenuAction){.type=MENU_IF_BUTTON,.component=0,.operation=5})==ACTION_FAILED && v->varps[id]==0 && v->mouse_buttons_option==77 && !memcmp(&before,&s->out_cipher,sizeof(before)),"failed button preserves option/value/cipher");s->stream.out_overflow=false;s->stream.out_len=4997;mm_check(client_action_dispatch(&a,(MenuAction){.type=MENU_IF_BUTTON,.component=0,.operation=5})==ACTION_SENT && v->varps[id]==1 && v->mouse_buttons_option==1 && s->stream.out_len==5000,"exact button recovery");
 cfg->instances[0]=saved;free(s);puts("PASS mouse mode guards:menu bounds,null inputs,button output failure andexact recovery retain varp/option/ISAAC");
}
static void mouse_mode_registered(PixFont *font){
 Arena arena=arena_create(48*1024*1024);Jagfile *config=jagfile_load_file("../engine/data/pack/client/config",&arena);mm_check(config!=NULL,"cache config");VarpTypeConfig types={0};mm_check(varp_type_unpack(&types,config),"varp metadata");VarpContext v={0};v.types=&types;Jagfile *media=jagfile_load_file("../engine/data/pack/client/media",&arena),*interfaces=jagfile_load_file("../engine/data/pack/client/interface",&arena);PixFont *fonts[4]={font,font,font,font};ComponentConfig cfg={0};mm_check(media && interfaces && component_unpack(&cfg,media,fonts,interfaces,&arena),"real cached settings interfaces");GameSession *s=calloc(1,sizeof(*s));ContextMenu *menu=calloc(1,sizeof(*menu));mm_check(s && menu,"controls allocation");s->stream.buffered=true;const char *path=getenv("RS2_MOUSE_MODE_ORACLE");mouse_mode_reference_run(path?path:"tests/fixtures/mouse-mode-reference.bin",&cfg,&v,s,menu,font,NULL);mouse_mode_faults(&cfg,&v);free(menu);free(s);varp_type_free(&types);arena_free(&arena);
}
#endif
