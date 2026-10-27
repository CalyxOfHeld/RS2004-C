#ifndef CONTINUE_REFERENCE_CONTROL_H
#define CONTINUE_REFERENCE_CONTROL_H
#include "ui/client_actions.h"
#include "ui/component_packets.h"
#include "gfx/pix2d.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int ct_record;
static void ct_check(bool ok,const char *what){if(!ok){fprintf(stderr,"continue record%d: %s\n",ct_record,what);exit(3);}}
static int ct_int(FILE *f){uint8_t b[4];ct_check(fread(b,1,4,f)==4,"fixture integer");return (int32_t)((uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3]);}
static bool ct_bool(FILE *f){int n=fgetc(f);ct_check(n==0 || n==1,"fixture boolean");return n!=0;}
static void ct_eq(FILE *f,int actual,const char *name){int expected=ct_int(f);if(expected!=actual){fprintf(stderr,"continue record%d %s expected%d actual%d\n",ct_record,name,expected,actual);exit(3);}}
static void ct_cipher(FILE *f,Isaac *c){ct_eq(f,c->count,"ISAAC count");ct_eq(f,(int32_t)c->a,"ISAAC a");ct_eq(f,(int32_t)c->b,"ISAAC b");ct_eq(f,(int32_t)c->c,"ISAAC c");for(int i=0;i<256;i++)ct_eq(f,(int32_t)c->rsl[i],"ISAAC rsl");for(int i=0;i<256;i++)ct_eq(f,(int32_t)c->mem[i],"ISAAC mem");}
typedef struct {void (*action)(int,int);void (*packet)(int,const uint8_t*,size_t);void (*wire)(GameSession*,int);void (*picture)(Component*,const uint32_t*,int);} ContinueHooks;
static void continue_reference_run(const char *path,const char *ui,ComponentConfig *cfg,GameSession *session,InterfaceState *state,Chatbox *chat,LastLoginInfo225 *login,MenuSelection *selected,const ContinueHooks *hooks){
 FILE *f=fopen(path,"rb");ct_check(f && ct_int(f)==0x43543235,"original fixture");int count=ct_int(f);ct_check(count==1134,"case count");uint32_t seed[4];for(int i=0;i<4;i++)seed[i]=(uint32_t)ct_int(f);
 for(ct_record=0;ct_record<count;ct_record++){
  int op=ct_int(f);cfg->pressed_continue=ct_bool(f);state->viewport=ct_int(f);state->sidebar=ct_int(f);state->chat=ct_int(f);int id=ct_int(f),clicks=ct_int(f);
  selected->item_selected=selected->spell_selected=true;isaac_init(&session->out_cipher,seed,4);session->stream.out_len=0;session->stream.out_overflow=false;session->in_game=true;
  ClientActions actions={.selection=selected,.components=cfg,.session=session,.viewport_interface=&state->viewport,.sidebar_interface=&state->sidebar,.chat_interface=&state->chat,.chatbox=chat,.live=true};
  if(op==0 || op==1){if(hooks)hooks->action(op,id);else if(op==1)client_action_dispatch(&actions,(MenuAction){.type=MENU_IF_CLOSE});else client_action_dispatch(&actions,(MenuAction){.type=MENU_IF_CONTINUE,.component=id});
   if(op==0)for(int j=1;j<clicks;j++){if(hooks)hooks->action(op,(id+1)&65535);else client_action_dispatch(&actions,(MenuAction){.type=MENU_IF_CONTINUE,.component=(id+1)&65535});}
  }else {
   uint8_t payload[9]={0};size_t n=op==129?0:op==28?4:op==140?9:2;
   if(op==140){payload[0]=id?192:0;payload[2]=2;payload[3]=1;payload[5]=1;payload[6]=201;payload[8]=1;}
   if(hooks)hooks->packet(op,payload,n);
   else if(op==140)ct_check(last_login_receive225(login,state,chat,cfg,session,payload,n)>0,"native read140");
   else if(op!=185){ct_check(interface_state_apply(state,op,payload,n)==1,"native interface packet");cfg->pressed_continue=false;}
  }
  ct_check(cfg->pressed_continue==ct_bool(f),"pressedContinueOption");ct_check(selected->item_selected==ct_bool(f) && selected->spell_selected==ct_bool(f),"item/spell selection");ct_eq(f,state->viewport,"viewport");ct_eq(f,state->sidebar,"sidebar");ct_eq(f,state->chat,"chat");
  int n=ct_int(f);ct_check(n>=0 && n<100,"expected wire length");uint8_t bytes[100];ct_check(fread(bytes,1,(size_t)n,f)==(size_t)n,"expected wire");if(hooks)hooks->wire(session,n);ct_check(session->stream.out_len==(size_t)n && !memcmp(bytes,session->stream.out_buf,(size_t)n),"full outgoing wire");ct_cipher(f,&session->out_cipher);
  Component saved=cfg->instances[0],child={.id=0,.type=4,.button_type=6,.width=180,.height=30,.over_layer=-1,.option="Click here to continue"};cfg->instances[0]=child;int child_id=0,x=12,y=10;Component root={.type=0,.width=512,.height=334,.child_count=1,.child_id=&child_id,.child_x=&x,.child_y=&y};ContextMenu menu={0};component_build_menu(cfg,&root,20,20,0,0,0,&menu,NULL);ct_eq(f,menu.size,"menu suppression");if(menu.size){ct_eq(f,44,"source action44");ct_eq(f,menu.actions[0].component,"menu component");ct_check(menu.actions[0].type==MENU_IF_CONTINUE,"native menu action");}cfg->instances[0]=saved;
 }
 ct_check(fgetc(f)==EOF,"fixture EOF");fclose(f);
 f=fopen(ui,"rb");ct_check(f && ct_int(f)==0x43553235 && ct_int(f)==48,"original UI fixture");
 selected->item_selected=selected->spell_selected=false; /* Isolate drawInterface from whole-client selection tooltips. */
 Component saved=cfg->instances[0];PixFont *fonts[3]={NULL};
 /* Fonts are the actual cached p11,p12,b12 objects, located by their IDs. */
 for(int i=0;i<cfg->count;i++){Component *c=&cfg->instances[i];if(c->type==4 && c->font_id>=0 && c->font_id<3 && c->font)fonts[c->font_id]=c->font;}
 for(int i=0;i<3;i++)ct_check(fonts[i]!=NULL,"cache font");
 uint32_t *expected=malloc(512*334*sizeof(*expected)),*actual=calloc(512*334,sizeof(*actual));ct_check(expected && actual,"UI buffers");
 for(int i=0;i<48;i++){
  ct_eq(f,i,"UI index");int script[]={11,0},*scripts[]={script},length=2,comparator=0,operand=1;
  cfg->pressed_continue=(i&1)!=0;Component child={.id=0,.type=4,.button_type=i<40?6:1,.width=180,.height=30,.over_layer=-1,.text="Click here to continue",.active_text="Active continue",.font=fonts[(i/2)%3],.center=(i&8)!=0,.shadowed=(i&16)!=0,.colour=0xffff00,.active_colour=0xff0000,.script_comparator_count=1,.script_comparator=&comparator,.script_operand=&operand,.script_count=1,.scripts=scripts,.script_lengths=&length};cfg->instances[0]=child;
  int id=0,x=12,y=10;Component root={.type=0,.width=512,.height=334,.child_count=1,.child_id=&id,.child_x=&x,.child_y=&y};VarpContext varps={0};varps.energy=(i&4)!=0?1:0;
  for(int j=0;j<512*334;j++)expected[j]=(uint32_t)ct_int(f);
  memset(actual,0,512*334*sizeof(*actual));pix2d_bind(actual,512,334);component_draw_with_varps(cfg,&root,0,0,0,&varps);
  for(int j=0;j<512*334;j++)if(actual[j]!=expected[j]){fprintf(stderr,"continue UI%d pixel%d expected%x actual%x\n",i,j,expected[j],actual[j]);exit(3);}
  if(hooks)hooks->picture(&root,expected,i);
 }
 cfg->instances[0]=saved;cfg->pressed_continue=false;ct_check(fgetc(f)==EOF,"UI EOF");fclose(f);free(expected);free(actual);
 printf("PASS original continue:1134 action/read/close states,full wire/ISAAC/menu;48 complete512x334 panels/8208384 pixels;global waiting/suppression/source-specific close quirks\n");
}
static void continue_fault_controls(ComponentConfig *cfg){
 GameSession *session=calloc(1,sizeof(*session));ct_check(session!=NULL,"fault session");uint32_t seed[]={225,235,129,44};session->in_game=true;session->stream.buffered=true;isaac_init(&session->out_cipher,seed,4);
 int viewport=42,sidebar=-1,chat=-1;ClientActions actions={.components=cfg,.session=session,.viewport_interface=&viewport,.sidebar_interface=&sidebar,.chat_interface=&chat,.live=true};Isaac before=session->out_cipher;
 for(int mode=0;mode<3;mode++){cfg->pressed_continue=false;session->stream.out_overflow=mode==1;session->stream.buffered=mode!=2;session->stream.connected=false;session->stream.sock=INVALID_SOCKET_VAL;session->stream.out_len=mode==0?4998:0;
  ct_check(client_action_dispatch(&actions,(MenuAction){.type=MENU_IF_CONTINUE,.component=0})==ACTION_FAILED && !cfg->pressed_continue && !memcmp(&before,&session->out_cipher,sizeof(before)),"failed resume retains waiting/cipher");
 }
 session->stream.buffered=true;session->stream.out_overflow=false;session->stream.out_len=4997;ct_check(client_action_dispatch(&actions,(MenuAction){.type=MENU_IF_CONTINUE,.component=65535})==ACTION_SENT && cfg->pressed_continue && session->stream.out_len==5000,"exact resume capacity");
 session->stream.out_len=0;before=session->out_cipher;ct_check(client_action_dispatch(&actions,(MenuAction){.type=MENU_IF_CONTINUE,.component=42})==ACTION_LOCAL && !session->stream.out_len && !memcmp(&before,&session->out_cipher,sizeof(before)),"duplicate has no wire/cipher effect");
 for(int bad=-1;bad<=65536;bad+=65537){ct_check(!protocol_send_resume_pausebutton(session,bad) && !memcmp(&before,&session->out_cipher,sizeof(before)),"invalid ID guard");}
 session->stream.out_len=5000;sidebar=1;ct_check(client_action_dispatch(&actions,(MenuAction){.type=MENU_IF_CLOSE})==ACTION_FAILED && cfg->pressed_continue && sidebar==1 && viewport==42 && !memcmp(&before,&session->out_cipher,sizeof(before)),"failed close retains state/cipher");
 session->stream.out_overflow=false;session->stream.out_len=4999;ct_check(client_action_dispatch(&actions,(MenuAction){.type=MENU_IF_CLOSE})==ACTION_SENT && !cfg->pressed_continue && sidebar==-1 && viewport==-1 && session->stream.out_len==5000,"exact close recovery");
 cfg->pressed_continue=false;free(session);puts("PASS continue guards:resume capacity/sticky/disconnect/cipher retention,exact recovery,duplicate,ID bounds and close-state retention");
}
static void continue_registered_reference(ComponentConfig *cfg){GameSession *session=calloc(1,sizeof(*session));ct_check(session!=NULL,"session");session->stream.buffered=true;InterfaceState state;Chatbox chat={0};LastLoginInfo225 login={0};MenuSelection selected={0};const char *path=getenv("RS2_CONTINUE_ORACLE"),*ui=getenv("RS2_CONTINUE_UI_ORACLE");continue_reference_run(path?path:"tests/fixtures/continue-reference.bin",ui?ui:"tests/fixtures/continue-ui.bin",cfg,session,&state,&chat,&login,&selected,NULL);free(session);continue_fault_controls(cfg);}
#endif
