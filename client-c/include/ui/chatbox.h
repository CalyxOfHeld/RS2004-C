#ifndef RS2_CHATBOX_H
#define RS2_CHATBOX_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "gfx/pixfont.h"
#include "gfx/pix8.h"

#define CHATBOX_MAX_MESSAGES 100
#define CHATBOX_MAX_INPUT_LEN 80

typedef struct {
    char *text[CHATBOX_MAX_MESSAGES];
    char *sender[CHATBOX_MAX_MESSAGES];
    int   type[CHATBOX_MAX_MESSAGES];
    int   message_count;

    char  input[CHATBOX_MAX_INPUT_LEN + 1];
    int   input_len;

    char  username[32];
    int   scroll_offset;
    bool  cursor_visible;
    int   cursor_cycle;
    bool  count_dialog_open;
    char  count_dialog_input[16];
} Chatbox;

void chatbox_init(Chatbox *chat, const char *username);
void chatbox_open_count_dialog(Chatbox *chat);
void chatbox_close_count_dialog(Chatbox *chat);
int chatbox_count_value(const Chatbox *chat);
void chatbox_add_message(Chatbox *chat, int type, const char *sender, const char *text);
void chatbox_key_press(Chatbox *chat, char ch);
void chatbox_backspace(Chatbox *chat);
const char *chatbox_submit(Chatbox *chat);
const char *chatbox_take_input(Chatbox *chat);
void chatbox_draw(Chatbox *chat, Pix8 *chatback, int x, int y, PixFont *font_p12);
void chatbox_free(Chatbox *chat);

#endif // RS2_CHATBOX_H
