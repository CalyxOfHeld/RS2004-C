#ifndef RS2_VERIFICATION_INPUT_H
#define RS2_VERIFICATION_INPUT_H
/* Optional replay of SDL events. It never edits inventory, varps or server state. */
typedef struct { int frame; char kind[16]; char value[256]; } VerificationInput;
static VerificationInput verification_input[512];
static int verification_input_count, verification_input_next;
static bool verification_follow_input;
static char verification_input_path[1024];
static long verification_input_offset;
static int verification_npc_type=-1,verification_cursor_x=-1,verification_cursor_y=-1;
static int verification_obj_id=-1,verification_inventory_slot=-1;
static char verification_option[64];
static bool verification_load_input(const char *path) {
    FILE *file=fopen(path,"r"); if(!file) return false;
    if(strlen(path)>=sizeof(verification_input_path)) {fclose(file);return false;}
    if(path!=verification_input_path) strcpy(verification_input_path,path);
    if(verification_input_offset && fseek(file,verification_input_offset,SEEK_SET)) {fclose(file);return false;}
    char line[320];int previous=verification_input_count?verification_input[verification_input_count-1].frame:-1;
    while(fgets(line,sizeof(line),file)) {
        if(line[0]=='#' || line[0]=='\n' || line[0]=='\r') continue;
        if(verification_input_count>=512) {fclose(file);return false;}
        VerificationInput *input=&verification_input[verification_input_count];
        if(sscanf(line,"%d %15s %255[^\r\n]",&input->frame,input->kind,input->value)!=3 || input->frame<previous) {fclose(file);return false;}
        if(strcmp(input->kind,"move") && strcmp(input->kind,"npc") && strcmp(input->kind,"obj") && strcmp(input->kind,"inv") && strcmp(input->kind,"option") && strcmp(input->kind,"right_cursor") && strcmp(input->kind,"left_cursor") && strcmp(input->kind,"quit") && strcmp(input->kind,"left") && strcmp(input->kind,"release") && strcmp(input->kind,"right") && strcmp(input->kind,"text") && strcmp(input->kind,"key")) {fclose(file);return false;}
        previous=input->frame;verification_input_count++;
    }
    verification_input_offset=ftell(file);fclose(file);return true;
}
static void verification_push_input(int frame) {
    if(verification_follow_input && frame%10==0 && verification_input_next==verification_input_count &&
        !verification_load_input(verification_input_path)) {
        fprintf(stderr,"Invalid appended verification input; follow disabled\n");verification_follow_input=false;
    }
    while(verification_input_next<verification_input_count && verification_input[verification_input_next].frame<frame) {
        fprintf(stderr,"Skipped late verification event at frame %d (current %d)\n",verification_input[verification_input_next].frame,frame);
        verification_input_next++;
    }
    while(verification_input_next<verification_input_count && verification_input[verification_input_next].frame==frame) {
        VerificationInput *input=&verification_input[verification_input_next++];SDL_Event event;memset(&event,0,sizeof(event));
        printf("[INPUT] frame=%d event=%s\n",frame,input->kind);
        if(!strcmp(input->kind,"npc")) {
            verification_npc_type=atoi(input->value);verification_obj_id=verification_inventory_slot=-1;verification_cursor_x=verification_cursor_y=-1;
        } else if(!strcmp(input->kind,"obj")) {
            verification_obj_id=atoi(input->value);verification_npc_type=verification_inventory_slot=-1;verification_cursor_x=verification_cursor_y=-1;
        } else if(!strcmp(input->kind,"inv")) {
            verification_inventory_slot=atoi(input->value);verification_npc_type=verification_obj_id=-1;verification_cursor_x=verification_cursor_y=-1;
        } else if(!strcmp(input->kind,"option")) {
            snprintf(verification_option,sizeof(verification_option),"%.63s",input->value);
        } else if(!strcmp(input->kind,"quit")) {
            event.type=SDL_QUIT;SDL_PushEvent(&event);
        } else if(!strcmp(input->kind,"right_cursor") || !strcmp(input->kind,"left_cursor")) {
            if(verification_cursor_x>=0) {
                event.type=SDL_MOUSEBUTTONDOWN;event.button.button=!strcmp(input->kind,"right_cursor")?SDL_BUTTON_RIGHT:SDL_BUTTON_LEFT;
                event.button.x=verification_cursor_x;event.button.y=verification_cursor_y;SDL_PushEvent(&event);
            } else fprintf(stderr,"[INPUT] no resolved cursor for %s at frame %d\n",input->kind,frame);
        } else if(!strcmp(input->kind,"key")) {
            event.type=SDL_KEYDOWN;event.key.keysym.sym=SDL_GetKeyFromName(input->value);SDL_PushEvent(&event);
        } else if(!strcmp(input->kind,"text")) {
            const char *text=input->value;while(*text) {
                event.type=SDL_TEXTINPUT;size_t size=strlen(text);if(size>31) size=31;
                memcpy(event.text.text,text,size);event.text.text[size]=0;text+=size;SDL_PushEvent(&event);
            }
        } else {
            int x,y;if(sscanf(input->value,"%d %d",&x,&y)!=2) continue;
            if(!strcmp(input->kind,"move")) {event.type=SDL_MOUSEMOTION;event.motion.x=x;event.motion.y=y;}
            else {event.type=!strcmp(input->kind,"release")?SDL_MOUSEBUTTONUP:SDL_MOUSEBUTTONDOWN;event.button.x=x;event.button.y=y;event.button.button=!strcmp(input->kind,"right")?SDL_BUTTON_RIGHT:SDL_BUTTON_LEFT;}
            SDL_PushEvent(&event);
        }
    }
}
#endif
