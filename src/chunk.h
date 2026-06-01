//
// Created by drago on 01.06.2026.
//

#ifndef VOXELCRAFT_CHUNK_H
#define VOXELCRAFT_CHUNK_H

#include <stdint.h>
#include "raylib.h"
#include "raymath.h"
#include <math.h>
#include "block.h"
#include <string.h>

#define CHUNK_W 16
#define CHUNK_H 64
#define CHUNK_D 16

#define MAX_VERTS (CHUNK_W * CHUNK_H * CHUNK_D * 6 * 4)

typedef struct {
    uint8_t blocks[CHUNK_W][CHUNK_H][CHUNK_D];
    int cx, cz;

    Mesh mesh;
    Model model;
    bool mesh_dirty;
} Chunk;

static float fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }
static float lerp_f(float a, float b, float t) { return a + t * (b - a); }
static float grad(int hash, float x, float y) {
    int h = hash & 7;
    float u = h < 4 ? x : y, v = h < 4 ? y : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}
static int perm[512];
static bool perm_init = false;
static void noise_init(unsigned seed) {
    if (perm_init) return;
    perm_init = true;
    for (int i = 0; i < 256; i++) perm[i] = i;
    // Fisher-Yates
    unsigned s = seed;
    for (int i = 255; i > 0; i--) {
        s = s * 1664525u + 1013904223u;
        int j = s % (i + 1);
        int tmp = perm[i]; perm[i] = perm[j]; perm[j] = tmp;
    }
    for (int i = 0; i < 256; i++) perm[i + 256] = perm[i];
}
static float noise2d(float x, float y) {
    int X = (int)floorf(x) & 255, Y = (int)floorf(y) & 255;
    x -= floorf(x); y -= floorf(y);
    float u = fade(x), v = fade(y);
    int a = perm[X]+Y, b = perm[X+1]+Y;
    return lerp_f(lerp_f(grad(perm[a],   x,   y),   grad(perm[b],   x-1, y),   u),
                  lerp_f(grad(perm[a+1], x,   y-1), grad(perm[b+1], x-1, y-1), u), v);
}
static float octave_noise(float x, float y, int oct) {
    float val=0, amp=1, freq=1, max=0;
    for (int i=0; i<oct; i++) {
        val += noise2d(x*freq, y*freq) * amp;
        max += amp; amp *= 0.5f; freq *= 2.0f;
    }
    return val / max;
}

void chunk_generate(Chunk *c) {
    noise_init(12345);
    for (int x = 0; x < CHUNK_W; x++) {
        for (int z = 0; z < CHUNK_D; z++) {
            float wx = (float)(c->cx * CHUNK_W + x);
            float wz = (float)(c->cz * CHUNK_D + z);
            float n = octave_noise(wx * 0.03f, wz * 0.03f, 4);
            int height = 12 + (int)(n * 14.0f);

            for (int y = 0; y < CHUNK_H; y++) {
                if      (y > height)    c->blocks[x][y][z] = BLOCK_AIR;
                else if (y == height)   c->blocks[x][y][z] = BLOCK_GRASS;
                else if (y >= height-3) c->blocks[x][y][z] = BLOCK_DIRT;
                else                    c->blocks[x][y][z] = BLOCK_STONE;
            }
        }
    }
    c->mesh_dirty = true;
}

static inline bool is_solid(const Chunk *c, int x, int y, int z) {
    if (x < 0 || x >= CHUNK_W) return false;
    if (y < 0 || y >= CHUNK_H) return false;
    if (z < 0 || z >= CHUNK_D) return false;
    return c->blocks[x][y][z] != BLOCK_AIR;
}

static float   tmp_verts  [MAX_VERTS * 3];
static float   tmp_normals[MAX_VERTS * 3];
static uint8_t tmp_colors [MAX_VERTS * 4];
static unsigned short tmp_indices[MAX_VERTS / 4 * 6];

static int vert_count = 0;
static int idx_count  = 0;

static void push_quad(
    Vector3 v0, Vector3 v1, Vector3 v2, Vector3 v3,
    Vector3 normal, Color col)
{
    int base = vert_count;
    float *vp = tmp_verts + base * 3;
    vp[0]=v0.x; vp[1]=v0.y; vp[2]=v0.z;
    vp[3]=v1.x; vp[4]=v1.y; vp[5]=v1.z;
    vp[6]=v2.x; vp[7]=v2.y; vp[8]=v2.z;
    vp[9]=v3.x; vp[10]=v3.y; vp[11]=v3.z;

    float *np = tmp_normals + base * 3;
    for (int i = 0; i < 4; i++) {
        np[i*3+0] = normal.x;
        np[i*3+1] = normal.y;
        np[i*3+2] = normal.z;
    }

    uint8_t *cp = tmp_colors + base * 4;
    for (int i = 0; i < 4; i++) {
        cp[i*4+0] = col.r;
        cp[i*4+1] = col.g;
        cp[i*4+2] = col.b;
        cp[i*4+3] = col.a;
    }

    unsigned short *ip = tmp_indices + idx_count;
    ip[0]=(unsigned short)(base+0);
    ip[1]=(unsigned short)(base+2);
    ip[2]=(unsigned short)(base+1);
    ip[3]=(unsigned short)(base+0);
    ip[4]=(unsigned short)(base+3);
    ip[5]=(unsigned short)(base+2);

    vert_count += 4;
    idx_count  += 6;
}

void chunk_build_mesh(Chunk *c) {
    vert_count = 0;
    idx_count  = 0;

    float ox = (float)(c->cx * CHUNK_W);
    float oz = (float)(c->cz * CHUNK_D);

    for (int x = 0; x < CHUNK_W; x++) {
        for (int y = 0; y < CHUNK_H; y++) {
            for (int z = 0; z < CHUNK_D; z++) {
                uint8_t b = c->blocks[x][y][z];
                if (b == BLOCK_AIR) continue;

                BlockColors *pal = &BLOCK_PALETTE[b];
                float fx = ox + x, fy = y, fz = oz + z;

                // +Y Up
                if (!is_solid(c, x, y+1, z))
                    push_quad(
                        (Vector3){fx,   fy+1, fz  },
                        (Vector3){fx+1, fy+1, fz  },
                        (Vector3){fx+1, fy+1, fz+1},
                        (Vector3){fx,   fy+1, fz+1},
                        (Vector3){0, 1, 0}, pal->top);

                // -Y Down
                if (!is_solid(c, x, y-1, z))
                    push_quad(
                        (Vector3){fx,   fy, fz+1},
                        (Vector3){fx+1, fy, fz+1},
                        (Vector3){fx+1, fy, fz  },
                        (Vector3){fx,   fy, fz  },
                        (Vector3){0, -1, 0}, pal->bottom);

                // +X Right
                if (!is_solid(c, x+1, y, z))
                    push_quad(
                        (Vector3){fx+1, fy,   fz  },
                        (Vector3){fx+1, fy,   fz+1},
                        (Vector3){fx+1, fy+1, fz+1},
                        (Vector3){fx+1, fy+1, fz  },
                        (Vector3){1, 0, 0}, pal->side);

                // -X Left
                if (!is_solid(c, x-1, y, z))
                    push_quad(
                        (Vector3){fx, fy,   fz+1},
                        (Vector3){fx, fy,   fz  },
                        (Vector3){fx, fy+1, fz  },
                        (Vector3){fx, fy+1, fz+1},
                        (Vector3){-1, 0, 0}, pal->side);

                // +Z Front
                if (!is_solid(c, x, y, z+1))
                    push_quad(
                        (Vector3){fx+1, fy,   fz+1},
                        (Vector3){fx,   fy,   fz+1},
                        (Vector3){fx,   fy+1, fz+1},
                        (Vector3){fx+1, fy+1, fz+1},
                        (Vector3){0, 0, 1}, pal->side);

                // -Z Back
                if (!is_solid(c, x, y, z-1))
                    push_quad(
                        (Vector3){fx,   fy,   fz},
                        (Vector3){fx+1, fy,   fz},
                        (Vector3){fx+1, fy+1, fz},
                        (Vector3){fx,   fy+1, fz},
                        (Vector3){0, 0, -1}, pal->side);
            }
        }
    }


    if (c->mesh.vertexCount > 0) {
        UnloadModel(c->model);
        c->mesh.vertexCount = 0;
    }

    Mesh m = {0};
    m.vertexCount   = vert_count;
    m.triangleCount = idx_count / 3;

    m.vertices = (float*)MemAlloc(vert_count * 3 * sizeof(float));
    m.normals  = (float*)MemAlloc(vert_count * 3 * sizeof(float));
    m.colors   = (unsigned char*)MemAlloc(vert_count * 4 * sizeof(unsigned char));
    m.indices  = (unsigned short*)MemAlloc(idx_count * sizeof(unsigned short));

    memcpy(m.vertices, tmp_verts,   vert_count * 3 * sizeof(float));
    memcpy(m.normals,  tmp_normals, vert_count * 3 * sizeof(float));
    memcpy(m.colors,   tmp_colors,  vert_count * 4 * sizeof(unsigned char));
    memcpy(m.indices,  tmp_indices, idx_count  * sizeof(unsigned short));

    UploadMesh(&m, false);
    c->mesh  = m;
    c->model = LoadModelFromMesh(m);
    c->mesh_dirty = false;
}

void chunk_draw(const Chunk *c) {
    if (c->mesh.vertexCount == 0) return;
    DrawModel(c->model, Vector3Zero(), 1.0f, WHITE);
}

#endif //VOXELCRAFT_CHUNK_H
