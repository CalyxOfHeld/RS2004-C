from pathlib import Path
p=Path('include/ui/context_menu.h');s=p.read_text().replace('typedef struct ContextMenu {','''typedef enum {
    MENU_LEGACY, MENU_CANCEL, MENU_WALK, MENU_IF_BUTTON, MENU_IF_CLOSE,
    MENU_IF_CONTINUE, MENU_HELD_OP, MENU_INV_BUTTON, MENU_ITEM_SELECT,
    MENU_SPELL_SELECT, MENU_ITEM_ON_ITEM, MENU_ITEM_EXAMINE, MENU_NPC_OP,
    MENU_NPC_EXAMINE, MENU_LOC_OP, MENU_LOC_EXAMINE, MENU_OBJ_OP,
    MENU_OBJ_EXAMINE, MENU_PLAYER_OP
} MenuActionType;
typedef struct {
    MenuActionType type;
    int operation, target, component, slot, x, z;
} MenuAction;
typedef struct {
    bool item_selected;
    int item, slot, component;
    char name[64];
} MenuSelection;

typedef struct ContextMenu {''').replace('    int action_ids[MAX_MENU_OPTIONS];','    int action_ids[MAX_MENU_OPTIONS]; /* compatibility with old fixture tests only */\n    MenuAction actions[MAX_MENU_OPTIONS];')
s=s.replace('void context_menu_open(','''void context_menu_add_action(ContextMenu *menu, const char *option, MenuAction action);
bool context_menu_pick_action(ContextMenu *menu,int x,int y,MenuAction *action);
void context_menu_open(''');p.write_text(s)
p=Path('src/ui/context_menu.c');s=p.read_text().replace('    menu->action_ids[menu->size] = action_id;','    menu->actions[menu->size] = (MenuAction){MENU_LEGACY,0,action_id,0,0,0,0};\n    menu->action_ids[menu->size] = action_id;');idx=s.index('void context_menu_open(');s=s[:idx]+'''void context_menu_add_action(ContextMenu *menu,const char *option,MenuAction action) {
    if(!menu || menu->size>=MAX_MENU_OPTIONS) return;
    int index=menu->size;
    context_menu_add_option(menu,option,0);
    if(menu->size>index) menu->actions[index]=action;
}
bool context_menu_pick_action(ContextMenu *menu,int x,int y,MenuAction *action) {
    if(!menu || !menu->visible || !action) return false;
    int index=-1;
    if(x>=menu->x && x<menu->x+menu->width && y>=menu->y+18 && y<menu->y+menu->height) {
        int row=(y-menu->y-18)/15;
        if(row<menu->size) index=menu->size-1-row;
    }
    menu->visible=false;
    if(index<0) return false;
    *action=menu->actions[index];return true;
}

'''+s[idx:];p.write_text(s)
p=Path('include/ui/component.h');s=p.read_text().replace('struct VarpContext;','struct VarpContext;\nstruct MenuSelection;').replace('    struct VarpContext *varp_ctx;','    struct VarpContext *varp_ctx;\n    struct MenuSelection *selection;');# need named struct MenuSelection
p.write_text(s)
p=Path('include/ui/context_menu.h');s=p.read_text().replace('typedef struct {\n    bool item_selected;', 'typedef struct MenuSelection {\n    bool item_selected;');p.write_text(s)
p=Path('src/ui/component.c');s=p.read_text();start=s.index('                            char opt_buf[128];',s.index('bool component_build_menu'));end=s.index('\n                        }',start)
s=s[:start]+'''                            char opt_buf[128];
                            MenuAction action={MENU_ITEM_EXAMINE,0,obj_id,child->id,slot,0,0};
                            snprintf(opt_buf,sizeof(opt_buf),"Examine @lre@%s",name);
                            context_menu_add_action(menu,opt_buf,action);
                            MenuSelection *selected=cfg->selection;
                            if(selected && selected->item_selected && child->interactable) {
                                if(selected->component!=child->id || selected->slot!=slot) {
                                    action.type=MENU_ITEM_ON_ITEM;
                                    snprintf(opt_buf,sizeof(opt_buf),"Use %.40s with @lre@%.40s",selected->name,name);
                                    context_menu_add_action(menu,opt_buf,action);
                                }
                            } else {
                                if(child->interactable) for(int o=4;o>=3;o--) {
                                    const char *op=has_obj && obj.iop[o][0] ? obj.iop[o] : o==4 ? "Drop" : NULL;
                                    if(op) {
                                        action.type=MENU_HELD_OP;action.operation=o+1;
                                        snprintf(opt_buf,sizeof(opt_buf),"%s @lre@%s",op,name);
                                        context_menu_add_action(menu,opt_buf,action);
                                    }
                                }
                                if(child->usable) {
                                    action.type=MENU_ITEM_SELECT;
                                    snprintf(opt_buf,sizeof(opt_buf),"Use @lre@%s",name);
                                    context_menu_add_action(menu,opt_buf,action);
                                }
                                if(child->interactable && has_obj) for(int o=2;o>=0;o--) {
                                    if(obj.iop[o][0]) {
                                        action.type=MENU_HELD_OP;action.operation=o+1;
                                        snprintf(opt_buf,sizeof(opt_buf),"%s @lre@%s",obj.iop[o],name);
                                        context_menu_add_action(menu,opt_buf,action);
                                    }
                                }
                                for(int o=4;o>=0;o--) if(child->iops[o]) {
                                    action.type=MENU_INV_BUTTON;action.operation=o+1;
                                    snprintf(opt_buf,sizeof(opt_buf),"%s @lre@%s",child->iops[o],name);
                                    context_menu_add_action(menu,opt_buf,action);
                                }
                            }
                            if(has_obj) obj_free_entry(&obj);'''+s[end:]
# Preserve component IDs as data, never as encoded numeric action ranges.
s=s.replace('context_menu_add_option(menu, child->option, 1000 + child->id);','context_menu_add_action(menu,child->option,(MenuAction){MENU_IF_BUTTON,1,0,child->id,0,0,0});').replace('context_menu_add_option(menu, opt, 2000 + child->id);','context_menu_add_action(menu,opt,(MenuAction){MENU_SPELL_SELECT,2,0,child->id,0,0,0});').replace('context_menu_add_option(menu, "Close", 3000 + child->id);','context_menu_add_action(menu,"Close",(MenuAction){MENU_IF_CLOSE,0,0,child->id,0,0,0});').replace('context_menu_add_option(menu, child->option, 4000 + child->id);','context_menu_add_action(menu,child->option,(MenuAction){MENU_IF_BUTTON,child->button_type,0,child->id,0,0,0});').replace('context_menu_add_option(menu, child->option ? child->option : "Click here to continue", 6000 + child->id);','context_menu_add_action(menu,child->option ? child->option : "Click here to continue",(MenuAction){MENU_IF_CONTINUE,0,0,child->id,0,0,0});');s=s.replace('        if (!child) continue;\n        int vx', '        if (!child || child->hide) continue;\n        int vx');p.write_text(s)
