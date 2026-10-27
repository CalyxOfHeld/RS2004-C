/* SMF transport, independent of client music policy. Events reference owned
 * SMF/RMID bytes; format 0/1 merge order is (tick, track, event). */
typedef struct {
    uint64_t tick, us;
    uint32_t order, message, length;
    const uint8_t *data;
    uint8_t kind; /* 0=short, 1=tempo, 2=F0, 3=F7, 4=meta/end */
} MidiEvent225;
typedef struct { MidiEvent225 *events; size_t count, capacity; uint64_t duration; } MidiSequence225;
static uint32_t ms_be225(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static uint32_t ms_le225(const uint8_t *p){return (uint32_t)p[3]<<24|(uint32_t)p[2]<<16|(uint32_t)p[1]<<8|p[0];}
static bool ms_vlq225(const uint8_t *p,size_t end,size_t *at,uint32_t *n){
    *n=0;for(int i=0;i<4;i++){if(*at>=end)return false;uint8_t b=p[(*at)++];*n=(*n<<7)|(b&127);if(!(b&128))return true;}return false;
}
static bool ms_add225(MidiSequence225 *s,MidiEvent225 e){
    if(s->count==s->capacity){size_t cap=s->capacity?s->capacity*2:256;
        if(cap>2u*1024u*1024u)return false;
        MidiEvent225 *p=realloc(s->events,cap*sizeof(*p));if(!p)return false;s->events=p;s->capacity=cap;}
    e.order=(uint32_t)s->count;s->events[s->count++]=e;return true;
}
static int ms_compare225(const void *a,const void *b){
    const MidiEvent225 *x=a,*y=b;if(x->tick!=y->tick)return x->tick<y->tick?-1:1;return x->order<y->order?-1:x->order>y->order;
}
static void ms_free225(MidiSequence225 *s){free(s->events);memset(s,0,sizeof(*s));}
static bool ms_parse225(const uint8_t *p,size_t length,MidiSequence225 *s){
    memset(s,0,sizeof(*s));size_t start=0,end=length;
    if(length>=12 && !memcmp(p,"RIFF",4) && !memcmp(p+8,"RMID",4)){
        uint32_t size=ms_le225(p+4);if(size>length-8 || size<4)return false;end=size+8;bool found=false;
        for(size_t at=12;at+8<=end;){uint32_t n=ms_le225(p+at+4);if(n>end-at-8)return false;
            if(!memcmp(p+at,"data",4)){start=at+8;end=start+n;found=true;break;}at+=8+(size_t)n+(n&1u);}
        if(!found)return false;
    }
    if(end-start<14 || memcmp(p+start,"MThd",4))return false;
    uint32_t header=ms_be225(p+start+4);if(header<6 || header>end-start-8)return false;
    unsigned format=(unsigned)p[start+8]*256+p[start+9],tracks=(unsigned)p[start+10]*256+p[start+11];
    unsigned division=(unsigned)p[start+12]*256+p[start+13];
    if(format>1 || !tracks || (!format && tracks!=1) || !division)return false;
    size_t chunk=start+8+header;unsigned parsed=0;
    while(parsed<tracks && chunk+8<=end){
        uint32_t n=ms_be225(p+chunk+4);size_t at=chunk+8;if(n>end-at)goto fail;size_t stop=at+n;
        if(memcmp(p+chunk,"MTrk",4)){chunk=stop;continue;}parsed++;
        uint64_t tick=0;unsigned running=0;bool ended=false;
        while(at<stop){uint32_t delta;if(!ms_vlq225(p,stop,&at,&delta) || at>=stop)goto fail;tick+=delta;
            unsigned status=p[at];if(status&128){at++;if(status<0xf0)running=status;else if(status!=0xff)running=0;}else{status=running;if(!status)goto fail;}
            MidiEvent225 e={0};e.tick=tick;
            if(status<0xf0){unsigned bytes=(status&0xe0)==0xc0?1:2;if(bytes>stop-at)goto fail;
                for(unsigned j=0;j<bytes;j++)if(p[at+j]&128)goto fail;
                e.message=status|(uint32_t)p[at]<<8;if(bytes==2)e.message|=(uint32_t)p[at+1]<<16;at+=bytes;
            }else if(status==0xff){if(at>=stop)goto fail;unsigned type=p[at++];uint32_t size;
                if(!ms_vlq225(p,stop,&at,&size) || size>stop-at)goto fail;
                e.kind=4;e.message=type;e.data=p+at;e.length=size;
                if(type==0x51){if(size!=3)goto fail;e.kind=1;e.message=(uint32_t)p[at]<<16|(uint32_t)p[at+1]<<8|p[at+2];if(!e.message)goto fail;}
                at+=size;if(type==0x2f){if(size)goto fail;ended=true;}
            }else if(status==0xf0 || status==0xf7){uint32_t size;if(!ms_vlq225(p,stop,&at,&size) || size>stop-at)goto fail;
                e.kind=status==0xf0?2:3;e.data=p+at;e.length=size;at+=size;
            }else goto fail;
            if(!ms_add225(s,e))goto fail;
            if(ended)break;
        }
        if(!ended)goto fail;
        chunk=stop;
    }
    if(parsed!=tracks)goto fail;
    qsort(s->events,s->count,sizeof(*s->events),ms_compare225);
    uint64_t tick=0,us=0,remainder=0;uint32_t tempo=500000,den=division,num=0;
    if(division&0x8000){int fps=256-(division>>8);unsigned resolution=division&255;if(!resolution)goto fail;
        if(fps==29){num=1000000u*1001u;den=30000u*resolution;}
        else if(fps==24 || fps==25 || fps==30){num=1000000;den=(unsigned)fps*resolution;}else goto fail;}
    for(size_t i=0;i<s->count;i++){MidiEvent225 *e=&s->events[i];uint64_t delta=e->tick-tick;
        uint32_t rate=num?num:tempo;if(delta>(UINT64_MAX-remainder)/rate)goto fail;
        uint64_t scaled=delta*rate+remainder;if(us>UINT64_MAX-scaled/den)goto fail;us+=scaled/den;remainder=scaled%den;e->us=us;tick=e->tick;
        if(e->kind==1 && !num)tempo=e->message;
    }
    s->duration=us;return true;
fail:ms_free225(s);return false;
}
