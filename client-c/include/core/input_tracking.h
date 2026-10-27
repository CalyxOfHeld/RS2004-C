#ifndef RS2_INPUT_TRACKING_H
#define RS2_INPUT_TRACKING_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
enum { TRACK_PRESS=1,TRACK_RELEASE=3,TRACK_MOVE=5,TRACK_KEY_PRESS=8,TRACK_KEY_RELEASE=9,
       TRACK_FOCUS_GAIN=10,TRACK_FOCUS_LOSS=11,TRACK_ENTER=12,TRACK_EXIT=13 };
typedef struct {
    bool enabled,out_present,old_present;
    size_t out_length,old_length;
    uint8_t out[500],old[500];
    int64_t last_time,last_move_time;
    int32_t tracked_count,last_x,last_y;
} InputTracking;
void input_tracking_init(InputTracking *tracking);
void input_tracking_enable(InputTracking *tracking,int64_t now);
void input_tracking_disable(InputTracking *tracking);
int input_tracking_shell_key(int key_code,int key_char,bool pressed);
/* GameShell-normalized key/button arguments. Invalid metadata leaves state alone. */
bool input_tracking_event(InputTracking *tracking,int kind,int x,int y,int key_or_meta,int64_t now);
/* >=0 returned prefix length;-1 no returned buffer;-2 invalid/capacity.
 * Flush takes old only. Stop returns current only and drops both buffers. */
int input_tracking_take(InputTracking *tracking,bool stop,uint8_t *bytes,size_t capacity);
#endif
