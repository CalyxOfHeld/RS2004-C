#ifndef RS2_COMPONENT_PACKETS_H
#define RS2_COMPONENT_PACKETS_H
#include "ui/component.h"
#include "ui/varp.h"
/* 1 applied, 0 not a component packet, -1 malformed. Uses production wire data. */
int component_apply_packet(ComponentConfig *cfg, VarpContext *varps, Arena *arena,
                           int opcode, const uint8_t *data, size_t length);
#endif
