#ifndef RS2_MIDI_H
#define RS2_MIDI_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "core/music225.h"

bool midi_init(void);
#define MIDI_MAX_DECODED (2u * 1024u * 1024u)
#define MIDI_MAX_PACKED (2u * 1024u * 1024u)
// Decode a complete cache .mid (g4 length + headerless BZip2). expected_size=0
// is explicitly local/no metadata; network songs supply their packed size/CRC.
// The caller owns *decoded on success. No device or file writes are involved.
bool midi_unpack(const uint8_t *packed, size_t packed_size, uint32_t expected_crc,
    size_t expected_size, uint8_t **decoded, size_t *decoded_size);
/* Legacy fixture conversion utility. Playback uses unmodified MIDI events
 * and exact output attenuation; it does not call this CC7 transformer. */
bool midi_gain225(const uint8_t *src,size_t length,int volume,uint8_t **out,size_t *out_length);
bool midi_play(const char *name, uint32_t crc, uint32_t size);
bool midi_play_jingle(const uint8_t *bzip_data, int compressed_len, int uncompressed_len, uint16_t delay);
// Original 50 Hz logic: delay is milliseconds, not ticks or playback duration.
bool midi_delay_step(uint16_t *delay);
void midi_tick(void);
void midi_stop(void);
void midi_set_volume(int vol_0_to_255); // compatibility API; client uses exact attenuation below
void midi_set_attenuation225(int hundredths_db);
/* Drain logical commands into the native worker. Live output attenuation updates
 * preserve the sequencer clock and notes; Off/On remain separate commands. */
void midi_apply_music225(Music225 *music);
bool midi_request_jingle225(const uint8_t *packed,int length,int decoded_length);
bool midi_is_playing(void);
const char* midi_current_song(void);
void midi_close(void);
// Headless replay: play requests report unavailable and no device APIs are used.
void midi_set_silent(bool silent);

#endif // RS2_MIDI_H
