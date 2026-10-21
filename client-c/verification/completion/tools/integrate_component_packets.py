from pathlib import Path
p=Path('Makefile');s=p.read_text().replace('UI_SRCS     = src/ui/component.c','UI_SRCS     = src/ui/component_packets.c src/ui/component.c');p.write_text(s)
p=Path('platform/sdl2_main.c');s=p.read_text().replace('#include "ui/component.h"','#include "ui/component.h"\n#include "ui/component_packets.h"');needle='''                while (protocol_poll_packet(&session, &opcode, payload, sizeof(payload), &pkt_len) > 0) {''';s=s.replace(needle,'''                int receive_status;
                while ((receive_status=protocol_poll_packet(&session, &opcode, payload, sizeof(payload), &pkt_len)) > 0) {
                    int update=component_apply_packet(&comp_cfg,&varp_ctx,&arena_perm,opcode,payload,pkt_len);
                    if(update<0) {
                        fprintf(stderr,"[Protocol] Malformed component packet %d (%zu bytes)\\n",opcode,pkt_len);
                        protocol_close_session(&session);live_server=false;break;
                    }
                    if(update>0) continue;''')
# Remove superseded handlers so tests and client cannot exercise divergent parsers.
for op in [150,175,193,201,44,98,213]:
    token=f'}} else if (opcode == {op})'
    start=s.index(token);brace=s.index('{',start+len(token));depth=1;end=brace+1
    while depth:
        if s[end]=='{':depth+=1
        elif s[end]=='}':depth-=1
        end+=1
    # Keep closing brace of previous branch and allow the following else to join it.
    s=s[:start+1]+s[end:]
# Explicitly detect socket failure rather than silently falling back to demo.
needle='''            // B. Camera & Player Movement Controls''';idx=s.index(needle);before=s[:idx];at=before.rfind('            }');s=s[:at]+'''                if(receive_status<0 && live_server) {
                    live_server=false;protocol_close_session(&session);
                    chatbox_add_message(&chatbox,0,NULL,"Connection lost.");
                }
'''+s[at:]
# Keepalive cadence derives from webclient/src/client/Client.ts NO_TIMEOUT handling.
s=s.replace('    int rendered_frames = 0;','    int rendered_frames = 0;\n    Uint32 last_keepalive=SDL_GetTicks();') if '    int rendered_frames = 0;' in s else s
# Locate declaration form if different.
if 'Uint32 last_keepalive' not in s:
    idx=s.index('    while (running)');s=s[:idx]+'    Uint32 last_keepalive=SDL_GetTicks();\n'+s[idx:]
s=s.replace('''                int opcode = 0;''','''                if(SDL_GetTicks()-last_keepalive>=1000) {
                    if(!protocol_send_keepalive(&session)) {protocol_close_session(&session);live_server=false;}
                    last_keepalive=SDL_GetTicks();
                }
                int opcode = 0;''')
p.write_text(s)

