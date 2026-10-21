#ifndef RS2_COMBAT_FEEDBACK_H
#define RS2_COMBAT_FEEDBACK_H
#include "entity/pathing_entity.h"
#include "gfx/media.h"
#include "gfx/pixfont.h"
void combat_feedback_draw(const PathingEntity *e,int cycle,const Media *media,PixFont *font,int bx,int by,int hx,int hy);
#endif
