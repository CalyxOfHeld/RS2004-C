#include "dash3d/camera.h"
#include "gfx/pix3d.h"
#include <string.h>
#include <math.h>
#include "core/jrandom.h"

void camera_init(Camera *cam, int initial_x, int initial_z) {
    if (!cam) return;
    memset(cam, 0, sizeof(Camera));
    cam->orbit_x = initial_x;
    cam->orbit_z = initial_z;
    cam->orbit_yaw = 0;
    cam->orbit_pitch = 128; // Authentic RS2 default pitch
    cam->yaw_velocity = 0;
    cam->pitch_velocity = 0;
    // client.cameraPitchClamp has no initializer (Java default 0) and only converges with
    // integer /24 and /80 steps, so it never reaches 32768 exactly from below.
    cam->pitch_clamp = 0;
}

int camera_get_height(int level, int x, int z, int heightmap[4][105][105], uint8_t tile_flags[4][104][104]) {
    if(level<0 || level>3 || !heightmap)return 0;
    int tx = x >> 7;
    int tz = z >> 7;
    if (tx < 0 || tx >= 104 || tz < 0 || tz >= 104) {
        return 0;
    }
    int eff_level = level;
    if (eff_level < 3 && tile_flags && (tile_flags[1][tx][tz] & 0x2) == 2) {
        eff_level = level + 1;
    }
    int rx = x & 0x7F;
    int rz = z & 0x7F;

    int h00 = heightmap[eff_level][tx][tz];
    int h10 = heightmap[eff_level][tx + 1][tz];
    int h01 = heightmap[eff_level][tx][tz + 1];
    int h11 = heightmap[eff_level][tx + 1][tz + 1];

    int h0 = (h00 * (128 - rx) + h10 * rx) >> 7;
    int h1 = (h01 * (128 - rx) + h11 * rx) >> 7;
    return (h0 * (128 - rz) + h1 * rz) >> 7;
}

void camera_update(Camera *cam, int target_x, int target_z, int current_level,
                   int heightmap[4][105][105], uint8_t tile_flags[4][104][104],
                   bool key_left, bool key_right, bool key_up, bool key_down) {
    if (!cam) return;
    // client.updateOrbitCamera tracks the player plus the anticheat offset; the
    // drawScene orbit height still samples the player position.
    int player_x = target_x, player_z = target_z;
    target_x += cam->anticheat_x;
    target_z += cam->anticheat_z;

    // 1. Smoothly track camera target (client.java:6658-6670)
    if (cam->orbit_x - target_x < -500 || cam->orbit_x - target_x > 500 ||
        cam->orbit_z - target_z < -500 || cam->orbit_z - target_z > 500) {
        cam->orbit_x = target_x;
        cam->orbit_z = target_z;
    }
    if (cam->orbit_x != target_x) {
        cam->orbit_x += (target_x - cam->orbit_x) / 16;
    }
    if (cam->orbit_z != target_z) {
        cam->orbit_z += (target_z - cam->orbit_z) / 16;
    }

    // 2. Keyboard Velocity Update (client.java:6671-6684)
    if (key_left) {
        cam->yaw_velocity += (-24 - cam->yaw_velocity) / 2;
    } else if (key_right) {
        cam->yaw_velocity += (24 - cam->yaw_velocity) / 2;
    } else {
        cam->yaw_velocity /= 2;
    }

    if (key_up) {
        cam->pitch_velocity += (12 - cam->pitch_velocity) / 2;
    } else if (key_down) {
        cam->pitch_velocity += (-12 - cam->pitch_velocity) / 2;
    } else {
        cam->pitch_velocity /= 2;
    }

    cam->orbit_yaw = (cam->orbit_yaw + cam->yaw_velocity / 2) & 0x7FF;
    cam->orbit_pitch += cam->pitch_velocity / 2;

    if (cam->orbit_pitch < 128) {
        cam->orbit_pitch = 128;
    }
    if (cam->orbit_pitch > 383) {
        cam->orbit_pitch = 383;
    }

    // 3. Terrain collision & pitch clamping (client.java:6694-6725)
    int var4 = cam->orbit_x >> 7;
    int var5 = cam->orbit_z >> 7;
    int var6 = camera_get_height(current_level, cam->orbit_x, cam->orbit_z, heightmap, tile_flags);
    int var7 = 0;
    int var8 = 0;

    if (var4 > 3 && var5 > 3 && var4 < 100 && var5 < 100) {
        for (var8 = var4 - 4; var8 <= var4 + 4; var8++) {
            for (int var9 = var5 - 4; var9 <= var5 + 4; var9++) {
                int var10 = current_level;
                if (var10 < 3 && tile_flags && (tile_flags[1][var8][var9] & 0x2) == 2) {
                    var10++;
                }
                int var11 = var6 - heightmap[var10][var8][var9];
                if (var11 > var7) {
                    var7 = var11;
                }
            }
        }
    }
    var8 = var7 * 192;
    if (var8 > 98048) var8 = 98048;
    if (var8 < 32768) var8 = 32768;

    if (var8 > cam->pitch_clamp) {
        cam->pitch_clamp += (var8 - cam->pitch_clamp) / 24;
    } else if (var8 < cam->pitch_clamp) {
        cam->pitch_clamp += (var8 - cam->pitch_clamp) / 80;
    }

    camera_prepare_draw(cam, player_x, player_z, current_level, heightmap, tile_flags);
}

void camera_prepare_draw(Camera *cam, int player_x, int player_z, int current_level,
                         int heightmap[4][105][105], uint8_t tile_flags[4][104][104]) {
    if(!cam || cam->cutscene)return;
    // Original drawScene calculates the view after updateGame's camera drift.
    int pitch = cam->orbit_pitch;
    if (cam->pitch_clamp / 256 > pitch) {
        pitch = cam->pitch_clamp / 256;
    }
    if(cam->modifier_enabled[4] && cam->modifier_scale[4]+128>pitch)pitch=cam->modifier_scale[4]+128;
    int yaw = (cam->orbit_yaw + cam->anticheat_yaw) & 0x7FF;   // client.java:2184
    int dist = pitch * 3 + 600;
    int target_y = camera_get_height(current_level, player_x, player_z, heightmap, tile_flags) - 50;

    // 5. Orbit Math (client.java:3326-3357)
    int var8_ang = (2048 - pitch) & 0x7FF;
    int var9_ang = (2048 - yaw) & 0x7FF;
    int var10 = 0;
    int var11 = 0;
    int var12 = dist;

    if (var8_ang != 0) {
        int sinP = pix3d_sin_table[var8_ang];
        int cosP = pix3d_cos_table[var8_ang];
        int var15 = (var11 * cosP - dist * sinP) >> 16;
        var12 = (var11 * sinP + dist * cosP) >> 16;
        var11 = var15;
    }

    if (var9_ang != 0) {
        int sinY = pix3d_sin_table[var9_ang];
        int cosY = pix3d_cos_table[var9_ang];
        int var15 = (var12 * sinY + var10 * cosY) >> 16;
        var12 = (var12 * cosY - var10 * sinY) >> 16;
        var10 = var15;
    }

    cam->eye_x = cam->orbit_x - var10;
    cam->eye_y = target_y - var11;
    cam->eye_z = cam->orbit_z - var12;
    cam->render_pitch = pitch;
    cam->render_yaw = yaw;
}

static void camera_angles225(Camera *c,int level,int hm[4][105][105],uint8_t flags[4][104][104],int *pitch,int *yaw) {
    int dx=c->dst_x*128+64-c->eye_x,dz=c->dst_z*128+64-c->eye_z;
    int dy=camera_get_height(level,c->dst_x,c->dst_z,hm,flags)-c->dst_height-c->eye_y;
    /* Java int multiplication/addition wraps; sqrt(negative) casts NaN to zero. */
    int32_t square=(int32_t)((uint32_t)dx*(uint32_t)dx+(uint32_t)dz*(uint32_t)dz);
    int distance=square<0?0:(int)sqrt((double)square);
    *pitch=(int)(atan2((double)dy,(double)distance)*325.949)&2047;
    *yaw=(int)(atan2((double)dx,(double)dz)*-325.949)&2047;
    if(*pitch<128)*pitch=128;
    if(*pitch>383)*pitch=383;
}
bool camera_packet225(Camera *cam,int opcode,const uint8_t *payload,size_t length,int level,int hm[4][105][105],uint8_t flags[4][104][104]) {
    if(!cam)return false;
    if(opcode==239){if(length)return false;cam->cutscene=false;memset(cam->modifier_enabled,0,sizeof(cam->modifier_enabled));return true;}
    if(!payload)return false;
    Camera c=*cam;
    if(opcode==13){if(length!=4 || payload[0]>=5)return false;int n=payload[0];c.modifier_enabled[n]=true;c.modifier_jitter[n]=payload[1];c.modifier_scale[n]=payload[2];c.modifier_speed[n]=payload[3];c.modifier_cycle[n]=0;}
    else if(opcode==3 || opcode==74){
        if(length!=6 || level<0 || level>3 || !hm)return false;
        c.cutscene=true;int height=payload[2]*256+payload[3];
        if(opcode==3){c.src_x=payload[0];c.src_z=payload[1];c.src_height=height;c.move_speed=payload[4];c.move_acceleration=payload[5];
            if(c.move_acceleration>=100){c.eye_x=c.src_x*128+64;c.eye_z=c.src_z*128+64;c.eye_y=camera_get_height(level,c.src_x,c.src_z,hm,flags)-height;}}
        else {c.dst_x=payload[0];c.dst_z=payload[1];c.dst_height=height;c.rotate_speed=payload[4];c.rotate_acceleration=payload[5];if(c.rotate_acceleration>=100)camera_angles225(&c,level,hm,flags,&c.render_pitch,&c.render_yaw);}
    }else return false;
    *cam=c;return true;
}
static int camera_approach225(int value,int target,int speed,int acceleration) {
    if(value<target){value+=speed+(target-value)*acceleration/1000;if(value>target)value=target;}
    if(value>target){value-=speed+(value-target)*acceleration/1000;if(value<target)value=target;}
    return value;
}
void camera_cutscene_tick225(Camera *c,int level,int hm[4][105][105],uint8_t flags[4][104][104]) {
    if(!c || !c->cutscene || !hm || level<0 || level>3)return;
    c->eye_x=camera_approach225(c->eye_x,c->src_x*128+64,c->move_speed,c->move_acceleration);
    c->eye_y=camera_approach225(c->eye_y,camera_get_height(level,c->src_x,c->src_z,hm,flags)-c->src_height,c->move_speed,c->move_acceleration);
    c->eye_z=camera_approach225(c->eye_z,c->src_z*128+64,c->move_speed,c->move_acceleration);
    int pitch,yaw;camera_angles225(c,level,hm,flags,&pitch,&yaw);
    c->render_pitch=camera_approach225(c->render_pitch,pitch,c->rotate_speed,c->rotate_acceleration);
    int delta=yaw-c->render_yaw;if(delta>1024)delta-=2048;if(delta< -1024)delta+=2048;
    if(delta>0)c->render_yaw=(c->render_yaw+c->rotate_speed+delta*c->rotate_acceleration/1000)&2047;
    if(delta<0)c->render_yaw=(c->render_yaw-c->rotate_speed-(-delta)*c->rotate_acceleration/1000)&2047;
    int after=yaw-c->render_yaw;if(after>1024)after-=2048;if(after< -1024)after+=2048;
    if((after<0 && delta>0) || (after>0 && delta<0))c->render_yaw=yaw;
}
void camera_modifier_tick225(Camera *c){if(c)for(int i=0;i<5;i++)c->modifier_cycle[i]=(int32_t)((uint32_t)c->modifier_cycle[i]+1u);}
void camera_shake225(const Camera *c,Camera *view) {
    if(!c || !view)return;
    *view=*c;
    for(int i=0;i<5;i++)if(c->modifier_enabled[i]){
        int d=(int)(math_random()*(c->modifier_jitter[i]*2+1)-c->modifier_jitter[i]+sin((double)c->modifier_cycle[i]*((double)c->modifier_speed[i]/100.0))*c->modifier_scale[i]);
        if(i==0)view->eye_x+=d;
        if(i==1)view->eye_y+=d;
        if(i==2)view->eye_z+=d;
        if(i==3)view->render_yaw=(view->render_yaw+d)&2047;
        if(i==4){view->render_pitch+=d;if(view->render_pitch<128)view->render_pitch=128;if(view->render_pitch>383)view->render_pitch=383;}
    }
}
int camera_cutscene_top225(const Camera *c,int level,int hm[4][105][105],uint8_t flags[4][104][104]) {
    if(!c || !hm || !flags || level<0 || level>3)return 3;
    int x=c->eye_x>>7,z=c->eye_z>>7;if(x<0 || x>=104 || z<0 || z>=104)return 3;
    return camera_get_height(level,c->eye_x,c->eye_z,hm,flags)-c->eye_y>=800 || !(flags[level][x][z]&4)?3:level;
}
