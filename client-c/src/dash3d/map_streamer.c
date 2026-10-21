#include "dash3d/map_streamer.h"
#include "core/bzip2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void map_streamer_init(MapStreamer *ms) {
    if (!ms) return;
    memset(ms, 0, sizeof(MapStreamer));
    ms->scene_state = 0;
}

bool map_streamer_parse_rebuild_packet(MapStreamer *ms, Packet *pkt) {
    if (!ms || !pkt) return false;
    if (pkt->length < pkt->pos + 4) return false;

    ms->center_zone_x = g2(pkt);
    ms->center_zone_z = g2(pkt);
    ms->base_tile_x = (ms->center_zone_x - 6) * 8;
    ms->base_tile_z = (ms->center_zone_z - 6) * 8;

    int remaining = (int)(pkt->length - pkt->pos);
    int count = remaining / 10;
    if (count > MAX_STREAMED_REGIONS) count = MAX_STREAMED_REGIONS;
    ms->region_count = count;

    for (int i = 0; i < count; i++) {
        ms->regions[i].rx = g1(pkt);
        ms->regions[i].rz = g1(pkt);
        ms->regions[i].land_crc = (uint32_t)g4(pkt);
        ms->regions[i].loc_crc  = (uint32_t)g4(pkt);
    }

    ms->scene_state = 1; // Loading state
    return true;
}

static uint8_t* decompress_map_file(const char *path, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        char alt[512];
        snprintf(alt, sizeof(alt), "../%s", path);
        f = fopen(alt, "rb");
    }
    if (!f) {
        char alt[512];
        snprintf(alt, sizeof(alt), "../../Server-main/%s", path);
        f = fopen(alt, "rb");
    }
    if (!f) {
        char alt[512];
        snprintf(alt, sizeof(alt), "../../../Server-main/%s", path);
        f = fopen(alt, "rb");
    }
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (sz < 4) {
        fclose(f);
        return NULL;
    }

    uint8_t *comp = (uint8_t*)malloc(sz);
    if (!comp) {
        fclose(f);
        return NULL;
    }

    size_t read_bytes = fread(comp, 1, sz, f);
    fclose(f);
    if (read_bytes != (size_t)sz) {
        free(comp);
        return NULL;
    }

    uint32_t uncomp_len = ((uint32_t)comp[0] << 24) |
                          ((uint32_t)comp[1] << 16) |
                          ((uint32_t)comp[2] << 8)  |
                          (uint32_t)comp[3];

    // Allocate uncompressed buffer
    uint8_t *uncomp = (uint8_t*)malloc(uncomp_len);
    if (!uncomp) {
        free(comp);
        return NULL;
    }

    bzip_decompress((int8_t*)uncomp, (int8_t*)comp, (int)sz - 4, 4);
    free(comp);

    *out_len = (size_t)uncomp_len;
    return uncomp;
}

bool map_streamer_rebuild(MapStreamer *ms,
                          World *world,
                          World3D *world3d,
                          CollisionMap collision_maps[4],
                          FloConfig *flo_cfg,
                          LocConfig *loc_cfg,
                          const char *maps_dir) {
    if (!ms || !world || !world3d || !collision_maps || !flo_cfg) return false;

    // Reset world and scene
    world_reset(world);
    world3d_reset(world3d);
    for (int l = 0; l < 4; l++) {
        collision_map_reset(&collision_maps[l]);
    }

    // If regions were not populated via packet, auto-calculate 2x2 or 3x3 grid covering the 104x104 scene
    if (ms->region_count == 0) {
        int min_rx = ms->base_tile_x / 64;
        int max_rx = (ms->base_tile_x + 103) / 64;
        int min_rz = ms->base_tile_z / 64;
        int max_rz = (ms->base_tile_z + 103) / 64;

        int reg_idx = 0;
        for (int rx = min_rx; rx <= max_rx && reg_idx < MAX_STREAMED_REGIONS; rx++) {
            for (int rz = min_rz; rz <= max_rz && reg_idx < MAX_STREAMED_REGIONS; rz++) {
                ms->regions[reg_idx].rx = rx;
                ms->regions[reg_idx].rz = rz;
                ms->regions[reg_idx].land_crc = 0;
                ms->regions[reg_idx].loc_crc = 0;
                reg_idx++;
            }
        }
        ms->region_count = reg_idx;
    }

    // 1. Load Ground Landscape for all regions
    for (int i = 0; i < ms->region_count; i++) {
        int rx = ms->regions[i].rx;
        int rz = ms->regions[i].rz;
        int x_offset = rx * 64 - ms->base_tile_x;
        int z_offset = rz * 64 - ms->base_tile_z;

        char path[256];
        snprintf(path, sizeof(path), "%s/m%d_%d", maps_dir, rx, rz);
        size_t m_len = 0;
        uint8_t *m_data = decompress_map_file(path, &m_len);
        if (m_data) {
            world_load_ground(world, m_data, m_len, x_offset, z_offset, ms->base_tile_x, ms->base_tile_z);
            free(m_data);
        }
    }

    // 2. Load Scenery Locations for all regions
    for (int i = 0; i < ms->region_count; i++) {
        int rx = ms->regions[i].rx;
        int rz = ms->regions[i].rz;
        int x_offset = rx * 64 - ms->base_tile_x;
        int z_offset = rz * 64 - ms->base_tile_z;

        char path[256];
        snprintf(path, sizeof(path), "%s/l%d_%d", maps_dir, rx, rz);
        size_t l_len = 0;
        uint8_t *l_data = decompress_map_file(path, &l_len);
        if (l_data) {
            world_load_locations(world, l_data, l_len, loc_cfg, world3d, collision_maps, x_offset, z_offset);
            free(l_data);
        }
    }

    // 3. Compile World3D scene graph with lighting, shademaps, and geometry
    world_build(world, world3d, flo_cfg, collision_maps);
    ms->scene_state = 2; // Active state
    return true;
}

bool map_streamer_needs_rebuild(const MapStreamer *ms, int local_tile_x, int local_tile_z) {
    if (!ms || ms->scene_state != 2) return false;
    // Boundary threshold from client.java (triggers shift when nearing edge)
    return (local_tile_x < 16 || local_tile_x >= 88 || local_tile_z < 16 || local_tile_z >= 88);
}
