#ifndef RS2_TITLE_SCREEN_H
#define RS2_TITLE_SCREEN_H

#include <stdint.h>
#include <stdbool.h>
#include "core/jagfile.h"
#include "gfx/pix8.h"
#include "gfx/pix32.h"
#include "gfx/pixfont.h"

// Revision-225 title/login screen (client.loadTitle, loadTitleBackground,
// loadTitleImages, updateTitle, drawTitleScreen, runFlames/updateFlames/
// drawFlames/updateFlameBuffer, unloadTitle and the login reply messages).

#define TITLE_SCREEN_WIDTH  789
#define TITLE_SCREEN_HEIGHT 532
#define TITLE_IMAGE_COUNT   9
#define TITLE_KEY_QUEUE     128

typedef struct {
    uint32_t *pixels;
    int32_t   width, height, x, y;   // PixMap and its fixed screen position
} TitleImage;

typedef enum { TITLE_IDLE, TITLE_LOGIN_REQUESTED } TitleAction;

typedef struct {
    bool       loaded;
    TitleImage images[TITLE_IMAGE_COUNT];   // imageTitle0..imageTitle8
    Pix8      *titlebox, *titlebutton, *runes[12];
    uint32_t  *flames_left, *flames_right;  // imageFlamesLeft/Right (128x265)
    int32_t    flame_gradient[256], flame_gradient0[256], flame_gradient1[256], flame_gradient2[256];
    int32_t   *flame_buffer0, *flame_buffer1, *flame_buffer2, *flame_buffer3;
    int32_t    flame_line_offset[256];
    int32_t    flame_cycle0, flame_gradient_cycle0, flame_gradient_cycle1;
    bool       flame_active;
    uint64_t   flame_last_ms;

    int        state;          // titleScreenState: 0 welcome, 2 login box, 3 new user
    int        login_field;    // titleLoginField: 0 username, 1 password
    char       username[13];   // at most 12 characters
    char       password[21];   // at most 20 characters
    char       message0[64];   // loginMessage0
    char       message1[64];   // loginMessage1
    int        keys[TITLE_KEY_QUEUE];
    int        key_read, key_write;
} TitleScreen;

void        title_screen_init(TitleScreen *title);
bool        title_screen_load(TitleScreen *title, Jagfile *title_jag);
void        title_screen_unload(TitleScreen *title);
void        title_screen_push_key(TitleScreen *title, int key);
// One 50 Hz client update while the title is shown. click_button is
// GameShell.mouseClickButton (1 = left) for this cycle.
TitleAction title_screen_update(TitleScreen *title, int click_button, int click_x, int click_y);
// The flame thread's body (two updateFlames, one drawFlames), run at most
// once per 40 ms like runFlames' adaptive sleep; returns true when it ran.
bool        title_screen_run_flames(TitleScreen *title, uint64_t now_ms, int loop_cycle);
void        title_screen_update_flames(TitleScreen *title, int loop_cycle);
void        title_screen_draw_flames(TitleScreen *title);
// drawTitleScreen plus the screen blits of every title PixMap into the
// currently bound Pix2D target (the 789x532 client framebuffer).
void        title_screen_draw(TitleScreen *title, PixFont *b12, int loop_cycle);
// The message pair client.login() sets for a server reply (-1 is IOException).
void        title_screen_set_login_reply(TitleScreen *title, int reply);
void        title_screen_logout_reset(TitleScreen *title);

#endif // RS2_TITLE_SCREEN_H
