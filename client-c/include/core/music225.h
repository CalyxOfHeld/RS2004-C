#ifndef RS2_MUSIC225_H
#define RS2_MUSIC225_H
#include <stdbool.h>
#include <stdint.h>

/* Logical client fields and setMidi's latest-request mailbox, not device state. */
typedef struct Music225 {
    bool active, low_memory, has_current, pending;
    int volume, delay, command, fade; /* command:0 unchanged,1 voladjust,2 stop */
    char current[64], requested[64];
    uint32_t crc, size, requested_crc, requested_size;
} Music225;
void music_init225(Music225 *music);
void music_setting225(Music225 *music,int value);
bool music_song225(Music225 *music,const char *name,uint32_t crc,uint32_t size);
bool music_jingle225(Music225 *music,int delay);
void music_tick225(Music225 *music);
void music_logout225(Music225 *music);
#endif
