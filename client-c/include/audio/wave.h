#ifndef RS2_WAVE_H
#define RS2_WAVE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "core/packet.h"
#include "core/jagfile.h"

typedef struct {
    int length;
    int *shape_delta;
    int *shape_peak;
    int start;
    int end;
    int form;
    int threshold;
    int position;
    int delta;
    int amplitude;
    int ticks;
} Envelope;

void envelope_read(Envelope *env, Packet *pkt);
void envelope_reset(Envelope *env);
int  envelope_evaluate(Envelope *env, int duration);
void envelope_free(Envelope *env);

typedef struct {
    Envelope *frequency_base;
    Envelope *amplitude_base;
    Envelope *frequency_mod_rate;
    Envelope *frequency_mod_range;
    Envelope *amplitude_mod_rate;
    Envelope *amplitude_mod_range;
    Envelope *release;
    Envelope *attack;
    int harmonic_volume[5];
    int harmonic_semitone[5];
    int harmonic_delay[5];
    int reverb_delay;
    int reverb_volume;
    int length;
    int start;
} Tone;

void tone_init(void);
void tone_read(Tone *tone, Packet *pkt);
void tone_free(Tone *tone);

typedef struct {
    Tone *tones[10];
    int loop_begin;
    int loop_end;
} Wave;

void wave_system_init(void);
bool wave_unpack(Jagfile *sounds_jag);
int  wave_trim(Wave *w);
uint8_t *wave_generate(int track_id, int loop_count, size_t *out_pcm_len, size_t *out_wav_len);
void wave_free_all(void);

// SDL2 Real-time Audio
bool audio_init(void);
void audio_play(int sound_id, int loop_count);
void audio_queue_sound(int sound_id, int loop_count, int delay_ticks);
void audio_tick(void);
int  audio_queued_count(void);
void audio_close(void);

#endif // RS2_WAVE_H
