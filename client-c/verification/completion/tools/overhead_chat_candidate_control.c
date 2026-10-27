/* Actual candidate projection and draw integration; no production connection. */
#include "../../../src/client/client.c"
#define main registered_chatbox_unused_main
#include "../../../tests/test_chatbox.c"
#undef main
static bool candidate_projection(void *user,const PathingEntity *actor,int *x,int *y) {
    OverheadFixture *f=user;
    OverheadProjection225 camera={f->camera[0],f->camera[1],f->camera[2],f->camera[3],f->camera[4]};
    bool ok=client_overhead_project225(&camera,actor,x,y);
    int expected_x,expected_y;bool expected=overhead_fixture_project(user,actor,&expected_x,&expected_y);
    assert(ok==expected);
    if(ok){assert(*x==expected_x && *y==expected_y);}
    return ok;
}
int main(void) {
    setvbuf(stdout,NULL,_IONBF,0);setvbuf(stderr,NULL,_IONBF,0);
    ClientOptions options;client_default_options(&options);options.offline=true;options.has_seed=true;options.seed=225;
    ClientPlatform platform={0};assert(client_init(&options,&platform));
    memset(world3d->heightmap,0,sizeof(world3d->heightmap));memset(world->tile_flags,0,sizeof(world->tile_flags));
    sync_ctx.current_plane=0;pix3d_center_x=VIEWPORT_X+256;pix3d_center_y=VIEWPORT_Y+167;
    overhead_controls(&arena_perm,candidate_projection,VIEWPORT_X,VIEWPORT_Y);
    int effects_id=-1;for(int i=0;i<texture_varp_types.count;i++)if(varp_type_clientcode(&texture_varp_types,i)==6)effects_id=i;
    assert(effects_id>=0 && has_fonts);
    player.base.x=6400;player.base.z=6400;player.base.height=0;player.visible=true;
    camera_init(&cam,6400,6400);cam.orbit_pitch=256;
    strcpy(player.name,"Self Test");strcpy(player.base.chat,"AAAAAA");player.base.chat_color=1;player.base.chat_style=3;player.base.chat_timer=150;
    sync_ctx.local_player=player;sync_ctx.local_player.base.height=200;public_chat_setting=0;varp_set(&varp_ctx,effects_id,0);
    int cycle=scene_draw_cycle225,tick=game_ticks;
    assert(client_draw());assert(scene_draw_cycle225==cycle+1 && game_ticks==tick && overhead_chat225.count==1);
    int px=overhead_chat225.x[0]+VIEWPORT_X,py=overhead_chat225.y[0]+VIEWPORT_Y;
    printf("Candidate overhead projection=%d,%d\n",px,py);
    assert(px>=VIEWPORT_X+60 && px<VIEWPORT_X+VIEWPORT_W-60 && py>=VIEWPORT_Y+30 && py<VIEWPORT_Y+300);
    uint32_t baseline[120*30];
    for(int y=0;y<30;y++)memcpy(baseline+y*120,pixels+(py-20+y)*CLIENT_WIDTH+px-60,120*sizeof(*pixels));
    varp_set(&varp_ctx,effects_id,1);assert(client_draw());assert(scene_draw_cycle225==cycle+2 && game_ticks==tick);
    uint32_t glyphs[512*334]={0};pix2d_bind(glyphs,512,334);pixfont_draw_string_center(&font_b12,px-VIEWPORT_X,py-VIEWPORT_Y+1,0,"AAAAAA");pixfont_draw_string_center(&font_b12,px-VIEWPORT_X,py-VIEWPORT_Y,0xffff00,"AAAAAA");
    int yellow=0,different=0;
    for(int y=0;y<30;y++)for(int x=0;x<120;x++) {
        int gx=px-60+x,gy=py-20+y;
        if(glyphs[(gy-VIEWPORT_Y)*512+gx-VIEWPORT_X]==0xffff00){assert(pixels[gy*CLIENT_WIDTH+gx]==0xffff00);yellow++;if(baseline[y*120+x]!=0xffff00)different++;}
    }
    assert(yellow>100 && different>100);
    pix2d_bind(pixels,CLIENT_WIDTH,CLIENT_HEIGHT);
    live_server=true;streamer.scene_state=1;cycle=scene_draw_cycle225;assert(client_draw());assert(scene_draw_cycle225==cycle);
    live_server=false;streamer.scene_state=2;scene_draw_cycle225=INT32_MAX;assert(client_draw());assert(scene_draw_cycle225==INT32_MIN);
    printf("PASS candidate overhead:actual projection/full original pixels at8,11; setting6 real draw %d yellow glyph pixels; scene draws advance without game ticks,loading holds,int32 wrap\n",yellow);
    client_shutdown();return 0;
}
