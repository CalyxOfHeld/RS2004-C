#include "core/input_tracking.h"
#include <string.h>
static bool valid(const InputTracking *t){return t && t->out_length<500 && t->old_length<500 && (!t->enabled || t->out_present);}
static int64_t difference(int64_t a,int64_t b){uint64_t bits=(uint64_t)a-(uint64_t)b;int64_t value;memcpy(&value,&bits,sizeof(value));return value;}
static uint8_t elapsed(InputTracking *t,int64_t now){int64_t delta=difference(now,t->last_time)/10;if(delta>250)delta=250;t->last_time=now;return (uint8_t)delta;}
static void capacity(InputTracking *t,size_t count){if(t->out_length+count>=500){memcpy(t->old,t->out,t->out_length);t->old_length=t->out_length;t->old_present=true;t->out_length=0;}}
static void byte(InputTracking *t,int value){t->out[t->out_length++]=(uint8_t)value;}
static void position(InputTracking *t,int x,int y){int value=x+(y<<10);byte(t,value>>16);byte(t,value>>8);byte(t,value);}
static int key(int code){if(code==1000)code=11;if(code==1001)code=12;if(code==1002)code=14;if(code==1003)code=15;if(code>=1008)code-=992;return code;}
void input_tracking_init(InputTracking *t){if(t)memset(t,0,sizeof(*t));}
void input_tracking_enable(InputTracking *t,int64_t now){if(!t)return;t->out_present=true;t->old_present=false;t->out_length=t->old_length=0;t->last_time=now;t->enabled=true;}
void input_tracking_disable(InputTracking *t){if(t){t->enabled=t->out_present=t->old_present=false;t->out_length=t->old_length=0;}}
int input_tracking_shell_key(int vk,int character,bool pressed){
    int code=(uint16_t)character;if(code<30)code=0;
    if(vk==37)code=1;
    if(vk==39)code=2;
    if(vk==38)code=3;
    if(vk==40)code=4;
    if(vk==17)code=5;
    if(vk==8 || vk==127)code=8;
    if(vk==9)code=9;
    if(vk==10)code=10;
    if(pressed){if(vk>=112 && vk<=123)code=vk+1008-112;if(vk==36)code=1000;if(vk==35)code=1001;if(vk==33)code=1002;if(vk==34)code=1003;}return code;
}
bool input_tracking_event(InputTracking *t,int kind,int x,int y,int code,int64_t now){
    if(!valid(t))return false;
    if(kind!=TRACK_PRESS && kind!=TRACK_RELEASE && kind!=TRACK_MOVE && (kind<TRACK_KEY_PRESS || kind>TRACK_EXIT))return false;
    if(!t->enabled)return true;
    if((kind==TRACK_PRESS || kind==TRACK_MOVE) && (x<0 || x>=789 || y<0 || y>=532))return true;
    if(kind==TRACK_MOVE){if(difference(now,t->last_move_time)<50)return true;t->last_move_time=now;}
    t->tracked_count=(int32_t)((uint32_t)t->tracked_count+1);uint8_t delta=elapsed(t,now);
    if(kind==TRACK_PRESS){capacity(t,5);byte(t,code==1?1:2);byte(t,delta);position(t,x,y);}
    else if(kind==TRACK_RELEASE){capacity(t,2);byte(t,code==1?3:4);byte(t,delta);}
    else if(kind==TRACK_MOVE){int dx=(int32_t)((uint32_t)x-(uint32_t)t->last_x),dy=(int32_t)((uint32_t)y-(uint32_t)t->last_y);
        if(dx<8 && dx>=-8 && dy<8 && dy>=-8){capacity(t,3);byte(t,5);byte(t,delta);byte(t,dx+8+((dy+8)<<4));}
        else if(dx<128 && dx>=-128 && dy<128 && dy>=-128){capacity(t,4);byte(t,6);byte(t,delta);byte(t,dx+128);byte(t,dy+128);}
        else {capacity(t,5);byte(t,7);byte(t,delta);position(t,x,y);}t->last_x=x;t->last_y=y;
    }else if(kind==TRACK_KEY_PRESS || kind==TRACK_KEY_RELEASE){capacity(t,3);byte(t,kind);byte(t,delta);byte(t,key(code));}
    else {capacity(t,2);byte(t,kind);byte(t,delta);}return true;
}
int input_tracking_take(InputTracking *t,bool stop,uint8_t *bytes,size_t size){
    if(!valid(t))return -2;
    bool present=stop?(t->enabled && t->out_present && t->out_length>0):(t->enabled && t->old_present);
    size_t length=stop?t->out_length:t->old_length;
    if(present && (length>size || (length && !bytes)))return -2;
    if(present && length)memcpy(bytes,stop?t->out:t->old,length);
    if(stop)input_tracking_disable(t);else {t->old_present=false;t->old_length=0;}
    return present?(int)length:-1;
}
