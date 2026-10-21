CC = x86_64-w64-mingw32-gcc
CFLAGS = -std=c99 -Wall -Wextra -O2 -Iinclude -Ilib/SDL2/x86_64-w64-mingw32/include
LDFLAGS = -Llib/SDL2/x86_64-w64-mingw32/lib -lmingw32 -lSDL2main -lSDL2 -lws2_32 -lwinmm -lm -Wl,--stack,16777216
TEST_LDFLAGS = -Llib/SDL2/x86_64-w64-mingw32/lib -lSDL2 -lws2_32 -lwinmm -lm -Wl,--stack,16777216

CORE_SRCS   = src/core/arena.c src/core/packet.c src/core/isaac.c src/core/bzip2.c src/core/jagfile.c src/core/jstring.c
GFX_SRCS    = src/gfx/pix2d.c src/gfx/pix8.c src/gfx/pix32.c src/gfx/pix3d.c src/gfx/model.c src/gfx/pixfont.c src/gfx/media.c src/gfx/anim.c
CONFIG_SRCS = src/config/flo.c src/config/seq.c src/config/loc.c src/config/npc.c src/config/obj.c src/config/idk.c src/config/spotanim.c
ENTITY_SRCS = src/entity/pathing_entity.c src/entity/player_entity.c src/entity/npc_entity.c
ENTITY_SRCS += src/entity/actor_model.c
NET_SRCS    = src/net/rsa.c src/net/client_stream.c src/net/protocol.c src/net/entity_sync.c src/net/zone_sync.c
DASH3D_SRCS = src/dash3d/collision_map.c src/dash3d/world3d.c src/dash3d/ground_items.c src/dash3d/world.c src/dash3d/minimap.c src/dash3d/pathfinder.c src/dash3d/map_streamer.c src/dash3d/camera.c src/dash3d/effects.c
UI_SRCS     = src/ui/client_actions.c src/ui/component_packets.c src/ui/component.c src/ui/context_menu.c src/ui/chatbox.c src/ui/mouse_picking.c src/ui/varp.c src/ui/click_feedback.c src/ui/combat_feedback.c
AUDIO_SRCS  = src/audio/wave.c src/audio/midi.c
DASH3D_SRCS += src/dash3d/location_anim.c
DASH3D_SRCS += src/dash3d/location_merge.c
DASH3D_SRCS += src/dash3d/scene_lighting.c
HEADERS = $(wildcard include/*/*.h) platform/verification_input.h
ALL_SRCS    = $(CORE_SRCS) $(GFX_SRCS) $(CONFIG_SRCS) $(ENTITY_SRCS) $(NET_SRCS) $(DASH3D_SRCS) $(UI_SRCS) $(AUDIO_SRCS)

TARGET        = bin/rs2client.exe
VIEWER_TARGET = bin/model_viewer.exe

.PHONY: all test clean

all: $(TARGET) $(VIEWER_TARGET)

ALL_OBJS = $(ALL_SRCS:%.c=build/%.o)
.SECONDARY: $(ALL_OBJS)

build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(ALL_OBJS:.o=.d)

bin/test_%.exe: tests/test_%.c $(ALL_OBJS) $(HEADERS)
	@mkdir -p bin
	$(CC) $(CFLAGS) $< $(ALL_OBJS) $(TEST_LDFLAGS) -o $@

$(TARGET): $(ALL_OBJS) $(HEADERS) platform/sdl2_main.c
	@mkdir -p bin
	$(CC) $(CFLAGS) platform/sdl2_main.c $(ALL_OBJS) $(LDFLAGS) -o $@
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
	@echo "================ ALL TESTS PASSED ================"

clean:
	rm -rf build bin/rs2client.exe bin/model_viewer.exe bin/test_*.exe
