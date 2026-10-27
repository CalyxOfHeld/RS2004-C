#ifndef RS2_CLIENT_H
#define RS2_CLIENT_H

#include <stdbool.h>
#include <stdint.h>

// The revision-225 client loop (client.java load/updateGame/draw), separated
// from the platform so it can run windowed (sdl2_main.c) or headless for
// replay. GameShell's 50 Hz pacing stays with the platform: per frame it calls
// client_begin_frame, queues input, client_process_events, client_update once
// per due cycle, client_draw, presents the framebuffer, then client_end_frame.

#define CLIENT_WIDTH  789
#define CLIENT_HEIGHT 532

typedef enum {
    CLIENT_EVENT_NONE,
    CLIENT_EVENT_QUIT,
    CLIENT_EVENT_MOUSE_MOVE,
    CLIENT_EVENT_MOUSE_DOWN,
    CLIENT_EVENT_MOUSE_UP,
    CLIENT_EVENT_MOUSE_WHEEL,
    CLIENT_EVENT_KEY_DOWN,
    CLIENT_EVENT_TEXT,
    CLIENT_EVENT_KEY_UP, /* append: preserve existing recording event numbers */
    CLIENT_EVENT_TRACK_KEY_DOWN,CLIENT_EVENT_TRACK_KEY_UP,
    CLIENT_EVENT_FOCUS_GAIN,CLIENT_EVENT_FOCUS_LOSS,CLIENT_EVENT_MOUSE_ENTER,CLIENT_EVENT_MOUSE_EXIT
} ClientEventType;

typedef enum { CLIENT_BUTTON_NONE, CLIENT_BUTTON_LEFT, CLIENT_BUTTON_MIDDLE, CLIENT_BUTTON_RIGHT } ClientButton;

// Only the keys the client reacts to on KEY_DOWN; printable input is TEXT.
typedef enum {
    CLIENT_KEY_NONE,
    CLIENT_KEY_ESCAPE,
    CLIENT_KEY_F1, CLIENT_KEY_F2, CLIENT_KEY_F3, CLIENT_KEY_F4, CLIENT_KEY_F5, CLIENT_KEY_F6,
    CLIENT_KEY_F7, CLIENT_KEY_F8, CLIENT_KEY_F9, CLIENT_KEY_F10, CLIENT_KEY_F11, CLIENT_KEY_F12,
    CLIENT_KEY_BACKSPACE,
    CLIENT_KEY_DELETE,
    CLIENT_KEY_TAB,
    CLIENT_KEY_RETURN,
    CLIENT_KEY_KP_ENTER
} ClientKey;

typedef struct {
    ClientEventType type;
    int x, y;        // mouse position for move/down/up
    int button;      // ClientButton for down/up
    int key;         // ClientKey for UI keys; GameShell code for TRACK_KEY_*
    int wheel;       // vertical wheel delta
    char text[32];   // UTF-8 for TEXT
} ClientEvent;

// Keys sampled as held state (GameShell key flags) for the current frame.
typedef struct {
    bool left, right, up, down;
    bool w, a, s, d;
    bool ctrl;
} ClientHeldKeys;

typedef struct {
    // Platform services. now_ms is a monotonic millisecond clock; save_bmp
    // writes a CLIENT_WIDTH x CLIENT_HEIGHT XRGB framebuffer.
    uint64_t (*now_ms)(void);
    bool (*save_bmp)(const char *path, const uint32_t *pixels, int width, int height);
    /* Immediate retained-frame presentation during synchronous recovery. */
    void (*present)(const uint32_t *pixels, int width, int height);
    /* Source System.currentTimeMillis for input tracking andsound; logged by platform. */
    uint64_t (*tracking_now_ms)(void);
} ClientPlatform;

typedef struct {
    bool offline, trace, follow_input;
    const char *input_path;
    const char *capture_path;
    bool capture_rebuild, capture_clicks;
    int capture_frame, capture_interval;
    int server_port;
    const char *server_host; // dotted IPv4; consoles reach the server over the LAN
    int walk_x, walk_z;
    int capture_yaw, capture_pitch;
    bool fixed_step;
    bool has_seed;
    int64_t seed;
    const char *login_username, *login_password;
    const char *record_path;   // platform: write a session log (core/session_log.h)
} ClientOptions;

void client_default_options(ClientOptions *options);
// Parses the shared command-line flags; returns false after printing an error.
bool client_parse_args(int argc, char **argv, ClientOptions *options);

bool client_init(const ClientOptions *options, const ClientPlatform *platform);
void client_shutdown(void);

void client_begin_frame(void);
void client_queue_event(const ClientEvent *event);
void client_set_held_keys(const ClientHeldKeys *keys);
void client_process_events(void);
// One updateGame cycle; false stops further cycles this frame.
bool client_update(void);
// Draws one frame into client_pixels(); false is a fatal capture error.
bool client_draw(void);
// After the platform presented (and captured) the frame.
void client_end_frame(void);

bool client_running(void);
void client_stop(void);
uint32_t *client_pixels(void);
int client_rendered_frames(void);
// The capture the platform must take of the presented frame, or NULL.
const char *client_frame_capture_path(void);

ClientKey client_key_from_name(const char *name);
// FNV-1a 64 over the framebuffer words, for recording and replay checks.
uint64_t client_frame_hash(void);

#endif // RS2_CLIENT_H
