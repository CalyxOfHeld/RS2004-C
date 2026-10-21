#ifndef RS2_MIDI_H
#define RS2_MIDI_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

bool midi_init(void);
#define MIDI_MAX_DECODED (2u * 1024u * 1024u)
#define MIDI_MAX_PACKED (2u * 1024u * 1024u)
// Decode a complete cache .mid (g4 length + headerless BZip2). expected_size=0
// is explicitly local/no metadata; network songs supply their packed size/CRC.
// The caller owns *decoded on success. No device or file writes are involved.
bool midi_unpack(const uint8_t *packed, size_t packed_size, uint32_t expected_crc,
    size_t expected_size, uint8_t **decoded, size_t *decoded_size);
bool midi_play(const char *name, uint32_t crc, uint32_t size);
bool midi_play_jingle(const uint8_t *bzip_data, int compressed_len, int uncompressed_len, uint16_t delay);
// Original 50 Hz logic: delay is milliseconds, not ticks or playback duration.
bool midi_delay_step(uint16_t *delay);
void midi_tick(void);
void midi_stop(void);
void midi_set_volume(int vol_0_to_255);
bool midi_is_playing(void);
const char* midi_current_song(void);
void midi_close(void);
// Headless replay: every play request reports unavailable and MCI is never used.
void midi_set_silent(bool silent);

#endif // RS2_MIDI_H
