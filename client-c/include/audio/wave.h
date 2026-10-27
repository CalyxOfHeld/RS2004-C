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
// Original Tone.init noise from the shared Math.random generator; opt-in.
void tone_init225(void);
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

#include "core/sound_queue.h"
const SoundQueue225 *audio_queue_state225(void);
void audio_queue_cold225(void);
void audio_queue_clear225(void);
void audio_tick225(int64_t (*clock)(void));
bool audio_start_worker225(void);
void audio_service225(void);
/* Borrows bytes until service; normal generation uses the stable shared Wave buffer. */
int audio_request_save225(const uint8_t *wav,int length);
int audio_request_replay225(void);
/* Transfers ownership on success. Shares the original single save-request slot. */
bool audio_request_midi225(uint8_t *bytes,int length,bool fade);
/* Main-thread device publication: caller owns returned bytes. */
bool audio_take_midi225(uint8_t **bytes,int *length,bool *fade,bool *ambient,int *slot);
int audio_midi_position225(void);
void audio_midi_fade225(bool fade);
bool audio_queue_get225(int index,AudioQueuedSound225 *out);
void audio_set_wave_volume225(int hundredths_db);
double audio_wave_gain225(void);
/* Original unsigned PCM normalized to float, attenuated without U8 rounding. */
float audio_scale_sample225(uint8_t sample);

// SDL2 Real-time Audio
bool audio_init(void);
void audio_play(int sound_id, int loop_count);
void audio_queue_sound(int sound_id, int loop_count, int delay_ticks);
void audio_tick(void);
int  audio_queued_count(void);
void audio_close(void);

#endif // RS2_WAVE_H
