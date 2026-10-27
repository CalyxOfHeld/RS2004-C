/* Fault only native UI strdup; actual production source included unchanged. */
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
static int fail_after=-1;
static char *controlled_strdup(const char *text) {
    if(fail_after==0)return NULL;
    if(fail_after>0)fail_after--;
    size_t n=strlen(text)+1;char *copy=malloc(n);if(copy)memcpy(copy,text,n);return copy;
}
#define strdup controlled_strdup
#include "../../../src/ui/chatbox.c"
#undef strdup
int main(void) {
    Arena arena=arena_create(8*1024*1024);Jagfile *jag=jagfile_load_file("../engine/data/pack/client/wordenc",&arena);assert(jag);WordFilterTables tables={0};assert(wordfilter_tables_load(&tables,jag));
    Chatbox chat;chatbox_init(&chat,"Alice");chatbox_add_message(&chat,0,NULL,"old");chat.public_mode=2;chat.private_mode=1;
    memset(chat.input,'Z',80);chat.input[80]=0;chat.input_len=80;
    PlayerEntity actor;player_entity_init(&actor);strcpy(actor.name,"Alice");strcpy(actor.base.chat,"old");actor.base.chat_timer=7;
    GameSession *session=calloc(1,sizeof(*session));assert(session);protocol_init_session(session);session->in_game=true;session->stream.buffered=true;uint32_t seed[]={225,158,244,80};isaac_init(&session->out_cipher,seed,4);
    Chatbox before=chat;PlayerEntity actor_before=actor;Isaac cipher=session->out_cipher;
    for(int fail=0;fail<2;fail++) {
        fail_after=fail;assert(!chatbox_send_public(&chat,&tables,&actor,session));fail_after=-1;
        assert(!memcmp(&chat,&before,sizeof(chat)) && !memcmp(&actor,&actor_before,sizeof(actor)) && !memcmp(&cipher,&session->out_cipher,sizeof(cipher)) && !session->stream.out_len);
    }
    for(size_t available=0;available<88;available++) {
        session->stream.out_len=5000-available;session->stream.out_overflow=false;memset(session->stream.out_buf,0xa5,5000);
        assert(!chatbox_send_public(&chat,&tables,&actor,session) && session->stream.out_overflow);
        assert(!memcmp(&chat,&before,sizeof(chat)) && !memcmp(&actor,&actor_before,sizeof(actor)) && !memcmp(&cipher,&session->out_cipher,sizeof(cipher)) && session->stream.out_len==5000-available);
        for(int i=0;i<5000;i++)assert(session->stream.out_buf[i]==0xa5);
    }
    session->stream.out_len=0;session->stream.out_overflow=true;assert(!chatbox_send_public(&chat,&tables,&actor,session));session->stream.out_overflow=false;
    session->stream.buffered=false;assert(!chatbox_send_public(&chat,&tables,&actor,session));session->stream.buffered=true;
    assert(!memcmp(&chat,&before,sizeof(chat)) && !memcmp(&actor,&actor_before,sizeof(actor)) && !memcmp(&cipher,&session->out_cipher,sizeof(cipher)) && !session->stream.out_len);
    session->stream.out_len=4912;assert(chatbox_send_public(&chat,&tables,&actor,session));assert(session->stream.out_len==5000 && chat.public_mode==3 && !chat.input_len && actor.base.chat_timer==150);
    chatbox_free(&chat);wordfilter_tables_free(&tables);arena_free(&arena);free(session);
    puts("PASS public-send failure control:2 UI allocation failures,88 insufficient capacities,exact88-byte combined success,preexisting overflow/disconnected write; UI/actor/cipher/output publication transactional");return 0;
}
