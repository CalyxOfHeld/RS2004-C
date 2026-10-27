#include "audio/midi.h"
#include <stdlib.h>
#include <string.h>

static uint32_t be225(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static uint32_t le225(const uint8_t *p){return (uint32_t)p[3]<<24|(uint32_t)p[2]<<16|(uint32_t)p[1]<<8|p[0];}
static void put225(uint8_t *p,uint32_t n,bool little){for(int i=0;i<4;i++)p[i]=(uint8_t)(n>>(little?8*i:24-8*i));}
static bool vlq225(const uint8_t *p,size_t end,size_t *at,uint32_t *n){*n=0;for(int i=0;i<4;i++){if(*at>=end)return false;unsigned b=p[(*at)++];*n=(*n<<7)|(b&127);if(!(b&128))return true;}return false;}
static bool track225(const uint8_t *src,size_t length,uint8_t *out,int volume){
 size_t at=0;unsigned running=0;
 while(at<length){uint32_t n;if(!vlq225(src,length,&at,&n) || at>=length)return false;unsigned status=src[at];
  if(status&128){at++;if(status<0xf0)running=status;}else {status=running;if(!status)return false;}
  if(status<0xf0){size_t data=at;unsigned bytes=(status&0xe0)==0xc0?1:2;if(bytes>length-at)return false;for(unsigned i=0;i<bytes;i++)if(src[at+i]&128)return false;
   if((status&0xf0)==0xb0 && src[data]==7)out[data+1]=(uint8_t)((src[data+1]*volume+127)/255);
   at+=bytes;
  }else if(status==0xff){if(at>=length)return false;at++;if(!vlq225(src,length,&at,&n) || n>length-at)return false;at+=n;}
  else if(status==0xf0 || status==0xf7){if(!vlq225(src,length,&at,&n) || n>length-at)return false;at+=n;}
  else return false;
 }
 return true;
}
bool midi_gain225(const uint8_t *src,size_t length,int volume,uint8_t **result,size_t *result_length){
 if(!result || !result_length)return false;
 *result=NULL;*result_length=0;
 if(!src || length<14 || length>MIDI_MAX_DECODED || volume<0 || volume>255)return false;
 if(volume==255){uint8_t *copy=malloc(length);if(!copy)return false;memcpy(copy,src,length);*result=copy;*result_length=length;return true;}
 size_t start=0,end=length,riff_data=0;
 if(length>=12 && !memcmp(src,"RIFF",4) && !memcmp(src+8,"RMID",4)){
  size_t at=12;bool found=false;if(le225(src+4)>length-8)return false;
  while(at+8<=length){uint32_t n=le225(src+at+4);if(n>length-at-8)return false;if(!memcmp(src+at,"data",4)){start=at+8;end=start+n;riff_data=at+4;found=true;break;}at+=8+n+(n&1u);}
  if(!found)return false;
 }
 if(end-start<14 || memcmp(src+start,"MThd",4))return false;
 uint32_t header=be225(src+start+4);if(header<6 || header>end-start-8)return false;
 int tracks=(int)src[start+10]*256+src[start+11];if(!tracks)return false;
 uint8_t *out=malloc(length+64);if(!out)return false;size_t at=start+8+header,insert=0;bool injected=false;
 while(at+8<=end && tracks){uint32_t n=be225(src+at+4);if(n>end-at-8)goto fail;
  if(!memcmp(src+at,"MTrk",4)){
   if(!injected){insert=at+8;memcpy(out,src,insert);for(int channel=0;channel<16;channel++){size_t p=insert+(size_t)channel*4;out[p]=0;out[p+1]=(uint8_t)(0xb0+channel);out[p+2]=7;out[p+3]=(uint8_t)((100*volume+127)/255);}memcpy(out+insert+64,src+insert,length-insert);put225(out+at+4,n+64,false);injected=true;}
   if(!track225(src+at+8,n,out+at+8+(injected?64:0),volume))goto fail;
   tracks--;
  }
  at+=8+n;
 }
 if(tracks || !injected)goto fail;
 if(riff_data){put225(out+riff_data,le225(src+riff_data)+64,true);put225(out+4,le225(src+4)+64,true);}
 *result=out;*result_length=length+64;return true;
fail:free(out);return false;
}
