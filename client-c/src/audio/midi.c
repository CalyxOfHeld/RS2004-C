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
static unsigned s_file_serial;
static char s_music_path[160], s_jingle_path[160];
static uint32_t s_current_crc, s_current_size;
static uint16_t s_jingle_delay;
static bool s_silent;   // no playback device: headless replay

void midi_set_silent(bool silent) { s_silent = silent; }

static void midi_close_jingle(void) {
#ifdef _WIN32
    mciSendStringA("close rs2jingle", NULL, 0, NULL);
#endif
    if (s_jingle_path[0]) remove(s_jingle_path);
    s_jingle_path[0] = '\0';
    s_jingle_delay = 0;
}

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

static uint32_t midi_crc32(const uint8_t *data, size_t length) {
    uint32_t crc = UINT32_MAX;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; bit++) crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

static bool midi_decode_stream(const uint8_t *src, size_t packed_size, size_t size, uint8_t **decoded) {
    if (!src || !packed_size || packed_size > MIDI_MAX_PACKED || size < 4 || size > MIDI_MAX_DECODED) return false;
    uint8_t *data = malloc(size);
    if (!data) return false;
    if (!bzip_decompress_exact(data, size, src, packed_size) ||
        (memcmp(data,"MThd",4) && !(size >= 12 && !memcmp(data,"RIFF",4) && !memcmp(data+8,"RMID",4)))) {
        free(data); return false;
    }
    // The actual cache includes an RMID-wrapped attack1 song as well as SMF.
    *decoded = data;
    return true;
}

bool midi_unpack(const uint8_t *packed, size_t packed_size, uint32_t expected_crc,
    size_t expected_size, uint8_t **decoded, size_t *decoded_size) {
    if (!decoded || !decoded_size) return false;
    *decoded = NULL; *decoded_size = 0;
    if (!packed || packed_size < 5 || packed_size > MIDI_MAX_PACKED ||
        (expected_size && (expected_size != packed_size ||
        (expected_crc != 12345678u && midi_crc32(packed,packed_size) != expected_crc)))) return false;
    uint32_t size = (uint32_t)packed[0]<<24 | (uint32_t)packed[1]<<16 | (uint32_t)packed[2]<<8 | packed[3];
    if (!midi_decode_stream(packed+4,packed_size-4,size,decoded)) return false;
    *decoded_size = size;
    return true;
}

static bool midi_save_start(const uint8_t *data, size_t size, const char *alias, char path[160]) {
    char next[160];
#ifdef _WIN32
    unsigned long pid = GetCurrentProcessId();
#else
    unsigned long pid = (unsigned long)getpid();
#endif
    snprintf(next,sizeof(next),"bin/%s-%lu-%u.mid",alias,pid,s_file_serial++);
    FILE *f = fopen(next,"wb");
    if (!f) return false;
    size_t written = fwrite(data,1,size,f);
    int closed = fclose(f);
    if (written != size || closed) { remove(next); return false; }
    // Write a different file before closing the previous sequencer handle.
    bool ok = midi_start_file(next,alias);
    if (path[0]) remove(path);
    path[0] = '\0';
    if (!ok) { remove(next); return false; }
    snprintf(path,160,"%s",next);
    return true;
}

bool midi_play(const char *name, uint32_t crc, uint32_t size) {
    if (!s_midi_inited) midi_init();
    if (s_silent && name && name[0]) return false;
    if (!name || name[0] == '\0') {
        midi_stop();
        return true;
    }

    size_t name_len = strlen(name);
    if (name_len >= sizeof(s_current_song) || size > MIDI_MAX_PACKED) return false;
    for (size_t i = 0; i < name_len; i++)
        if (!((name[i] >= 'a' && name[i] <= 'z') || (name[i] >= '0' && name[i] <= '9') ||
            name[i] == '_' || name[i] == '-' || name[i] == ' ')) return false;

    // A new song packet cancels the jingle's pending ambient restart.
    midi_close_jingle();

    if (strcmp(s_current_song, name) == 0 && crc == s_current_crc && size == s_current_size && midi_is_playing()) {
        return true;
    }

    char found_path[512];
    FILE *f = open_song_file(name, found_path, sizeof(found_path));
    if (!f) {
        fprintf(stderr, "[MIDI] Song not found: %s\n", name);
        return false;
    }

    if (fseek(f, 0, SEEK_END)) { fclose(f); return false; }
    long file_size = ftell(f);
    if (fseek(f, 0, SEEK_SET)) { fclose(f); return false; }

    if (file_size < 8 || (unsigned long)file_size > MIDI_MAX_PACKED) {
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

    uint8_t *decompressed;
    size_t uncomp_len;
    bool decoded = midi_unpack(file_data,(size_t)file_size,crc,size,&decompressed,&uncomp_len);
    free(file_data);
    if (!decoded) { fprintf(stderr,"[MIDI] Rejected song data/metadata: %s\n",name); return false; }

    s_is_playing = false;
    s_current_song[0] = '\0';
    bool started = midi_save_start(decompressed,uncomp_len,"rs2midi",s_music_path);
    free(decompressed);
    if (!started) return false;
#ifdef _WIN32
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "setaudio rs2midi volume to %d", (s_volume * 1000) / 255);
    mciSendStringA(cmd, NULL, 0, NULL);
#endif

    strncpy(s_current_song, name, sizeof(s_current_song) - 1);
    s_current_song[sizeof(s_current_song) - 1] = '\0';
    s_current_crc = crc; s_current_size = size;
    s_is_playing = true;
    printf("[MIDI] Playing music track: %s (%zu bytes uncompressed)\n", name, uncomp_len);
    return true;
}

bool midi_play_jingle(const uint8_t *bzip_data, int compressed_len, int uncompressed_len, uint16_t delay) {
    if (s_silent) return false;
    if (compressed_len <= 0 || uncompressed_len < 4) return false;
    uint8_t *decompressed;
    if (!midi_decode_stream(bzip_data,(size_t)compressed_len,(size_t)uncompressed_len,&decompressed)) return false;
    // The original saveMidi replaces the sequencer's current song. Keep its
    // metadata for the original delayed setMidi restart, never mix both tracks.
    // Windows sequencers can refuse a second play while the first is open.
#ifdef _WIN32
    mciSendStringA("close rs2midi", NULL, 0, NULL);
#endif
    if (s_music_path[0]) remove(s_music_path);
    s_music_path[0] = '\0';
    s_is_playing = false;
    bool started = midi_save_start(decompressed,(size_t)uncompressed_len,"rs2jingle",s_jingle_path);
    free(decompressed);
    if (!started) {
        char song[sizeof(s_current_song)];memcpy(song,s_current_song,sizeof(song));
        midi_play(song,s_current_crc,s_current_size);
        return false;
    }
    s_jingle_delay = delay;

    printf("[MIDI] Playing jingle (%d bytes uncompressed)\n", uncompressed_len);
    return true;
}

bool midi_delay_step(uint16_t *delay) {
    if (!delay || !*delay) return false;
    *delay = *delay > 20 ? (uint16_t)(*delay - 20) : 0;
    return *delay == 0;
}

void midi_tick(void) {
    // client.java:7711-7717. A zero-delay jingle never schedules a restart.
    if (midi_delay_step(&s_jingle_delay)) {
        char song[sizeof(s_current_song)];
        memcpy(song,s_current_song,sizeof(song));
        uint32_t crc=s_current_crc,size=s_current_size;
        if (!midi_play(song,crc,size))
            fprintf(stderr,"[MIDI] Ambient restart rejected or unavailable\n");
    }
}

void midi_stop(void) {
#ifdef _WIN32
    if (!s_silent) {
        mciSendStringA("stop rs2midi", NULL, 0, NULL);
        mciSendStringA("close rs2midi", NULL, 0, NULL);
        mciSendStringA("stop rs2jingle", NULL, 0, NULL);
        mciSendStringA("close rs2jingle", NULL, 0, NULL);
    }
#endif
    s_is_playing = false;
    s_current_song[0] = '\0';
    s_jingle_delay = 0;
    if (s_music_path[0]) remove(s_music_path);
    if (s_jingle_path[0]) remove(s_jingle_path);
    s_music_path[0] = s_jingle_path[0] = '\0';
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
