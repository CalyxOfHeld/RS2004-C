#include "ui/click_feedback.h"
#include "gfx/pix2d.h"

void click_feedback_start(ClickFeedback *cross, int mode, int x, int y) {
    if (!cross || (mode != CLICK_WALK && mode != CLICK_ACTION)) return;
    *cross = (ClickFeedback){mode, 0, x, y};
}
void click_feedback_tick(ClickFeedback *cross) {
    if (!cross || cross->mode == CLICK_NONE) return;
    cross->cycle += 20;
    if (cross->cycle >= 400) cross->mode = CLICK_NONE;
}
int click_feedback_frame(const ClickFeedback *cross) {
    if (!cross || cross->mode == CLICK_NONE || cross->cycle < 0 || cross->cycle >= 400) return -1;
    return cross->cycle / 100 + (cross->mode == CLICK_ACTION ? 4 : 0);
}
void click_feedback_draw(const ClickFeedback *cross, const Media *media,
    int viewport_x, int viewport_y, int viewport_w, int viewport_h) {
    int frame = click_feedback_frame(cross);
    if (!media || frame < 0 || !media->crosses[frame]) return;
    int left=pix2d_clip_left,top=pix2d_clip_top,right=pix2d_clip_right,bottom=pix2d_clip_bottom;
    int x0=left>viewport_x?left:viewport_x, y0=top>viewport_y?top:viewport_y;
    int x1=right<viewport_x+viewport_w?right:viewport_x+viewport_w;
    int y1=bottom<viewport_y+viewport_h?bottom:viewport_y+viewport_h;
    pix2d_set_clip(x0,y0,x1,y1);
    // Original viewport-buffer coordinates are click minus 8 minus viewport origin.
    pix32_draw(media->crosses[frame],cross->x-8,cross->y-8);
    pix2d_set_clip(left,top,right,bottom);
}
