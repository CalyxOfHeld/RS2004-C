#include "ui/chatbox.h"
#include "ui/component.h"
#include "ui/component.h"
#include "core/packet.h"
#include "core/jstring.h"
#include "core/wordpack.h"
#include <ctype.h>
#include "gfx/pix2d.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

// strdup is POSIX, not C99; newlib (Wii U) does not declare it under -std=c99.
static char *chat_strdup(const char *s){size_t n=strlen(s)+1;char *copy=malloc(n);if(copy)memcpy(copy,s,n);return copy;}

int chatbox_count_value(const Chatbox *chat) {
    // Original Integer.parseInt rejects overflow and submits zero on failure.
    if(!chat) return 0;
    int value=0;
    for(const char *p=chat->count_dialog_input;*p;p++) {
        if(*p<'0' || *p>'9' || value>(INT_MAX-(*p-'0'))/10) return 0;
        value=value*10+(*p-'0');
    }
    return value;
}

void chatbox_init(Chatbox *chat, const char *username) {
    if (!chat) return;
    memset(chat, 0, sizeof(Chatbox));
    chat->report_interface=-1;
    if (username && username[0] != '\0') {
        strncpy(chat->username, username, sizeof(chat->username) - 1);
    } else {
        strncpy(chat->username, "Player", sizeof(chat->username) - 1);
    }
}

void chatbox_open_count_dialog(Chatbox *chat) {
    if (!chat) return;
    chat->private_prompt_open=false;
    chat->count_dialog_open = true;
    chat->count_dialog_input[0] = '\0';
}

void chatbox_close_count_dialog(Chatbox *chat) {
    if (!chat) return;
    chat->count_dialog_open = false;
    chat->count_dialog_input[0] = '\0';
}

static void publish_message(Chatbox *chat,int type,char *sender,char *text) {
    free(chat->text[99]);free(chat->sender[99]);
    for(int i=99;i>0;i--){chat->text[i]=chat->text[i-1];chat->sender[i]=chat->sender[i-1];chat->type[i]=chat->type[i-1];}
    chat->text[0]=text;chat->sender[0]=sender;chat->type[0]=type;
    if(chat->message_count<100)chat->message_count++;
}
static bool push_message(Chatbox *chat,int type,const char *sender,const char *text) {
    if(!chat || !text)return false;
    char *new_text=chat_strdup(text),*new_sender=sender && *sender?chat_strdup(sender):NULL;
    if(!new_text || (sender && *sender && !new_sender)){free(new_text);free(new_sender);return false;}
    publish_message(chat,type,new_sender,new_text);return true;
}

void chatbox_add_message(Chatbox *chat,int type,const char *sender,const char *text) {
    (void)push_message(chat,type,sender,text);
}
bool chatbox_apply_ignores(Chatbox *chat,const uint8_t *payload,size_t length) {
    if(!chat || (!payload && length) || length>800 || length%8)return false;
    Packet p=packet_wrap((uint8_t*)payload,length);
    size_t count=length/8;
    for(size_t i=0;i<count;i++)chat->ignore_ids[i]=g8(&p);
    chat->ignore_count=count;return true;
}
bool chatbox_add_private_message(Chatbox *chat,const WordFilterTables *tables,
    const uint8_t *payload,size_t length,bool override_chat) {
    if(!chat || !payload || length<13 || length>255 || chat->ignore_count>100 || chat->private_message_next>=100)return false;
    Packet p=packet_wrap((uint8_t*)payload,length);
    uint64_t sender=g8(&p);int32_t id=g4(&p);int rights=g1(&p);
    for(int i=0;i<100;i++)if(chat->private_message_ids[i]==id)return true;
    if(rights<=1)for(size_t i=0;i<chat->ignore_count;i++)if(chat->ignore_ids[i]==sender)return true;
    if(override_chat)return true;
    /* Original client.read remembers the ID inside try before unpack/filter. */
    chat->private_message_ids[chat->private_message_next]=id;
    chat->private_message_next=(chat->private_message_next+1)%100;
    char decoded[WORDPACK_MAX_OUTPUT+1],text[WORDPACK_MAX_OUTPUT+1],name[32];
    size_t n=0,filtered_n=0;uint16_t original[WORDPACK_MAX_OUTPUT],filtered[WORDPACK_MAX_OUTPUT+1];
    if(!wordpack_unpack(&p,length-13,decoded,sizeof(decoded),&n))return false;
    for(size_t i=0;i<n;i++)original[i]=(uint8_t)decoded[i];
    if(!wordfilter_apply(tables,original,n,filtered,WORDPACK_MAX_OUTPUT+1,&filtered_n))return false;
    for(size_t i=0;i<filtered_n;i++)text[i]=(char)filtered[i];
    text[filtered_n]=0;
    jstring_from_base37(sender,name,sizeof(name));jstring_format_name(name);
    return push_message(chat,rights>1?7:3,name,text);
}

bool chatbox_add_public_message(Chatbox *chat,const WordFilterTables *tables,
    PlayerEntity *player,const uint8_t *packed,size_t length,int colour,int rights,bool override_chat) {
    if(!chat || !player || (!packed && length) || length>255 || chat->ignore_count>100 || colour<0 || colour>65535 || rights<0 || rights>255)return false;
    if(!player->name[0])return true;
    uint64_t sender=jstring_to_base37(player->name);
    if(rights<=1)for(size_t i=0;i<chat->ignore_count;i++)if(chat->ignore_ids[i]==sender)return true;
    if(override_chat)return true;
    Packet p=packet_wrap((uint8_t*)packed,length);
    char decoded[WORDPACK_MAX_OUTPUT+1],text[WORDPACK_MAX_OUTPUT+1];
    uint16_t units[WORDPACK_MAX_OUTPUT],filtered[WORDPACK_MAX_OUTPUT+1];size_t n=0,result=0;
    if(!wordpack_unpack(&p,length,decoded,sizeof(decoded),&n))return false;
    for(size_t i=0;i<n;i++)units[i]=(uint8_t)decoded[i];
    if(!wordfilter_apply(tables,units,n,filtered,WORDPACK_MAX_OUTPUT+1,&result))return false;
    for(size_t i=0;i<result;i++)text[i]=(char)filtered[i];
    text[result]=0;
    /* Stage text/history allocation before publishing the actor state. */
    if(!push_message(chat,rights>1?1:2,player->name,text))return false;
    memcpy(player->base.chat,text,result+1);player->base.chat_color=colour>>8;
    player->base.chat_style=colour&255;player->base.chat_timer=150;return true;
}

bool chatbox_send_public(Chatbox *chat,const WordFilterTables *tables,PlayerEntity *player,GameSession *session) {
    if(!chat || !player || !session || !session->in_game || chat->input_len<1 || chat->input_len>80 ||
        chat->public_mode<0 || chat->public_mode>3 || chat->private_mode<0 || chat->private_mode>2 || chat->trade_mode<0 || chat->trade_mode>2)return false;
    size_t length=(size_t)chat->input_len;
    if(chat->input[length]!=0 || (length>=2 && chat->input[0]==':' && chat->input[1]==':'))return false;
    for(size_t i=0;i<length;i++)if((uint8_t)chat->input[i]<32 || (uint8_t)chat->input[i]>122)return false;
    const char *body=chat->input;int colour=0,style=0;
    static const char *colours[]={"yellow:","red:","green:","cyan:","purple:","white:","flash1:","flash2:","flash3:","glow1:","glow2:","glow3:"};
    static const char *styles[]={"wave:","scroll:"};
    for(int i=0;i<12;i++){size_t n=strlen(colours[i]);if(length>=n && !memcmp(body,colours[i],n)){colour=i;body+=n;length-=n;}}
    for(int i=0;i<2;i++){size_t n=strlen(styles[i]);if(length>=n && !memcmp(body,styles[i],n)){style=i+1;body+=n;length-=n;}}
    uint16_t sentence[80],filtered[81];size_t result=0;bool initial=true;
    for(size_t i=0;i<length;i++) {
        uint8_t ch=(uint8_t)body[i];if(ch>='A' && ch<='Z')ch+='a'-'A';
        sentence[i]=initial && ch>='a' && ch<='z'?(uint16_t)(ch+'A'-'a'):ch;
        if(initial && ch>='a' && ch<='z')initial=false;
        if(ch=='.' || ch=='!')initial=true;
    }
    if(!wordfilter_apply(tables,sentence,length,filtered,81,&result))return false;
    char text[81];for(size_t i=0;i<result;i++)text[i]=(char)filtered[i];
    text[result]=0;
    char *new_text=chat_strdup(text),*new_sender=player->name[0]?chat_strdup(player->name):NULL;
    if(!new_text || (player->name[0] && !new_sender)){free(new_text);free(new_sender);return false;}
    bool reveal=chat->public_mode==2;
    if(!protocol_send_public_chat(session,body,length,colour,style,reveal,chat->private_mode,chat->trade_mode)){free(new_text);free(new_sender);return false;}
    publish_message(chat,2,new_sender,new_text);memcpy(player->base.chat,text,result+1);
    player->base.chat_color=colour;player->base.chat_style=style;player->base.chat_timer=150;
    if(reveal)chat->public_mode=3;
    chat->input[0]=0;chat->input_len=0;return true;
}

bool chatbox_report_key(Chatbox *chat,int viewport,int key) {
    if(!chat || viewport==-1 || viewport!=chat->report_interface)return false;
    size_t length=strlen(chat->report_input);
    if(key==8 && length)chat->report_input[--length]=0;
    if(length<12 && ((key>='a' && key<='z') || (key>='A' && key<='Z') || (key>='0' && key<='9') || key==' ')){chat->report_input[length]=(char)key;chat->report_input[length+1]=0;}
    return true;
}
void chatbox_prepare_report_component(Chatbox *chat,Component *c,bool rights,int cycle) {
    if(!chat || !c)return;
    if(c->client_code==600){snprintf(chat->report_cursor,sizeof(chat->report_cursor),"%s%c",chat->report_input,cycle%20<10?'|':' ');c->text=chat->report_cursor;}
    if(c->client_code==613){if(!rights)c->text="";else {c->text=chat->report_mute?"Moderator option: Mute player for 48 hours: <ON>":"Moderator option: Mute player for 48 hours: <OFF>";c->colour=chat->report_mute?16711680:16777215;}}
}

bool chatbox_open_social(Chatbox *chat,int action) {
    if(!chat || (action!=1 && action!=2 && action!=4 && action!=5))return false;
    static const char *headers[]={"","Enter name of friend to add to list","Enter name of friend to delete from list","","Enter name of player to add to list","Enter name of player to delete from list"};
    chat->count_dialog_open=false;chat->private_prompt_open=true;chat->social_action=action;chat->private_input_len=0;chat->private_input[0]=0;
    snprintf(chat->private_prompt,sizeof(chat->private_prompt),"%s",headers[action]);return true;
}
bool chatbox_social_action(Chatbox *chat,GameSession *session,int action,uint64_t id,const char *self) {
    if(!chat || chat->friend_count<0 || chat->friend_count>100 || chat->ignore_count>100 || (action!=1 && action!=2 && action!=4 && action!=5))return false;
    if(!id)return true;
    char name[32],error[160]={0};jstring_from_base37(id,name,sizeof(name));jstring_format_name(name);
    if(action==1 || action==4) {
        bool friend=action==1;size_t count=friend?(size_t)chat->friend_count:chat->ignore_count;
        if(count>=100)snprintf(error,sizeof(error),"%s",friend?"Your friends list is full. Max of 100 hit":"Your ignore list is full. Max of 100 hit");
        if(!error[0])for(size_t i=0;i<count;i++)if((friend?chat->friend_ids[i]:chat->ignore_ids[i])==id){snprintf(error,sizeof(error),"%s is already on your %s list",name,friend?"friend":"ignore");break;}
        if(!error[0])for(size_t i=0;i<(friend?chat->ignore_count:(size_t)chat->friend_count);i++)if((friend?chat->ignore_ids[i]:chat->friend_ids[i])==id){snprintf(error,sizeof(error),"Please remove %s from your %s list first",name,friend?"ignore":"friend");break;}
        if(error[0]){
            char *new_text=chat_strdup(error),*new_sender=chat_strdup("");
            if(!new_text || !new_sender){free(new_text);free(new_sender);return false;}
            publish_message(chat,0,new_sender,new_text);return true;
        }
        if(friend && self && !strcmp(name,self))return true;
        if(!protocol_send_social(session,friend?118:79,id))return false;
        if(friend){size_t at=(size_t)chat->friend_count;chat->friend_ids[at]=id;strcpy(chat->friend_names[at],name);chat->friend_worlds[at]=0;chat->friend_count++;}
        else chat->ignore_ids[chat->ignore_count++]=id;
    }else {
        bool friend=action==2;size_t count=friend?(size_t)chat->friend_count:chat->ignore_count,index=0;
        for(;index<count;index++)if((friend?chat->friend_ids[index]:chat->ignore_ids[index])==id)break;
        if(index==count)return true;
        if(!protocol_send_social(session,friend?11:171,id))return false;
        if(friend){chat->friend_count--;for(size_t i=index;i<(size_t)chat->friend_count;i++){chat->friend_ids[i]=chat->friend_ids[i+1];memcpy(chat->friend_names[i],chat->friend_names[i+1],32);chat->friend_worlds[i]=chat->friend_worlds[i+1];}}
        else {chat->ignore_count--;for(size_t i=index;i<chat->ignore_count;i++)chat->ignore_ids[i]=chat->ignore_ids[i+1];}
    }
    return true;
}
bool chatbox_submit_social(Chatbox *chat,GameSession *session,const char *self) {
    if(!chat || !chat->private_prompt_open || chat->friend_count<0 || chat->friend_count>100 || chat->ignore_count>100 || chat->private_input_len<0 || chat->private_input_len>80 || chat->private_input[chat->private_input_len])return false;
    if(chat->social_action!=1 && chat->social_action!=2 && chat->social_action!=4 && chat->social_action!=5)return false;
    for(int i=0;i<chat->private_input_len;i++)if((uint8_t)chat->private_input[i]<32 || (uint8_t)chat->private_input[i]>122)return false;
    bool skip=(chat->social_action==2 && !chat->friend_count) || (chat->social_action==4 && chat->ignore_count>=100) || (chat->social_action==5 && !chat->ignore_count);
    if(!skip && !chatbox_social_action(chat,session,chat->social_action,jstring_to_base37(chat->private_input),self))return false;
    chat->private_prompt_open=false;return true;
}
bool chatbox_open_private(Chatbox *chat,uint64_t recipient) {
    if(!chat || chat->friend_count<0 || chat->friend_count>100)return false;
    chat->count_dialog_open=false;
    for(int i=0;i<chat->friend_count;i++)if(chat->friend_ids[i]==recipient) {
        if(chat->friend_worlds[i]<=0)return false;
        chat->social_action=3;chat->count_dialog_open=false;chat->private_prompt_open=true;chat->private_input[0]=0;chat->private_input_len=0;
        chat->private_recipient=recipient;snprintf(chat->private_prompt,sizeof(chat->private_prompt),"Enter message to send to %.31s",chat->friend_names[i]);return true;
    }
    return false;
}
bool chatbox_send_private(Chatbox *chat,const WordFilterTables *tables,GameSession *session) {
    if(!chat || !chat->private_prompt_open || chat->private_input_len<0 || chat->private_input_len>80)return false;
    size_t length=(size_t)chat->private_input_len;
    if(chat->private_input[length]!=0)return false;
    if(!length){chat->private_prompt_open=false;return true;}
    if(chat->public_mode<0 || chat->public_mode>3 || chat->private_mode<0 || chat->private_mode>2 || chat->trade_mode<0 || chat->trade_mode>2)return false;
    uint16_t sentence[80],filtered[81];size_t result=0;bool initial=true;
    for(size_t i=0;i<length;i++) {
        int ch=(uint8_t)chat->private_input[i];if(ch<32 || ch>122)return false;
        if(ch>=65 && ch<=90)ch+=32;
        if(initial && ch>=97 && ch<=122){ch-=32;initial=false;}
        if(ch=='.' || ch=='!')initial=true;
        sentence[i]=(uint16_t)ch;
    }
    if(!wordfilter_apply(tables,sentence,length,filtered,81,&result))return false;
    char text[81],name[32];for(size_t i=0;i<result;i++)text[i]=(char)filtered[i];text[result]=0;
    jstring_from_base37(chat->private_recipient,name,sizeof(name));jstring_format_name(name);
    char *new_text=chat_strdup(text),*new_sender=chat_strdup(name);
    if(!new_text || !new_sender){free(new_text);free(new_sender);return false;}
    bool reveal=chat->private_mode==2;
    if(!protocol_send_private_chat(session,chat->private_recipient,chat->private_input,length,reveal,chat->public_mode,chat->trade_mode)){free(new_text);free(new_sender);return false;}
    publish_message(chat,6,new_sender,new_text);memcpy(chat->private_input,text,result+1);chat->private_input_len=(int)result;
    if(reveal)chat->private_mode=1;
    chat->private_prompt_open=false;return true;
}
void chatbox_draw_private_prompt(Chatbox *chat,int x,int y,PixFont *font) {
    if(!chat || !chat->private_prompt_open || !font)return;
    char input[83];snprintf(input,sizeof(input),"%s*",chat->private_input);
    pixfont_draw_string_center(font,x+239,y+40,0,chat->private_prompt);
    pixfont_draw_string_center(font,x+239,y+60,128,input);
}
void chatbox_prepare_friend_component(Chatbox *chat,Component *c,int node_id) {
    if(!chat || !c || chat->friend_count<0 || chat->friend_count>100)return;
    int code=c->client_code,index=code>=101?code-101:code-1;
    if(code>=1 && code<=200) {
        c->button_type=index<chat->friend_count?1:0;
        if(index>=chat->friend_count)c->text="";
        else if(code<=100)c->text=chat->friend_names[index];
        else if(!chat->friend_worlds[index]){snprintf(chat->friend_status[index],40,"@red@Offline");c->text=chat->friend_status[index];}
        else {snprintf(chat->friend_status[index],40,"%sWorld-%d",chat->friend_worlds[index]==node_id?"@gre@":"@yel@",chat->friend_worlds[index]-9);c->text=chat->friend_status[index];}
    }else if(code==203){c->scroll=chat->friend_count*15+20;if(c->scroll<=c->height)c->scroll=c->height+1;}
    else if(code>=401 && code<=500 && chat->ignore_count<=100){index=code-401;c->button_type=index<(int)chat->ignore_count?1:0;
        if(index<(int)chat->ignore_count){jstring_from_base37(chat->ignore_ids[index],chat->ignore_names[index],32);jstring_format_name(chat->ignore_names[index]);c->text=chat->ignore_names[index];}else c->text="";
    }else if(code==503 && chat->ignore_count<=100){c->scroll=(int)chat->ignore_count*15+20;if(c->scroll<=c->height)c->scroll=c->height+1;}
}
bool chatbox_build_friend_menu(Chatbox *chat,Component *c,ContextMenu *menu) {
    if(!chat || !c || !menu || chat->friend_count<0 || chat->friend_count>100)return false;
    int code=c->client_code;
    if(code>=401 && code<=500){char option[80];snprintf(option,sizeof(option),"Remove @whi@%s",c->text?c->text:"");context_menu_add_option(menu,option,556);return true;}
    if(code<1 || code>200)return false;
    int index=code>=101?code-101:code-1;if(index>=chat->friend_count)return true;
    char option[80];snprintf(option,sizeof(option),"Remove @whi@%s",chat->friend_names[index]);context_menu_add_option(menu,option,557);
    snprintf(option,sizeof(option),"Message @whi@%s",chat->friend_names[index]);
    int at=menu->size;context_menu_add_action(menu,option,(MenuAction){.type=MENU_PRIVATE_MESSAGE,.name37=chat->friend_ids[index]});
    if(menu->size>at)menu->action_ids[at]=679;
    return true;
}

/* Original client.read4: exact suffix, first-colon raw sender, no word filter.
 * Packet4 has a one-byte length. Reject embedded NUL rather than truncate C text. */
bool chatbox_add_game_message225(Chatbox *chat,const uint8_t *payload,size_t length,bool override_chat,bool *added) {
    if(added)*added=false;
    if(!chat || !payload || !length || length>255 || chat->ignore_count>100)return false;
    const uint8_t *end=memchr(payload,10,length);
    if(!end || memchr(payload,0,(size_t)(end-payload)))return false;
    size_t n=(size_t)(end-payload);char message[256],sender[256];memcpy(message,payload,n);message[n]=0;
    int type=0;const char *text=message;sender[0]=0;
    if(n>=10 && !memcmp(message+n-10,":tradereq:",10)){type=4;text="wishes to trade with you.";}
    else if(n>=9 && !memcmp(message+n-9,":duelreq:",9)){type=8;text="wishes to duel with you.";}
    if(type){size_t name_length=(size_t)(strchr(message,':')-message);memcpy(sender,message,name_length);sender[name_length]=0;
        uint64_t name=jstring_to_base37(sender);
        for(size_t i=0;i<chat->ignore_count;i++)if(chat->ignore_ids[i]==name)return true;
        if(override_chat)return true;
    }
    char *new_text=chat_strdup(text),*new_sender=chat_strdup(sender);
    if(!new_text || !new_sender){free(new_text);free(new_sender);return false;}
    publish_message(chat,type,new_sender,new_text);if(added)*added=true;return true;
}
bool chatbox_add_game_message(Chatbox *chat,uint8_t *payload,size_t length) {
    return chatbox_add_game_message225(chat,payload,length,false,NULL);
}

void chatbox_key_press(Chatbox *chat, char ch) {
    if (!chat) return;
    if(chat->private_prompt_open) {
        if(ch>=32 && ch<=122 && chat->private_input_len<80){chat->private_input[chat->private_input_len++]=ch;chat->private_input[chat->private_input_len]=0;}
        return;
    }
    if (chat->count_dialog_open) {
        size_t len = strlen(chat->count_dialog_input);
        if (ch >= '0' && ch <= '9' && len < 10) {
            chat->count_dialog_input[len] = ch;
            chat->count_dialog_input[len + 1] = '\0';
        }
        return;
    }
    if (ch >= 32 && ch <= 122 && chat->input_len < CHATBOX_MAX_INPUT_LEN) {
        chat->input[chat->input_len++] = ch;
        chat->input[chat->input_len] = '\0';
    }
}

void chatbox_backspace(Chatbox *chat) {
    if (!chat) return;
    if(chat->private_prompt_open){if(chat->private_input_len>0)chat->private_input[--chat->private_input_len]=0;return;}
    if (chat->count_dialog_open) {
        size_t len = strlen(chat->count_dialog_input);
        if (len > 0) {
            chat->count_dialog_input[len - 1] = '\0';
        }
        return;
    }
    if (chat->input_len > 0) {
        chat->input[--chat->input_len] = '\0';
    }
}

static const char *submit_input(Chatbox *chat,bool echo) {
    if (!chat || chat->input_len == 0) return NULL;

    static char s_submitted[CHATBOX_MAX_INPUT_LEN + 1];
    strncpy(s_submitted, chat->input, sizeof(s_submitted) - 1);
    s_submitted[sizeof(s_submitted) - 1] = '\0';

    if (echo && s_submitted[0] != ':') {
        // Normal public chat message
        chatbox_add_message(chat, 2, chat->username, s_submitted);
    }

    chat->input[0] = '\0';
    chat->input_len = 0;
    return s_submitted;
}

const char *chatbox_submit(Chatbox *chat) {return submit_input(chat,true);}
const char *chatbox_take_input(Chatbox *chat) {return submit_input(chat,false);}

static bool name_equal(const char *a, const char *b) {
    if (!a || !b) return false;
    while (*a && *b) {
        if (tolower((unsigned char)*a++) != tolower((unsigned char)*b++)) return false;
    }
    return *a == *b;
}

bool chatbox_is_friend_as(const Chatbox *chat,const char *name,const char *self) {
    if(!chat || !name)return false;
    for(int i=0;i<chat->friend_count;i++)if(name_equal(name,chat->friend_names[i]))return true;
    return name_equal(name,self);
}
bool chatbox_is_friend(const Chatbox *chat,const char *name) {
    return chatbox_is_friend_as(chat,name,chat?chat->username:NULL);
}

/* client.read opcode152: membership is independent of online status. */
bool chatbox_apply_friend(Chatbox *chat, uint8_t *payload, size_t length, int node_id) {
    if (!chat || !payload || length != 9) return false;
    Packet packet = packet_wrap(payload, length);
    uint64_t id = (uint64_t)g8(&packet);
    int world = g1(&packet);
    if (packet.error) return false;
    char name[32];
    jstring_from_base37(id, name, sizeof(name));
    jstring_format_name(name);
    int i;
    for (i = 0; i < chat->friend_count; i++) if (chat->friend_ids[i] == id) break;
    if (i < chat->friend_count) {
        if (chat->friend_worlds[i] != world) {
            char message[80];
            snprintf(message, sizeof(message), "%s has logged %s.", name, world > 0 ? "in" : "out");
            chatbox_add_message(chat, 5, NULL, message);
            chat->friend_worlds[i] = world;
        }
    } else if (chat->friend_count < 100) {
        chat->friend_ids[i] = id;
        snprintf(chat->friend_names[i], sizeof(chat->friend_names[i]), "%s", name);
        chat->friend_worlds[i] = world;
        chat->friend_count++;
    }
    /* Original stable order: current world first, then other online, then offline. */
    bool sorted = false;
    while (!sorted) {
        sorted = true;
        for (i = 0; i + 1 < chat->friend_count; i++) {
            int a = chat->friend_worlds[i], b = chat->friend_worlds[i + 1];
            if ((a != node_id && b == node_id) || (a == 0 && b != 0)) {
                chat->friend_worlds[i] = b; chat->friend_worlds[i + 1] = a;
                uint64_t swap = chat->friend_ids[i];
                chat->friend_ids[i] = chat->friend_ids[i + 1]; chat->friend_ids[i + 1] = swap;
                char tmp[32]; memcpy(tmp, chat->friend_names[i], sizeof(tmp));
                memcpy(chat->friend_names[i], chat->friend_names[i + 1], sizeof(tmp));
                memcpy(chat->friend_names[i + 1], tmp, sizeof(tmp));
                sorted = false;
            }
        }
    }
    return true;
}

bool chatbox_message_visible(const Chatbox *chat, int i, bool split) {
    if (!chat || i < 0 || i >= CHATBOX_MAX_MESSAGES || !chat->text[i]) return false;
    int type = chat->type[i];
    bool friend = chatbox_is_friend(chat, chat->sender[i]);
    if (split) {
        if (!chat->split_private) return false;
        if (type == 3 || type == 7)
            return type == 7 || chat->private_mode == 0 || (chat->private_mode == 1 && friend);
        return (type == 5 || type == 6) && chat->private_mode < 2;
    }
    switch (type) {
    case 0: case 1: return true;
    case 2: return chat->public_mode == 0 || (chat->public_mode == 1 && friend);
    case 3: case 7:
        return !chat->split_private && (type == 7 || chat->private_mode == 0 || (chat->private_mode == 1 && friend));
    case 4: case 8: return chat->trade_mode == 0 || (chat->trade_mode == 1 && friend);
    case 5: case 6: return !chat->split_private && chat->private_mode < 2;
    default: return false;
    }
}

void chatbox_draw_private(Chatbox *chat, int x, int y, PixFont *font, int update_timer) {
    if (!chat || !font || !chat->split_private) return;
    int row = update_timer != 0 ? 1 : 0;
    for (int i = 0; i < CHATBOX_MAX_MESSAGES; i++) {
        if (!chatbox_message_visible(chat, i, true)) continue;
        char text[512];
        const char *sender = chat->sender[i] ? chat->sender[i] : "";
        if (chat->type[i] == 3 || chat->type[i] == 7)
            snprintf(text, sizeof(text), "From %s: %s", sender, chat->text[i]);
        else if (chat->type[i] == 6)
            snprintf(text, sizeof(text), "To %s: %s", sender, chat->text[i]);
        else snprintf(text, sizeof(text), "%s", chat->text[i]);
        int baseline = y + 329 - row * 13;
        pixfont_draw_string(font, x + 4, baseline, 0, text);
        pixfont_draw_string(font, x + 4, baseline - 1, 0x00ffff, text);
        if (++row >= 5) return;
    }
}

static void chatbox_sender_menu(ContextMenu *menu, const char *sender, bool rights, bool split) {
    char option[96];
    if (!sender) sender = "";
    if (rights) {
        snprintf(option, sizeof(option), "Report abuse @whi@%s", sender);
        context_menu_add_option(menu, option, split ? 2034 : 34);
    }
    snprintf(option, sizeof(option), "Add ignore @whi@%s", sender);
    context_menu_add_option(menu, option, split ? 2436 : 436);
    snprintf(option, sizeof(option), "Add friend @whi@%s", sender);
    context_menu_add_option(menu, option, split ? 2406 : 406);
}

/* mouse coordinates are absolute, as in client.handleInput. The original normal
 * chat hover baseline is four pixels below its draw baseline. */
void chatbox_build_menu(Chatbox *chat, ContextMenu *menu, int mx, int my,
                        bool split, bool rights, int update_timer) {
    if (!chat || !menu || (split && !chat->split_private)) return;
    if (!split && !(mx > 22 && mx < 431 && my > 375 && my < 471)) return;
    int row = split && update_timer != 0 ? 1 : 0;
    for (int i = 0; i < CHATBOX_MAX_MESSAGES; i++) {
        if (!chatbox_message_visible(chat, i, split)) continue;
        int type = chat->type[i];
        int baseline = split ? 329 - row * 13 : 74 - row * 14 + chat->scroll_offset;
        if (!split && baseline < -20) break;
        bool hit = split ? (mx > 8 && mx < 520 && my - 11 > baseline - 10 && my - 11 <= baseline + 3)
                         : (my - 375 > baseline - 14 && my - 375 <= baseline);
        if (hit) {
            if ((type == 1 || type == 2) && chat->sender[i] && strcmp(chat->sender[i], chat->username))
                chatbox_sender_menu(menu, chat->sender[i], rights, false);
            else if (type == 3 || type == 7)
                chatbox_sender_menu(menu, chat->sender[i], rights, split);
            else if (type == 4 || type == 8) {
                char option[96];
                snprintf(option, sizeof(option), "Accept %s @whi@%s", type == 4 ? "trade" : "duel", chat->sender[i] ? chat->sender[i] : "");
                context_menu_add_option(menu, option, type == 4 ? 903 : 363);
            }
        }
        if (++row >= 5 && split) return;
    }
}

static void chatbox_draw_internal(Chatbox *chat, Pix8 *chatback, int x, int y, PixFont *font_p12,const char *login_name,Pix8 *scrollbar0,Pix8 *scrollbar1) {
    if (!chat || !font_p12) return;

    // Draw background shell if provided
    if (chat->count_dialog_open) {
        if (chatback) pix8_draw(chatback, x, y);
        pixfont_draw_string_center(font_p12, x + 239, y + 40, 0x000000, "Enter amount:");
        char typing_buf[32];
        snprintf(typing_buf, sizeof(typing_buf), "%s*", chat->count_dialog_input);
        pixfont_draw_string_center(font_p12, x + 239, y + 60, 0x000080, typing_buf);
        return;
    }

    if (chatback) {
        pix8_draw(chatback, x, y);
    }

    // Save clip state
    int32_t prev_l = pix2d_clip_left;
    int32_t prev_r = pix2d_clip_right;
    int32_t prev_t = pix2d_clip_top;
    int32_t prev_b = pix2d_clip_bottom;

    // Clip to chat message viewport (y=0..77 relative to chatback)
    pix2d_set_clip(x, y, x + 463, y + 77);

    int line_idx = 0;
    for (int i = 0; i < CHATBOX_MAX_MESSAGES; i++) {
        if (!chatbox_message_visible(chat, i, false)) continue;
        int line_y = y + 70 - line_idx * 14 + chat->scroll_offset;
        if (line_y > y && line_y < y + 110) {
            int type = chat->type[i];
            const char *sender = chat->sender[i] ? chat->sender[i] : "";
            const char *text = chat->text[i];
            char prefix[96];
            if (type == 0 || type == 5) {
                pixfont_draw_string(font_p12, x + 4, line_y, type == 0 ? 0 : 0x800000, text);
            } else if (type == 4 || type == 8) {
                char request[512];
                snprintf(request, sizeof(request), "%s %s", sender, text);
                pixfont_draw_string(font_p12, x + 4, line_y, type == 4 ? 0x800080 : 0xcbb789, request);
            } else {
                const char *direction = type == 3 || type == 7 ? "From " : type == 6 ? "To " : "";
                snprintf(prefix, sizeof(prefix) - 1, "%s%s", direction, sender);
                int text_x = x + pixfont_string_width(font_p12, prefix) + 12;
                size_t len = strlen(prefix); prefix[len] = ':'; prefix[len + 1] = 0;
                pixfont_draw_string(font_p12, x + 4, line_y, type == 1 ? 0xffffff : 0, prefix);
                pixfont_draw_string(font_p12, text_x, line_y, type == 1 || type == 2 ? 0xff : 0x800000, text);
            }
        }
        line_idx++;
    }

    // Restore clipping
    pix2d_set_clip(prev_l, prev_t, prev_r, prev_b);

    if(login_name) {
        int scroll_height=line_idx*14+7;if(scroll_height<78)scroll_height=78;
        draw_scrollbar(x+463,y,scroll_height-chat->scroll_offset-77,scroll_height,77,scrollbar0,scrollbar1);
    }
    // Separator line (at y = 77 relative to chatback)
    pix2d_fill_rect(x, y + 77, 479, 1, 0x000000);

    // Prompt line (at y = 90 relative to chatback)
    char prompt[64], formatted_name[32];
    snprintf(formatted_name, sizeof(formatted_name), "%s", login_name ? login_name : chat->username);
    jstring_format_name(formatted_name);
    snprintf(prompt, sizeof(prompt), "%s:", formatted_name);
    pixfont_draw_string(font_p12, x + 4, y + 90, 0x000000, prompt);

    int prompt_w = pixfont_string_width(font_p12, prompt) + 4;
    if(login_name) {
        snprintf(prompt,sizeof(prompt),"%s: ",login_name);
        prompt_w=pixfont_string_width(font_p12,prompt)+2;
    }

    // Original revision-225 chat input always displays the asterisk.
    char typing_buf[CHATBOX_MAX_INPUT_LEN + 4];
    snprintf(typing_buf, sizeof(typing_buf), "%s*", chat->input);
    pixfont_draw_string(font_p12, x + 4 + prompt_w, y + 90, 0x0000FF, typing_buf);
}

void chatbox_draw(Chatbox *chat,Pix8 *chatback,int x,int y,PixFont *font) {
    chatbox_draw_internal(chat,chatback,x,y,font,NULL,NULL,NULL);
}
void chatbox_draw225(Chatbox *chat,Pix8 *chatback,int x,int y,PixFont *font,const char *login_name,Pix8 *scrollbar0,Pix8 *scrollbar1) {
    chatbox_draw_internal(chat,chatback,x,y,font,login_name,scrollbar0,scrollbar1);
}

void chatbox_free(Chatbox *chat) {
    if (!chat) return;
    for (int i = 0; i < CHATBOX_MAX_MESSAGES; i++) {
        if (chat->text[i]) {
            free(chat->text[i]);
            chat->text[i] = NULL;
        }
        if (chat->sender[i]) {
            free(chat->sender[i]);
            chat->sender[i] = NULL;
        }
    }
    chat->message_count = 0;
}
