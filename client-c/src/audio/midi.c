#include "audio/midi.h"
#include "core/bzip2.h"
#include "audio/wave.h"
#include "core/session_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static bool s_midi_inited;
static bool s_is_playing;
static char s_current_song[64];
static int s_attenuation225;
static uint32_t s_current_crc, s_current_size;
static uint16_t s_jingle_delay;
static bool s_silent, s_stage225;
static char s_request_names225[5][64];
static uint32_t s_request_crc225[5],s_request_size225[5];
#include "midi_device225.h"

void midi_set_silent(bool silent) {
    if(silent && !s_silent)md_close225();
    s_silent=silent;
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

bool midi_play(const char *name, uint32_t crc, uint32_t size) {
    if (!s_midi_inited) midi_init();
    if (s_silent && !s_stage225 && name && name[0]) return false;
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
    if(!s_stage225)s_jingle_delay=0;

    if (!s_stage225 && strcmp(s_current_song, name) == 0 && crc == s_current_crc && size == s_current_size && midi_is_playing()) {
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
    if(s_stage225){bool accepted=audio_request_midi225(decompressed,(int)uncomp_len,true);
        if(accepted){int slot=audio_midi_position225();strcpy(s_request_names225[slot],name);s_request_crc225[slot]=crc;s_request_size225[slot]=size;}else free(decompressed);
        return true;
    }

    s_is_playing = false;
    s_current_song[0] = '\0';
    bool started = md_submit225(decompressed,uncomp_len,true,s_attenuation225);
    free(decompressed);
    if (!started) return false;

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
    // One worker-owned stream: a jingle replaces ambient playback.
    s_is_playing = false;
    bool started = md_submit225(decompressed,(size_t)uncompressed_len,false,s_attenuation225);
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

bool midi_request_jingle225(const uint8_t *packed,int length,int decoded_length){
    uint8_t *decoded=NULL;
    if(length<=0 || decoded_length<4 || !midi_decode_stream(packed,(size_t)length,(size_t)decoded_length,&decoded))return false;
    bool accepted=audio_request_midi225(decoded,decoded_length,false);
    if(!accepted)free(decoded);
    return true; /* Original saveMidi is void: a busy shared slot drops it. */
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
    if(!s_silent)md_stop225();
    s_is_playing=false;s_current_song[0]=0;s_jingle_delay=0;
}

void midi_set_attenuation225(int attenuation) {
    if(attenuation>0)attenuation=0;
    if(attenuation < -10000)attenuation=-10000;
    s_attenuation225=attenuation;
    if(!s_silent)md_set_volume225(attenuation);
}
void midi_set_volume(int volume) {
    if(volume<0)volume=0;
    if(volume>255)volume=255;
    midi_set_attenuation225(volume?(int)floor(2000.0*log10((double)volume/255.0)+0.5):-10000);
}

void midi_apply_music225(Music225 *music) {
    if(!music)return;
    uint8_t *published=NULL;int length=0,slot=0;bool fade=false,ambient=false;
    if(audio_take_midi225(&published,&length,&fade,&ambient,&slot)){
        if(!s_silent && session_log_mode()!=SESSION_LOG_REPLAY){
            s_jingle_delay=0;
            s_is_playing=false;
            bool accepted=md_submit225(published,(size_t)length,ambient,s_attenuation225);
            if(ambient){s_is_playing=accepted;strcpy(s_current_song,s_request_names225[slot]);s_current_crc=s_request_crc225[slot];s_current_size=s_request_size225[slot];}
            midi_set_attenuation225(s_attenuation225);
        }
        free(published);
    }
    if(music->command==2){audio_midi_fade225(false);midi_stop();}
    /* client.java setMidiVolume publishes the original signed hundredths-dB
     * value. Do not round it through an 8-bit MIDI controller or alter CC7. */
    if(music->volume!=s_attenuation225 || music->command==1)midi_set_attenuation225(music->volume);
    music->command=0;
    if(music->pending){
        music->pending=false;
        s_stage225=true;
        if(!music->requested[0] || !midi_play(music->requested,music->requested_crc,music->requested_size))
            fprintf(stderr,"[MIDI] Requested song unavailable or rejected\n");
        s_stage225=false;
    }
}

bool midi_is_playing(void) {
    if(!s_is_playing || s_silent)return false;
    if(md_playing225())return true;
    s_is_playing=false;s_current_song[0]=0;return false;
}

const char* midi_current_song(void) {
    return s_current_song;
}

void midi_close(void) {
    midi_stop();
    md_close225();
    s_midi_inited = false;
}
