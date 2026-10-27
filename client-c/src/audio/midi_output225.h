#ifndef RS2_MIDI_OUTPUT225_H
#define RS2_MIDI_OUTPUT225_H
/* Windows host MIDI adapter. Retain the OS General MIDI collection used by
 * Windows MIDI playback, but apply Java's millibel values to an owned output
 * buffer. Neither MIDI controller data nor the system master volume changes. */
#ifndef MIDI_OUTPUT225_TEST
#ifndef COBJMACROS
#define COBJMACROS
#endif
#include <dsound.h>
#include <dmusici.h>
typedef struct {
    IDirectMusic *music;
    IDirectMusicPort *port;
    IDirectMusicLoader *loader;
    IDirectMusicCollection *collection;
    IDirectMusicBuffer *events;
    IDirectSound *sound;
    IDirectSoundBuffer *buffer;
    IReferenceClock *clock;
    IDirectMusicDownloadedInstrument **instruments;
    size_t instrument_count;
    bool com;
} MidiOutput225;
static bool mo_ok225(HRESULT result,const char *operation){
    if(SUCCEEDED(result))return true;
    fprintf(stderr,"[MIDI] %s failed (HRESULT %08lx)\n",operation,(unsigned long)result);return false;
}
static void mo_close225(MidiOutput225 *o){
    if(!o)return;
    if(o->port){IDirectMusicPort_Activate(o->port,FALSE);
        for(size_t i=0;i<o->instrument_count;i++){IDirectMusicPort_UnloadInstrument(o->port,o->instruments[i]);IDirectMusicDownloadedInstrument_Release(o->instruments[i]);}}
    free(o->instruments);
    if(o->clock)IReferenceClock_Release(o->clock);
    if(o->events)IDirectMusicBuffer_Release(o->events);
    if(o->collection)IDirectMusicCollection_Release(o->collection);
    if(o->loader)IDirectMusicLoader_Release(o->loader);
    if(o->port)IDirectMusicPort_Release(o->port);
    if(o->buffer)IDirectSoundBuffer_Release(o->buffer);
    if(o->music)IDirectMusic_Release(o->music);
    if(o->sound)IDirectSound_Release(o->sound);
    if(o->com)CoUninitialize();
    free(o);
}
static bool mo_gain225(MidiOutput225 *o,int attenuation){return mo_ok225(IDirectSoundBuffer_SetVolume(o->buffer,attenuation),"output attenuation");}
static bool mo_open225(MidiOutput225 **out,int attenuation){
    MidiOutput225 *o=calloc(1,sizeof(*o));if(!o)return false;
    WAVEFORMATEX *format=NULL;HRESULT hr=CoInitializeEx(NULL,COINIT_MULTITHREADED);
    if(!mo_ok225(hr,"COM initialization"))goto fail;
    o->com=true;
    if(!mo_ok225(DirectSoundCreate(NULL,&o->sound,NULL),"DirectSound") ||
       !mo_ok225(IDirectSound_SetCooperativeLevel(o->sound,GetDesktopWindow(),DSSCL_NORMAL),"cooperative level") ||
       !mo_ok225(CoCreateInstance(&CLSID_DirectMusic,NULL,CLSCTX_INPROC_SERVER,&IID_IDirectMusic,(void**)&o->music),"DirectMusic") ||
       !mo_ok225(IDirectMusic_SetDirectSound(o->music,o->sound,GetDesktopWindow()),"sound binding"))goto fail;
    DMUS_PORTPARAMS params={0};params.dwSize=sizeof(params);
    params.dwValidParams=DMUS_PORTPARAMS_CHANNELGROUPS|DMUS_PORTPARAMS_AUDIOCHANNELS|DMUS_PORTPARAMS_SHARE;
    params.dwChannelGroups=1;params.dwAudioChannels=2;params.fShare=FALSE;
    if(!mo_ok225(IDirectMusic_CreatePort(o->music,&CLSID_DirectMusicSynth,&params,&o->port,NULL),"private Windows synth"))goto fail;
    DWORD size=0,bytes=0;
    if(!mo_ok225(IDirectMusicPort_GetFormat(o->port,NULL,&size,&bytes),"synth format size") || size<sizeof(WAVEFORMATEX) || size>65536 || !bytes)goto fail;
    format=calloc(1,size);if(!format)goto fail;
    if(!mo_ok225(IDirectMusicPort_GetFormat(o->port,format,&size,&bytes),"synth format"))goto fail;
    DSBUFFERDESC desc={0};desc.dwSize=sizeof(desc);desc.dwFlags=DSBCAPS_CTRLVOLUME|DSBCAPS_GLOBALFOCUS;desc.dwBufferBytes=bytes;desc.lpwfxFormat=format;
    if(!mo_ok225(IDirectSound_CreateSoundBuffer(o->sound,&desc,&o->buffer,NULL),"owned output buffer") ||
       !mo_ok225(IDirectMusicPort_SetDirectSound(o->port,o->sound,o->buffer),"port buffer binding") || !mo_gain225(o,attenuation))goto fail;
    free(format);format=NULL;
    if(!mo_ok225(CoCreateInstance(&CLSID_DirectMusicLoader,NULL,CLSCTX_INPROC_SERVER,&IID_IDirectMusicLoader,(void**)&o->loader),"GM loader"))goto fail;
    DMUS_OBJECTDESC object={0};object.dwSize=sizeof(object);object.dwValidData=DMUS_OBJ_CLASS|DMUS_OBJ_OBJECT;
    object.guidClass=CLSID_DirectMusicCollection;object.guidObject=GUID_DefaultGMCollection;
    if(!mo_ok225(IDirectMusicLoader_GetObject(o->loader,&object,&IID_IDirectMusicCollection,(void**)&o->collection),"Windows GM collection"))goto fail;
    for(DWORD i=0;;i++){
        DWORD patch;HRESULT result=IDirectMusicCollection_EnumInstrument(o->collection,i,&patch,NULL,0);
        if(result==S_FALSE)break;
        if(!mo_ok225(result,"GM instrument enumeration") || i>=4096)goto fail;
        IDirectMusicInstrument *instrument=NULL;IDirectMusicDownloadedInstrument *downloaded=NULL;
        if(!mo_ok225(IDirectMusicCollection_GetInstrument(o->collection,patch,&instrument),"GM instrument"))goto fail;
        result=IDirectMusicPort_DownloadInstrument(o->port,instrument,&downloaded,NULL,0);IDirectMusicInstrument_Release(instrument);
        if(!mo_ok225(result,"GM download"))goto fail;
        void *next=realloc(o->instruments,(o->instrument_count+1)*sizeof(*o->instruments));
        if(!next){IDirectMusicPort_UnloadInstrument(o->port,downloaded);IDirectMusicDownloadedInstrument_Release(downloaded);goto fail;}
        o->instruments=next;o->instruments[o->instrument_count++]=downloaded;
    }
    DMUS_BUFFERDESC events={0};events.dwSize=sizeof(events);events.cbBuffer=MIDI_MAX_DECODED+32;
    if(!mo_ok225(IDirectMusic_CreateMusicBuffer(o->music,&events,&o->events,NULL),"MIDI event buffer") ||
       !mo_ok225(IDirectMusicPort_GetLatencyClock(o->port,&o->clock),"synth clock"))goto fail;
    *out=o;return true;
fail:free(format);mo_close225(o);return false;
}
static bool mo_send225(MidiOutput225 *o,DWORD message,const uint8_t *bytes,size_t length){
    REFERENCE_TIME now;
    if(!mo_ok225(IReferenceClock_GetTime(o->clock,&now),"synth time") ||
       !mo_ok225(IDirectMusicBuffer_Flush(o->events),"event flush") ||
       !mo_ok225(IDirectMusicBuffer_SetStartTime(o->events,now),"event time"))return false;
    HRESULT result=bytes?IDirectMusicBuffer_PackUnstructured(o->events,0,1,(DWORD)length,(BYTE*)bytes):IDirectMusicBuffer_PackStructured(o->events,0,1,message);
    return mo_ok225(result,"MIDI event") && mo_ok225(IDirectMusicPort_PlayBuffer(o->port,o->events),"MIDI output");
}
static bool mo_start225(MidiOutput225 *o){return mo_ok225(IDirectMusicPort_Activate(o->port,TRUE),"synth activation");}
static bool mo_reset225(MidiOutput225 *o){
    /* Discard queued events and sounding voices at Off/song boundaries only. */
    return mo_ok225(IDirectMusicPort_Activate(o->port,FALSE),"synth stop");
}
#endif

#endif
