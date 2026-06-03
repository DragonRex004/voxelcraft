//
// Created by drago on 01.06.2026.
//

#ifndef VOXELCRAFT_CHUNK_H
#define VOXELCRAFT_CHUNK_H

#include <stdint.h>
#include "raylib.h"
#include "raymath.h"
#include <string.h>
#include <stdio.h>

#define CHUNK_W 16
#define CHUNK_H 64
#define CHUNK_D 16
#define MAX_VERTS (CHUNK_W * CHUNK_H * CHUNK_D * 6 * 4)
#define MAX_INDICES (CHUNK_W * CHUNK_H * CHUNK_D * 6 * 2 * 3)

typedef struct Chunk Chunk;

struct Chunk {
    uint8_t blocks[CHUNK_W][CHUNK_H][CHUNK_D];
    int cx, cz;

    Mesh mesh;
    Model model;
    bool mesh_dirty;

    Chunk *neighbor_px; // +X
    Chunk *neighbor_nx; // -X
    Chunk *neighbor_pz; // +Z
    Chunk *neighbor_nz; // -Z
};

float fade(float t);
float lerp_f(float a, float b, float t);
float grad(int hash, float x, float y);
static int perm[512];
static bool perm_init = false;
void noise_init(unsigned seed);
float noise2d(float x, float y);
float octave_noise(float x, float y, int oct);

void chunk_save(Chunk *c, FILE *f);
void chunk_load(Chunk *c, FILE *f);

void chunk_generate(Chunk *c);

static bool is_solid(const Chunk *c, int x, int y, int z);

static float   tmp_verts  [MAX_VERTS * 3];
static float   tmp_normals[MAX_VERTS * 3];
static float   tmp_texcoords[MAX_VERTS * 2];
static unsigned short tmp_indices[MAX_VERTS / 4 * 6];

static int vert_count = 0;
static int idx_count  = 0;

typedef struct { float u, v; } UV;

static void push_quad(
Vector3 v0, Vector3 v1, Vector3 v2, Vector3 v3,
Vector3 normal,
Rectangle atlas_uv);

void chunk_build_mesh(Chunk *c);

void chunk_draw(const Chunk *c);

#endif //VOXELCRAFT_CHUNK_H
