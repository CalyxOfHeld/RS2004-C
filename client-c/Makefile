CC = x86_64-w64-mingw32-gcc
CFLAGS = -std=c99 -Wall -Wextra -O2 -Iinclude -Ilib/SDL2/x86_64-w64-mingw32/include
LDFLAGS = -Llib/SDL2/x86_64-w64-mingw32/lib -lmingw32 -lSDL2main -lSDL2 -lws2_32 -lwinmm -lole32 -ldsound -ldxguid -luuid -lm -Wl,--stack,16777216
TEST_LDFLAGS = -Llib/SDL2/x86_64-w64-mingw32/lib -lSDL2 -lws2_32 -lwinmm -lole32 -ldsound -ldxguid -luuid -lm -Wl,--stack,16777216

CORE_SRCS   = src/core/arena.c src/core/packet.c src/core/isaac.c src/core/bzip2.c src/core/jagfile.c src/core/jstring.c src/core/jrandom.c src/core/session_log.c src/core/wordpack.c src/core/wordfilter.c
CORE_SRCS += src/core/input_tracking.c src/core/sound_queue.c src/core/music225.c
GFX_SRCS    = src/gfx/pix2d.c src/gfx/pix8.c src/gfx/pix32.c src/gfx/pix3d.c src/gfx/pix3d_flat225.c src/gfx/pix3d_gouraud225.c src/gfx/pix3d_texture225.c src/gfx/pix3d_texture_triangle225.c src/gfx/pix3d_texture_cache225.c src/gfx/model.c src/gfx/pixfont.c src/gfx/media.c src/gfx/anim.c src/gfx/jpeg.c
CONFIG_SRCS = src/config/flo.c src/config/seq.c src/config/loc.c src/config/npc.c src/config/obj.c src/config/idk.c src/config/spotanim.c src/config/varp_type.c
ENTITY_SRCS = src/entity/pathing_entity.c src/entity/player_entity.c src/entity/npc_entity.c
ENTITY_SRCS += src/entity/actor_model.c
NET_SRCS    = src/net/rsa.c src/net/client_stream.c src/net/protocol.c src/net/entity_sync.c src/net/zone_sync.c
DASH3D_SRCS = src/dash3d/collision_map.c src/dash3d/world3d.c src/dash3d/ground_items.c src/dash3d/world.c src/dash3d/minimap.c src/dash3d/pathfinder.c src/dash3d/map_streamer.c src/dash3d/camera.c src/dash3d/effects.c
UI_SRCS     = src/ui/client_actions.c src/ui/component_packets.c src/ui/component.c src/ui/context_menu.c src/ui/chatbox.c src/ui/overhead_chat.c src/ui/mouse_picking.c src/ui/varp.c src/ui/click_feedback.c src/ui/combat_feedback.c src/ui/title_screen.c
AUDIO_SRCS  = src/audio/wave.c src/audio/midi.c src/audio/midi_gain225.c
DASH3D_SRCS += src/dash3d/location_anim.c
DASH3D_SRCS += src/dash3d/location_merge.c
DASH3D_SRCS += src/dash3d/scene_lighting.c
DASH3D_SRCS += src/dash3d/ground225.c
DASH3D_SRCS += src/dash3d/visibility225.c
DASH3D_SRCS += src/dash3d/scene_selection225.c
DASH3D_SRCS += src/dash3d/occlusion225.c
DASH3D_SRCS += src/dash3d/scene_raster225.c src/dash3d/scene_traversal225.c src/dash3d/scene_prepare225.c src/dash3d/scene_build225.c src/dash3d/scene_runtime225.c
HEADERS = $(wildcard include/*/*.h)
ALL_SRCS    = $(CORE_SRCS) $(GFX_SRCS) $(CONFIG_SRCS) $(ENTITY_SRCS) $(NET_SRCS) $(DASH3D_SRCS) $(UI_SRCS) $(AUDIO_SRCS)

TARGET        = bin/rs2client.exe
REPLAY_TARGET = bin/rs2replay.exe
VIEWER_TARGET = bin/model_viewer.exe

.PHONY: all test clean parity

all: $(TARGET) $(REPLAY_TARGET) $(VIEWER_TARGET)

ALL_OBJS = $(ALL_SRCS:%.c=build/%.o)
# The client loop (client.java load/updateGame/draw), linked by the platforms.
# Normal launch and replay use the same revision225 loop as the qualified candidate.
CLIENT_OBJS = build/src/client/client225.o build/src/client/client_log.o
.SECONDARY: $(ALL_OBJS) $(CLIENT_OBJS)

build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(ALL_OBJS:.o=.d) $(CLIENT_OBJS:.o=.d)

bin/test_%.exe: tests/test_%.c $(ALL_OBJS) $(HEADERS)
	@mkdir -p bin
	$(CC) $(CFLAGS) $< $(ALL_OBJS) $(TEST_LDFLAGS) -o $@

# Keep the traversal regression isolated from the installed renderer.
bin/test_scene_traversal225.exe: tests/test_scene_traversal225.c src/dash3d/scene_traversal225.c src/dash3d/occlusion225.c include/dash3d/scene_traversal225.h include/dash3d/occlusion225.h
	@mkdir -p bin
	$(CC) $(CFLAGS) tests/test_scene_traversal225.c src/dash3d/scene_traversal225.c src/dash3d/occlusion225.c -o $@

# Opt-in complete-frame integration; the installed World3D path is unchanged.
SCENE225_SRCS = verification/completion/tools/scene_build225_fixture.c
bin/scene_raster225.dll: tests/test_painter_trace_reference.c $(SCENE225_SRCS) $(ALL_OBJS) $(HEADERS)
	@mkdir -p bin
	$(CC) $(CFLAGS) -Werror -DPAINTER_TRACE_LIBRARY -shared tests/test_painter_trace_reference.c $(SCENE225_SRCS) $(ALL_OBJS) $(TEST_LDFLAGS) -o $@

$(TARGET): $(ALL_OBJS) $(CLIENT_OBJS) $(HEADERS) platform/sdl2_main.c
	@mkdir -p bin
	$(CC) $(CFLAGS) platform/sdl2_main.c $(CLIENT_OBJS) $(ALL_OBJS) $(LDFLAGS) -o $@
	@cp -u lib/SDL2/x86_64-w64-mingw32/bin/SDL2.dll bin/

# Separate playable candidate; normal executable is not overwritten by this rule.
build/src/client/client225.o: src/client/client.c $(HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Werror -DRS2_SCENE225_RUNTIME=1 -DRS2_PIX3D_REV225_TEXTURE=1 -c $< -o $@
bin/rs2client225.exe: $(ALL_OBJS) build/src/client/client225.o build/src/client/client_log.o $(HEADERS) platform/sdl2_main.c
	$(CC) $(CFLAGS) platform/sdl2_main.c build/src/client/client225.o build/src/client/client_log.o $(ALL_OBJS) $(LDFLAGS) -o $@
	@cp -u lib/SDL2/x86_64-w64-mingw32/bin/SDL2.dll bin/
bin/rs2replay225.exe: $(ALL_OBJS) build/src/client/client225.o build/src/client/client_log.o $(HEADERS) platform/replay_main.c
	$(CC) $(CFLAGS) platform/replay_main.c build/src/client/client225.o build/src/client/client_log.o $(ALL_OBJS) $(TEST_LDFLAGS) -o $@
	@cp -u lib/SDL2/x86_64-w64-mingw32/bin/SDL2.dll bin/

# Headless session-log replay: console program, no window or audio device.
bin/overhead_chat_candidate_control.exe: verification/completion/tools/overhead_chat_candidate_control.c tests/test_chatbox.c src/client/client.c $(ALL_OBJS) $(HEADERS)
	$(CC) $(CFLAGS) -Werror -DRS2_SCENE225_RUNTIME=1 -DRS2_PIX3D_REV225_TEXTURE=1 $< $(ALL_OBJS) $(TEST_LDFLAGS) -o $@

bin/test_map_streaming.exe: verification/completion/tools/map_download_reference.h
bin/map_download_candidate_control.exe: verification/completion/tools/map_download_candidate_control.c verification/completion/tools/map_download_reference.h src/client/client.c $(ALL_OBJS) $(HEADERS)
	$(CC) $(CFLAGS) -Werror -DRS2_SCENE225_RUNTIME=1 -DRS2_PIX3D_REV225_TEXTURE=1 $< $(ALL_OBJS) $(TEST_LDFLAGS) -o $@
bin/map_download_failure_control.exe: verification/completion/tools/map_download_failure_control.c src/dash3d/map_streamer.c $(ALL_OBJS) $(HEADERS)
	$(CC) $(CFLAGS) -Werror $< $(filter-out build/src/dash3d/map_streamer.o,$(ALL_OBJS)) $(TEST_LDFLAGS) -o $@
bin/test_audio.exe: verification/completion/tools/wave_setting_reference_control.h verification/completion/tools/sound_queue_reference_control.h
bin/test_audio.exe: verification/completion/tools/music_state_reference_control.h
bin/music_candidate_control.exe: verification/completion/tools/music_candidate_control.c verification/completion/tools/music_state_reference_control.h verification/completion/tools/wave_setting_candidate_control.c src/client/client.c src/audio/midi.c src/audio/wave.c $(ALL_OBJS) $(HEADERS)
	$(CC) $(CFLAGS) -Werror -DRS2_SCENE225_RUNTIME=1 -DRS2_PIX3D_REV225_TEXTURE=1 $< $(filter-out build/src/audio/wave.o build/src/audio/midi.o,$(ALL_OBJS)) $(TEST_LDFLAGS) -o $@
bin/test_context_menu.exe: verification/completion/tools/mouse_mode_reference_control.h
bin/test_component.exe: verification/completion/tools/last_login_reference_control.h verification/completion/tools/continue_reference_control.h
bin/test_protocol_transport.exe: verification/completion/tools/input_tracking_reference_control.h
bin/input_tracking_candidate_control.exe: verification/completion/tools/input_tracking_candidate_control.c verification/completion/tools/input_tracking_reference_control.h src/client/client.c $(ALL_OBJS) build/src/client/client_log.o $(HEADERS)
	$(CC) $(CFLAGS) -Werror -DRS2_SCENE225_RUNTIME=1 -DRS2_PIX3D_REV225_TEXTURE=1 $< build/src/client/client_log.o $(ALL_OBJS) $(TEST_LDFLAGS) -o $@
bin/sound_queue_candidate_control.exe: verification/completion/tools/sound_queue_candidate_control.c verification/completion/tools/sound_queue_reference_control.h src/client/client.c src/audio/wave.c $(ALL_OBJS) build/src/client/client_log.o $(HEADERS)
	$(CC) $(CFLAGS) -Werror -DRS2_SCENE225_RUNTIME=1 -DRS2_PIX3D_REV225_TEXTURE=1 $< build/src/client/client_log.o $(filter-out build/src/audio/wave.o,$(ALL_OBJS)) $(TEST_LDFLAGS) -o $@
bin/wave_setting_candidate_control.exe: verification/completion/tools/wave_setting_candidate_control.c verification/completion/tools/wave_setting_reference_control.h src/client/client.c src/audio/wave.c $(ALL_OBJS) $(HEADERS)
	$(CC) $(CFLAGS) -Werror -DRS2_SCENE225_RUNTIME=1 -DRS2_PIX3D_REV225_TEXTURE=1 $< $(filter-out build/src/audio/wave.o,$(ALL_OBJS)) $(TEST_LDFLAGS) -o $@
bin/mouse_mode_candidate_control.exe: verification/completion/tools/mouse_mode_candidate_control.c verification/completion/tools/mouse_mode_reference_control.h src/client/client.c $(ALL_OBJS) $(HEADERS)
	$(CC) $(CFLAGS) -Werror -DRS2_SCENE225_RUNTIME=1 -DRS2_PIX3D_REV225_TEXTURE=1 $< $(ALL_OBJS) $(TEST_LDFLAGS) -o $@
bin/input_phase_candidate_control.exe: verification/completion/tools/input_phase_candidate_control.c src/client/client.c $(ALL_OBJS) $(HEADERS)
	$(CC) $(CFLAGS) -Werror -DRS2_SCENE225_RUNTIME=1 -DRS2_PIX3D_REV225_TEXTURE=1 $< $(ALL_OBJS) $(TEST_LDFLAGS) -o $@
bin/continue_candidate_control.exe: verification/completion/tools/continue_candidate_control.c verification/completion/tools/continue_reference_control.h src/client/client.c $(ALL_OBJS) $(HEADERS)
	$(CC) $(CFLAGS) -Werror -DRS2_SCENE225_RUNTIME=1 -DRS2_PIX3D_REV225_TEXTURE=1 $< $(ALL_OBJS) $(TEST_LDFLAGS) -o $@
bin/last_login_candidate_control.exe: verification/completion/tools/last_login_candidate_control.c verification/completion/tools/last_login_reference_control.h src/client/client.c $(ALL_OBJS) $(HEADERS)
	$(CC) $(CFLAGS) -Werror -DRS2_SCENE225_RUNTIME=1 -DRS2_PIX3D_REV225_TEXTURE=1 $< $(ALL_OBJS) $(TEST_LDFLAGS) -o $@

bin/private_send_loop_control.exe: verification/completion/tools/private_send_loop_control.c tests/test_chatbox.c src/client/client.c $(ALL_OBJS) $(HEADERS)
	$(CC) $(CFLAGS) -Werror -DRS2_SCENE225_RUNTIME=1 -DRS2_PIX3D_REV225_TEXTURE=1 $< $(ALL_OBJS) $(TEST_LDFLAGS) -o $@
bin/public_send_loop_control.exe: verification/completion/tools/public_send_loop_control.c tests/test_chatbox.c src/client/client.c $(ALL_OBJS) $(HEADERS)
	$(CC) $(CFLAGS) -Werror -DRS2_SCENE225_RUNTIME=1 -DRS2_PIX3D_REV225_TEXTURE=1 $< $(ALL_OBJS) $(TEST_LDFLAGS) -o $@
bin/private_send_failure_control.exe: verification/completion/tools/private_send_failure_control.c src/ui/chatbox.c $(ALL_OBJS) $(HEADERS)
	$(CC) $(CFLAGS) -Werror $< $(filter-out build/src/ui/chatbox.o,$(ALL_OBJS)) $(TEST_LDFLAGS) -o $@

bin/social_actions_loop_control.exe: verification/completion/tools/social_actions_loop_control.c tests/test_chatbox.c src/client/client.c $(ALL_OBJS) $(HEADERS)
	$(CC) $(CFLAGS) -Werror -DRS2_SCENE225_RUNTIME=1 -DRS2_PIX3D_REV225_TEXTURE=1 $< $(ALL_OBJS) $(TEST_LDFLAGS) -o $@
bin/social_actions_failure_control.exe: verification/completion/tools/social_actions_failure_control.c src/ui/chatbox.c $(ALL_OBJS) $(HEADERS)
	$(CC) $(CFLAGS) -Werror $< $(filter-out build/src/ui/chatbox.o,$(ALL_OBJS)) $(TEST_LDFLAGS) -o $@

bin/report_abuse_loop_control.exe: verification/completion/tools/report_abuse_loop_control.c tests/test_chatbox.c src/client/client.c $(ALL_OBJS) $(HEADERS)
	$(CC) $(CFLAGS) -Werror -DRS2_SCENE225_RUNTIME=1 -DRS2_PIX3D_REV225_TEXTURE=1 $< $(ALL_OBJS) $(TEST_LDFLAGS) -o $@
bin/report_abuse_failure_control.exe: verification/completion/tools/report_abuse_failure_control.c $(ALL_OBJS) $(HEADERS)
	$(CC) $(CFLAGS) -Werror $< $(ALL_OBJS) $(TEST_LDFLAGS) -o $@

bin/game_requests_loop_control.exe: verification/completion/tools/game_requests_loop_control.c tests/test_chatbox.c src/client/client.c $(ALL_OBJS) $(HEADERS)
	$(CC) $(CFLAGS) -Werror -DRS2_SCENE225_RUNTIME=1 -DRS2_PIX3D_REV225_TEXTURE=1 $< $(ALL_OBJS) $(TEST_LDFLAGS) -o $@
bin/game_requests_failure_control.exe: verification/completion/tools/game_requests_failure_control.c src/ui/chatbox.c $(ALL_OBJS) $(HEADERS)
	$(CC) $(CFLAGS) -Werror $< $(filter-out build/src/ui/chatbox.o,$(ALL_OBJS)) $(TEST_LDFLAGS) -o $@

bin/test_camera.exe: tests/test_camera.c verification/completion/tools/camera_reference_control.h $(ALL_OBJS) $(HEADERS)
	$(CC) $(CFLAGS) tests/test_camera.c $(ALL_OBJS) $(TEST_LDFLAGS) -o $@
bin/test_component.exe: tests/test_component.c verification/completion/tools/interface_models_reference_control.h $(ALL_OBJS) $(HEADERS)
	$(CC) $(CFLAGS) tests/test_component.c $(ALL_OBJS) $(TEST_LDFLAGS) -o $@

bin/test_entity_sync.exe: tests/test_entity_sync.c verification/completion/tools/reset_multiway_reference_control.h $(ALL_OBJS) $(HEADERS)
	$(CC) $(CFLAGS) tests/test_entity_sync.c $(ALL_OBJS) $(TEST_LDFLAGS) -o $@

bin/camera_candidate_control.exe: verification/completion/tools/camera_candidate_control.c verification/completion/tools/camera_reference_control.h src/client/client.c $(ALL_OBJS) $(HEADERS)
	$(CC) $(CFLAGS) -Werror -DRS2_SCENE225_RUNTIME=1 -DRS2_PIX3D_REV225_TEXTURE=1 $< $(ALL_OBJS) $(TEST_LDFLAGS) -o $@

bin/interface_models_candidate_control.exe: verification/completion/tools/interface_models_candidate_control.c verification/completion/tools/interface_models_reference_control.h src/client/client.c $(ALL_OBJS) $(HEADERS)
	$(CC) $(CFLAGS) -Werror -DRS2_SCENE225_RUNTIME=1 -DRS2_PIX3D_REV225_TEXTURE=1 $< $(ALL_OBJS) $(TEST_LDFLAGS) -o $@
bin/interface_models_failure_control.exe: verification/completion/tools/interface_models_failure_control.c src/ui/component_packets.c $(ALL_OBJS) $(HEADERS)
	$(CC) $(CFLAGS) -Werror $< $(filter-out build/src/ui/component_packets.o,$(ALL_OBJS)) $(TEST_LDFLAGS) -o $@

bin/reset_multiway_candidate_control.exe: verification/completion/tools/reset_multiway_candidate_control.c verification/completion/tools/reset_multiway_reference_control.h src/client/client.c $(ALL_OBJS) $(HEADERS)
	$(CC) $(CFLAGS) -Werror -DRS2_SCENE225_RUNTIME=1 -DRS2_PIX3D_REV225_TEXTURE=1 $< $(ALL_OBJS) $(TEST_LDFLAGS) -o $@

$(REPLAY_TARGET): $(ALL_OBJS) $(CLIENT_OBJS) $(HEADERS) platform/replay_main.c
	@mkdir -p bin
	$(CC) $(CFLAGS) platform/replay_main.c $(CLIENT_OBJS) $(ALL_OBJS) $(TEST_LDFLAGS) -o $@
	@cp -u lib/SDL2/x86_64-w64-mingw32/bin/SDL2.dll bin/

$(VIEWER_TARGET): $(ALL_OBJS) $(HEADERS) platform/model_viewer.c
	@mkdir -p bin
	$(CC) $(CFLAGS) platform/model_viewer.c $(ALL_OBJS) $(LDFLAGS) -o $@
	@cp -u lib/SDL2/x86_64-w64-mingw32/bin/SDL2.dll bin/

TEST_BINS = bin/test_coplanar_visibility.exe bin/test_zone_sync.exe bin/test_ground_items.exe bin/test_collision_reference.exe bin/test_protocol_transport.exe bin/test_interactions.exe bin/test_isaac.exe bin/test_packet.exe bin/test_jagfile.exe bin/test_pix2d.exe bin/test_sprites.exe bin/test_rasterizer.exe bin/test_scene_rasterizer.exe bin/test_model.exe bin/test_config.exe bin/test_rsa.exe bin/test_world.exe bin/test_entity_sync.exe bin/test_audio_reference.exe bin/test_net_login.exe bin/test_media_ui.exe bin/test_component.exe bin/test_anim.exe bin/test_context_menu.exe bin/test_item_icon.exe bin/test_minimap.exe bin/test_audio.exe bin/test_chatbox.exe bin/test_mouse_picking.exe bin/test_pathfinding.exe bin/test_map_streaming.exe bin/test_interface_scripts.exe bin/test_camera.exe bin/test_full_ui.exe bin/test_net_sync.exe bin/test_dialogue_head.exe bin/test_bank_interface.exe

TEST_BINS += bin/test_navigation_reference.exe
TEST_BINS += bin/test_minimap_reference.exe
TEST_BINS += bin/test_midi_data.exe
TEST_BINS += bin/test_click_feedback.exe
TEST_BINS += bin/test_target_interactions.exe
TEST_BINS += bin/test_effects.exe
TEST_BINS += bin/test_location_anim.exe
TEST_BINS += bin/test_location_merge.exe
TEST_BINS += bin/test_exact_movement.exe
TEST_BINS += bin/test_actor_models.exe
TEST_BINS += bin/test_actor_clock.exe
TEST_BINS += bin/test_facing.exe
TEST_BINS += bin/test_bridge_levels.exe
TEST_BINS += bin/test_scene_lighting.exe
TEST_BINS += bin/test_wall_decor.exe
TEST_BINS += bin/test_title_screen.exe
TEST_BINS += bin/test_data_reference.exe
TEST_BINS += bin/test_pix3d_flat_reference.exe
TEST_BINS += bin/test_pix3d_gouraud_reference.exe
TEST_BINS += bin/test_pix3d_palette_reference.exe
TEST_BINS += bin/test_pix3d_texture_span_reference.exe
TEST_BINS += bin/test_pix3d_texture_triangle_reference.exe
TEST_BINS += bin/test_pix3d_texture_cache_reference.exe
TEST_BINS += bin/test_pix3d_brightness_reference.exe
TEST_BINS += bin/test_model_draw_reference.exe
TEST_BINS += bin/test_model_world_reference.exe
TEST_BINS += bin/test_model_picking_reference.exe
TEST_BINS += bin/test_ground_reference.exe
TEST_BINS += bin/test_tile_construction_reference.exe
TEST_BINS += bin/test_visibility_reference.exe
TEST_BINS += bin/test_scene_selection_reference.exe
TEST_BINS += bin/test_occlusion_reference.exe
TEST_BINS += bin/test_painter_trace_reference.exe
TEST_BINS += bin/test_scene_traversal225.exe

test: $(TEST_BINS)
	@echo "================ RUNNING TESTS ================"
	@./bin/test_coplanar_visibility.exe
	@./bin/test_zone_sync.exe
	@./bin/test_ground_items.exe
	@./bin/test_collision_reference.exe
	@./bin/test_protocol_transport.exe
	@./bin/test_interactions.exe
	@./bin/test_isaac.exe
	@./bin/test_packet.exe
	@./bin/test_jagfile.exe
	@./bin/test_pix2d.exe
	@./bin/test_sprites.exe
	@./bin/test_rasterizer.exe
	@./bin/test_scene_rasterizer.exe
	@./bin/test_model.exe
	@./bin/test_config.exe
	@./bin/test_rsa.exe
	@./bin/test_world.exe
	@./bin/test_entity_sync.exe
	@./bin/test_audio_reference.exe tests/fixtures/audio-reference.bin
	@./bin/test_net_login.exe
	@./bin/test_media_ui.exe
	@./bin/test_component.exe
	@./bin/test_anim.exe
	@./bin/test_context_menu.exe
	@./bin/test_item_icon.exe
	@./bin/test_minimap.exe
	@./bin/test_minimap_reference.exe
	@./bin/test_audio.exe
	@./bin/test_midi_data.exe
	@./bin/test_click_feedback.exe
	@./bin/test_target_interactions.exe
	@./bin/test_effects.exe
	@./bin/test_location_anim.exe
	@./bin/test_location_merge.exe
	@./bin/test_exact_movement.exe
	@./bin/test_actor_models.exe
	@./bin/test_actor_clock.exe
	@./bin/test_facing.exe
	@./bin/test_bridge_levels.exe
	@./bin/test_scene_lighting.exe
	@./bin/test_wall_decor.exe
	@./bin/test_title_screen.exe
	@./bin/test_chatbox.exe
	@./bin/test_mouse_picking.exe
	@./bin/test_pathfinding.exe
	@./bin/test_navigation_reference.exe
	@./bin/test_map_streaming.exe
	@./bin/test_interface_scripts.exe
	@./bin/test_camera.exe
	@./bin/test_full_ui.exe
	@./bin/test_net_sync.exe
	@./bin/test_dialogue_head.exe
	@./bin/test_bank_interface.exe
	@./bin/test_data_reference.exe --check ../engine/data/pack/client tests/fixtures/data-reference.digest
	@./bin/test_pix3d_flat_reference.exe
	@./bin/test_pix3d_gouraud_reference.exe
	@./bin/test_pix3d_palette_reference.exe ../engine/data/pack/client/textures tests/fixtures/pix3d-palette-reference.bin
	@./bin/test_pix3d_texture_span_reference.exe tests/fixtures/pix3d-texture-span-reference.bin
	@./bin/test_pix3d_texture_triangle_reference.exe tests/fixtures/pix3d-texture-triangle-reference.bin
	@./bin/test_pix3d_texture_cache_reference.exe ../engine/data/pack/client/textures tests/fixtures/pix3d-texture-span-reference.bin
	@./bin/test_pix3d_brightness_reference.exe ../engine/data/pack/client/textures tests/fixtures/pix3d-brightness-reference.bin
	@./bin/test_model_draw_reference.exe ../engine/data/pack/client/models tests/fixtures/model-draw-reference.bin ../engine/data/pack/client/textures
	@./bin/test_model_world_reference.exe ../engine/data/pack/client/models tests/fixtures/model-world-reference.bin ../engine/data/pack/client/textures
	@./bin/test_model_picking_reference.exe ../engine/data/pack/client/models tests/fixtures/model-picking-reference.bin ../engine/data/pack/client/textures
	@./bin/test_ground_reference.exe ../engine/data/pack/client/textures tests/fixtures/ground-reference.bin
	@./bin/test_tile_construction_reference.exe tests/fixtures/tile-construction-reference.bin
	@./bin/test_visibility_reference.exe tests/fixtures/visibility-reference.bin
	@./bin/test_scene_selection_reference.exe tests/fixtures/scene-selection-reference.bin
	@./bin/test_occlusion_reference.exe tests/fixtures/occlusion-reference.bin
	@./bin/test_painter_trace_reference.exe ../engine/data/pack/client/textures tests/fixtures/painter-trace-reference.bin
	@./bin/test_scene_traversal225.exe
	@echo "================ ALL TESTS PASSED ================"

# Differential parity against the unchanged Java client on a recorded session.
# Needs javac/java, python and the session's password in RS2_PASSWORD.
PARITY_LOG ?= verification/completion/runs/2026-10-22-phaseB-replay/live-record6/session.rec
PARITY_OUT ?= build/parity
parity: all
	powershell -NoProfile -ExecutionPolicy Bypass -File verification/completion/tools/parity.ps1 -Log $(PARITY_LOG) -OutDir $(PARITY_OUT)

clean:
	rm -rf build bin/rs2client.exe bin/model_viewer.exe bin/test_*.exe
