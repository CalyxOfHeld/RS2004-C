#ifndef RS2_MIDI_H
#define RS2_MIDI_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

bool midi_init(void);
bool midi_play(const char *name, int crc, int size);
bool midi_play_jingle(const uint8_t *bzip_data, int compressed_len, int uncompressed_len);
void midi_stop(void);
void midi_set_volume(int vol_0_to_255);
bool midi_is_playing(void);
const char* midi_current_song(void);
void midi_close(void);

#endif // RS2_MIDI_H
