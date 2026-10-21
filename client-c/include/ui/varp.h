#ifndef RS2_VARP_H
#define RS2_VARP_H

#include <stdint.h>
#include <stdbool.h>
#include "ui/component.h"

#define MAX_VARPS 2000

typedef struct VarpContext {
    int varps[MAX_VARPS];
    int skill_levels[25];
    int skill_base_levels[25];
    int skill_exp[25];
    int energy;
    int weight;
    int combat_level;
} VarpContext;

void varp_init(VarpContext *ctx);
void varp_set(VarpContext *ctx, int varp_id, int value);
int  varp_get(const VarpContext *ctx, int varp_id);

/**
 * Evaluates an individual component condition script (client.java:9070-9146).
 */
int varp_eval_script(const VarpContext *ctx, const ComponentConfig *cfg, const Component *com, int script_idx);

/**
 * Evaluates all conditional triggers of a component (client.java:8337-8365).
 * Returns true if the component satisfies all trigger conditions (active state).
 */
bool varp_eval_component_triggers(const VarpContext *ctx, const ComponentConfig *cfg, const Component *com);

/**
 * Handles clicking a component to toggle or update varps (client.java:5647-5701).
 */
bool varp_handle_component_click(VarpContext *ctx, const Component *com);

/**
 * Formats interface dynamic text templates (%1..%5 client script substitution)
 * matching LostCity RS2 deob client.java:4352-4384.
 */
void varp_format_text(const VarpContext *ctx, const ComponentConfig *cfg, const Component *com, const char *raw_text, char *out_buf, size_t out_sz);

/**
 * Draws a component tree with dynamic varp trigger evaluation.
 */
void component_draw_with_varps(ComponentConfig *cfg, Component *com, int x, int y, int scroll_y, const VarpContext *varps);

#endif // RS2_VARP_H
