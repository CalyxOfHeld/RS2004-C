#ifndef RS2_COMBAT_FEEDBACK_H
#define RS2_COMBAT_FEEDBACK_H
#include "entity/pathing_entity.h"
#include "gfx/media.h"
#include "gfx/pixfont.h"
void combat_feedback_draw(const PathingEntity *e,int cycle,const Media *media,PixFont *font,int bx,int by,int hx,int hy);
int combat_multiway_y225(int world_x,int world_z);
void combat_multiway_draw225(const Media *media,int flag,int world_x,int world_z,int viewport_x,int viewport_y);
#endif
