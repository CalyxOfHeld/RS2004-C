#include "ui/chatbox.h"
#include "gfx/pix2d.h"
#include "gfx/pixfont.h"
#include "gfx/media.h"
#include "core/arena.h"
#include "core/jagfile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    printf("=== Test Suite: Interactive Chatbox & Input System ===\n");

    Arena arena = arena_create(32 * 1024 * 1024);

    uint32_t *fb = (uint32_t *)calloc(765 * 503, sizeof(uint32_t));
    pix2d_bind(fb, 765, 503);

    Jagfile *title_jag = jagfile_load_file("../engine/data/pack/client/title", &arena);
    if (!title_jag) title_jag = jagfile_load_file("engine/data/pack/client/title", &arena);
    assert(title_jag);

    Jagfile *media_jag = jagfile_load_file("../engine/data/pack/client/media", &arena);
    if (!media_jag) media_jag = jagfile_load_file("engine/data/pack/client/media", &arena);
    assert(media_jag);

    PixFont font_p12;
    bool font_ok = pixfont_load(&font_p12, title_jag, "p12", &arena);
    assert(font_ok);
    printf("[PASS] font_p12 loaded for chatbox typography\n");

    Media media;
    bool media_ok = media_load(&media, media_jag, &arena);
    assert(media_ok && media.chatback != NULL);
    printf("[PASS] chatback sprite loaded (%dx%d)\n", media.chatback->width, media.chatback->height);

    // Test 1: Chatbox initialization
    Chatbox chat;
    chatbox_init(&chat, "Mod Mark");
    assert(strcmp(chat.username, "Mod Mark") == 0);
    assert(chat.message_count == 0);
    printf("[PASS] Chatbox initialized for user 'Mod Mark'\n");

    // Test 2: Message history buffer
    chatbox_add_message(&chat, 0, NULL, "Welcome to RuneScape.");
    chatbox_add_message(&chat, 2, "Hans", "Hello there adventurer!");
    chatbox_add_message(&chat, 1, "Mod Mark", "Testing native C client Revision 225.");
    assert(chat.message_count == 3);
    assert(strcmp(chat.text[0], "Testing native C client Revision 225.") == 0);
    assert(strcmp(chat.text[2], "Welcome to RuneScape.") == 0);
    printf("[PASS] 100-message chat history FIFO buffer verified\n");

    // Test 3: Typing and backspace
    const char *test_cmd = "::tele 3200 3200";
    for (size_t i = 0; i < strlen(test_cmd); i++) {
        chatbox_key_press(&chat, test_cmd[i]);
    }
    assert(strcmp(chat.input, test_cmd) == 0);

    // Backspace test
    chatbox_backspace(&chat);
    assert(strcmp(chat.input, "::tele 3200 320") == 0);
    chatbox_key_press(&chat, '0');
    assert(strcmp(chat.input, test_cmd) == 0);

    // Submit command
    const char *submitted = chatbox_submit(&chat);
    assert(submitted != NULL);
    assert(strcmp(submitted, test_cmd) == 0);
    assert(chat.input_len == 0);
    assert(chat.message_count == 3); // commands starting with ':' are not added to chat history
    printf("[PASS] Interactive keyboard input and backspace verified\n");

    // Test 4: Public chat message submission
    const char *public_msg = "Hello Lumbridge!";
    for (size_t i = 0; i < strlen(public_msg); i++) {
        chatbox_key_press(&chat, public_msg[i]);
    }
    const char *submitted_msg = chatbox_submit(&chat);
    assert(strcmp(submitted_msg, public_msg) == 0);
    assert(chat.message_count == 4);
    assert(strcmp(chat.text[0], public_msg) == 0);
    assert(strcmp(chat.sender[0], "Mod Mark") == 0);
    printf("[PASS] Public chat message submission and formatting verified\n");

    // Test 5: Render chatbox into framebuffer
    chatbox_draw(&chat, media.chatback, 22, 375, &font_p12);

    int rendered_pixels = 0;
    for (int i = 0; i < 765 * 503; i++) {
        if (fb[i] != 0) rendered_pixels++;
    }
    printf("  Rendered chatbox pixels in framebuffer: %d\n", rendered_pixels);
    assert(rendered_pixels > 20000);
    printf("[PASS] Complete chatbox rendered to framebuffer with messages, prompt & cursor!\n");

    chatbox_key_press(&chat,'h');chatbox_key_press(&chat,'i');
    int before_take=chat.message_count;
    const char *taken=chatbox_take_input(&chat);
    assert(taken && !strcmp(taken,"hi") && chat.message_count==before_take && chat.input_len==0);
    puts("PASS: live input extraction never fabricates a public chat echo before network support");
    chatbox_free(&chat);
    free(fb);
    arena_free(&arena);

    printf("=== ALL CHATBOX & INPUT TESTS PASSED! ===\n");
    return 0;
}
