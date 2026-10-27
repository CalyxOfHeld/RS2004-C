#include "audio/wave.h"
#include "core/jrandom.h"
#include "core/session_log.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Tone precalculated lookup tables
static int s_noise[32768];
static int s_sin[32768];
static int s_tone_buffer[220500];
static uint8_t s_wave_bytes[441000];

static Wave *s_tracks[1000];
static int s_delays[1000];
static bool s_tone_inited = false;
static SDL_AudioDeviceID s_audio_dev = 0;
static SDL_mutex *s_wave_mutex225;
static SDL_Thread *s_wave_worker225;
static SDL_atomic_t s_wave_stop225;
typedef struct WaveOutput225 {struct WaveOutput225 *next;size_t length,position;uint8_t samples[];} WaveOutput225;
static SDL_mutex *s_output_mutex225;
static WaveOutput225 *s_output_head225,*s_output_tail225;
static SDL_atomic_t s_output_attenuation225;
void audio_set_wave_volume225(int volume){if(volume==0 || volume==-400 || volume==-800 || volume==-1200)SDL_AtomicSet(&s_output_attenuation225,volume);}
double audio_wave_gain225(void){return pow(10.0,(double)SDL_AtomicGet(&s_output_attenuation225)/2000.0);}
float audio_scale_sample225(uint8_t sample){return (float)(((int)sample-128)*(audio_wave_gain225()/128.0));}

// Envelope implementation
void envelope_read(Envelope *env, Packet *pkt) {
    if (!env || !pkt) return;
    env->form = g1(pkt);
    env->start = (int)g4(pkt);
    env->end = (int)g4(pkt);
    env->length = g1(pkt);
    if(pkt->error || env->length==0) {pkt->error=true;return;}
    env->shape_delta = (int *)malloc(env->length * sizeof(int));
    env->shape_peak = (int *)malloc(env->length * sizeof(int));
    for (int i = 0; i < env->length; i++) {
        env->shape_delta[i] = g2(pkt);
        env->shape_peak[i] = g2(pkt);
    }
}

void envelope_reset(Envelope *env) {
    if (!env) return;
    env->threshold = 0;
    env->position = 0;
    env->delta = 0;
    env->amplitude = 0;
    env->ticks = 0;
}

int envelope_evaluate(Envelope *env, int duration) {
    if (!env) return 0;
    if (env->ticks >= env->threshold) {
        env->amplitude = env->shape_peak[env->position++] << 15;
        if (env->position >= env->length) {
            env->position = env->length - 1;
        }
        env->threshold = (int)((double)env->shape_delta[env->position] / 65536.0 * (double)duration);
        if (env->threshold > env->ticks) {
            env->delta = ((env->shape_peak[env->position] << 15) - env->amplitude) / (env->threshold - env->ticks);
        }
    }
    env->amplitude += env->delta;
    env->ticks++;
    return (env->amplitude - env->delta) >> 15;
}

void envelope_free(Envelope *env) {
    if (!env) return;
    if (env->shape_delta) free(env->shape_delta);
    if (env->shape_peak) free(env->shape_peak);
    free(env);
}

// Tone implementation
void tone_init(void) {
    if (s_tone_inited) return;

    // Deterministic pseudo-random noise table
    uint32_t seed = 0x12345678;
    for (int i = 0; i < 32768; i++) {
        seed = seed * 1103515245 + 12345;
        s_noise[i] = (seed & 0x80000000) ? 1 : -1;
    }

    // 16384 amplitude sine wave table
    for (int i = 0; i < 32768; i++) {
        s_sin[i] = (int)(sin((double)i / 5215.1903) * 16384.0);
    }

    s_tone_inited = true;
}

void tone_init225(void) {
    tone_init();
    for (int i = 0; i < 32768; i++) s_noise[i] = math_random() > 0.5 ? 1 : -1;
}

static int tone_sample(int form, int amp, int phase) {
    if (form == 1) { // Square
        return (phase & 0x7FFF) < 16384 ? amp : -amp;
    } else if (form == 2) { // Sine
        return (s_sin[phase & 0x7FFF] * amp) >> 14;
    } else if (form == 3) { // Sawtooth
        return (((phase & 0x7FFF) * amp) >> 14) - amp;
    } else if (form == 4) { // Noise
        return s_noise[(phase / 2607) & 0x7FFF] * amp;
    }
    return 0;
}

static int *tone_generate(Tone *t, int sample_count, int duration) {
    for (int i = 0; i < sample_count; i++) {
        s_tone_buffer[i] = 0;
    }
    if (duration < 10) return s_tone_buffer;

    double step = (double)sample_count / (double)duration;
    envelope_reset(t->frequency_base);
    envelope_reset(t->amplitude_base);

    int fmod_rate_step = 0;
    int fmod_rate_start = 0;
    int fmod_phase = 0;
    if (t->frequency_mod_rate) {
        envelope_reset(t->frequency_mod_rate);
        envelope_reset(t->frequency_mod_range);
        fmod_rate_step = (int)((double)(t->frequency_mod_rate->end - t->frequency_mod_rate->start) * 32.768 / step);
        fmod_rate_start = (int)((double)t->frequency_mod_rate->start * 32.768 / step);
    }

    int amod_rate_step = 0;
    int amod_rate_start = 0;
    int amod_phase = 0;
    if (t->amplitude_mod_rate) {
        envelope_reset(t->amplitude_mod_rate);
        envelope_reset(t->amplitude_mod_range);
        amod_rate_step = (int)((double)(t->amplitude_mod_rate->end - t->amplitude_mod_rate->start) * 32.768 / step);
        amod_rate_start = (int)((double)t->amplitude_mod_rate->start * 32.768 / step);
    }

    int tmp_phases[5] = {0};
    int tmp_delays[5] = {0};
    int tmp_volumes[5] = {0};
    int tmp_semitones[5] = {0};
    int tmp_starts[5] = {0};

    for (int h = 0; h < 5; h++) {
        if (t->harmonic_volume[h] != 0) {
            tmp_phases[h] = 0;
            tmp_delays[h] = (int)((double)t->harmonic_delay[h] * step);
            tmp_volumes[h] = (t->harmonic_volume[h] << 14) / 100;
            tmp_semitones[h] = (int)((double)(t->frequency_base->end - t->frequency_base->start) * 32.768 *
                                     pow(1.0057929410678534, (double)t->harmonic_semitone[h]) / step);
            tmp_starts[h] = (int)((double)t->frequency_base->start * 32.768 / step);
        }
    }

    for (int i = 0; i < sample_count; i++) {
        int f = envelope_evaluate(t->frequency_base, sample_count);
        int a = envelope_evaluate(t->amplitude_base, sample_count);

        if (t->frequency_mod_rate) {
            int rate = envelope_evaluate(t->frequency_mod_rate, sample_count);
            int range = envelope_evaluate(t->frequency_mod_range, sample_count);
            f += tone_sample(t->frequency_mod_rate->form, range, fmod_phase) >> 1;
            fmod_phase += (rate * fmod_rate_step >> 16) + fmod_rate_start;
        }

        if (t->amplitude_mod_rate) {
            int rate = envelope_evaluate(t->amplitude_mod_rate, sample_count);
            int range = envelope_evaluate(t->amplitude_mod_range, sample_count);
            a = a * ((tone_sample(t->amplitude_mod_rate->form, range, amod_phase) >> 1) + 32768) >> 15;
            amod_phase += (rate * amod_rate_step >> 16) + amod_rate_start;
        }

        for (int h = 0; h < 5; h++) {
            if (t->harmonic_volume[h] != 0) {
                int dst_idx = i + tmp_delays[h];
                if (dst_idx < sample_count) {
                    s_tone_buffer[dst_idx] += tone_sample(t->frequency_base->form,
                                                          a * tmp_volumes[h] >> 15,
                                                          tmp_phases[h]);
                    tmp_phases[h] += (f * tmp_semitones[h] >> 16) + tmp_starts[h];
                }
            }
        }
    }

    // Release / attack gating
    if (t->release) {
        envelope_reset(t->release);
        envelope_reset(t->attack);
        int gate_counter = 0;
        bool state = true;
        for (int i = 0; i < sample_count; i++) {
            int rel_eval = envelope_evaluate(t->release, sample_count);
            int att_eval = envelope_evaluate(t->attack, sample_count);
            int threshold = state ? (t->release->start + ((t->release->end - t->release->start) * rel_eval >> 8))
                                  : (t->release->start + ((t->release->end - t->release->start) * att_eval >> 8));
            gate_counter += 256;
            if (gate_counter >= threshold) {
                gate_counter = 0;
                state = !state;
            }
            if (state) {
                s_tone_buffer[i] = 0;
            }
        }
    }

    // Reverb filter
    if (t->reverb_delay > 0 && t->reverb_volume > 0) {
        int r_delay = (int)((double)t->reverb_delay * step);
        for (int i = r_delay; i < sample_count; i++) {
            s_tone_buffer[i] += s_tone_buffer[i - r_delay] * t->reverb_volume / 100;
        }
    }

    // 16-bit clamp
    for (int i = 0; i < sample_count; i++) {
        if (s_tone_buffer[i] < -32768) s_tone_buffer[i] = -32768;
        if (s_tone_buffer[i] > 32767)  s_tone_buffer[i] = 32767;
    }

    return s_tone_buffer;
}

void tone_read(Tone *tone, Packet *pkt) {
    if (!tone || !pkt) return;
    tone->frequency_base = (Envelope *)calloc(1, sizeof(Envelope));
    envelope_read(tone->frequency_base, pkt);

    tone->amplitude_base = (Envelope *)calloc(1, sizeof(Envelope));
    envelope_read(tone->amplitude_base, pkt);

    int code = g1(pkt);
    if (code != 0) {
        pkt->pos--;
        tone->frequency_mod_rate = (Envelope *)calloc(1, sizeof(Envelope));
        envelope_read(tone->frequency_mod_rate, pkt);
        tone->frequency_mod_range = (Envelope *)calloc(1, sizeof(Envelope));
        envelope_read(tone->frequency_mod_range, pkt);
    }

    code = g1(pkt);
    if (code != 0) {
        pkt->pos--;
        tone->amplitude_mod_rate = (Envelope *)calloc(1, sizeof(Envelope));
        envelope_read(tone->amplitude_mod_rate, pkt);
        tone->amplitude_mod_range = (Envelope *)calloc(1, sizeof(Envelope));
        envelope_read(tone->amplitude_mod_range, pkt);
    }

    code = g1(pkt);
    if (code != 0) {
        pkt->pos--;
        tone->release = (Envelope *)calloc(1, sizeof(Envelope));
        envelope_read(tone->release, pkt);
        tone->attack = (Envelope *)calloc(1, sizeof(Envelope));
        envelope_read(tone->attack, pkt);
    }

    // Original reads a zero terminator even after all five harmonic slots.
    // Omitting that read shifts the remaining tone and every following track.
    for (int i = 0; i < 10; i++) {
        int vol = gsmarts(pkt);
        if (vol == 0) break;
        if(i>=5 || pkt->error) {pkt->error=true;return;}
        tone->harmonic_volume[i] = vol;
        tone->harmonic_semitone[i] = gsmart(pkt);
        tone->harmonic_delay[i] = gsmarts(pkt);
    }

    tone->reverb_delay = gsmarts(pkt);
    tone->reverb_volume = gsmarts(pkt);
    tone->length = g2(pkt);
    tone->start = g2(pkt);
}

void tone_free(Tone *tone) {
    if (!tone) return;
    envelope_free(tone->frequency_base);
    envelope_free(tone->amplitude_base);
    envelope_free(tone->frequency_mod_rate);
    envelope_free(tone->frequency_mod_range);
    envelope_free(tone->amplitude_mod_rate);
    envelope_free(tone->amplitude_mod_range);
    envelope_free(tone->release);
    envelope_free(tone->attack);
    free(tone);
}

// Wave implementation
void wave_system_init(void) {
    wave_free_all();
    tone_init();
    memset(s_tracks, 0, sizeof(s_tracks));
    memset(s_delays, 0, sizeof(s_delays));
}

int wave_trim(Wave *w) {
    int min_start = 9999999;
    for (int i = 0; i < 10; i++) {
        if (w->tones[i] && w->tones[i]->start / 20 < min_start) {
            min_start = w->tones[i]->start / 20;
        }
    }
    if (w->loop_begin < w->loop_end && w->loop_begin / 20 < min_start) {
        min_start = w->loop_begin / 20;
    }
    if (min_start == 9999999 || min_start == 0) {
        return 0;
    }
    for (int i = 0; i < 10; i++) {
        if (w->tones[i]) {
            w->tones[i]->start -= min_start * 20;
        }
    }
    if (w->loop_begin < w->loop_end) {
        w->loop_begin -= min_start * 20;
        w->loop_end -= min_start * 20;
    }
    return min_start;
}

bool wave_unpack(Jagfile *sounds_jag) {
    if (!sounds_jag) return false;
    wave_system_init();

    size_t sz = 0;
    uint8_t *data = jagfile_read(sounds_jag, "sounds.dat", NULL, &sz, NULL);
    if (!data) return false;

    Packet pkt = packet_wrap(data, sz);
    while (pkt.pos < pkt.length) {
        int track_id = g2(&pkt);
        if (track_id == 65535) break;
        if(pkt.error || track_id<0 || track_id>=1000 || s_tracks[track_id]) {
            free(data);wave_free_all();return false;
        }

        Wave *w = (Wave *)calloc(1, sizeof(Wave));
        for (int i = 0; i < 10; i++) {
            int code = g1(&pkt);
            if (code != 0) {
                pkt.pos--;
                w->tones[i] = (Tone *)calloc(1, sizeof(Tone));
                tone_read(w->tones[i], &pkt);
            }
            if(pkt.error) break;
        }
        w->loop_begin = g2(&pkt);
        w->loop_end = g2(&pkt);

        if(pkt.error) {
            for(int i=0;i<10;i++) tone_free(w->tones[i]);
            free(w);free(data);wave_free_all();return false;
        }

        s_tracks[track_id] = w;
        s_delays[track_id] = wave_trim(w);
    }

    free(data);
    return true;
}

static void write_le4(uint8_t *buf, uint32_t val) {
    buf[0] = (uint8_t)(val & 0xFF);
    buf[1] = (uint8_t)((val >> 8) & 0xFF);
    buf[2] = (uint8_t)((val >> 16) & 0xFF);
    buf[3] = (uint8_t)((val >> 24) & 0xFF);
}

static void write_le2(uint8_t *buf, uint16_t val) {
    buf[0] = (uint8_t)(val & 0xFF);
    buf[1] = (uint8_t)((val >> 8) & 0xFF);
}

static uint8_t *wave_generate_inner225(int track_id, int loop_count, size_t *out_pcm_len, size_t *out_wav_len) {
    if (track_id < 0 || track_id >= 1000 || !s_tracks[track_id]) return NULL;

    Wave *w = s_tracks[track_id];

    int max_dur = 0;
    for (int i = 0; i < 10; i++) {
        if (w->tones[i] && w->tones[i]->length + w->tones[i]->start > max_dur) {
            max_dur = w->tones[i]->length + w->tones[i]->start;
        }
    }
    if (max_dur == 0) return NULL;

    int total_samples = max_dur * 22050 / 1000;
    int loop_start = w->loop_begin * 22050 / 1000;
    int loop_end = w->loop_end * 22050 / 1000;

    if (loop_start < 0 || loop_start > total_samples || loop_end < 0 || loop_end > total_samples || loop_start >= loop_end) {
        loop_count = 0;
    }

    // Preserve original Wave.generate arithmetic even when invalid loop points
    // set loop_count to zero. Several actual cache tracks have reversed points.
    int64_t requested=(int64_t)total_samples+(int64_t)(loop_end-loop_start)*(loop_count-1);
    if(requested<0 || requested>(int64_t)sizeof(s_wave_bytes)-44 || total_samples>220500) return NULL;
    int pcm_samples=(int)requested;

    // Center baseline for 8-bit unsigned PCM is 128 (0x80)
    for (int i = 44; i < pcm_samples + 44; i++) {
        s_wave_bytes[i] = 128;
    }

    // Synthesize tones
    for (int i = 0; i < 10; i++) {
        if (w->tones[i]) {
            int t_samples = w->tones[i]->length * 22050 / 1000;
            int t_start   = w->tones[i]->start * 22050 / 1000;
            int *buf = tone_generate(w->tones[i], t_samples, w->tones[i]->length);
            for (int s = 0; s < t_samples; s++) {
                int dst = s + t_start + 44;
                if (dst < pcm_samples + 44) {
                    s_wave_bytes[dst] += (uint8_t)(buf[s] >> 8);
                }
            }
        }
    }

    // Duplicate loop segment if requested
    if (loop_count > 1) {
        int l_start = loop_start + 44;
        int l_end   = loop_end + 44;
        int orig_end = total_samples + 44;
        int tail_len = pcm_samples + 44 - orig_end;
        for (int i = orig_end - 1; i >= l_end; i--) {
            s_wave_bytes[i + tail_len] = s_wave_bytes[i];
        }
        for (int r = 1; r < loop_count; r++) {
            int step = (loop_end - loop_start) * r;
            for (int s = l_start; s < l_end; s++) {
                s_wave_bytes[s + step] = s_wave_bytes[s];
            }
        }
    }

    // 44-byte standard RIFF WAV header
    memcpy(s_wave_bytes, "RIFF", 4);
    write_le4(s_wave_bytes + 4, pcm_samples + 36);
    memcpy(s_wave_bytes + 8, "WAVE", 4);
    memcpy(s_wave_bytes + 12, "fmt ", 4);
    write_le4(s_wave_bytes + 16, 16); // PCM header size
    write_le2(s_wave_bytes + 20, 1);  // PCM format
    write_le2(s_wave_bytes + 22, 1);  // 1 Channel (Mono)
    write_le4(s_wave_bytes + 24, 22050); // Sample rate
    write_le4(s_wave_bytes + 28, 22050); // Byte rate
    write_le2(s_wave_bytes + 32, 1);  // Block align
    write_le2(s_wave_bytes + 34, 8);  // 8 bits per sample
    memcpy(s_wave_bytes + 36, "data", 4);
    write_le4(s_wave_bytes + 40, pcm_samples);

    if (out_pcm_len) *out_pcm_len = pcm_samples;
    if (out_wav_len) *out_wav_len = pcm_samples + 44;
    return s_wave_bytes;
}

uint8_t *wave_generate(int id,int loops,size_t *pcm,size_t *wav){
    if(s_wave_mutex225)SDL_LockMutex(s_wave_mutex225);
    uint8_t *bytes=wave_generate_inner225(id,loops,pcm,wav);
    if(s_wave_mutex225)SDL_UnlockMutex(s_wave_mutex225);
    return bytes;
}

void wave_free_all(void) {
    for (int i = 0; i < 1000; i++) {
        if (s_tracks[i]) {
            for (int t = 0; t < 10; t++) {
                if (s_tracks[i]->tones[t]) {
                    tone_free(s_tracks[i]->tones[t]);
                }
            }
            free(s_tracks[i]);
            s_tracks[i] = NULL;
        }
    }
}

/* Preserve original U8 source samples in the playback FIFO. Attenuation is
 * applied in float at playback time, never requantized into the source bytes.
 * The callback has a separate short lock, independent of waveform generation. */
static void SDLCALL audio_mix225(void *unused,Uint8 *stream,int bytes){
    (void)unused;memset(stream,0,(size_t)bytes);float *output=(float*)stream;size_t count=(size_t)bytes/sizeof(float);
    double gain=pow(10.0,(double)SDL_AtomicGet(&s_output_attenuation225)/2000.0);
    SDL_LockMutex(s_output_mutex225);
    for(size_t i=0;i<count && s_output_head225;i++){
        WaveOutput225 *chunk=s_output_head225;
        output[i]=(float)(((int)chunk->samples[chunk->position++]-128)*(gain/128.0));
        if(chunk->position==chunk->length){s_output_head225=chunk->next;if(!s_output_head225)s_output_tail225=NULL;free(chunk);}
    }
    SDL_UnlockMutex(s_output_mutex225);
}

// Real-time audio via SDL2
bool audio_init(void) {
    if (s_audio_dev != 0) return true;

    SDL_AudioSpec want, have;
    memset(&want, 0, sizeof(want));
    want.freq = 22050;
    want.format = AUDIO_F32SYS;
    want.channels = 1;
    want.samples = 512;
    want.callback = audio_mix225;
    if(!s_output_mutex225)s_output_mutex225=SDL_CreateMutex();
    if(!s_output_mutex225)return false;

    s_audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (s_audio_dev != 0) {
        SDL_PauseAudioDevice(s_audio_dev, 0);
        return true;
    }
    return false;
}

static SoundQueue225 s_sound_queue225={.last_id=-1,.last_loops=-1};
static uint8_t s_wave_slots225[5][441000];
static int s_wave_sizes225[5],s_wave_pos225,s_wave_pending225,s_wave_save_length225;
static const uint8_t *s_wave_save225;
static uint8_t *s_midi_pending225,*s_midi_slots225[5];
static int s_midi_sizes225[5],s_midi_pos225,s_midi_published225=-1;
static bool s_midi_fade225;
static bool s_midi_pending_ambient225,s_midi_ambient225[5];
static bool ensure_wave225(void){if(!s_wave_mutex225)s_wave_mutex225=SDL_CreateMutex();return s_wave_mutex225!=NULL;}
static void submit_wave225(const uint8_t *wav,int length){
    if(!s_audio_dev || !wav || length<44 || length>441000)return;
    size_t pcm=(size_t)length-44;if(!pcm)return;
    WaveOutput225 *chunk=malloc(sizeof(*chunk)+pcm);if(!chunk)return;
    chunk->next=NULL;chunk->length=pcm;chunk->position=0;memcpy(chunk->samples,wav+44,pcm);
    SDL_LockMutex(s_output_mutex225);
    if(s_output_tail225)s_output_tail225->next=chunk;else s_output_head225=chunk;
    s_output_tail225=chunk;SDL_UnlockMutex(s_output_mutex225);
#ifdef WAVE_PUBLICATION_OBSERVER225
    WAVE_PUBLICATION_OBSERVER225(wav+44,pcm);
#endif
}
void audio_play(int id,int loops){
    if(!ensure_wave225())return;
    SDL_LockMutex(s_wave_mutex225);size_t length=0;uint8_t *wav=wave_generate_inner225(id,loops,NULL,&length);if(wav)submit_wave225(wav,(int)length);SDL_UnlockMutex(s_wave_mutex225);
}
static uint64_t wave_hash225(const uint8_t *bytes,int length){uint64_t hash=1469598103934665603ULL;for(int i=0;i<length;i++){hash^=bytes[i];hash*=1099511628211ULL;}return hash;}
static int wave_outcome225(int kind,int length,uint64_t hash,int result){
    if(session_log_mode()==SESSION_LOG_REPLAY){long long k,n,h,r;if(!session_log_take("AQ") || !session_log_take_int(&k) || !session_log_take_int(&n) || !session_log_take_int(&h) || !session_log_take_int(&r) || k!=kind || n!=length || (uint64_t)h!=hash || r<0 || r>1){session_log_desync("sound request differs");return -1;}return (int)r;}
    if(session_log_mode()==SESSION_LOG_RECORD){session_log_begin("AQ");session_log_int(kind);session_log_int(length);session_log_int((long long)hash);session_log_int(result);session_log_end();}
    return result;
}
int audio_request_save225(const uint8_t *wav,int length){
    if(!wav)return 1; /* client.saveWave(null) succeeds without a signlink request. */
    if(length<0 || length>441000)return 0;
    uint64_t hash=wave_hash225(wav,length);if(session_log_mode()==SESSION_LOG_REPLAY)return wave_outcome225(1,length,hash,0);
    if(!ensure_wave225())return 0;
    SDL_LockMutex(s_wave_mutex225);int result=0;if(!s_wave_pending225){s_wave_pos225=(s_wave_pos225+1)%5;s_wave_save225=wav;s_wave_save_length225=length;s_wave_pending225=1;result=1;}SDL_UnlockMutex(s_wave_mutex225);
    return wave_outcome225(1,length,hash,result);
}
int audio_request_replay225(void){
    if(session_log_mode()==SESSION_LOG_REPLAY)return wave_outcome225(2,0,0,0);
    if(!ensure_wave225())return 0;
    SDL_LockMutex(s_wave_mutex225);int result=0;if(!s_wave_pending225){s_wave_save225=NULL;s_wave_pending225=2;result=1;}SDL_UnlockMutex(s_wave_mutex225);
    return wave_outcome225(2,0,0,result);
}
bool audio_request_midi225(uint8_t *bytes,int length,bool fade){
    audio_midi_fade225(fade); /* client.saveMidi updates it even when midisave is busy. */
    if(!bytes || length<0 || length>2000000)return false;
    if(!ensure_wave225())return false;
    uint64_t hash=wave_hash225(bytes,length);
    if(session_log_mode()==SESSION_LOG_REPLAY){long long n,h,f,r;
        if(!session_log_take("MQ") || !session_log_take_int(&n) || !session_log_take_int(&h) || !session_log_take_int(&f) || !session_log_take_int(&r) || n!=length || (uint64_t)h!=hash || f!=(fade?1:0) || r<0 || r>1){session_log_desync("MIDI request differs");return false;}
        if(r){free(bytes);SDL_LockMutex(s_wave_mutex225);s_midi_pos225=(s_midi_pos225+1)%5;SDL_UnlockMutex(s_wave_mutex225);}
        return r!=0;
    }
    if(!ensure_wave225())return false;
    SDL_LockMutex(s_wave_mutex225);bool accepted=!s_wave_pending225;
    if(accepted){s_midi_pos225=(s_midi_pos225+1)%5;s_midi_pending225=bytes;s_midi_pending_ambient225=fade;s_wave_save_length225=length;s_wave_pending225=3;}
    SDL_UnlockMutex(s_wave_mutex225);
    if(session_log_mode()==SESSION_LOG_RECORD){session_log_begin("MQ");session_log_int(length);session_log_int((long long)hash);session_log_int(fade?1:0);session_log_int(accepted?1:0);session_log_end();}
    return accepted;
}
void audio_midi_fade225(bool fade){if(!ensure_wave225())return;SDL_LockMutex(s_wave_mutex225);s_midi_fade225=fade;SDL_UnlockMutex(s_wave_mutex225);}
int audio_midi_position225(void){if(!ensure_wave225())return 0;SDL_LockMutex(s_wave_mutex225);int slot=s_midi_pos225;SDL_UnlockMutex(s_wave_mutex225);return slot;}
bool audio_take_midi225(uint8_t **bytes,int *length,bool *fade,bool *ambient,int *slot_out){
    if(!bytes || !length || !fade || !ambient || !slot_out)return false;
    if(session_log_mode()==SESSION_LOG_REPLAY){const char *tag=session_log_peek();if(!tag || strcmp(tag,"MP"))return false;
        uint8_t *copy=malloc(2000000);size_t n=0;long long f,a,slot;
        if(!copy || !session_log_take("MP") || !session_log_take_int(&f) || f<0 || f>1 || !session_log_take_int(&a) || a<0 || a>1 || !session_log_take_int(&slot) || slot<0 || slot>4 || !session_log_take_bytes(copy,2000000,&n)){free(copy);session_log_desync("MIDI publication differs");return false;}
        *bytes=copy;*length=(int)n;*fade=f!=0;*ambient=a!=0;*slot_out=(int)slot;return true;
    }
    if(!s_wave_mutex225)return false;
    SDL_LockMutex(s_wave_mutex225);bool available=s_midi_published225>=0;
    if(available){int slot=s_midi_published225,n=s_midi_sizes225[slot];uint8_t *copy=malloc(n?(size_t)n:1);
        if(copy){memcpy(copy,s_midi_slots225[slot],(size_t)n);*bytes=copy;*length=n;*fade=s_midi_fade225;*ambient=s_midi_ambient225[slot];*slot_out=slot;s_midi_published225=-1;}else available=false;
    }
    SDL_UnlockMutex(s_wave_mutex225);
    if(available && session_log_mode()==SESSION_LOG_RECORD){session_log_begin("MP");session_log_int(*fade?1:0);session_log_int(*ambient?1:0);session_log_int(*slot_out);session_log_bytes(*bytes,(size_t)*length);session_log_end();}
    return available;
}
void audio_service225(void){
    if(!s_wave_mutex225)return;
    SDL_LockMutex(s_wave_mutex225);
    if(s_wave_pending225==3){free(s_midi_slots225[s_midi_pos225]);s_midi_slots225[s_midi_pos225]=s_midi_pending225;s_midi_pending225=NULL;s_midi_sizes225[s_midi_pos225]=s_wave_save_length225;s_midi_ambient225[s_midi_pos225]=s_midi_pending_ambient225;s_midi_published225=s_midi_pos225;s_wave_pending225=0;}
    else if(s_wave_pending225){if(s_wave_pending225==1){memcpy(s_wave_slots225[s_wave_pos225],s_wave_save225,(size_t)s_wave_save_length225);s_wave_sizes225[s_wave_pos225]=s_wave_save_length225;}submit_wave225(s_wave_slots225[s_wave_pos225],s_wave_sizes225[s_wave_pos225]);s_wave_pending225=0;}
    SDL_UnlockMutex(s_wave_mutex225);
}
static int wave_worker225(void *unused){(void)unused;while(!SDL_AtomicGet(&s_wave_stop225)){audio_service225();SDL_Delay(50);}return 0;}
bool audio_start_worker225(void){if(s_wave_worker225)return true;if(!ensure_wave225())return false;SDL_AtomicSet(&s_wave_stop225,0);s_wave_worker225=SDL_CreateThread(wave_worker225,"wave225",NULL);return s_wave_worker225!=NULL;}
void audio_queue_cold225(void){sound_queue_init225(&s_sound_queue225);if(s_wave_mutex225)SDL_LockMutex(s_wave_mutex225);s_wave_pending225=s_wave_pos225=s_wave_save_length225=0;s_wave_save225=NULL;memset(s_wave_sizes225,0,sizeof(s_wave_sizes225));free(s_midi_pending225);s_midi_pending225=NULL;for(int i=0;i<5;i++){free(s_midi_slots225[i]);s_midi_slots225[i]=NULL;s_midi_sizes225[i]=0;}s_midi_pos225=0;s_midi_published225=-1;s_midi_fade225=false;if(s_wave_mutex225)SDL_UnlockMutex(s_wave_mutex225);}
void audio_queue_clear225(void){s_sound_queue225.count=0;}
const SoundQueue225 *audio_queue_state225(void){return &s_sound_queue225;}
void audio_queue_sound(int id,int loops,int delay){if(id<0 || id>=1000)return;uint32_t bits=(uint32_t)delay+(uint32_t)s_delays[id];int32_t total;memcpy(&total,&bits,4);(void)sound_queue_add225(&s_sound_queue225,id,loops,total);}
static int queue_generate225(void *unused,int id,int loops,const uint8_t **bytes){(void)unused;size_t length=0;*bytes=wave_generate(id,loops,NULL,&length);return *bytes?(int)length:-1;}
static int queue_save225(void *unused,const uint8_t *bytes,int length){(void)unused;return audio_request_save225(bytes,length);}
static int queue_replay225(void *unused){(void)unused;return audio_request_replay225();}
static int64_t queue_now225(void *user){int64_t (*clock)(void)=*(int64_t (**)(void))user;return clock();}
void audio_tick225(int64_t (*clock)(void)){if(!clock)return;SoundQueueOps225 ops={&clock,queue_generate225,queue_now225,queue_save225,queue_replay225};(void)sound_queue_tick225(&s_sound_queue225,&ops);if(!s_wave_worker225 && session_log_mode()!=SESSION_LOG_REPLAY)audio_service225();}
static int64_t compatibility_clock225(void){return (int64_t)SDL_GetTicks64();}
void audio_tick(void){audio_tick225(compatibility_clock225);}
bool audio_queue_get225(int index,AudioQueuedSound225 *out){if(!out || index<0 || index>=s_sound_queue225.count)return false;*out=s_sound_queue225.entries[index];return true;}
int audio_queued_count(void){return s_sound_queue225.count;}
void audio_close(void){
    if(s_wave_worker225){SDL_AtomicSet(&s_wave_stop225,1);SDL_WaitThread(s_wave_worker225,NULL);s_wave_worker225=NULL;}
    if(s_wave_mutex225)SDL_LockMutex(s_wave_mutex225);
    if(s_audio_dev){SDL_CloseAudioDevice(s_audio_dev);s_audio_dev=0;}
    if(s_output_mutex225){
        while(s_output_head225){WaveOutput225 *next=s_output_head225->next;free(s_output_head225);s_output_head225=next;}
        s_output_tail225=NULL;SDL_DestroyMutex(s_output_mutex225);s_output_mutex225=NULL;
    }
    s_wave_pending225=0;if(s_wave_mutex225){SDL_UnlockMutex(s_wave_mutex225);SDL_DestroyMutex(s_wave_mutex225);s_wave_mutex225=NULL;}
    audio_queue_cold225();
}
