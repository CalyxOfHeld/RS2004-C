#ifndef RS2_MAP_STREAMER_H
#define RS2_MAP_STREAMER_H

#include <stdint.h>
#include <stdbool.h>
#include "core/packet.h"
#include "dash3d/world.h"
#include "dash3d/world3d.h"
#include "dash3d/collision_map.h"
#include "config/flo.h"
#include "config/loc.h"

#define MAX_STREAMED_REGIONS 16

typedef struct {
    int rx;
    int rz;
    uint32_t land_crc;
    uint32_t loc_crc;
} MapRegionDescriptor;

typedef struct {
    int center_zone_x;
    int center_zone_z;
    int base_tile_x;
    int base_tile_z;
    int scene_state; // 0 = empty, 1 = loading, 2 = active
    int current_level;

    int region_count;
    MapRegionDescriptor regions[MAX_STREAMED_REGIONS];
} MapStreamer;

void map_streamer_init(MapStreamer *ms);

/**
 * Parses Packet 237 (REBUILD_NORMAL) matching client.java:9911-9950.
 */
bool map_streamer_parse_rebuild_packet(MapStreamer *ms, Packet *pkt);

/**
 * REBUILD_NORMAL's cache check (client.java:9935-9980): for each mapsquare in
 * packet order, a land file (type 0) then a loc file (type 1) whose CRC is
 * nonzero and which is absent from maps_dir or fails the CRC is listed as
 * (type, x, z). Returns the number of bytes written to entries (3 per file).
 */
int map_streamer_missing_files(const MapStreamer *ms, const char *maps_dir, uint8_t *entries, int capacity);

/**
 * Unpacks and stitches map chunks into active 104x104 scene graph.
 */
bool map_streamer_rebuild(MapStreamer *ms,
                          World *world,
                          World3D *world3d,
                          CollisionMap collision_maps[4],
                          FloConfig *flo_cfg,
                          LocConfig *loc_cfg,
                          const char *maps_dir);

/**
 * Checks if player position (local tile 0..103) approaches scene borders.
 */
bool map_streamer_needs_rebuild(const MapStreamer *ms, int local_tile_x, int local_tile_z);

#endif // RS2_MAP_STREAMER_H
