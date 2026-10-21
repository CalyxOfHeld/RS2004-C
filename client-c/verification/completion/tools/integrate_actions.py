from pathlib import Path
p=Path('src/ui/mouse_picking.c');s=p.read_text();s=s.replace('context_menu_add_option(menu, "Cancel", 0);','context_menu_add_action(menu,"Cancel",(MenuAction){.type=MENU_CANCEL});').replace('context_menu_add_option(menu, "Walk here", 1);','context_menu_add_action(menu,"Walk here",(MenuAction){.type=MENU_WALK});')
s=s.replace('context_menu_add_option(menu, examine_str, 2000 + id);','context_menu_add_action(menu,examine_str,(MenuAction){.type=MENU_NPC_EXAMINE,.target=id});').replace('context_menu_add_option(menu, op_str, 1000 + o * 100 + id);','context_menu_add_action(menu,op_str,(MenuAction){.type=MENU_NPC_OP,.operation=o+1,.target=id});').replace('context_menu_add_option(menu, examine_str, 4000 + id);','context_menu_add_action(menu,examine_str,(MenuAction){.type=MENU_LOC_EXAMINE,.target=id});').replace('context_menu_add_option(menu, op_str, 3000 + o * 100 + id);','context_menu_add_action(menu,op_str,(MenuAction){.type=MENU_LOC_OP,.operation=o+1,.target=id,.x=(int)(bitset&127),.z=(int)((bitset>>7)&127)});').replace('context_menu_add_option(menu, ex_str, 5000 + id);','context_menu_add_action(menu,ex_str,(MenuAction){.type=MENU_PLAYER_OP,.operation=3,.target=id});').replace('context_menu_add_option(menu, ex_str, 5100 + id);','context_menu_add_action(menu,ex_str,(MenuAction){.type=MENU_PLAYER_OP,.operation=4,.target=id});').replace('context_menu_add_option(menu, ex_str, 6000 + id);','context_menu_add_action(menu,ex_str,(MenuAction){.type=MENU_OBJ_EXAMINE,.target=id});').replace('context_menu_add_option(menu, ex_str, 6100 + id);','context_menu_add_action(menu,ex_str,(MenuAction){.type=MENU_OBJ_OP,.operation=3,.target=id,.x=(int)(bitset&127),.z=(int)((bitset>>7)&127)});');p.write_text(s)
p=Path('src/ui/client_actions.c');s=p.read_text().replace(r'\\n',r'\n');p.write_text(s)
p=Path('src/net/protocol.c');s=p.read_text().replace('for(int i=1;i<count-1 && points<24;i++)','for(int i=1;i<count-1;i++)').replace('if(dx!=x[i+1]-x[i] || dz!=z[i+1]-z[i]) indices[points++]=i;', 'if(dx!=x[i+1]-x[i] || dz!=z[i+1]-z[i]) {\n            if(points>=24) return false;\n            indices[points++]=i;\n        }');p.write_text(s)
p=Path('platform/sdl2_main.c');s=p.read_text().replace('#include "ui/component_packets.h"','#include "ui/component_packets.h"\n#include "ui/client_actions.h"').replace('#define CLIENT_WIDTH  765','#define CLIENT_WIDTH  789').replace('#define CLIENT_HEIGHT 503','#define CLIENT_HEIGHT 532');s=s.replace('if (inv_grid && inv_grid->inv_slot_obj_id) {','if (offline && inv_grid && inv_grid->inv_slot_obj_id) {',1)
# Explicit mode separation: a failed login does not become simulated gameplay.
s=s.replace('''    } else {
        printf("[INFO] Server offline (code %d). Starting in standalone Lumbridge simulation mode.\\n", login_res);
    }''','''    } else if(offline) {
        printf("[MODE] Explicit offline demonstration; inventory/entities are fixtures, no gameplay verification.\\n");
    } else {
        fprintf(stderr,"Login failed (%d); use --offline only for the explicit demonstration.\\n",login_res);
        SDL_Quit();return 1;
    }''')
# Use configurable test accounts without logging credentials. Existing defaults remain a documented login-UI gap.
s=s.replace('protocol_login(&session, "127.0.0.1", server_port, "antigravity", "antigravity", false)','protocol_login(&session,"127.0.0.1",server_port,getenv("RS2_USERNAME")?getenv("RS2_USERNAME"):"antigravity",getenv("RS2_PASSWORD")?getenv("RS2_PASSWORD"):"antigravity",false)')
idx=s.index('    SDL_StartTextInput();');s=s[:idx]+'''    MenuSelection selection={0};comp_cfg.selection=&selection;
    ClientActions actions={&session,&comp_cfg,&varp_ctx,&obj_cfg,&selection,&chatbox,
        &viewport_interface_id,&chat_interface_id,live_server,true};
'''+s[idx:]
start=s.index('                if (event.button.button == SDL_BUTTON_RIGHT)');end=s.index('\n        // 50 Hz Client Logic Tick Accumulator',start)
# Replace the complete mouse button branch, retaining the event-loop closing braces.
s=s[:start]+'''                mouse_x=mx;mouse_y=my;
                actions.live=live_server;
                if(event.button.button==SDL_BUTTON_RIGHT) {
                    if(has_fonts && menu.size>0) context_menu_open(&menu,mx,my,&font_b12);
                } else if(event.button.button==SDL_BUTTON_LEFT) {
                    MenuAction action={.type=MENU_CANCEL};bool chosen=false;
                    if(menu.visible) chosen=context_menu_pick_action(&menu,mx,my,&action);
                    else {
                        int tab=-1;
                        const int left[14]={549,579,607,635,676,704,732,550,582,609,637,679,706,734};
                        const int right[14]={583,609,637,679,706,734,766,584,612,639,681,709,736,768};
                        for(int i=0;i<14;i++) {
                            bool row=i<7 ? my>=194 && my<231 : my>=492 && my<529;
                            if(row && mx>=left[i] && mx<=right[i] && tab_interface_id[i]!=-1) {tab=i;break;}
                        }
                        if(tab>=0) {selected_tab=tab;selection.item_selected=false;}
                        else if(mx<520 && my>=490 && my<532) {
                            if(mx>=8 && mx<=108) public_chat_setting=(public_chat_setting+1)%4;
                            else if(mx>=137 && mx<=237) private_chat_setting=(private_chat_setting+1)%3;
                            else if(mx>=275 && mx<=375) trade_chat_setting=(trade_chat_setting+1)%3;
                            if(live_server) protocol_send_chat_setmode(&session,public_chat_setting,private_chat_setting,trade_chat_setting);
                        } else {
                            if(menu.size>0) {action=menu.actions[menu.size-1];chosen=true;}
                        }
                    }
                    if(chosen) {
                        ActionResult result=client_action_dispatch(&actions,action);
                        if(result==ACTION_WALK && viewport_interface_id==-1) {
                            pending_click=true;pending_click_x=mx;pending_click_y=my;pending_click_mode=1;
                        } else if(result==ACTION_UNAVAILABLE) {
                            fprintf(stderr,"[ACTION] Unsupported action type %d; no packet sent\\n",action.type);
                        }
                    }
                }
            }
        }
''' +s[end:]
# Freeze an open menu so its data cannot be overwritten while the user selects.
start=s.index('        // Dynamically build context menu');idx=s.index('        context_menu_clear(&menu);',start);s=s[:idx]+'        if(!menu.visible) {\n'+s[idx:];idx=s.index('        static char tooltip_buf',idx);s=s[:idx]+'        }\n\n'+s[idx:]
# Move all live path wire construction to a single source-derived encoder.
import re
pattern=r'''if \(live_server && res.step_count > 1\) \{.*?protocol_send_walk\(&session, res.steps_x\[0\], res.steps_z\[0\], count, rel_x, rel_z, false\);\s*\}'''
s,n=re.subn(pattern,'''if(live_server) {
                            if(!protocol_send_route(&session,streamer.base_tile_x,streamer.base_tile_z,
                                res.steps_x,res.steps_z,res.step_count,false)) fprintf(stderr,"[ACTION] Route send failed\\\\n");
                        }''',s,flags=re.S);assert n==2,n
# Keep demo NPC rendering and dots out of live sessions.
s=s.replace('        if (hans_rest_model) {','        if (offline && hans_rest_model) {')
s=s.replace('                minimap_draw_dot(player.base.x, player.base.z, hans.base.x, hans.base.z, camera_yaw, 561, 5, media.mapdots[1]);','                if(offline) minimap_draw_dot(player.base.x,player.base.z,hans.base.x,hans.base.z,camera_yaw,561,5,media.mapdots[1]);')
s=s.replace('mouse_picker_build_menu(&picker, &menu, &npc_cfg, &loc_cfg, &obj_cfg, &hans, 1, &player, 1, player.combat_level);','mouse_picker_build_menu(&picker,&menu,&npc_cfg,&loc_cfg,&obj_cfg,offline?&hans:NULL,offline?1:0,&player,1,player.combat_level);')
p.write_text(s)
