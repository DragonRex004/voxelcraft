//
// Created by drago on 01.06.2026.
//

#ifndef VOXELCRAFT_BLOCK_H
#define VOXELCRAFT_BLOCK_H

#include "raylib.h"

typedef enum {
    BLOCK_AIR   = 0,
    BLOCK_GRASS = 1,
    BLOCK_DIRT  = 2,
    BLOCK_STONE = 3,
    BLOCK_COUNT
} BlockType;

typedef struct { Color top, side, bottom; } BlockColors;

static BlockColors BLOCK_PALETTE[BLOCK_COUNT] = {
    { {0,0,0,0},           {0,0,0,0},           {0,0,0,0}           }, // AIR
    { {106,145,80,255},    {86,125,70,255},      {121,85,58,255}     }, // GRASS
    { {131,95,68,255},     {121,85,58,255},      {111,75,48,255}     }, // DIRT
    { {148,148,148,255},   {128,128,128,255},    {108,108,108,255}   }, // STONE
};

#endif //VOXELCRAFT_BLOCK_H
