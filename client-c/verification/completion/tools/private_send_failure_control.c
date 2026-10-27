/* Fail each UI allocation in unchanged native sender, then capacity/write faults. */
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
static int fail_after=-1;
static char *private_control_strdup(const char *s){if(fail_after==0)return NULL;if(fail_after>0)fail_after--;size_t n=strlen(s)+1;char *p=malloc(n);if(p)memcpy(p,s,n);return p;}
#define strdup private_control_strdup
#include "../../../src/ui/chatbox.c"
#undef strdup
int main(void) {
    Arena arena=arena_create(8*1024*1024);Jagfile *jag=jagfile_load_file("../engine/data/pack/client/wordenc",&arena);assert(jag);WordFilterTables filter={0};assert(wordfilter_tables_load(&filter,jag));
    Chatbox chat;chatbox_init(&chat,"Self");chat.friend_count=1;strcpy(chat.friend_names[0],"Alice");chat.friend_ids[0]=jstring_to_base37("Alice");chat.friend_worlds[0]=10;chatbox_add_message(&chat,0,NULL,"old");assert(chatbox_open_private(&chat,chat.friend_ids[0]));
    memset(chat.private_input,'W',80);chat.private_input[80]=0;chat.private_input_len=80;chat.private_mode=2;chat.public_mode=3;chat.trade_mode=2;
    GameSession *session=calloc(1,sizeof(*session));assert(session);protocol_init_session(session);session->in_game=true;session->stream.buffered=true;uint32_t seed[]={225,148,244,80};isaac_init(&session->out_cipher,seed,4);
    Chatbox before=chat;Isaac cipher=session->out_cipher;
    for(int fail=0;fail<2;fail++){fail_after=fail;assert(!chatbox_send_private(&chat,&filter,session));fail_after=-1;assert(!memcmp(&chat,&before,sizeof(chat)) && !memcmp(&cipher,&session->out_cipher,sizeof(cipher)) && !session->stream.out_len);}
    for(size_t available=0;available<94;available++) {
        session->stream.out_len=5000-available;session->stream.out_overflow=false;memset(session->stream.out_buf,0xa5,5000);
        assert(!chatbox_send_private(&chat,&filter,session) && session->stream.out_overflow);assert(!memcmp(&chat,&before,sizeof(chat)) && !memcmp(&cipher,&session->out_cipher,sizeof(cipher)) && session->stream.out_len==5000-available);
        for(int i=0;i<5000;i++)assert(session->stream.out_buf[i]==0xa5);
    }
    session->stream.out_len=0;session->stream.out_overflow=true;assert(!chatbox_send_private(&chat,&filter,session));session->stream.out_overflow=false;session->stream.buffered=false;assert(!chatbox_send_private(&chat,&filter,session));session->stream.buffered=true;
    assert(!memcmp(&chat,&before,sizeof(chat)) && !memcmp(&cipher,&session->out_cipher,sizeof(cipher)) && !session->stream.out_len);
    session->stream.out_len=4906;assert(chatbox_send_private(&chat,&filter,session));assert(session->stream.out_len==5000 && !chat.private_prompt_open && chat.private_mode==1 && chat.type[0]==6 && !strcmp(chat.sender[0],"Alice"));
    chatbox_free(&chat);wordfilter_tables_free(&filter);arena_free(&arena);free(session);
    puts("PASS private-send failure:2 UI allocation failures,94 insufficient capacities,exact94-byte combined148/244 success,sticky overflow/disconnected write; prompt/history/privacy/cipher/output transactional");return 0;
}
