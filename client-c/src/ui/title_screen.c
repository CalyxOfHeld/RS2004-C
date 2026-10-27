#include "ui/title_screen.h"
#include "gfx/jpeg.h"
#include "gfx/pix2d.h"
#include "core/jrandom.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Sizes and screen positions from client.loadTitle and drawTitleScreen.
static const int32_t title_image_layout[TITLE_IMAGE_COUNT][4] = {
    {128, 265,   0,   0}, {128, 265, 661,   0}, {533, 186, 128,   0},
    {360, 146, 214, 386}, {360, 200, 214, 186}, {214, 267,   0, 265},
    {215, 267, 574, 265}, { 86,  79, 128, 186}, { 87,  79, 574, 186}
};
// loadTitleBackground blitOpaque offsets, before and after the mirror pass.
static const int32_t title_background_offsets[TITLE_IMAGE_COUNT][2] = {
    {0, 0}, {-661, 0}, {-128, 0}, {-214, -386}, {-214, -186},
    {0, -265}, {-574, -265}, {-128, -186}, {-574, -186}
};
static const int32_t title_mirror_offsets[TITLE_IMAGE_COUNT][2] = {
    {394, 0}, {-267, 0}, {266, 0}, {180, -386}, {180, -186},
    {394, -265}, {-180, -265}, {212, -186}, {-180, -186}
};
static const char title_charset[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!\"\xA3$%^&*()-_=+[{]};:'@#~,<.>/?\\| ";

// The flame code draws from the same shared Math.random() generator as the rest of the client.
static double title_random(TitleScreen *title) {
    (void)title;
    return math_random();
}

static void title_blit_opaque(TitleImage *dst, const uint32_t *src, int32_t sw, int32_t sh, int32_t x, int32_t y) {
    for (int32_t row = 0; row < sh; row++) {
        int32_t dy = y + row;
        if (dy < 0 || dy >= dst->height) continue;
        for (int32_t col = 0; col < sw; col++) {
            int32_t dx = x + col;
            if (dx >= 0 && dx < dst->width) dst->pixels[dy * dst->width + dx] = src[row * sw + col];
        }
    }
}

// Java int arithmetic: the sum wraps at 32 bits and ">> 8" is arithmetic.
static uint32_t title_shift8(uint32_t sum) {
    return (sum >> 8) | ((sum & 0x80000000u) ? 0xFF000000u : 0u);
}

// client.mix
static int32_t title_mix(int32_t src, int32_t alpha, int32_t dst) {
    uint32_t inv = (uint32_t)(256 - alpha), a = (uint32_t)alpha;
    return (int32_t)title_shift8(((((uint32_t)src & 0xFF00FF) * inv + ((uint32_t)dst & 0xFF00FF) * a) & 0xFF00FF00u) +
                                 ((((uint32_t)src & 0xFF00) * inv + ((uint32_t)dst & 0xFF00) * a) & 0xFF0000u));
}

void title_screen_init(TitleScreen *title) {
    memset(title, 0, sizeof(*title));
}

// updateFlameBuffer: diffuse 5000 random seeds, then cut out a rune shape.
static void title_update_flame_buffer(TitleScreen *title, const Pix8 *rune) {
    memset(title->flame_buffer0, 0, 32768 * sizeof(int32_t));
    for (int i = 0; i < 5000; i++) {
        int index = (int)(title_random(title) * 128.0 * 256.0);
        title->flame_buffer0[index] = (int)(title_random(title) * 256.0);
    }
    for (int pass = 0; pass < 20; pass++) {
        for (int y = 1; y < 255; y++) for (int x = 1; x < 127; x++) {
            int i = x + (y << 7);
            title->flame_buffer1[i] = (title->flame_buffer0[i - 1] + title->flame_buffer0[i + 1] +
                                       title->flame_buffer0[i - 128] + title->flame_buffer0[i + 128]) / 4;
        }
        int32_t *swap = title->flame_buffer0;
        title->flame_buffer0 = title->flame_buffer1;
        title->flame_buffer1 = swap;
    }
    if (!rune) return;
    int offset = 0;
    for (int y = 0; y < rune->height; y++) for (int x = 0; x < rune->width; x++) {
        if (rune->pixels[offset++] != 0) {
            int i = (x + 16 + rune->crop_x) + ((y + 16 + rune->crop_y) << 7);
            if (i >= 0 && i < 32768) title->flame_buffer0[i] = 0;
        }
    }
}

bool title_screen_load(TitleScreen *title, Jagfile *title_jag) {
    if (title->loaded) return true;
    if (!title_jag) return false;
    for (int i = 0; i < TITLE_IMAGE_COUNT; i++) {
        TitleImage *image = &title->images[i];
        image->width = title_image_layout[i][0]; image->height = title_image_layout[i][1];
        image->x = title_image_layout[i][2]; image->y = title_image_layout[i][3];
        image->pixels = (uint32_t *)calloc((size_t)image->width * image->height, sizeof(uint32_t));
        if (!image->pixels) { title_screen_unload(title); return false; }
    }
    title->loaded = true;

    // loadTitleBackground
    size_t size = 0;
    uint8_t *jpeg = jagfile_read(title_jag, "title.dat", NULL, &size, NULL);
    int32_t bw = 0, bh = 0; uint32_t *background = NULL;
    bool decoded = jpeg && jpeg_decode(jpeg, size, &bw, &bh, &background);
    free(jpeg);
    if (!decoded) { title_screen_unload(title); return false; }
    for (int i = 0; i < TITLE_IMAGE_COUNT; i++)
        title_blit_opaque(&title->images[i], background, bw, bh, title_background_offsets[i][0], title_background_offsets[i][1]);
    for (int y = 0; y < bh; y++) for (int x = 0; x < bw / 2; x++) {
        uint32_t *row = background + (size_t)y * bw, swap = row[x];
        row[x] = row[bw - x - 1]; row[bw - x - 1] = swap;
    }
    for (int i = 0; i < TITLE_IMAGE_COUNT; i++)
        title_blit_opaque(&title->images[i], background, bw, bh, title_mirror_offsets[i][0], title_mirror_offsets[i][1]);
    free(background);
    Pix32 *logo = pix32_load(title_jag, "logo", 0, NULL);
    if (logo) {
        TitleImage *target = &title->images[2];
        pix2d_bind(target->pixels, target->width, target->height);
        pix32_draw(logo, TITLE_SCREEN_WIDTH / 2 - logo->width / 2 - 128, 18);
        pix32_free(logo);
    }

    // loadTitleImages
    title->titlebox = pix8_load(title_jag, "titlebox", 0, NULL);
    title->titlebutton = pix8_load(title_jag, "titlebutton", 0, NULL);
    for (int i = 0; i < 12; i++) title->runes[i] = pix8_load(title_jag, "runes", i, NULL);
    title->flames_left = (uint32_t *)malloc(33920 * sizeof(uint32_t));
    title->flames_right = (uint32_t *)malloc(33920 * sizeof(uint32_t));
    title->flame_buffer0 = (int32_t *)calloc(32768, sizeof(int32_t));
    title->flame_buffer1 = (int32_t *)calloc(32768, sizeof(int32_t));
    title->flame_buffer2 = (int32_t *)calloc(32768, sizeof(int32_t));
    title->flame_buffer3 = (int32_t *)calloc(32768, sizeof(int32_t));
    if (!logo || !title->titlebox || !title->titlebutton || !title->flames_left || !title->flames_right ||
        !title->flame_buffer0 || !title->flame_buffer1 || !title->flame_buffer2 || !title->flame_buffer3) {
        title_screen_unload(title); return false;
    }
    for (int i = 0; i < 12; i++) if (!title->runes[i]) { title_screen_unload(title); return false; }
    memcpy(title->flames_left, title->images[0].pixels, 33920 * sizeof(uint32_t));
    memcpy(title->flames_right, title->images[1].pixels, 33920 * sizeof(uint32_t));
    for (int i = 0; i < 64; i++) {
        title->flame_gradient0[i] = i * 262144;
        title->flame_gradient0[i + 64] = i * 1024 + 16711680;
        title->flame_gradient0[i + 128] = i * 4 + 16776960;
        title->flame_gradient0[i + 192] = 16777215;
        title->flame_gradient1[i] = i * 1024;
        title->flame_gradient1[i + 64] = i * 4 + 65280;
        title->flame_gradient1[i + 128] = i * 262144 + 65535;
        title->flame_gradient1[i + 192] = 16777215;
        title->flame_gradient2[i] = i * 4;
        title->flame_gradient2[i + 64] = i * 262144 + 255;
        title->flame_gradient2[i + 128] = i * 1024 + 16711935;
        title->flame_gradient2[i + 192] = 16777215;
    }
    title_update_flame_buffer(title, NULL);
    title->flame_active = true;
    title->flame_last_ms = 0;
    return true;
}

void title_screen_unload(TitleScreen *title) {
    // unloadTitle followed by prepareGameScreen releasing imageTitle0..8.
    title->flame_active = false;
    pix8_free(title->titlebox); pix8_free(title->titlebutton);
    title->titlebox = title->titlebutton = NULL;
    for (int i = 0; i < 12; i++) { pix8_free(title->runes[i]); title->runes[i] = NULL; }
    free(title->flame_buffer0); free(title->flame_buffer1); free(title->flame_buffer2); free(title->flame_buffer3);
    title->flame_buffer0 = title->flame_buffer1 = title->flame_buffer2 = title->flame_buffer3 = NULL;
    free(title->flames_left); free(title->flames_right);
    title->flames_left = title->flames_right = NULL;
    for (int i = 0; i < TITLE_IMAGE_COUNT; i++) { free(title->images[i].pixels); title->images[i].pixels = NULL; }
    title->loaded = false;
}

void title_screen_push_key(TitleScreen *title, int key) {
    if (key <= 4) return;   // GameShell.keyPressed only queues codes above 4
    title->keys[title->key_write] = key;
    title->key_write = (title->key_write + 1) & (TITLE_KEY_QUEUE - 1);
}

static int title_poll_key(TitleScreen *title) {
    if (title->key_read == title->key_write) return -1;
    int key = title->keys[title->key_read];
    title->key_read = (title->key_read + 1) & (TITLE_KEY_QUEUE - 1);
    return key;
}

static bool title_button_hit(int click_button, int click_x, int click_y, int x, int y) {
    return click_button == 1 && click_x >= x - 75 && click_x <= x + 75 && click_y >= y - 20 && click_y <= y + 20;
}

static void title_edit(char *text, size_t capacity, int key, bool valid) {
    size_t length = strlen(text);
    if (key == 8 && length > 0) text[--length] = '\0';
    if (valid && length + 1 < capacity) { text[length++] = (char)key; text[length] = '\0'; }
}

TitleAction title_screen_update(TitleScreen *title, int click_button, int click_x, int click_y) {
    TitleAction action = TITLE_IDLE;
    const int sw = TITLE_SCREEN_WIDTH, sh = TITLE_SCREEN_HEIGHT;
    title->field_clicked = false;
    if (title->state == 0) {
        int y = sh / 2 + 20 + 20;
        if (title_button_hit(click_button, click_x, click_y, sw / 2 - 80, y)) {
            title->state = 3; title->login_field = 0;
        }
        if (title_button_hit(click_button, click_x, click_y, sw / 2 + 80, y)) {
            title->message0[0] = '\0';
            snprintf(title->message1, sizeof(title->message1), "Enter your username & password.");
            title->state = 2; title->login_field = 0;
        }
    } else if (title->state == 2) {
        int y = sh / 2 - 40 + 30 + 25;
        if (click_button == 1 && click_y >= y - 15 && click_y < y) { title->login_field = 0; title->field_clicked = true; }
        y += 15;
        if (click_button == 1 && click_y >= y - 15 && click_y < y) { title->login_field = 1; title->field_clicked = true; }
        int button_y = sh / 2 + 50 + 20;
        if (title_button_hit(click_button, click_x, click_y, sw / 2 - 80, button_y)) action = TITLE_LOGIN_REQUESTED;
        if (title_button_hit(click_button, click_x, click_y, sw / 2 + 80, button_y)) {
            title->state = 0; title->username[0] = '\0'; title->password[0] = '\0';
        }
        for (int key; (key = title_poll_key(title)) != -1;) {
            bool valid = key > 0 && key < 256 && strchr(title_charset, key) != NULL;
            if (title->login_field == 0) {
                title_edit(title->username, sizeof(title->username), key, valid);
                if (key == 9 || key == 10 || key == 13) title->login_field = 1;
            } else if (title->login_field == 1) {
                title_edit(title->password, sizeof(title->password), key, valid);
                if (key == 9 || key == 10 || key == 13) title->login_field = 0;
            }
        }
    } else if (title->state == 3) {
        if (title_button_hit(click_button, click_x, click_y, sw / 2, sh / 2 + 50 + 20)) title->state = 0;
    }
    // GameShell.run discards unread keys after every update.
    title->key_read = title->key_write;
    return action;
}

void title_screen_update_flames(TitleScreen *title, int loop_cycle) {
    if (!title->flame_buffer3) return;
    for (int x = 10; x < 117; x++) {
        if ((int)(title_random(title) * 100.0) < 50) title->flame_buffer3[x + (254 << 7)] = 255;
    }
    for (int i = 0; i < 100; i++) {
        int x = (int)(title_random(title) * 124.0) + 2;
        int y = (int)(title_random(title) * 128.0) + 128;
        title->flame_buffer3[x + (y << 7)] = 192;
    }
    for (int y = 1; y < 255; y++) for (int x = 1; x < 127; x++) {
        int i = x + (y << 7);
        title->flame_buffer2[i] = (title->flame_buffer3[i - 1] + title->flame_buffer3[i + 1] +
                                   title->flame_buffer3[i - 128] + title->flame_buffer3[i + 128]) / 4;
    }
    title->flame_cycle0 += 128;
    if (title->flame_cycle0 > 32768) {
        title->flame_cycle0 -= 32768;
        title_update_flame_buffer(title, title->runes[(int)(title_random(title) * 12.0)]);
    }
    for (int y = 1; y < 255; y++) for (int x = 1; x < 127; x++) {
        int i = x + (y << 7);
        int v = title->flame_buffer2[i + 128] - title->flame_buffer0[(i + title->flame_cycle0) & 32767] / 5;
        title->flame_buffer3[i] = v < 0 ? 0 : v;
    }
    for (int y = 0; y < 255; y++) title->flame_line_offset[y] = title->flame_line_offset[y + 1];
    title->flame_line_offset[255] = (int)(sin(loop_cycle / 14.0) * 16.0 + sin(loop_cycle / 15.0) * 14.0 +
                                          sin(loop_cycle / 16.0) * 12.0);
    if (title->flame_gradient_cycle0 > 0) title->flame_gradient_cycle0 -= 4;
    if (title->flame_gradient_cycle1 > 0) title->flame_gradient_cycle1 -= 4;
    if (title->flame_gradient_cycle0 == 0 && title->flame_gradient_cycle1 == 0) {
        int r = (int)(title_random(title) * 2000.0);
        if (r == 0) title->flame_gradient_cycle0 = 1024;
        if (r == 1) title->flame_gradient_cycle1 = 1024;
    }
}

static uint32_t title_blend(uint32_t value, int32_t alpha, uint32_t background) {
    uint32_t inv = (uint32_t)(256 - alpha), a = (uint32_t)alpha;
    return title_shift8((((value & 0xFF00FF) * a + (background & 0xFF00FF) * inv) & 0xFF00FF00u) +
                        (((value & 0xFF00) * a + (background & 0xFF00) * inv) & 0xFF0000u));
}

void title_screen_draw_flames(TitleScreen *title) {
    if (!title->flame_buffer3 || !title->flames_left) return;
    const int32_t *g0 = title->flame_gradient0, *g1 = title->flame_gradient1, *g2 = title->flame_gradient2;
    for (int i = 0; i < 256; i++) {
        int c0 = title->flame_gradient_cycle0, c1 = title->flame_gradient_cycle1;
        if (c0 > 0) title->flame_gradient[i] = c0 > 768 ? title_mix(g0[i], 1024 - c0, g1[i]) : c0 > 256 ? g1[i] : title_mix(g1[i], 256 - c0, g0[i]);
        else if (c1 > 0) title->flame_gradient[i] = c1 > 768 ? title_mix(g0[i], 1024 - c1, g2[i]) : c1 > 256 ? g2[i] : title_mix(g2[i], 256 - c1, g0[i]);
        else title->flame_gradient[i] = g0[i];
    }
    uint32_t *left = title->images[0].pixels, *right = title->images[1].pixels;
    memcpy(left, title->flames_left, 33920 * sizeof(uint32_t));
    int src = 0, dst = 1152;
    for (int y = 1; y < 255; y++) {
        int offset = title->flame_line_offset[y] * (256 - y) / 256;
        int step = offset + 22;
        if (step < 0) step = 0;
        src += step;
        for (int x = step; x < 128; x++) {
            int value = title->flame_buffer3[src++];
            if (value == 0) dst++;
            else { left[dst] = title_blend((uint32_t)title->flame_gradient[value], value, left[dst]); dst++; }
        }
        dst += step;
    }
    memcpy(right, title->flames_right, 33920 * sizeof(uint32_t));
    src = 0; dst = 1176;
    for (int y = 1; y < 255; y++) {
        int offset = title->flame_line_offset[y] * (256 - y) / 256;
        int step = 103 - offset;
        dst += offset;
        for (int x = 0; x < step; x++) {
            int value = title->flame_buffer3[src++];
            if (value == 0) dst++;
            else { right[dst] = title_blend((uint32_t)title->flame_gradient[value], value, right[dst]); dst++; }
        }
        src += 128 - step;
        dst += 128 - step - offset;
    }
}

bool title_screen_run_flames(TitleScreen *title, uint64_t now_ms, int loop_cycle) {
    if (!title->flame_active || !title->loaded) return false;
    if (title->flame_last_ms && now_ms - title->flame_last_ms < 40) return false;
    title->flame_last_ms = now_ms;
    title_screen_update_flames(title, loop_cycle);
    title_screen_update_flames(title, loop_cycle);
    title_screen_draw_flames(title);
    return true;
}

static void title_blit_image(const TitleImage *image, uint32_t *target, int32_t tw, int32_t th) {
    for (int32_t y = 0; y < image->height; y++) {
        int32_t ty = image->y + y;
        if (ty < 0 || ty >= th) continue;
        int32_t x0 = image->x < 0 ? -image->x : 0, x1 = image->width;
        if (image->x + x1 > tw) x1 = tw - image->x;
        if (x1 > x0) memcpy(target + (size_t)ty * tw + image->x + x0, image->pixels + (size_t)y * image->width + x0,
                            (size_t)(x1 - x0) * sizeof(uint32_t));
    }
}

void title_screen_draw(TitleScreen *title, PixFont *b12, int loop_cycle) {
    if (!title->loaded) return;
    uint32_t *target = pix2d_pixels;
    int32_t tw = pix2d_width, th = pix2d_height;
    TitleImage *box = &title->images[4];
    pix2d_bind(box->pixels, box->width, box->height);
    pix8_draw(title->titlebox, 0, 0);
    const int w = 360, h = 200;
    const uint32_t yellow = 16776960, white = 16777215;
    if (title->state == 0) {
        pixfont_draw_string_taggable_center(b12, w / 2, h / 2 - 20, yellow, "Welcome to RuneScape", true);
        int x = w / 2 - 80, y = h / 2 + 20;
        pix8_draw(title->titlebutton, x - 73, y - 20);
        pixfont_draw_string_taggable_center(b12, x, y + 5, white, "New user", true);
        x = w / 2 + 80;
        pix8_draw(title->titlebutton, x - 73, y - 20);
        pixfont_draw_string_taggable_center(b12, x, y + 5, white, "Existing User", true);
    } else if (title->state == 2) {
        int y = h / 2 - 40;
        if (title->message0[0]) {
            pixfont_draw_string_taggable_center(b12, w / 2, y - 15, yellow, title->message0, true);
            pixfont_draw_string_taggable_center(b12, w / 2, y, yellow, title->message1, true);
        } else {
            pixfont_draw_string_taggable_center(b12, w / 2, y - 7, yellow, title->message1, true);
        }
        y += 30;
        bool blink = loop_cycle % 40 < 20;
        char line[64];
        snprintf(line, sizeof(line), "Username: %s%s", title->username, title->login_field == 0 && blink ? "@yel@|" : "");
        pixfont_draw_string_taggable(b12, w / 2 - 90, y, white, line, true);
        y += 15;
        char stars[sizeof(title->password)];
        size_t n = strlen(title->password);
        memset(stars, '*', n); stars[n] = '\0';
        snprintf(line, sizeof(line), "Password: %s%s", stars, title->login_field == 1 && blink ? "@yel@|" : "");
        pixfont_draw_string_taggable(b12, w / 2 - 88, y, white, line, true);
        int x = w / 2 - 80; y = h / 2 + 50;
        pix8_draw(title->titlebutton, x - 73, y - 20);
        pixfont_draw_string_taggable_center(b12, x, y + 5, white, "Login", true);
        x = w / 2 + 80;
        pix8_draw(title->titlebutton, x - 73, y - 20);
        pixfont_draw_string_taggable_center(b12, x, y + 5, white, "Cancel", true);
    } else if (title->state == 3) {
        pixfont_draw_string_taggable_center(b12, w / 2, h / 2 - 60, yellow, "Create a free account", true);
        int y = h / 2 - 35;
        pixfont_draw_string_taggable_center(b12, w / 2, y, white, "To create a new account you need to", true); y += 15;
        pixfont_draw_string_taggable_center(b12, w / 2, y, white, "go back to the main RuneScape webpage", true); y += 15;
        pixfont_draw_string_taggable_center(b12, w / 2, y, white, "and choose the red 'create account'", true); y += 15;
        pixfont_draw_string_taggable_center(b12, w / 2, y, white, "button at the top right of that page.", true);
        int x = w / 2; y = h / 2 + 50;
        pix8_draw(title->titlebutton, x - 73, y - 20);
        pixfont_draw_string_taggable_center(b12, x, y + 5, white, "Cancel", true);
    }
    pix2d_bind(target, tw, th);
    // The applet keeps earlier blits on screen; this client recomposes each
    // frame, including the flame thread's imageTitle0/1 output.
    for (int i = 0; i < TITLE_IMAGE_COUNT; i++) title_blit_image(&title->images[i], target, tw, th);
}

void title_screen_set_login_reply(TitleScreen *title, int reply) {
    static const struct { int reply; const char *line0, *line1; } replies[] = {
        {3,  "", "Invalid username or password."},
        {4,  "Your account has been disabled.", "Please check your message-centre for details."},
        {5,  "Your account is already logged in.", "Try again in 60 secs..."},
        {6,  "RuneScape has been updated!", "Please reload this page."},
        {7,  "This world is full.", "Please use a different world."},
        {8,  "Unable to connect.", "Login server offline."},
        {9,  "Login limit exceeded.", "Too many connections from your address."},
        {10, "Unable to connect.", "Bad session id."},
        {12, "You need a members account to login to this world.", "Please subscribe, or use a different world."},
        {13, "Could not complete login.", "Please try using a different world."},
        {14, "The server is being updated.", "Please wait 1 minute and try again."},
        {16, "Login attempts exceeded.", "Please wait 1 minute and try again."},
        {17, "You are standing in a members-only area.", "To play on this world move to a free area first"},
        {-1, "", "Error connecting to server."}
    };
    if (reply == 11) {   // the original assigns loginMessage1 twice and leaves loginMessage0
        snprintf(title->message1, sizeof(title->message1), "Please try again.");
        return;
    }
    for (size_t i = 0; i < sizeof(replies) / sizeof(replies[0]); i++) {
        if (replies[i].reply != reply) continue;
        snprintf(title->message0, sizeof(title->message0), "%s", replies[i].line0);
        snprintf(title->message1, sizeof(title->message1), "%s", replies[i].line1);
        return;
    }
    // Other codes leave both messages as "Connecting to server..." was set,
    // which is what the original's unmatched branch does.
}

void title_screen_logout_reset(TitleScreen *title) {
    title->state = 0;
    title->username[0] = '\0';
    title->password[0] = '\0';
}
