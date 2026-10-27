#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
static int fail_after=-1;
static char *controlled_strdup(const char *text){if(fail_after==0)return NULL;if(fail_after>0)fail_after--;size_t n=strlen(text)+1;char *copy=malloc(n);if(copy)memcpy(copy,text,n);return copy;}
#define strdup controlled_strdup
#include "../../../src/ui/chatbox.c"
#undef strdup
int main(void) {
    Chatbox chat;chatbox_init(&chat,"Self");chatbox_add_message(&chat,0,NULL,"old");strcpy(chat.input,"kept");Chatbox before=chat;bool added;
    uint8_t *messages[]={(uint8_t*)"ordinary\n",(uint8_t*)"Alice:tradereq:\n",(uint8_t*)"Alice:duelreq:\n"};
    for(int i=0;i<3;i++)for(int allocation=0;allocation<2;allocation++){fail_after=allocation;added=true;assert(!chatbox_add_game_message225(&chat,messages[i],strlen((char*)messages[i]),false,&added) && !added);assert(!memcmp(&chat,&before,sizeof(chat)));}
    fail_after=-1;
    uint8_t missing[]="missing",nul[]={'A',0,'B',10},oversize[256];memset(oversize,'A',sizeof(oversize));oversize[255]=10;
    assert(!chatbox_add_game_message225(NULL,messages[0],9,false,&added));assert(!chatbox_add_game_message225(&chat,NULL,1,false,&added));assert(!chatbox_add_game_message225(&chat,missing,0,false,&added));assert(!chatbox_add_game_message225(&chat,missing,sizeof(missing)-1,false,&added));assert(!chatbox_add_game_message225(&chat,nul,sizeof(nul),false,&added));assert(!chatbox_add_game_message225(&chat,oversize,sizeof(oversize),false,&added));chat.ignore_count=101;assert(!chatbox_add_game_message225(&chat,messages[0],9,false,&added));chat=before;assert(!memcmp(&chat,&before,sizeof(chat)));
    fail_after=0;assert(chatbox_add_game_message225(&chat,messages[1],strlen((char*)messages[1]),true,&added) && !added);chat.ignore_count=1;chat.ignore_ids[0]=jstring_to_base37("alice");Chatbox ignored=chat;assert(chatbox_add_game_message225(&chat,messages[2],strlen((char*)messages[2]),false,&added) && !added && !memcmp(&chat,&ignored,sizeof(chat)));fail_after=-1;
    uint8_t trailing[]={'=','=',10,0,':','d','u','e','l','r','e','q',':'};assert(chatbox_add_game_message225(&chat,trailing,sizeof(trailing),true,&added) && added && chat.type[0]==0 && chat.sender[0] && !*chat.sender[0] && !strcmp(chat.text[0],"=="));
    chatbox_free(&chat);puts("PASS game-request faults:6 allocation failures,7 malformed/metadata guards,allocation-free ignore/override suppression,first-terminator trailing bytes and empty sender preserved");return 0;
}
