#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "ui/component.h"
#include "ui/component_packets.h"
#include "ui/chatbox.h"
#include "net/protocol.h"
#include "core/arena.h"

int main(void) {
    printf("--- Running Bank & Interface Enhancements Test ---\n");

    InterfaceState panels;
    interface_state_init(&panels);
    const uint8_t bank_panels[]={0x14,0xac,0x07,0xd5}; // 5292 / 2005
    assert(interface_state_apply(&panels,28,bank_panels,4)==1);
    assert(panels.viewport==5292 && panels.sidebar==2005 && panels.chat==-1);
    InterfaceState previous=panels;
    assert(interface_state_apply(&panels,28,bank_panels,3)==-1);
    assert(!memcmp(&panels,&previous,sizeof(panels)));
    assert(interface_state_apply(&panels,129,NULL,0)==1);
    assert(panels.viewport==-1 && panels.sidebar==-1 && panels.chat==-1);
    assert(interface_state_apply(&panels,28,bank_panels,4)==1);
    const uint8_t dialog[]={0x13,0x12}; // npcchat1 4882
    assert(interface_state_apply(&panels,14,dialog,2)==1);
    assert(panels.chat==4882 && panels.sidebar==-1 && panels.viewport==-1);
    assert(interface_state_apply(&panels,195,bank_panels+2,2)==1);
    assert(panels.chat==-1 && panels.sidebar==2005 && panels.viewport==-1);
    assert(interface_state_apply(&panels,168,bank_panels,2)==1);
    assert(panels.viewport==5292 && panels.sidebar==-1 && panels.chat==-1);
    puts("  [PASS] production bank/chat/side modal transitions and malformed-packet atomic rejection");

    Arena arena = arena_create(16 * 1024 * 1024);

    // 1. Setup mock ComponentConfig and interface hierarchy
    ComponentConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.count = 6000;
    cfg.instances = (Component *)arena_alloc(&arena, cfg.count * sizeof(Component));
    memset(cfg.instances, 0, cfg.count * sizeof(Component));

    // Parent Bank container (id 5292)
    Component *bank = &cfg.instances[5292];
    bank->id = 5292;
    bank->type = COMPONENT_TYPE_CONTAINER;
    bank->width = 480;
    bank->height = 320;
    bank->child_count = 1;
    bank->child_id = (int *)arena_alloc(&arena, sizeof(int));
    bank->child_x = (int *)arena_alloc(&arena, sizeof(int));
    bank->child_y = (int *)arena_alloc(&arena, sizeof(int));
    bank->child_id[0] = 5382;
    bank->child_x[0] = 20;
    bank->child_y[0] = 30;

    // Bank Item Grid (id 5382)
    Component *grid = &cfg.instances[5382];
    grid->id = 5382;
    grid->type = COMPONENT_TYPE_INV_GRID;
    grid->x = 4;
    grid->y = 6;
    grid->width = 8;
    grid->height = 6;
    grid->margin_x = 12;
    grid->margin_y = 8;
    grid->draggable = true;
    grid->interactable = true;

    int total_slots = grid->width * grid->height; // 48 slots
    grid->inv_slot_obj_id = (int *)arena_alloc(&arena, total_slots * sizeof(int));
    grid->inv_slot_obj_count = (int *)arena_alloc(&arena, total_slots * sizeof(int));
    memset(grid->inv_slot_obj_id, 0, total_slots * sizeof(int));
    memset(grid->inv_slot_obj_count, 0, total_slots * sizeof(int));

    // Place 995 (Coins) in slot 0, 1205 (Bronze dagger) in slot 5
    grid->inv_slot_obj_id[0] = 995 + 1;
    grid->inv_slot_obj_count[0] = 1000000;
    grid->inv_slot_obj_id[5] = 1205 + 1;
    grid->inv_slot_obj_count[5] = 1;

    // 2. Verify recursive slot picking (component_find_inv_slot)
    // Slot 0 absolute position: bank(0,0) + child(20,30) + grid(4,6) + col 0 (0), row 0 (0) = (24, 36)
    int found_slot = -1;
    Component *found_com = component_find_inv_slot(&cfg, bank, 24 + 10, 36 + 10, 0, 0, 0, &found_slot);
    assert(found_com == grid);
    assert(found_slot == 0);
    printf("  [PASS] component_find_inv_slot found slot 0 at (34, 46)\n");

    // Slot 5 position: col 5 -> 24 + 5 * (12+32) = 24 + 220 = 244, row 0 -> 36
    found_slot = -1;
    found_com = component_find_inv_slot(&cfg, bank, 244 + 5, 36 + 5, 0, 0, 0, &found_slot);
    assert(found_com == grid);
    assert(found_slot == 5);
    printf("  [PASS] component_find_inv_slot found slot 5 at (249, 41)\n");

    // Out of bounds
    found_slot = -1;
    found_com = component_find_inv_slot(&cfg, bank, 10, 10, 0, 0, 0, &found_slot);
    assert(found_com == NULL);
    assert(found_slot == -1);
    printf("  [PASS] component_find_inv_slot correctly rejected out-of-bounds click\n");

    // 3. Test item drag lifecycle and local slot swap
    cfg.drag.active = true;
    cfg.drag.threshold = false;
    cfg.drag.cycles = 0;
    cfg.drag.component_id = grid->id;
    cfg.drag.slot = 0;
    cfg.drag.grab_x = 34;
    cfg.drag.grab_y = 46;

    // Simulate drag movement
    int drag_cur_x = 249;
    int drag_cur_y = 41;
    if (abs(drag_cur_x - cfg.drag.grab_x) > 5 || abs(drag_cur_y - cfg.drag.grab_y) > 5) {
        cfg.drag.threshold = true;
    }
    assert(cfg.drag.threshold == true);

    // Simulate 5 ticks
    for (int t = 0; t < 5; t++) cfg.drag.cycles++;
    assert(cfg.drag.cycles >= 5);

    // Release on destination slot 5
    int dest_slot = -1;
    Component *dest_com = component_find_inv_slot(&cfg, bank, drag_cur_x, drag_cur_y, 0, 0, 0, &dest_slot);
    assert(dest_com == grid);
    assert(dest_slot == 5);

    assert(component_drag_release(&cfg,dest_com,dest_slot));
    assert(!cfg.drag.active);

    // Check slot 0 now has Dagger, slot 5 has Coins
    assert(grid->inv_slot_obj_id[0] == 1205 + 1);
    assert(grid->inv_slot_obj_count[0] == 1);
    assert(grid->inv_slot_obj_id[5] == 995 + 1);
    assert(grid->inv_slot_obj_count[5] == 1000000);
    printf("  [PASS] Item drag swap verified: slot 0 and slot 5 exchanged successfully\n");

    for(int scenario=0;scenario<4;scenario++) {
        int before_id=grid->inv_slot_obj_id[0];
        cfg.drag=(ComponentDragState){.active=true,.threshold=true,.cycles=5,
            .component_id=grid->id,.slot=0};
        if(scenario==0) cfg.drag.cycles=4;
        if(scenario==1) cfg.drag.threshold=false;
        if(scenario==2) cfg.drag.component_id=grid->id+1;
        assert(!component_drag_release(&cfg,grid,scenario==3?total_slots:5));
        assert(!cfg.drag.active && grid->inv_slot_obj_id[0]==before_id);
    }
    puts("  [PASS] short clicks, unmoved pointer, another component and out-of-range drop do not swap");

    // 4. Test Chatbox Amount Input Dialog (P_COUNTDIALOG)
    Chatbox chatbox;
    chatbox_init(&chatbox, "Player");
    assert(!chatbox.count_dialog_open);

    chatbox_open_count_dialog(&chatbox);
    assert(chatbox.count_dialog_open);
    assert(strlen(chatbox.count_dialog_input) == 0);

    // Filter characters: only digits accepted
    const char *test_input = "5a0B0";
    for (size_t i = 0; i < strlen(test_input); i++) {
        char ch = test_input[i];
        chatbox_key_press(&chatbox,ch);
    }
    assert(strcmp(chatbox.count_dialog_input, "500") == 0);
    printf("  [PASS] Chatbox quantity dialog input filtered non-digits: input='500'\n");

    assert(chatbox_count_value(&chatbox)==500);
    chatbox_open_count_dialog(&chatbox);
    const char *max_count="2147483647";
    for(const char *c=max_count;*c;c++) chatbox_key_press(&chatbox,*c);
    assert(chatbox_count_value(&chatbox)==2147483647);
    chatbox_key_press(&chatbox,'8');
    assert(strlen(chatbox.count_dialog_input)==10);
    chatbox_backspace(&chatbox);chatbox_key_press(&chatbox,'8');
    assert(chatbox_count_value(&chatbox)==0);
    puts("  [PASS] production quantity input filters digits, limits length and rejects 32-bit overflow");

    chatbox_close_count_dialog(&chatbox);
    assert(!chatbox.count_dialog_open);

    // 5. Test Scrollbar Drag and Bounds
    Component scroll_com;
    memset(&scroll_com, 0, sizeof(scroll_com));
    scroll_com.scroll = 600;
    scroll_com.height = 300;
    scroll_com.scroll_position = 0;

    // Down arrow click at (sx=100, sy + 300 - 8 = 392)
    bool scrolled = component_handle_scroll(&scroll_com, 108, 392, 100, 100, 1);
    assert(scrolled);
    assert(scroll_com.scroll_position == 4); // drag_cycles * 4

    // Drag to track
    scrolled = component_handle_scroll(&scroll_com, 108, 250, 100, 100, 1);
    assert(scrolled);
    assert(scroll_com.scroll_position > 4);
    assert(scroll_com.scroll_position <= 300); // cannot exceed scroll - height

    // Up arrow click at (sx=100, sy=108)
    scrolled = component_handle_scroll(&scroll_com, 108, 108, 100, 100, 1);
    assert(scrolled);
    printf("  [PASS] Component scrollbar click & drag clamping verified\n");

    arena_free(&arena);
    printf("=== ALL BANK & INTERFACE ENHANCEMENT TESTS PASSED ===\n");
    return 0;
}
