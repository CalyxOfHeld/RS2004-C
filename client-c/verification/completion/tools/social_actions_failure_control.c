#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
static int fail_after=-1;
static char *social_strdup(const char *s){if(fail_after==0)return NULL;if(fail_after>0)fail_after--;size_t n=strlen(s)+1;char *p=malloc(n);if(p)memcpy(p,s,n);return p;}
#define strdup social_strdup
#include "../../../src/ui/chatbox.c"
#undef strdup
int main(void) {
    Chatbox chat;chatbox_init(&chat,"Self");chat.friend_count=1;strcpy(chat.friend_names[0],"Alice");chat.friend_ids[0]=jstring_to_base37("Alice");chat.friend_worlds[0]=10;chat.ignore_count=1;chat.ignore_ids[0]=jstring_to_base37("Bob");chatbox_add_message(&chat,0,NULL,"old");Chatbox before=chat;
    GameSession *session=calloc(1,sizeof(*session));assert(session);protocol_init_session(session);session->in_game=true;session->stream.buffered=true;uint32_t seed[]={225,118,79,171};isaac_init(&session->out_cipher,seed,4);Isaac initial=session->out_cipher;
    int operations[]={1,2,4,5};uint64_t ids[]={jstring_to_base37("Carol"),chat.friend_ids[0],jstring_to_base37("Carol"),chat.ignore_ids[0]};
    for(int op=0;op<4;op++) {
        chat=before;session->out_cipher=initial;
        for(size_t available=0;available<9;available++){session->stream.out_len=5000-available;session->stream.out_overflow=false;memset(session->stream.out_buf,0xa5,5000);assert(!chatbox_social_action(&chat,session,operations[op],ids[op],"Self"));assert(session->stream.out_overflow && session->stream.out_len==5000-available && !memcmp(&chat,&before,sizeof(chat)) && !memcmp(&session->out_cipher,&initial,sizeof(initial)));for(int i=0;i<5000;i++)assert(session->stream.out_buf[i]==0xa5);}
        session->stream.out_len=0;session->stream.out_overflow=true;assert(!chatbox_social_action(&chat,session,operations[op],ids[op],"Self"));session->stream.out_overflow=false;session->stream.buffered=false;assert(!chatbox_social_action(&chat,session,operations[op],ids[op],"Self"));session->stream.buffered=true;assert(!memcmp(&chat,&before,sizeof(chat)) && !memcmp(&session->out_cipher,&initial,sizeof(initial)));
        session->stream.out_len=4991;assert(chatbox_social_action(&chat,session,operations[op],ids[op],"Self") && session->stream.out_len==5000);
    }
    chat=before;session->out_cipher=initial;session->stream.out_len=0;
    for(int op=0;op<2;op++)for(int fault=0;fault<2;fault++){fail_after=fault;assert(!chatbox_social_action(&chat,session,op?4:1,op?chat.ignore_ids[0]:chat.friend_ids[0],"Self"));fail_after=-1;assert(!memcmp(&chat,&before,sizeof(chat)) && !session->stream.out_len && !memcmp(&session->out_cipher,&initial,sizeof(initial)));}
    assert(chatbox_open_social(&chat,1));strcpy(chat.private_input,"Carol");chat.private_input_len=5;Chatbox prompt=chat;session->stream.out_len=4992;assert(!chatbox_submit_social(&chat,session,"Self") && session->stream.out_overflow && !memcmp(&chat,&prompt,sizeof(chat)));
    chat=before;chat.friend_count=101;assert(!chatbox_social_action(&chat,session,1,1,"Self"));chat=before;chat.ignore_count=101;assert(!chatbox_social_action(&chat,session,4,1,"Self"));chat=before;assert(!chatbox_social_action(&chat,session,3,1,"Self"));assert(!protocol_send_social(session,999,1));
    chatbox_free(&chat);free(session);puts("PASS social faults:36 capacity rejections,4 exact9-byte successes,4 error-UI allocation faults,all4 sticky/disconnected writes,prompt-retaining submit failure,malformed counts/action/opcode; local lists/history/cipher/output safe");return 0;
}
