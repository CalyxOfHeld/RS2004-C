from pathlib import Path
p=Path('platform/sdl2_main.c');s=p.read_text();idx=s.index('int main(');s=s[:idx]+'#include "verification_input.h"\n\n'+s[idx:];s=s.replace('    bool offline = false;','    bool offline = false;\n    bool trace=false;\n    const char *input_path=NULL;');s=s.replace('        else if (strcmp(argv[i], "--rebuild")', '        else if (strcmp(argv[i], "--trace") == 0) trace=true;\n        else if (strcmp(argv[i], "--input-script") == 0 && i+1<argc) input_path=argv[++i];\n        else if (strcmp(argv[i], "--rebuild")');idx=s.index('    printf("[Lost City 2004 C] Starting');s=s[:idx]+'''    if(input_path && (!capture_path || !verification_load_input(input_path))) {
        fprintf(stderr,"Invalid input script; --capture is required\\n");return 1;
    }
'''+s[idx:];s=s.replace('&viewport_interface_id,&chat_interface_id,live_server,true}', '&viewport_interface_id,&chat_interface_id,live_server,trace}');s=s.replace('        while (SDL_PollEvent(&event)) {','        verification_push_input(rendered_frames);\n        while (SDL_PollEvent(&event)) {')
# Offline-only shortcuts never impersonate server replies in a live session.
s=s.replace('if (live_server && cmd[0] == \':\') {', "if (live_server && cmd[0] == ':' && cmd[1] == ':') {")
s=s.replace('''                        if (strncmp(cmd, "::coords", 8) == 0) {''','''                        if(live_server) continue;
                        if (strncmp(cmd, "::coords", 8) == 0) {''')
s=s.replace('''                    if(update>0) continue;''','''                    if(update>0) {
                        if(trace && (opcode==150 || opcode==175)) {
                            int id=(payload[0]<<8)|payload[1];printf("[STATE] varp=%d value=%d\\n",id,varp_get(&varp_ctx,id));
                        }
                        if(trace && (opcode==98 || opcode==213 || opcode==15)) {
                            int id=(payload[0]<<8)|payload[1];Component *inv=component_get(&comp_cfg,id);
                            printf("[STATE] inventory=%d",id);
                            if(inv && inv->inv_slot_obj_id) for(int slot=0;slot<inv->width*inv->height;slot++)
                                if(inv->inv_slot_obj_id[slot]) printf(" slot%d=%dx%d",slot,inv->inv_slot_obj_id[slot]-1,inv->inv_slot_obj_count[slot]);
                            printf("\\n");
                        }
                        continue;
                    }''')
p.write_text(s)
# Rebuild when platform verification support changes.
p=Path('Makefile');s=p.read_text().replace('$(TARGET): $(ALL_SRCS) platform/sdl2_main.c','$(TARGET): $(ALL_SRCS) platform/sdl2_main.c platform/verification_input.h');p.write_text(s)
