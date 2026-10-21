#ifndef RS2_VERIFICATION_INPUT_H
#define RS2_VERIFICATION_INPUT_H
/* Optional replay of SDL events. It never edits inventory, varps or server state. */
typedef struct { int frame; char kind[16]; char value[256]; } VerificationInput;
static VerificationInput verification_input[512];
static int verification_input_count, verification_input_next;
static bool verification_load_input(const char *path) {
    FILE *file=fopen(path,"r"); if(!file) return false;
    char line[320];int previous=-1;
    while(fgets(line,sizeof(line),file)) {
        if(line[0]=='#' || line[0]=='\n' || line[0]=='\r') continue;
        if(verification_input_count>=512) {fclose(file);return false;}
        VerificationInput *input=&verification_input[verification_input_count];
        if(sscanf(line,"%d %15s %255[^\r\n]",&input->frame,input->kind,input->value)!=3 || input->frame<previous) {fclose(file);return false;}
        if(strcmp(input->kind,"move") && strcmp(input->kind,"left") && strcmp(input->kind,"right") && strcmp(input->kind,"text") && strcmp(input->kind,"key")) {fclose(file);return false;}
        previous=input->frame;verification_input_count++;
    }
    fclose(file);return true;
}
static void verification_push_input(int frame) {
    while(verification_input_next<verification_input_count && verification_input[verification_input_next].frame==frame) {
        VerificationInput *input=&verification_input[verification_input_next++];SDL_Event event;memset(&event,0,sizeof(event));
        printf("[INPUT] frame=%d event=%s\n",frame,input->kind);
        if(!strcmp(input->kind,"key")) {
            event.type=SDL_KEYDOWN;event.key.keysym.sym=SDL_GetKeyFromName(input->value);SDL_PushEvent(&event);
        } else if(!strcmp(input->kind,"text")) {
            const char *text=input->value;while(*text) {
                event.type=SDL_TEXTINPUT;size_t size=strlen(text);if(size>31) size=31;
                memcpy(event.text.text,text,size);event.text.text[size]=0;text+=size;SDL_PushEvent(&event);
            }
        } else {
            int x,y;if(sscanf(input->value,"%d %d",&x,&y)!=2) continue;
            if(!strcmp(input->kind,"move")) {event.type=SDL_MOUSEMOTION;event.motion.x=x;event.motion.y=y;}
            else {event.type=SDL_MOUSEBUTTONDOWN;event.button.x=x;event.button.y=y;event.button.button=!strcmp(input->kind,"left")?SDL_BUTTON_LEFT:SDL_BUTTON_RIGHT;}
            SDL_PushEvent(&event);
        }
    }
}
#endif
