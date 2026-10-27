#define SDL_MAIN_HANDLED
#include "audio/wave.h"
#include "audio/midi.h"
#include "core/arena.h"
#include "core/jagfile.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../verification/completion/tools/wave_setting_reference_control.h"
#include "../verification/completion/tools/sound_queue_reference_control.h"
#include "../verification/completion/tools/music_state_reference_control.h"

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    printf("=== Test Suite: Sound Synthesizer & Audio Engine ===\n");

    Arena arena = arena_create(32 * 1024 * 1024);

    Jagfile *sounds_jag = jagfile_load_file("../engine/data/pack/client/sounds", &arena);
    if (!sounds_jag) sounds_jag = jagfile_load_file("engine/data/pack/client/sounds", &arena);
    assert(sounds_jag && "sounds archive must load");
    printf("[PASS] sounds.jag archive loaded successfully\n");

    // Test 1: Unpack sounds.dat
    bool unpack_ok = wave_unpack(sounds_jag);
    assert(unpack_ok);
    printf("[PASS] sounds.dat unpacked successfully\n");

    // Test 2: Synthesize sound effects
    int test_tracks[5] = { 2, 10, 50, 74, 100 };
    int generated_count = 0;

    for (int i = 0; i < 5; i++) {
        int tid = test_tracks[i];
        size_t pcm_len = 0;
        size_t wav_len = 0;
        uint8_t *wav = wave_generate(tid, 1, &pcm_len, &wav_len);
        if (wav && pcm_len > 0) {
            generated_count++;
            printf("  Track #%d synthesized: %zu PCM samples (%zu bytes with RIFF header)\n",
                   tid, pcm_len, wav_len);

            // Verify RIFF header
            assert(memcmp(wav, "RIFF", 4) == 0);
            assert(memcmp(wav + 8, "WAVEfmt ", 8) == 0);
            assert(memcmp(wav + 36, "data", 4) == 0);

            // Verify sample rate 22,050 Hz
            uint32_t sample_rate = wav[24] | (wav[25] << 8) | (wav[26] << 16) | (wav[27] << 24);
            assert(sample_rate == 22050);

            // Verify 8 bits per sample
            uint16_t bits = wav[34] | (wav[35] << 8);
            assert(bits == 8);

            // Verify dynamic range (non-flat audio)
            int deviations = 0;
            for (size_t s = 44; s < wav_len; s++) {
                if (wav[s] != 128) deviations++;
            }
            assert(deviations > 10);
            printf("    -> Verified 22,050 Hz 8-bit PCM wave with %d dynamic sample variations\n", deviations);
        }
    }

    assert(generated_count > 0);
    printf("[PASS] Cache sounds produce non-flat 22,050 Hz 8-bit WAV output; Java sample parity is unverified\n");

    // Test 3: Sound queue and tick delay processing
    audio_queue_sound(2, 1, 5); // delay 5 ticks
    assert(audio_queued_count() == 1);
    for (int t = 0; t < 4; t++) {
        audio_tick();
    }
    // Still in queue because 5 ticks haven't elapsed
    assert(audio_queued_count() == 1);
    // 5th and 6th tick should trigger and clear from queue
    audio_tick();
    audio_tick();
    assert(audio_queued_count() == 0);
    printf("[PASS] Audio sound queue and tick delay decrement verified!\n");

    // Test 4: SDL2 audio subsystem initialization
    SDL_Init(SDL_INIT_AUDIO);
    bool audio_ok = audio_init();
    printf("  SDL2 Audio device initialized: %s\n", audio_ok ? "SUCCESS" : "DISABLED (No hardware audio)");
    if (audio_ok) {
        printf("[PASS] SDL2 22,050 Hz Real-Time Audio Engine online!\n");
        audio_play(2, 1);
        audio_close();
    }

    // Test 5: MIDI subsystem and song decompression
    bool midi_ok = midi_init();
    assert(midi_ok);
    printf("[PASS] MIDI subsystem initialized\n");

    bool play_ok = midi_play("adventure", 0, 0);
    if (play_ok) {
        printf("[PASS] MIDI song 'adventure' successfully unpacked, verified MThd header, and scheduled!\n");
        assert(midi_current_song() != NULL && strcmp(midi_current_song(), "adventure") == 0);
        midi_set_volume(180);
        FILE *jingle=fopen("../engine/data/pack/client/jingles/advance cooking.mid","rb");
        assert(jingle);
        assert(!fseek(jingle,0,SEEK_END));long packed_size=ftell(jingle);rewind(jingle);
        assert(packed_size>4 && packed_size<(long)MIDI_MAX_PACKED);
        uint8_t *packed=malloc((size_t)packed_size);assert(packed);
        assert(fread(packed,1,(size_t)packed_size,jingle)==(size_t)packed_size);fclose(jingle);
        uint32_t decoded_size=(uint32_t)packed[0]<<24 | (uint32_t)packed[1]<<16 | (uint32_t)packed[2]<<8 | packed[3];
        bool jingle_ok=midi_play_jingle(packed+4,(int)packed_size-4,(int)decoded_size,40);
        free(packed);
        if(jingle_ok) {
            assert(!midi_is_playing());
            midi_tick();assert(!midi_is_playing());
            midi_tick();assert(midi_is_playing());
            assert(!strcmp(midi_current_song(),"adventure"));
            puts("[PASS] MCI accepts jingle replacement and ambient restart after two original logic ticks (audibility unverified)");
        } else puts("[SKIP] Jingle device playback unavailable/rejected");
        midi_stop();
        assert(midi_current_song()[0] == '\0');
        printf("[PASS] MIDI playback lifecycle, volume, and stop verified!\n");
    } else {
        printf("[SKIP] MIDI device playback unavailable or rejected; cache decoding is tested independently\n");
    }
    midi_close();

    sound_queue_registered();
    music_reference225();
    wave_setting_registered();
    wave_free_all();
    arena_free(&arena);
    SDL_Quit();

    printf("=== ALL AUDIO SYNTHESIZER & MIDI TESTS PASSED! ===\n");
    return 0;
}
