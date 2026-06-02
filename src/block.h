//
// Created by drago on 01.06.2026.
//

#ifndef VOXELCRAFT_BLOCK_H
#define VOXELCRAFT_BLOCK_H

#include <stdint.h>
#include <stdbool.h>
#include "raylib.h"

#define MAX_BLOCK_TYPES 4096
#define NAMESPACE_LEN   64
#define NAME_LEN        64

typedef struct {
    uint16_t  runtime_id;
    char      namespace[64];
    char      name[64];
    char      full_name[129];

    bool      solid;
    bool      transparent;
    uint8_t   light_emission;

    char      tex_top_key[128];
    char      tex_side_key[128];
    char      tex_bottom_key[128];
    
    Rectangle tex_top;
    Rectangle tex_side;
    Rectangle tex_bottom;
} BlockDef;

extern BlockDef  g_blocks[MAX_BLOCK_TYPES];
extern int       g_block_count;

uint16_t block_register(const char *namespace, const char *name);
BlockDef *block_get_by_id(uint16_t id);
BlockDef *block_get_by_name(const char *full_name);
uint16_t block_get_id(const char *full_name);

#endif //VOXELCRAFT_BLOCK_H
