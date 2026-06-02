//
// Created by drago on 02.06.2026.
//

#include "block.h"
#include "block.h"
#include <string.h>
#include <stdio.h>

BlockDef g_blocks[MAX_BLOCK_TYPES] = {0};
int      g_block_count = 0;

uint16_t block_register(const char *namespace, const char *name) {
    char full[NAMESPACE_LEN + NAME_LEN + 1];
    snprintf(full, sizeof(full), "%s:%s", namespace, name);

    for (int i = 0; i < g_block_count; i++) {
        if (strcmp(g_blocks[i].full_name, full) == 0) {
            TraceLog(LOG_WARNING, "Block already registered: %s", full);
            return g_blocks[i].runtime_id;
        }
    }

    if (g_block_count >= MAX_BLOCK_TYPES) {
        TraceLog(LOG_ERROR, "Block registry full!");
        return 0;
    }

    uint16_t id = (uint16_t)g_block_count++;
    BlockDef *def = &g_blocks[id];
    def->runtime_id = id;
    strncpy(def->namespace, namespace, NAMESPACE_LEN - 1);
    strncpy(def->name,      name,      NAME_LEN - 1);
    strncpy(def->full_name, full,      sizeof(def->full_name) - 1);

    TraceLog(LOG_INFO, "Registered block [%d] %s", id, full);
    return id;
}

BlockDef *block_get_by_id(uint16_t id) {
    if (id >= g_block_count) return NULL;
    return &g_blocks[id];
}

BlockDef *block_get_by_name(const char *full_name) {
    for (int i = 0; i < g_block_count; i++)
        if (strcmp(g_blocks[i].full_name, full_name) == 0)
            return &g_blocks[i];
    return NULL;
}

uint16_t block_get_id(const char *full_name) {
    BlockDef *def = block_get_by_name(full_name);
    return def ? def->runtime_id : 0; // 0 = AIR als Fallback
}

