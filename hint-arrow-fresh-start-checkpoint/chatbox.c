#include "ui/chatbox.h"
#include "gfx/pix2d.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void chatbox_init(Chatbox *chat, const char *username) {
    if (!chat) return;
    memset(chat, 0, sizeof(Chatbox));
    if (username && username[0] != '\0') {
        strncpy(chat->username, username, sizeof(chat->username) - 1);
    } else {
        strncpy(chat->username, "Player", sizeof(chat->username) - 1);
    }
}

void chatbox_open_count_dialog(Chatbox *chat) {
    if (!chat) return;
    chat->count_dialog_open = true;
    chat->count_dialog_input[0] = '\0';
}

void chatbox_close_count_dialog(Chatbox *chat) {
    if (!chat) return;
    chat->count_dialog_open = false;
    chat->count_dialog_input[0] = '\0';
}

void chatbox_add_message(Chatbox *chat, int type, const char *sender, const char *text) {
    if (!chat || !text) return;

    // Free oldest message if at capacity
    if (chat->text[CHATBOX_MAX_MESSAGES - 1]) {
        free(chat->text[CHATBOX_MAX_MESSAGES - 1]);
        chat->text[CHATBOX_MAX_MESSAGES - 1] = NULL;
    }
    if (chat->sender[CHATBOX_MAX_MESSAGES - 1]) {
        free(chat->sender[CHATBOX_MAX_MESSAGES - 1]);
        chat->sender[CHATBOX_MAX_MESSAGES - 1] = NULL;
    }

    // Shift messages down
    for (int i = CHATBOX_MAX_MESSAGES - 1; i > 0; i--) {
        chat->text[i] = chat->text[i - 1];
        chat->sender[i] = chat->sender[i - 1];
        chat->type[i] = chat->type[i - 1];
    }

    chat->text[0] = strdup(text);
    chat->sender[0] = (sender && sender[0] != '\0') ? strdup(sender) : NULL;
    chat->type[0] = type;

    if (chat->message_count < CHATBOX_MAX_MESSAGES) {
        chat->message_count++;
    }
}

void chatbox_key_press(Chatbox *chat, char ch) {
    if (!chat) return;
    if (chat->count_dialog_open) {
        size_t len = strlen(chat->count_dialog_input);
        if (ch >= '0' && ch <= '9' && len < sizeof(chat->count_dialog_input) - 1) {
            chat->count_dialog_input[len] = ch;
            chat->count_dialog_input[len + 1] = '\0';
        }
        return;
    }
    if (ch >= 32 && ch <= 126 && chat->input_len < CHATBOX_MAX_INPUT_LEN) {
        chat->input[chat->input_len++] = ch;
        chat->input[chat->input_len] = '\0';
    }
}

void chatbox_backspace(Chatbox *chat) {
    if (!chat) return;
    if (chat->count_dialog_open) {
        size_t len = strlen(chat->count_dialog_input);
        if (len > 0) {
            chat->count_dialog_input[len - 1] = '\0';
        }
        return;
    }
    if (chat->input_len > 0) {
        chat->input[--chat->input_len] = '\0';
    }
}

static const char *submit_input(Chatbox *chat,bool echo) {
    if (!chat || chat->input_len == 0) return NULL;

    static char s_submitted[CHATBOX_MAX_INPUT_LEN + 1];
    strncpy(s_submitted, chat->input, sizeof(s_submitted) - 1);
    s_submitted[sizeof(s_submitted) - 1] = '\0';

    if (echo && s_submitted[0] != ':') {
        // Normal public chat message
        chatbox_add_message(chat, 2, chat->username, s_submitted);
    }

    chat->input[0] = '\0';
    chat->input_len = 0;
    return s_submitted;
}

const char *chatbox_submit(Chatbox *chat) {return submit_input(chat,true);}
const char *chatbox_take_input(Chatbox *chat) {return submit_input(chat,false);}

void chatbox_draw(Chatbox *chat, Pix8 *chatback, int x, int y, PixFont *font_p12) {
    if (!chat || !font_p12) return;

    // Draw background shell if provided
    if (chat->count_dialog_open) {
        if (chatback) pix8_draw(chatback, x, y);
        pixfont_draw_string(font_p12, x + 190, y + 35, 0x000000, "Enter amount:");
        chat->cursor_cycle++;
        char typing_buf[32];
        if ((chat->cursor_cycle / 15) % 2 == 0) {
            snprintf(typing_buf, sizeof(typing_buf), "%s*", chat->count_dialog_input);
        } else {
            snprintf(typing_buf, sizeof(typing_buf), "%s", chat->count_dialog_input);
        }
        pixfont_draw_string(font_p12, x + 220, y + 55, 0x0000FF, typing_buf);
        return;
    }

    if (chatback) {
        pix8_draw(chatback, x, y);
    }

    // Save clip state
    int32_t prev_l = pix2d_clip_left;
    int32_t prev_r = pix2d_clip_right;
    int32_t prev_t = pix2d_clip_top;
    int32_t prev_b = pix2d_clip_bottom;

    // Clip to chat message viewport (y=0..77 relative to chatback)
    pix2d_set_clip(x, y, x + 463, y + 77);

    int line_idx = 0;
    for (int i = 0; i < CHATBOX_MAX_MESSAGES; i++) {
        if (!chat->text[i]) continue;

        int line_y = y + 70 - line_idx * 14 + chat->scroll_offset;
        if (line_y > y - 5 && line_y < y + 80) {
            int msg_type = chat->type[i];
            const char *sender = chat->sender[i];
            const char *text = chat->text[i];

            if (msg_type == 0) {
                // Server message (black)
                pixfont_draw_string(font_p12, x + 4, line_y, 0x000000, text);
            } else if (msg_type == 1) {
                // Mod message
                char prefix[64];
                snprintf(prefix, sizeof(prefix), "%s: ", sender ? sender : "Mod");
                pixfont_draw_string(font_p12, x + 4, line_y, 0xFFFFFF, prefix);
                int p_len = pixfont_string_width(font_p12, prefix);
                pixfont_draw_string(font_p12, x + 4 + p_len, line_y, 0x0000FF, text);
            } else if (msg_type == 2) {
                // Public chat (sender in black, message in blue)
                char prefix[64];
                snprintf(prefix, sizeof(prefix), "%s: ", sender ? sender : "Player");
                pixfont_draw_string(font_p12, x + 4, line_y, 0x000000, prefix);
                int p_len = pixfont_string_width(font_p12, prefix);
                pixfont_draw_string(font_p12, x + 4 + p_len, line_y, 0x0000FF, text);
            } else {
                // Other (red / maroon)
                pixfont_draw_string(font_p12, x + 4, line_y, 0x800000, text);
            }
        }
        line_idx++;
    }

    // Restore clipping
    pix2d_set_clip(prev_l, prev_t, prev_r, prev_b);

    // Separator line (at y = 77 relative to chatback)
    pix2d_fill_rect(x, y + 77, 479, 1, 0x000000);

    // Prompt line (at y = 90 relative to chatback)
    char prompt[64];
    snprintf(prompt, sizeof(prompt), "%s: ", chat->username);
    pixfont_draw_string(font_p12, x + 4, y + 90, 0x000000, prompt);

    int prompt_w = pixfont_string_width(font_p12, prompt);

    // Blinking cursor
    chat->cursor_cycle++;
    char typing_buf[CHATBOX_MAX_INPUT_LEN + 4];
    if ((chat->cursor_cycle / 15) % 2 == 0) {
        snprintf(typing_buf, sizeof(typing_buf), "%s*", chat->input);
    } else {
        snprintf(typing_buf, sizeof(typing_buf), "%s", chat->input);
    }
    pixfont_draw_string(font_p12, x + 4 + prompt_w, y + 90, 0x0000FF, typing_buf);
}

void chatbox_free(Chatbox *chat) {
    if (!chat) return;
    for (int i = 0; i < CHATBOX_MAX_MESSAGES; i++) {
        if (chat->text[i]) {
            free(chat->text[i]);
            chat->text[i] = NULL;
        }
        if (chat->sender[i]) {
            free(chat->sender[i]);
            chat->sender[i] = NULL;
        }
    }
    chat->message_count = 0;
}
