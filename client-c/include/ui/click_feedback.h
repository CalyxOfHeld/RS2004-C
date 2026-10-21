#ifndef RS2_CLICK_FEEDBACK_H
#define RS2_CLICK_FEEDBACK_H
#include "gfx/media.h"
typedef struct { int mode, cycle, x, y; } ClickFeedback;
enum { CLICK_NONE, CLICK_WALK, CLICK_ACTION };
void click_feedback_start(ClickFeedback *cross, int mode, int x, int y);
void click_feedback_tick(ClickFeedback *cross);
int click_feedback_frame(const ClickFeedback *cross);
void click_feedback_draw(const ClickFeedback *cross, const Media *media,
    int viewport_x, int viewport_y, int viewport_w, int viewport_h);
#endif
