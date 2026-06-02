//
// Created by drago on 02.06.2026.
//

#ifndef VOXELCRAFT_ATLAS_H
#define VOXELCRAFT_ATLAS_H

#include "raylib.h"
#include "block.h"

#define ATLAS_TILE_SIZE  16
#define ATLAS_TILES_ROW  16
#define ATLAS_SIZE       (ATLAS_TILE_SIZE * ATLAS_TILES_ROW)

typedef struct {
    char      full_key[128];
    Rectangle uv;
    int       tile_index;
} AtlasEntry;

#define MAX_ATLAS_ENTRIES 256

extern Texture2D    g_atlas_texture;
extern AtlasEntry   g_atlas_entries[MAX_ATLAS_ENTRIES];
extern int          g_atlas_entry_count;

void atlas_build(void);
Rectangle atlas_get_uv(const char *full_key);
#endif //VOXELCRAFT_ATLAS_H
