CC = x86_64-w64-mingw32-gcc
CFLAGS = -std=c99 -Wall -Wextra -O2 -Iinclude -Ilib/SDL2/x86_64-w64-mingw32/include
LDFLAGS = -Llib/SDL2/x86_64-w64-mingw32/lib -lmingw32 -lSDL2main -lSDL2 -lws2_32 -lwinmm -lm -Wl,--stack,16777216
TEST_LDFLAGS = -Llib/SDL2/x86_64-w64-mingw32/lib -lSDL2 -lws2_32 -lwinmm -lm -Wl,--stack,16777216

CORE_SRCS   = src/core/arena.c src/core/packet.c src/core/isaac.c src/core/bzip2.c src/core/jagfile.c src/core/jstring.c
GFX_SRCS    = src/gfx/pix2d.c src/gfx/pix8.c src/gfx/pix32.c src/gfx/pix3d.c src/gfx/model.c src/gfx/pixfont.c src/gfx/media.c src/gfx/anim.c
CONFIG_SRCS = src/config/flo.c src/config/seq.c src/config/loc.c src/config/npc.c src/config/obj.c src/config/idk.c
ENTITY_SRCS = src/entity/pathing_entity.c src/entity/player_entity.c src/entity/npc_entity.c
NET_SRCS    = src/net/rsa.c src/net/client_stream.c src/net/protocol.c src/net/entity_sync.c src/net/zone_sync.c
DASH3D_SRCS = src/dash3d/collision_map.c src/dash3d/world3d.c src/dash3d/world.c src/dash3d/minimap.c src/dash3d/pathfinder.c src/dash3d/map_streamer.c src/dash3d/camera.c
UI_SRCS     = src/ui/client_actions.c src/ui/component_packets.c src/ui/component.c src/ui/context_menu.c src/ui/chatbox.c src/ui/mouse_picking.c src/ui/varp.c
AUDIO_SRCS  = src/audio/wave.c src/audio/midi.c
HEADERS = $(wildcard include/*/*.h) platform/verification_input.h
ALL_SRCS    = $(CORE_SRCS) $(GFX_SRCS) $(CONFIG_SRCS) $(ENTITY_SRCS) $(NET_SRCS) $(DASH3D_SRCS) $(UI_SRCS) $(AUDIO_SRCS)

TARGET        = bin/rs2client.exe
VIEWER_TARGET = bin/model_viewer.exe

.PHONY: all test clean

all: $(TARGET) $(VIEWER_TARGET)

bin/test_bank_interface.exe: tests/test_bank_interface.c $(ALL_SRCS) $(HEADERS)
	@mkdir -p bin
	$(CC) $(CFLAGS) tests/test_bank_interface.c $(ALL_SRCS) $(TEST_LDFLAGS) -o $@

bin/test_dialogue_head.exe: tests/test_dialogue_head.c $(ALL_SRCS) $(HEADERS)
	@mkdir -p bin
	$(CC) $(CFLAGS) tests/test_dialogue_head.c $(ALL_SRCS) $(TEST_LDFLAGS) -o $@

bin/test_audio.exe: tests/test_audio.c $(ALL_SRCS) $(HEADERS)
	@mkdir -p bin
	$(CC) $(CFLAGS) tests/test_audio.c $(ALL_SRCS) $(TEST_LDFLAGS) -o $@

bin/test_minimap.exe: tests/test_minimap.c $(ALL_SRCS) $(HEADERS)
	@mkdir -p bin
	$(CC) $(CFLAGS) tests/test_minimap.c $(ALL_SRCS) $(TEST_LDFLAGS) -o $@

bin/test_coplanar_visibility.exe: tests/test_coplanar_visibility.c $(ALL_SRCS) $(HEADERS)
	@mkdir -p bin
	$(CC) $(CFLAGS) tests/test_coplanar_visibility.c $(ALL_SRCS) $(TEST_LDFLAGS) -o $@

bin/test_zone_sync.exe: tests/test_zone_sync.c $(ALL_SRCS) $(HEADERS)
	@mkdir -p bin
	$(CC) $(CFLAGS) tests/test_zone_sync.c $(ALL_SRCS) $(TEST_LDFLAGS) -o $@

bin/test_collision_reference.exe: tests/test_collision_reference.c src/dash3d/collision_map.c src/dash3d/pathfinder.c include/dash3d/collision_map.h include/dash3d/pathfinder.h tests/fixtures/collision-reference.bin
	@mkdir -p bin
	$(CC) $(CFLAGS) tests/test_collision_reference.c src/dash3d/collision_map.c src/dash3d/pathfinder.c -o $@

bin/test_protocol_transport.exe: tests/test_protocol_transport.c src/net/protocol.c src/net/client_stream.c src/net/rsa.c src/core/packet.c src/core/isaac.c include/net/protocol.h include/net/client_stream.h
	@mkdir -p bin
	$(CC) $(CFLAGS) tests/test_protocol_transport.c src/net/protocol.c src/net/client_stream.c src/net/rsa.c src/core/packet.c src/core/isaac.c -lws2_32 -lwinmm -lm -o $@

bin/test_net_sync.exe: $(ALL_SRCS) tests/test_net_sync.c
	@mkdir -p bin
	$(CC) $(CFLAGS) tests/test_net_sync.c $(ALL_SRCS) $(TEST_LDFLAGS) -o $@

bin/test_interactions.exe: $(ALL_SRCS) tests/test_interactions.c
	@mkdir -p bin
	$(CC) $(CFLAGS) tests/test_interactions.c $(ALL_SRCS) $(TEST_LDFLAGS) -o $@

bin/test_world.exe: $(ALL_SRCS) tests/test_world.c
	@mkdir -p bin
	$(CC) $(CFLAGS) tests/test_world.c $(CORE_SRCS) $(GFX_SRCS) $(CONFIG_SRCS) $(DASH3D_SRCS) -lm -o $@

$(TARGET): $(ALL_SRCS) $(HEADERS) platform/sdl2_main.c
	@mkdir -p bin
	$(CC) $(CFLAGS) platform/sdl2_main.c $(ALL_SRCS) $(LDFLAGS) -o $@
	@cp -u lib/SDL2/x86_64-w64-mingw32/bin/SDL2.dll bin/

$(VIEWER_TARGET): $(ALL_SRCS) $(HEADERS) platform/model_viewer.c
	@mkdir -p bin
	$(CC) $(CFLAGS) platform/model_viewer.c $(ALL_SRCS) $(LDFLAGS) -o $@
	@cp -u lib/SDL2/x86_64-w64-mingw32/bin/SDL2.dll bin/

test: bin/test_coplanar_visibility.exe bin/test_zone_sync.exe bin/test_collision_reference.exe bin/test_protocol_transport.exe bin/test_interactions.exe $(ALL_SRCS) tests/test_isaac.c tests/test_packet.c tests/test_jagfile.c tests/test_pix2d.c tests/test_sprites.c tests/test_rasterizer.c tests/test_scene_rasterizer.c tests/test_model.c tests/test_config.c tests/test_rsa.c tests/test_net_login.c tests/test_world.c tests/test_entity_sync.c tests/test_net_sync.c
	@mkdir -p bin
	$(CC) $(CFLAGS) tests/test_isaac.c src/core/isaac.c -o bin/test_isaac.exe
	$(CC) $(CFLAGS) tests/test_packet.c src/core/packet.c src/core/isaac.c -o bin/test_packet.exe
	$(CC) $(CFLAGS) tests/test_jagfile.c $(CORE_SRCS) -o bin/test_jagfile.exe
	$(CC) $(CFLAGS) tests/test_pix2d.c src/gfx/pix2d.c -lm -o bin/test_pix2d.exe
	$(CC) $(CFLAGS) tests/test_sprites.c $(GFX_SRCS) $(CORE_SRCS) -lm -o bin/test_sprites.exe
	$(CC) $(CFLAGS) tests/test_rasterizer.c src/gfx/pix3d.c src/gfx/pix2d.c -lm -o bin/test_rasterizer.exe
	$(CC) $(CFLAGS) tests/test_scene_rasterizer.c src/gfx/pix3d.c src/gfx/pix2d.c -lm -o bin/test_scene_rasterizer.exe
	$(CC) $(CFLAGS) tests/test_model.c $(ALL_SRCS) $(TEST_LDFLAGS) -o bin/test_model.exe
	$(CC) $(CFLAGS) tests/test_config.c $(CORE_SRCS) $(CONFIG_SRCS) $(GFX_SRCS) -lm -o bin/test_config.exe
	$(CC) $(CFLAGS) tests/test_rsa.c src/net/rsa.c -o bin/test_rsa.exe
	$(CC) $(CFLAGS) tests/test_net_login.c $(ALL_SRCS) $(TEST_LDFLAGS) -o bin/test_net_login.exe
	$(CC) $(CFLAGS) tests/test_world.c $(CORE_SRCS) $(GFX_SRCS) $(CONFIG_SRCS) $(DASH3D_SRCS) -lm -o bin/test_world.exe
	$(CC) $(CFLAGS) tests/test_entity_sync.c $(ALL_SRCS) $(TEST_LDFLAGS) -o bin/test_entity_sync.exe
	$(CC) $(CFLAGS) tests/test_media_ui.c $(ALL_SRCS) $(TEST_LDFLAGS) -o bin/test_media_ui.exe
	$(CC) $(CFLAGS) tests/test_component.c $(ALL_SRCS) $(TEST_LDFLAGS) -o bin/test_component.exe
	$(CC) $(CFLAGS) tests/test_anim.c $(ALL_SRCS) $(TEST_LDFLAGS) -o bin/test_anim.exe
	$(CC) $(CFLAGS) tests/test_context_menu.c $(ALL_SRCS) $(TEST_LDFLAGS) -o bin/test_context_menu.exe
	$(CC) $(CFLAGS) tests/test_item_icon.c $(ALL_SRCS) $(TEST_LDFLAGS) -o bin/test_item_icon.exe
	$(CC) $(CFLAGS) tests/test_minimap.c $(ALL_SRCS) $(TEST_LDFLAGS) -o bin/test_minimap.exe
	$(CC) $(CFLAGS) tests/test_audio.c $(ALL_SRCS) $(TEST_LDFLAGS) -o bin/test_audio.exe
	$(CC) $(CFLAGS) tests/test_chatbox.c $(ALL_SRCS) $(TEST_LDFLAGS) -o bin/test_chatbox.exe
	$(CC) $(CFLAGS) tests/test_mouse_picking.c $(ALL_SRCS) $(TEST_LDFLAGS) -o bin/test_mouse_picking.exe
	$(CC) $(CFLAGS) tests/test_pathfinding.c $(ALL_SRCS) $(TEST_LDFLAGS) -o bin/test_pathfinding.exe
	$(CC) $(CFLAGS) tests/test_map_streaming.c $(ALL_SRCS) $(TEST_LDFLAGS) -o bin/test_map_streaming.exe
	$(CC) $(CFLAGS) tests/test_interface_scripts.c $(ALL_SRCS) $(TEST_LDFLAGS) -o bin/test_interface_scripts.exe
	$(CC) $(CFLAGS) tests/test_camera.c $(ALL_SRCS) $(TEST_LDFLAGS) -o bin/test_camera.exe
	$(CC) $(CFLAGS) tests/test_full_ui.c $(ALL_SRCS) $(TEST_LDFLAGS) -o bin/test_full_ui.exe
	$(CC) $(CFLAGS) tests/test_net_sync.c $(ALL_SRCS) $(TEST_LDFLAGS) -o bin/test_net_sync.exe
	$(CC) $(CFLAGS) tests/test_dialogue_head.c $(ALL_SRCS) $(TEST_LDFLAGS) -o bin/test_dialogue_head.exe
	$(CC) $(CFLAGS) tests/test_bank_interface.c $(ALL_SRCS) $(TEST_LDFLAGS) -o bin/test_bank_interface.exe
	@echo "================ RUNNING TESTS ================"
	@./bin/test_coplanar_visibility.exe
	@./bin/test_zone_sync.exe
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
	@./bin/test_net_login.exe
	@./bin/test_media_ui.exe
	@./bin/test_component.exe
	@./bin/test_anim.exe
	@./bin/test_context_menu.exe
	@./bin/test_item_icon.exe
	@./bin/test_minimap.exe
	@./bin/test_audio.exe
	@./bin/test_chatbox.exe
	@./bin/test_mouse_picking.exe
	@./bin/test_pathfinding.exe
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
