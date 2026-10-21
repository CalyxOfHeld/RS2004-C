#include "audio/midi.h"
#include "core/bzip2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#endif

static bool s_midi_inited = false;
static bool s_is_playing = false;
static char s_current_song[64] = {0};
static int  s_volume = 200; // 0..255

#ifdef _WIN32
static bool midi_mci_ok(MCIERROR err, const char *operation) {
    if (err == 0) return true;
    fprintf(stderr, "[MIDI] %s failed (MCI error %lu)\n", operation, (unsigned long)err);
    return false;
}
#endif

static bool midi_start_file(const char *path, const char *alias) {
#ifdef _WIN32
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "close %s", alias);
    mciSendStringA(cmd, NULL, 0, NULL);
    snprintf(cmd, sizeof(cmd), "open \"%s\" type sequencer alias %s", path, alias);
    if (!midi_mci_ok(mciSendStringA(cmd, NULL, 0, NULL), "open")) return false;
    snprintf(cmd, sizeof(cmd), "play %s", alias);
    if (!midi_mci_ok(mciSendStringA(cmd, NULL, 0, NULL), "play")) {
        snprintf(cmd, sizeof(cmd), "close %s", alias);
        mciSendStringA(cmd, NULL, 0, NULL);
        return false;
    }
    return true;
#else
    (void)path;
    (void)alias;
    fprintf(stderr, "[MIDI] No sequencer backend available\n");
    return false;
#endif
}

static const char *s_song_search_dirs[] = {
    "../engine/data/pack/client/songs",
    "engine/data/pack/client/songs",
    "content/songs",
    "data/pack/client/songs",
    NULL
};

bool midi_init(void) {
    s_midi_inited = true;
    s_is_playing = false;
    s_current_song[0] = '\0';
    return true;
}

static FILE* open_song_file(const char *name, char *out_path, size_t out_path_len) {
    for (int i = 0; s_song_search_dirs[i] != NULL; i++) {
        snprintf(out_path, out_path_len, "%s/%s.mid", s_song_search_dirs[i], name);
        FILE *f = fopen(out_path, "rb");
        if (f) return f;
    }
    return NULL;
}

bool midi_play(const char *name, int crc, int size) {
    (void)crc;
    (void)size;
    if (!s_midi_inited) midi_init();
    if (!name || name[0] == '\0') {
        midi_stop();
        return true;
    }

    if (strcmp(s_current_song, name) == 0 && midi_is_playing()) {
        return true;
    }

    char found_path[512];
    FILE *f = open_song_file(name, found_path, sizeof(found_path));
    if (!f) {
        fprintf(stderr, "[MIDI] Song not found: %s\n", name);
        return false;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (file_size < 8) {
        fclose(f);
        return false;
    }

    uint8_t *file_data = (uint8_t *)malloc(file_size);
    if (!file_data) {
        fclose(f);
        return false;
    }

    if (fread(file_data, 1, file_size, f) != (size_t)file_size) {
        free(file_data);
        fclose(f);
        return false;
    }
    fclose(f);

    uint32_t uncomp_len = ((uint32_t)file_data[0] << 24) |
                          ((uint32_t)file_data[1] << 16) |
                          ((uint32_t)file_data[2] << 8)  |
                          ((uint32_t)file_data[3]);

    if (uncomp_len < 4 || uncomp_len > 2 * 1024 * 1024) {
        free(file_data);
        return false;
    }

    uint8_t *decompressed = (uint8_t *)malloc(uncomp_len);
    if (!decompressed) {
        free(file_data);
        return false;
    }

    bzip_decompress((int8_t *)decompressed, (int8_t *)file_data, (int)(file_size - 4), 4);
    free(file_data);

    if (memcmp(decompressed, "MThd", 4) != 0) {
        fprintf(stderr, "[MIDI] Decompressed data does not contain MThd header: %s\n", name);
        free(decompressed);
        return false;
    }

    // Write standard MIDI to temporary path for sequencer playback
    const char *temp_midi_path = "bin/current_music.mid";
    FILE *out_f = fopen(temp_midi_path, "wb");
    if (!out_f) {
        temp_midi_path = "current_music.mid";
        out_f = fopen(temp_midi_path, "wb");
    }
    if (!out_f) {
        free(decompressed);
        return false;
    }

    size_t written = fwrite(decompressed, 1, uncomp_len, out_f);
    int close_result = fclose(out_f);
    free(decompressed);
    if (written != uncomp_len || close_result != 0) {
        fprintf(stderr, "[MIDI] Could not write music file: %s\n", temp_midi_path);
        return false;
    }

    s_is_playing = false;
    s_current_song[0] = '\0';
    if (!midi_start_file(temp_midi_path, "rs2midi")) return false;
#ifdef _WIN32
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "setaudio rs2midi volume to %d", (s_volume * 1000) / 255);
    mciSendStringA(cmd, NULL, 0, NULL);
#endif

    strncpy(s_current_song, name, sizeof(s_current_song) - 1);
    s_current_song[sizeof(s_current_song) - 1] = '\0';
    s_is_playing = true;
    printf("[MIDI] Playing music track: %s (%u bytes uncompressed)\n", name, uncomp_len);
    return true;
}

bool midi_play_jingle(const uint8_t *bzip_data, int compressed_len, int uncompressed_len) {
    if (!bzip_data || compressed_len <= 0 || uncompressed_len < 4) return false;

    uint8_t *decompressed = (uint8_t *)malloc(uncompressed_len);
    if (!decompressed) return false;

    bzip_decompress((int8_t *)decompressed, (int8_t *)bzip_data, compressed_len, 0);

    if (memcmp(decompressed, "MThd", 4) != 0) {
        free(decompressed);
        return false;
    }

    const char *temp_jingle_path = "bin/current_jingle.mid";
    FILE *out_f = fopen(temp_jingle_path, "wb");
    if (!out_f) {
        temp_jingle_path = "current_jingle.mid";
        out_f = fopen(temp_jingle_path, "wb");
    }
    if (!out_f) {
        free(decompressed);
        return false;
    }

    size_t written = fwrite(decompressed, 1, uncompressed_len, out_f);
    int close_result = fclose(out_f);
    free(decompressed);
    if (written != (size_t)uncompressed_len || close_result != 0) {
        fprintf(stderr, "[MIDI] Could not write jingle file: %s\n", temp_jingle_path);
        return false;
    }
    if (!midi_start_file(temp_jingle_path, "rs2jingle")) return false;

    printf("[MIDI] Playing jingle (%d bytes uncompressed)\n", uncompressed_len);
    return true;
}

void midi_stop(void) {
#ifdef _WIN32
    mciSendStringA("stop rs2midi", NULL, 0, NULL);
    mciSendStringA("close rs2midi", NULL, 0, NULL);
    mciSendStringA("stop rs2jingle", NULL, 0, NULL);
    mciSendStringA("close rs2jingle", NULL, 0, NULL);
#endif
    s_is_playing = false;
    s_current_song[0] = '\0';
}

void midi_set_volume(int vol_0_to_255) {
    if (vol_0_to_255 < 0) vol_0_to_255 = 0;
    if (vol_0_to_255 > 255) vol_0_to_255 = 255;
    s_volume = vol_0_to_255;
#ifdef _WIN32
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "setaudio rs2midi volume to %d", (s_volume * 1000) / 255);
    mciSendStringA(cmd, NULL, 0, NULL);
#endif
}

bool midi_is_playing(void) {
#ifdef _WIN32
    if (!s_is_playing) return false;
    char mode[64] = {0};
    if (!midi_mci_ok(mciSendStringA("status rs2midi mode", mode, sizeof(mode) - 1, NULL), "status")) {
        s_is_playing = false;
        s_current_song[0] = '\0';
        return false;
    }
    if (strcmp(mode, "playing") == 0) return true;
    if (strcmp(mode, "stopped") == 0) {
        // Auto-loop ambient music
        if (midi_mci_ok(mciSendStringA("play rs2midi from 0", NULL, 0, NULL), "loop")) return true;
        mciSendStringA("close rs2midi", NULL, 0, NULL);
    }
    s_is_playing = false;
    s_current_song[0] = '\0';
    return false;
#endif
    return s_is_playing;
}

const char* midi_current_song(void) {
    return s_current_song;
}

void midi_close(void) {
    midi_stop();
    s_midi_inited = false;
}
