#include "ui/client_actions.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
int main(void) {
    Chatbox chat;chatbox_init(&chat,"Self");strcpy(chat.report_input,"Alice");chat.report_mute=true;chat.report_interface=5875;chat.count_dialog_open=true;
    Component components[3]={0};components[0].client_code=600;components[0].layer=5875;components[1].client_code=601;components[2].client_code=613;ComponentConfig cfg={0};cfg.count=3;cfg.instances=components;
    GameSession *session=calloc(1,sizeof(*session));assert(session);protocol_init_session(session);session->in_game=true;session->stream.buffered=true;uint32_t seed[]={225,190,231,12};isaac_init(&session->out_cipher,seed,4);Isaac before_cipher=session->out_cipher;
    int viewport=5875,dialog=27,sidebar=55;ClientActions ctx={0};ctx.session=session;ctx.chatbox=&chat;ctx.components=&cfg;ctx.viewport_interface=&viewport;ctx.chat_interface=&dialog;ctx.sidebar_interface=&sidebar;ctx.live=true;Chatbox before=chat;
    for(int phase=0;phase<4;phase++)for(size_t available=0;available<(phase==1?12:1);available++) {
        session->stream.out_len=5000-available;session->stream.out_overflow=false;memset(session->stream.out_buf,0xa5,5000);
        if(phase==0)assert(client_report_open(&ctx,"Bob",true)==ACTION_FAILED);
        if(phase==1)assert(client_action_dispatch(&ctx,(MenuAction){.type=MENU_IF_BUTTON,.component=1})==ACTION_FAILED);
        if(phase==2){chat.report_input[0]=0;Chatbox empty=chat;assert(client_action_dispatch(&ctx,(MenuAction){.type=MENU_IF_BUTTON,.component=1})==ACTION_FAILED);assert(!memcmp(&chat,&empty,sizeof(chat)));chat=before;}
        if(phase==3)assert(client_action_dispatch(&ctx,(MenuAction){.type=MENU_IF_CLOSE})==ACTION_FAILED);
        assert(session->stream.out_overflow && session->stream.out_len==5000-available && !memcmp(&chat,&before,sizeof(chat)) && !memcmp(&session->out_cipher,&before_cipher,sizeof(before_cipher)) && viewport==5875 && dialog==27 && sidebar==55);for(int i=0;i<5000;i++)assert(session->stream.out_buf[i]==0xa5);
    }
    session->stream.out_len=0;session->stream.out_overflow=true;assert(client_report_open(&ctx,"Bob",true)==ACTION_FAILED);session->stream.out_overflow=false;session->stream.buffered=false;assert(client_report_open(&ctx,"Bob",true)==ACTION_FAILED);assert(client_action_dispatch(&ctx,(MenuAction){.type=MENU_IF_BUTTON,.component=1})==ACTION_FAILED);assert(!memcmp(&chat,&before,sizeof(chat)) && !memcmp(&session->out_cipher,&before_cipher,sizeof(before_cipher)));session->stream.buffered=true;
    assert(!protocol_send_report(session,true,1,-1,false) && !protocol_send_report(session,true,1,12,false));
    session->stream.out_len=4988;assert(client_action_dispatch(&ctx,(MenuAction){.type=MENU_IF_BUTTON,.component=1})==ACTION_LOCAL && session->stream.out_len==5000 && viewport==-1 && dialog==-1 && sidebar==-1);
    session->stream.out_len=4999;assert(client_report_open(&ctx,"Bob",false)==ACTION_LOCAL && session->stream.out_len==5000 && viewport==5875 && !chat.report_mute && !strcmp(chat.report_input,"Bob"));
    chatbox_free(&chat);free(session);puts("PASS report faults:15 capacity rejections,exact12-byte close/report and1-byte open successes,sticky/disconnected writes,invalid categories; interfaces/editor/mute/cipher/output retained on failure");return 0;
}
