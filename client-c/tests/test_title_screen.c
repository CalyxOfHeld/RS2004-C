#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include "ui/title_screen.h"
#include "gfx/pix2d.h"
#include "net/protocol.h"
#include "net/client_stream.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Replays tests/fixtures/title-reference.bin, written by TitleOracle.java from
// the unchanged revision-225 client.loadTitle, drawTitleScreen and drawFlames.

static int32_t rd(FILE *f) {
    unsigned char b[4]; assert(fread(b, 1, 4, f) == 4);
    return (int32_t)((uint32_t)b[0] << 24 | (uint32_t)b[1] << 16 | (uint32_t)b[2] << 8 | b[3]);
}

static void rd_text(FILE *f, char *out, size_t capacity) {
    int32_t n = rd(f); assert(n >= 0 && (size_t)n < capacity);
    assert(fread(out, 1, (size_t)n, f) == (size_t)n); out[n] = '\0';
}

static int compare_image(FILE *f, const TitleImage *image, int index, const char *what) {
    assert(rd(f) == index); assert(rd(f) == image->width); assert(rd(f) == image->height);
    int differ = 0;
    for (int i = 0; i < image->width * image->height; i++) {
        uint32_t expected = (uint32_t)rd(f);
        if (expected != image->pixels[i]) {
            if (!differ) fprintf(stderr, "%s imageTitle%d first difference at (%d,%d): java=%08X native=%08X\n", what, index,
                                 i % image->width, i / image->width, expected, image->pixels[i]);
            differ++;
        }
    }
    if (differ) { fprintf(stderr, "%s imageTitle%d: %d differing pixels\n", what, index, differ); abort(); }
    return image->width * image->height;
}

static void click(TitleScreen *t, int x, int y) { title_screen_update(t, 1, x, y); }
static void type(TitleScreen *t, const char *text) {
    for (; *text; text++) title_screen_push_key(t, (unsigned char)*text);
    title_screen_update(t, 0, 0, 0);
}

static void test_update_title(void) {
    TitleScreen t; title_screen_init(&t);
    // updateTitle state 0: "New user" (314,306) and "Existing User" (474,306), 75x20 half extents.
    click(&t, 314 - 76, 306); assert(t.state == 0);
    click(&t, 314 + 75, 306 + 20); assert(t.state == 3);
    title_screen_update(&t, 2, 394, 336); assert(t.state == 3);   // right click is not mouseClickButton 1
    click(&t, 394, 336); assert(t.state == 0);
    click(&t, 474, 306); assert(t.state == 2 && t.login_field == 0);
    assert(!strcmp(t.message0, "") && !strcmp(t.message1, "Enter your username & password."));
    // Username: charset filter, 12 character cap, backspace; Tab/Enter switch fields.
    type(&t, "Ab~\x01" "cdefghijklmnop");
    assert(!strcmp(t.username, "Ab~cdefghijk"));
    title_screen_push_key(&t, 8); title_screen_push_key(&t, 8); type(&t, "");
    assert(!strcmp(t.username, "Ab~cdefghi"));
    title_screen_push_key(&t, 9); type(&t, ""); assert(t.login_field == 1);
    type(&t, "0123456789abcdefghijXYZ"); assert(!strcmp(t.password, "0123456789abcdefghij"));
    title_screen_push_key(&t, 10); type(&t, ""); assert(t.login_field == 0);
    title_screen_push_key(&t, 13); type(&t, "\xA3"); assert(t.login_field == 1 && !strcmp(t.password, "0123456789abcdefghij"));
    // Field selection rows at y [266,281) and [281,296), any x.
    click(&t, 10, 280); assert(t.login_field == 0);
    click(&t, 10, 265); assert(t.login_field == 0);
    click(&t, 700, 281); assert(t.login_field == 1);
    click(&t, 700, 296); assert(t.login_field == 1);
    click(&t, 700, 266); assert(t.login_field == 0);
    click(&t, 700, 295); assert(t.login_field == 1);
    // Keys queued before an update in another state are discarded with that update.
    TitleScreen q; title_screen_init(&q);
    title_screen_push_key(&q, 'x'); title_screen_update(&q, 0, 0, 0);
    click(&q, 474, 306); type(&q, ""); assert(!strcmp(q.username, ""));
    // Login (314,336) requests client.login; Cancel (474,336) clears both fields.
    assert(title_screen_update(&t, 1, 314, 336) == TITLE_LOGIN_REQUESTED && t.state == 2);
    click(&t, 474, 336); assert(t.state == 0 && !t.username[0] && !t.password[0]);
    // client.login reply messages and client.logout.
    title_screen_set_login_reply(&t, 5);
    assert(!strcmp(t.message0, "Your account is already logged in.") && !strcmp(t.message1, "Try again in 60 secs..."));
    title_screen_set_login_reply(&t, 11);
    assert(!strcmp(t.message0, "Your account is already logged in.") && !strcmp(t.message1, "Please try again."));
    title_screen_set_login_reply(&t, -1);
    assert(!strcmp(t.message0, "") && !strcmp(t.message1, "Error connecting to server."));
    title_screen_set_login_reply(&t, 17);
    assert(!strcmp(t.message1, "To play on this world move to a free area first"));
    snprintf(t.message1, sizeof(t.message1), "Connecting to server...");
    title_screen_set_login_reply(&t, 19);
    assert(!strcmp(t.message1, "Connecting to server..."));
    t.state = 2; snprintf(t.username, sizeof(t.username), "a"); snprintf(t.password, sizeof(t.password), "b");
    title_screen_logout_reset(&t); assert(t.state == 0 && !t.username[0] && !t.password[0]);
    puts("PASS TITLE-02: updateTitle buttons, field rows, charset/length limits, key queue and login/logout messages");
}

// Loopback login server: sends the seed, records the login opcode, replies.
typedef struct { socket_t listener; int replies[3], count, opcodes[3]; } FakeServer;
static int fake_server(void *data) {
    FakeServer *s = data;
    for (int i = 0; i < s->count; i++) {
        socket_t peer = accept(s->listener, NULL, NULL); assert(peer != INVALID_SOCKET_VAL);
        unsigned char seed[8] = {1, 2, 3, 4, 5, 6, 7, 8}, head[2], body[256];
        assert(send(peer, (const char *)seed, 8, 0) == 8);
        int got = 0; while (got < 2) { int n = recv(peer, (char *)head + got, 2 - got, 0); assert(n > 0); got += n; }
        got = 0; while (got < head[1]) { int n = recv(peer, (char *)body + got, head[1] - got, 0); assert(n > 0); got += n; }
        s->opcodes[i] = head[0];
        unsigned char reply = (unsigned char)s->replies[i];
        assert(send(peer, (const char *)&reply, 1, 0) == 1);
        if (s->replies[i] != 2 && s->replies[i] != 15 && s->replies[i] != 18) {
#ifdef _WIN32
            closesocket(peer);
#else
            close(peer);
#endif
        }
    }
    return 0;
}

static LoginResponse fake_login(FakeServer *s, bool reconnect, GameSession *session) {
    s->listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); assert(s->listener != INVALID_SOCKET_VAL);
    struct sockaddr_in address = {0}; address.sin_family = AF_INET; address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    assert(bind(s->listener, (struct sockaddr *)&address, sizeof(address)) == 0 && listen(s->listener, 2) == 0);
#ifdef _WIN32
    int size = sizeof(address);
#else
    socklen_t size = sizeof(address);
#endif
    assert(getsockname(s->listener, (struct sockaddr *)&address, &size) == 0);
    SDL_Thread *thread = SDL_CreateThread(fake_server, "fake-login", s); assert(thread);
    LoginResponse reply = protocol_login_ex(session, "127.0.0.1", ntohs(address.sin_port), "sollog", "pw", false, reconnect);
    SDL_WaitThread(thread, NULL);
#ifdef _WIN32
    closesocket(s->listener);
#else
    close(s->listener);
#endif
    return reply;
}

static void test_login_replies(void) {
    assert(client_stream_global_init());
    GameSession session; protocol_init_session(&session);
    FakeServer reconnect = {.replies = {15}, .count = 1};
    assert(fake_login(&reconnect, true, &session) == LOGIN_RECONNECT_SUCCESS && session.in_game && reconnect.opcodes[0] == 18);
    protocol_close_session(&session);
    FakeServer retry = {.replies = {1, 2}, .count = 2};
    Uint64 start = SDL_GetTicks64();
    assert(fake_login(&retry, false, &session) == LOGIN_SUCCESS && session.in_game && !session.is_mod);
    assert(retry.opcodes[0] == 16 && retry.opcodes[1] == 16 && SDL_GetTicks64() - start >= 1900);
    protocol_close_session(&session);
    FakeServer busy = {.replies = {5}, .count = 1};
    assert(fake_login(&busy, false, &session) == LOGIN_ACCOUNT_LOGGED_IN && !session.in_game && busy.opcodes[0] == 16);
    client_stream_global_cleanup();
    puts("PASS TITLE-03: login opcode 16, reconnect opcode 18 with reply 15, reply 1 retried after 2s, reply 5 rejected");
}

int main(void) {
    Arena perm = arena_create(8 * 1024 * 1024);
    Jagfile *title_jag = jagfile_load_file("../engine/data/pack/client/title", &perm);
    assert(title_jag);
    PixFont b12; assert(pixfont_load(&b12, title_jag, "b12", &perm));
    TitleScreen t; title_screen_init(&t);
    assert(title_screen_load(&t, title_jag));
    uint32_t *screen = calloc(TITLE_SCREEN_WIDTH * TITLE_SCREEN_HEIGHT, sizeof(uint32_t)); assert(screen);

    FILE *f = fopen("tests/fixtures/title-reference.bin", "rb"); assert(f && rd(f) == 0x54544c31);
    long pixels = 0;
    for (int i = 0; i < 9; i++) if (i != 4) pixels += compare_image(f, &t.images[i], i, "loadTitleBackground");
    int states = rd(f);
    for (int c = 0; c < states; c++) {
        t.state = rd(f); t.login_field = rd(f); int loop_cycle = rd(f);
        rd_text(f, t.message0, sizeof(t.message0)); rd_text(f, t.message1, sizeof(t.message1));
        rd_text(f, t.username, sizeof(t.username)); rd_text(f, t.password, sizeof(t.password));
        pix2d_bind(screen, TITLE_SCREEN_WIDTH, TITLE_SCREEN_HEIGHT);
        title_screen_draw(&t, &b12, loop_cycle);
        pixels += compare_image(f, &t.images[4], 4, "drawTitleScreen");
        // The frame is the composition of every PixMap at its client position.
        for (int i = 0; i < TITLE_IMAGE_COUNT; i++) {
            const TitleImage *image = &t.images[i];
            for (int y = 0; y < image->height; y++) for (int x = 0; x < image->width; x++)
                assert(screen[(image->y + y) * TITLE_SCREEN_WIDTH + image->x + x] == image->pixels[y * image->width + x]);
        }
    }
    int flames = rd(f);
    for (int i = 0; i < 32768; i++) t.flame_buffer3[i] = i % 7 == 0 ? 0 : (i * 37 + (i >> 7) * 11) & 255;
    for (int y = 0; y < 256; y++) t.flame_line_offset[y] = (y * 13) % 61 - 30;
    for (int c = 0; c < flames; c++) {
        t.flame_gradient_cycle0 = rd(f); t.flame_gradient_cycle1 = rd(f);
        title_screen_draw_flames(&t);
        pixels += compare_image(f, &t.images[0], 0, "drawFlames");
        pixels += compare_image(f, &t.images[1], 1, "drawFlames");
    }
    assert(fgetc(f) == EOF); fclose(f);
    printf("PASS TITLE-01: %ld pixels match the original title background, logo, %d drawTitleScreen states and %d drawFlames gradient branches\n",
           pixels, states, flames);

    // A logout reload restores identical artwork.
    uint32_t first = t.images[2].pixels[1234];
    title_screen_unload(&t); assert(!t.loaded && !t.images[0].pixels);
    assert(title_screen_load(&t, title_jag) && t.images[2].pixels[1234] == first);
    title_screen_unload(&t);
    free(screen);

    test_update_title();
    test_login_replies();
    arena_free(&perm);
    return 0;
}
