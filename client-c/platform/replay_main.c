#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif
#define SDL_MAIN_HANDLED   // a console program; SDL is used only for BMP files
#include <SDL2/SDL.h>

#include "client/client.h"
#include "client/client_log.h"
#include "core/session_log.h"
#include "audio/midi.h"

// Headless replay of a session log written by `rs2client --record`. No window,
// audio or socket is opened: frames, cycles, input, clock reads and stream
// results come from the log, and the unchanged client reproduces the run. Each
// frame's framebuffer hash and every outgoing byte must equal the recording.
//
//   rs2replay <log> [--out DIR] [--dump-every N] [--dump-frame N]...
//             [--regions FILE --region-hashes OUT]
//
// --regions lists screen rectangles ("name x y w h" per line); each frame's
// hash of every rectangle is written to OUT for the Java parity oracle.
//
// RS2_PASSWORD supplies the password for a recorded login; it is not logged.

static char out_dir[1024];

typedef struct { char name[32]; int x, y, w, h; } Region;
static Region regions[64];
static int region_count;

static bool load_regions(const char *path) {
    FILE *file = fopen(path, "r");
    if (!file) return false;
    Region r;
    while (region_count < 64 && fscanf(file, "%31s %d %d %d %d", r.name, &r.x, &r.y, &r.w, &r.h) == 5) {
        if (r.x < 0 || r.y < 0 || r.w < 1 || r.h < 1 || r.x + r.w > CLIENT_WIDTH || r.y + r.h > CLIENT_HEIGHT) { fclose(file); return false; }
        regions[region_count++] = r;
    }
    fclose(file);
    return region_count > 0;
}

// FNV-1a 64 over the low 24 bits of each pixel, little-endian, row by row.
static uint64_t region_hash(const uint32_t *pixels, const Region *r) {
    uint64_t h = 0xcbf29ce484222325ULL;
    for (int y = r->y; y < r->y + r->h; y++)
        for (int x = r->x; x < r->x + r->w; x++) {
            uint32_t p = pixels[y * CLIENT_WIDTH + x] & 0xFFFFFF;
            for (int b = 0; b < 4; b++) { h ^= (p >> (8 * b)) & 0xFF; h *= 0x100000001b3ULL; }
        }
    return h;
}

static uint64_t replay_clock(void) { return 0; }
static uint64_t replay_now_ms(void) { return client_log_time(replay_clock); }

static bool replay_save_bmp(const char *path, const uint32_t *pixels, int width, int height) {
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormatFrom((void *)pixels, width, height,
        32, width * (int)sizeof(uint32_t), SDL_PIXELFORMAT_RGB888);
    bool saved = surface && SDL_SaveBMP(surface, path) == 0;
    if (!saved) fprintf(stderr, "SDL_SaveBMP: %s\n", SDL_GetError());
    if (surface) SDL_FreeSurface(surface);
    return saved;
}

// Recorded capture paths point at the recording run; replay writes beside --out.
static const char *redirect(const char *path, char *buffer, size_t size) {
    const char *base = path;
    for (const char *p = path; *p; p++) if (*p == '/' || *p == '\\') base = p + 1;
    snprintf(buffer, size, "%s/%s", out_dir, base);
    return buffer;
}

int main(int argc, char *argv[]) {
    const char *log_path = NULL;
    int dump_every = 0;
    int dump_frames[64], dump_count = 0;
    const char *regions_path = NULL, *region_out_path = NULL;
    snprintf(out_dir, sizeof(out_dir), ".");
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--out") && i + 1 < argc) snprintf(out_dir, sizeof(out_dir), "%s", argv[++i]);
        else if (!strcmp(argv[i], "--dump-every") && i + 1 < argc) dump_every = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--dump-frame") && i + 1 < argc && dump_count < 64) dump_frames[dump_count++] = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--regions") && i + 1 < argc) regions_path = argv[++i];
        else if (!strcmp(argv[i], "--region-hashes") && i + 1 < argc) region_out_path = argv[++i];
        else if (!log_path && argv[i][0] != '-') log_path = argv[i];
        else { fprintf(stderr, "Unknown or incomplete option: %s\n", argv[i]); return 2; }
    }
    if (!log_path) { fprintf(stderr, "usage: rs2replay <log> [--out DIR] [--dump-every N] [--dump-frame N]...\n"); return 2; }
    if (!regions_path != !region_out_path) { fprintf(stderr, "--regions and --region-hashes go together\n"); return 2; }
    if (regions_path && !load_regions(regions_path)) { fprintf(stderr, "Cannot read regions %s\n", regions_path); return 2; }
    FILE *region_out = NULL;
    if (region_out_path && !(region_out = fopen(region_out_path, "w"))) { fprintf(stderr, "Cannot write %s\n", region_out_path); return 2; }
    setbuf(stdout, NULL);
    if (!session_log_replay(log_path)) { fprintf(stderr, "Cannot read session log %s\n", log_path); return 2; }

    ClientLogHeader header;
    if (!client_log_read_header(&header)) return 2;
    ClientOptions options;
    if (!client_parse_args(header.argc, header.argv, &options)) return 2;
    options.has_seed = true;
    options.seed = header.seed;
    options.login_username = header.username;
    options.login_password = getenv("RS2_PASSWORD");
    char capture[1024];
    if (options.capture_path) options.capture_path = redirect(options.capture_path, capture, sizeof(capture));
    if (options.input_path) printf("[REPLAY] input script %s (must be unchanged since recording)\n", options.input_path);

    // The same working directory as the client, for its relative cache paths.
    char *executable_dir = SDL_GetBasePath();
    if (!executable_dir) { fprintf(stderr, "Cannot locate client directory\n"); return 2; }
    char client_dir[1024];
    snprintf(client_dir, sizeof(client_dir), "%s..", executable_dir);
    SDL_free(executable_dir);
#ifdef _WIN32
    int directory_result = _chdir(client_dir);
#else
    int directory_result = chdir(client_dir);
#endif
    if (directory_result != 0) { perror("Cannot enter client directory"); return 2; }

    midi_set_silent(true);
    ClientPlatform platform = { replay_now_ms, replay_save_bmp };
    if (!client_init(&options, &platform)) return 2;

    int frames = 0;
    uint64_t last_hash = 0;
    while (client_running() && !session_log_failed()) {
        long long frame = -1;
        if (!session_log_take("F") || !session_log_take_int(&frame)) break;
        if (frame != client_rendered_frames()) { session_log_desync("frame number"); break; }
        client_begin_frame();
        const char *tag;
        while ((tag = session_log_peek()) && !strcmp(tag, "E")) {
            ClientEvent event;
            if (client_log_take_event(&event)) client_queue_event(&event);
        }
        if ((tag = session_log_peek()) && !strcmp(tag, "K")) {
            ClientHeldKeys held;
            if (client_log_take_held(&held)) client_set_held_keys(&held);
        }
        client_process_events();
        while ((tag = session_log_peek()) && !strcmp(tag, "U")) {
            session_log_take("U");
            if (!client_update()) break;
        }
        if (!client_draw()) { fprintf(stderr, "[REPLAY] draw failed at frame %lld\n", frame); break; }
        long long recorded = 0;
        if (!session_log_take("D") || !session_log_take_int(&recorded)) break;
        last_hash = client_frame_hash();
        if ((uint64_t)recorded != last_hash) {
            char what[128];
            snprintf(what, sizeof(what), "frame %lld framebuffer hash %016llx, recorded %016llx",
                frame, (unsigned long long)last_hash, (unsigned long long)recorded);
            session_log_desync(what);
        }
        for (int i = 0; region_out && i < region_count; i++)
            fprintf(region_out, "%lld %s %016llx\n", frame, regions[i].name, (unsigned long long)region_hash(client_pixels(), &regions[i]));
        bool dump = dump_every > 0 && (frame + 1) % dump_every == 0;
        for (int i = 0; i < dump_count; i++) dump |= dump_frames[i] == frame;
        const char *capture_path = client_frame_capture_path();
        if (capture_path) { replay_save_bmp(capture_path, client_pixels(), CLIENT_WIDTH, CLIENT_HEIGHT); printf("[CAPTURE] replay -> %s\n", capture_path); }
        if (dump || session_log_failed()) {
            char path[1100];
            snprintf(path, sizeof(path), "%s/replay-frame-%06lld.bmp", out_dir, frame);
            replay_save_bmp(path, client_pixels(), CLIENT_WIDTH, CLIENT_HEIGHT);
        }
        client_end_frame();
        frames++;
    }

    // Shutdown's own stream calls are part of the log, then the end marker.
    client_shutdown();
    if (!session_log_failed() && session_log_take("Q") && !session_log_at_end())
        session_log_desync("the log continues after the end marker");
    bool ok = !session_log_failed();
    long line = session_log_line();
    session_log_close();
    if (region_out) fclose(region_out);
    client_log_free_header(&header);
    if (ok) printf("PASS REPLAY: %d frames reproduced (%ld log lines), final hash %016llx\n", frames, line, (unsigned long long)last_hash);
    else printf("FAIL REPLAY: diverged after %d frames at log line %ld\n", frames, line);
    return ok ? 0 : 1;
}
