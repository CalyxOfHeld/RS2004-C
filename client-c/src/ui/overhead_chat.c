/* Original client.draw2DEntityElements speech collection/overlap/colour/styles. */
#include "ui/overhead_chat.h"
#include "gfx/pix2d.h"
#include <string.h>
static uint32_t chat_colour(int code,int cycle,int timer) {
    static const uint32_t colours[]={16776960,16711680,65280,65535,16711935,16777215};
    if(code<6)return colours[code];
    if(code==6)return cycle%20<10?16711680:16776960;
    if(code==7)return cycle%20<10?255:65535;
    if(code==8)return cycle%20<10?45056:8454016;
    int elapsed=150-timer;
    if(code==9){if(elapsed<50)return elapsed*1280+16711680;if(elapsed<100)return 16776960-(elapsed-50)*327680;if(elapsed<150)return (elapsed-100)*5+65280;}
    if(code==10){if(elapsed<50)return elapsed*5+16711680;if(elapsed<100)return 16711935-(elapsed-50)*327680;if(elapsed<150)return (elapsed-100)*327680+255-(elapsed-100)*5;}
    if(code==11){if(elapsed<50)return 16777215-elapsed*327685;if(elapsed<100)return (elapsed-50)*327685+65280;if(elapsed<150)return 16777215-(elapsed-100)*327680;}
    return 16776960;
}
bool overhead_chat_draw(OverheadChat *list,const EntitySyncContext *ctx,const Chatbox *chat,PixFont *font,
    bool (*project)(void*,const PathingEntity*,int*,int*),void *user,int cycle,int effects,int ox,int oy) {
    if(!list || !ctx || !chat || !font || !project || ctx->player_count<0 || ctx->player_count>MAX_PLAYERS || ctx->npc_count<0 || ctx->npc_count>MAX_NPCS || chat->friend_count<0 || chat->friend_count>100)return false;
    list->count=0;
    for(int index=-1;index<ctx->player_count+ctx->npc_count;index++) {
        const PathingEntity *actor=NULL;const char *name=NULL;bool is_player=index<ctx->player_count,visible=false;
        if(is_player) {
            const PlayerEntity *p=NULL;
            if(index<0)p=&ctx->local_player;
            else {int id=ctx->player_indices[index];if(id<0 || id>=MAX_PLAYERS)return false;p=ctx->players[id];}
            if(p){actor=&p->base;visible=p->visible;name=p->name[0]?p->name:NULL;}
        } else {
            int id=ctx->npc_indices[index-ctx->player_count];if(id<0 || id>=MAX_NPCS)return false;
            const NpcEntity *n=ctx->npcs[id];if(n){actor=&n->base;visible=n->has_type;}
        }
        if(!actor || !visible || (!actor->chat[0] && actor->chat_timer<=0))continue;
        if(is_player && !(chat->public_mode==0 || chat->public_mode==3 || (chat->public_mode==1 && chatbox_is_friend_as(chat,name,ctx->local_player.name))))continue;
        int x,y;if(!project(user,actor,&x,&y) || x<0 || list->count>=50)continue;
        if(!memchr(actor->chat,0,sizeof(actor->chat)) || actor->chat_color<0 || actor->chat_timer<0 || actor->chat_timer>150)return false;
        int at=list->count;
        list->width[at]=pixfont_string_width(font,actor->chat)/2;list->height[at]=font->height;
        list->x[at]=x;list->y[at]=y;list->colour[at]=actor->chat_color;list->style[at]=actor->chat_style;list->timer[at]=actor->chat_timer;
        strcpy(list->text[at],actor->chat);list->present[at]=true;list->count++;
        /* Java postincrements chatCount before applying style adjustment: these
         * writes affect the NEXT slot, not the just-collected chat. */
        if(effects==0 && (actor->chat_style==1 || actor->chat_style==2)) {
            if(list->count==50)return false;
            if(actor->chat_style==1){list->height[list->count]+=10;list->y[list->count]+=5;}
            if(actor->chat_style==2)list->width[list->count]=60;
        }
    }
    for(int i=0;i<list->count;i++) {
        int x=list->x[i],y=list->y[i],w=list->width[i],h=list->height[i];bool moved=true;
        while(moved){moved=false;for(int j=0;j<i;j++)if(y+2>list->y[j]-list->height[j] && y-h<list->y[j]+2 && x-w<list->x[j]+list->width[j] && x+w>list->x[j]-list->width[j] && list->y[j]-list->height[j]<y){y=list->y[j]-list->height[j];moved=true;}}
        list->y[i]=y;x+=ox;y+=oy;
        const char *text=list->text[i];
        if(effects!=0){pixfont_draw_string_center(font,x,y+1,0,text);pixfont_draw_string_center(font,x,y,16776960,text);continue;}
        uint32_t colour=chat_colour(list->colour[i],cycle,list->timer[i]);
        if(list->style[i]==0){pixfont_draw_string_center(font,x,y+1,0,text);pixfont_draw_string_center(font,x,y,colour,text);}
        if(list->style[i]==1){pixfont_draw_centered_wave225(font,x,y+1,0,text,cycle);pixfont_draw_centered_wave225(font,x,y,colour,text,cycle);}
        if(list->style[i]==2){int offset=(150-list->timer[i])*(pixfont_string_width(font,text)+100)/150;
            pix2d_set_clip(x-50,oy,x+50,oy+334);pixfont_draw_string(font,x+50-offset,y+1,0,text);pixfont_draw_string(font,x+50-offset,y,colour,text);pix2d_set_clip(ox,oy,ox+512,oy+334);}
    }
    return true;
}
