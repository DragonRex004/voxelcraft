//
// Created by drago on 02.06.2026.
//

#include "chunk.h"

#include "atlas.h"
#include "block.h"

float fade(float t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

float lerp_f(float a, float b, float t) {
    return a + t * (b - a);
}

float grad(int hash, float x, float y) {
    int h = hash & 7;
    float u = h < 4 ? x : y, v = h < 4 ? y : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

void noise_init(unsigned seed) {
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

float noise2d(float x, float y) {
    int X = (int)floorf(x) & 255, Y = (int)floorf(y) & 255;
    x -= floorf(x); y -= floorf(y);
    float u = fade(x), v = fade(y);
    int a = perm[X]+Y, b = perm[X+1]+Y;
    return lerp_f(lerp_f(grad(perm[a],   x,   y),   grad(perm[b],   x-1, y),   u),
                  lerp_f(grad(perm[a+1], x,   y-1), grad(perm[b+1], x-1, y-1), u), v);
}

float octave_noise(float x, float y, int oct) {
    float val=0, amp=1, freq=1, max=0;
    for (int i=0; i<oct; i++) {
        val += noise2d(x*freq, y*freq) * amp;
        max += amp; amp *= 0.5f; freq *= 2.0f;
    }
    return val / max;
}

void chunk_save(Chunk *c, FILE *f) {
    for (int x=0; x<CHUNK_W; x++)
        for (int y=0; y<CHUNK_H; y++)
            for (int z=0; z<CHUNK_D; z++) {
                uint16_t id = c->blocks[x][y][z];
                BlockDef *def = block_get_by_id(id);
                fprintf(f, "%s\n", def ? def->full_name : "base:air");
            }
}

void chunk_load(Chunk *c, FILE *f) {
    char full_name[128];
    for (int x=0; x<CHUNK_W; x++)
        for (int y=0; y<CHUNK_H; y++)
            for (int z=0; z<CHUNK_D; z++) {
                fscanf(f, "%127s", full_name);
                c->blocks[x][y][z] = block_get_id(full_name);
            }
}

void chunk_generate(Chunk *c) {
    noise_init(12345);

    uint16_t ID_AIR   = block_get_id("base:air");
    uint16_t ID_GRASS = block_get_id("base:grass");
    uint16_t ID_DIRT  = block_get_id("base:dirt");
    uint16_t ID_STONE = block_get_id("base:stone");

    for (int x = 0; x < CHUNK_W; x++) {
        for (int z = 0; z < CHUNK_D; z++) {
            float wx = (float)(c->cx * CHUNK_W + x);
            float wz = (float)(c->cz * CHUNK_D + z);

            float n = octave_noise(wx * 0.03f, wz * 0.03f, 4);
            int height = 12 + (int)(n * 14.0f);

            for (int y = 0; y < CHUNK_H; y++) {
                if      (y > height)    c->blocks[x][y][z] = ID_AIR;
                else if (y == height)   c->blocks[x][y][z] = ID_GRASS;
                else if (y >= height-3) c->blocks[x][y][z] = ID_DIRT;
                else                    c->blocks[x][y][z] = ID_STONE;
            }
        }
    }

    c->mesh_dirty = true;
}

bool is_solid(const Chunk *c, int x, int y, int z) {
    if (y < 0 || y >= CHUNK_H) return false;

    if (x < 0) {
        if (!c->neighbor_nx) return false;
        return c->neighbor_nx->blocks[CHUNK_W + x][y][z] != 0;
    }
    if (x >= CHUNK_W) {
        if (!c->neighbor_px) return false;
        return c->neighbor_px->blocks[x - CHUNK_W][y][z] != 0;
    }

    if (z < 0) {
        if (!c->neighbor_nz) return false;
        return c->neighbor_nz->blocks[x][y][CHUNK_D + z] != 0;
    }
    if (z >= CHUNK_D) {
        if (!c->neighbor_pz) return false;
        return c->neighbor_pz->blocks[x][y][z - CHUNK_D] != 0;
    }

    return c->blocks[x][y][z] != 0;
}

void push_quad_uv(
    Vector3 v0, Vector3 v1, Vector3 v2, Vector3 v3,
    Vector3 normal,
    Rectangle atlas_uv)
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

    float u0 = atlas_uv.x;
    float v0_ = atlas_uv.y;
    float u1 = atlas_uv.x + atlas_uv.width;
    float v1_ = atlas_uv.y + atlas_uv.height;

    float *tp = tmp_texcoords + base * 2;

    tp[0] = u0;  tp[1] = v0_;
    tp[2] = u1;  tp[3] = v0_;
    tp[4] = u1;  tp[5] = v1_;
    tp[6] = u0;  tp[7] = v1_;

    unsigned short *ip = tmp_indices + idx_count;
    ip[0]=(unsigned short)(base+0);
    ip[1]=(unsigned short)(base+1);
    ip[2]=(unsigned short)(base+2);
    ip[3]=(unsigned short)(base+0);
    ip[4]=(unsigned short)(base+2);
    ip[5]=(unsigned short)(base+3);

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
                uint16_t bid = c->blocks[x][y][z];
                if (bid == 0) continue;

                BlockDef *def = block_get_by_id(bid);
                if (!def) continue;

                float fx = ox + x, fy = (float)y, fz = oz + z;

                Rectangle uv_top    = def->tex_top;
                Rectangle uv_side   = def->tex_side;
                Rectangle uv_bottom = def->tex_bottom;

                // +Y up
                if (!is_solid(c, x, y+1, z))
                    push_quad_uv(
                        (Vector3){fx,   fy+1, fz  },
                        (Vector3){fx+1, fy+1, fz  },
                        (Vector3){fx+1, fy+1, fz+1},
                        (Vector3){fx,   fy+1, fz+1},
                        (Vector3){0, 1, 0}, uv_top);

                // -Y down
                if (!is_solid(c, x, y-1, z))
                    push_quad_uv(
                        (Vector3){fx,   fy, fz+1},
                        (Vector3){fx+1, fy, fz+1},
                        (Vector3){fx+1, fy, fz  },
                        (Vector3){fx,   fy, fz  },
                        (Vector3){0, -1, 0}, uv_bottom);

                // +X right
                if (!is_solid(c, x+1, y, z))
                    push_quad_uv(
                        (Vector3){fx+1, fy,   fz  },
                        (Vector3){fx+1, fy,   fz+1},
                        (Vector3){fx+1, fy+1, fz+1},
                        (Vector3){fx+1, fy+1, fz  },
                        (Vector3){1, 0, 0}, uv_side);

                // -X left
                if (!is_solid(c, x-1, y, z))
                    push_quad_uv(
                        (Vector3){fx, fy,   fz+1},
                        (Vector3){fx, fy,   fz  },
                        (Vector3){fx, fy+1, fz  },
                        (Vector3){fx, fy+1, fz+1},
                        (Vector3){-1, 0, 0}, uv_side);

                // +Z front
                if (!is_solid(c, x, y, z+1))
                    push_quad_uv(
                        (Vector3){fx+1, fy,   fz+1},
                        (Vector3){fx,   fy,   fz+1},
                        (Vector3){fx,   fy+1, fz+1},
                        (Vector3){fx+1, fy+1, fz+1},
                        (Vector3){0, 0, 1}, uv_side);

                // -Z back
                if (!is_solid(c, x, y, z-1))
                    push_quad_uv(
                        (Vector3){fx,   fy,   fz},
                        (Vector3){fx+1, fy,   fz},
                        (Vector3){fx+1, fy+1, fz},
                        (Vector3){fx,   fy+1, fz},
                        (Vector3){0, 0, -1}, uv_side);
            }
        }
    }

    if (c->mesh.vertexCount > 0) {
        UnloadModel(c->model);
        c->mesh.vertexCount = 0;
    }

    if (vert_count == 0) return;

    Mesh m = {0};
    m.vertexCount   = vert_count;
    m.triangleCount = idx_count / 3;

    m.vertices  = (float*)MemAlloc(vert_count * 3 * sizeof(float));
    m.normals   = (float*)MemAlloc(vert_count * 3 * sizeof(float));
    m.texcoords = (float*)MemAlloc(vert_count * 2 * sizeof(float)); // NEU
    m.indices   = (unsigned short*)MemAlloc(idx_count * sizeof(unsigned short));

    memcpy(m.vertices,   tmp_verts,     vert_count * 3 * sizeof(float));
    memcpy(m.normals,    tmp_normals,   vert_count * 3 * sizeof(float));
    memcpy(m.texcoords,  tmp_texcoords, vert_count * 2 * sizeof(float));
    memcpy(m.indices,    tmp_indices,   idx_count  * sizeof(unsigned short));

    UploadMesh(&m, false);
    c->mesh  = m;
    c->model = LoadModelFromMesh(m);

    c->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = g_atlas_texture;

    c->mesh_dirty = false;
}

void chunk_draw(const Chunk *c) {
    if (c->mesh.vertexCount == 0) return;
    DrawModel(c->model, Vector3Zero(), 1.0f, WHITE);
}