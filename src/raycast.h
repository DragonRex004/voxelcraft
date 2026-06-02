//
// Created by drago on 01.06.2026.
//

#ifndef VOXELCRAFT_RAYCAST_H
#define VOXELCRAFT_RAYCAST_H

#include "raylib.h"
#include "chunk.h"
#include "world.h"

typedef struct {
    bool  hit;
    int   bx, by, bz;
    int   nx, ny, nz;
} RaycastResult;

RaycastResult world_raycast(Chunk world[][WORLD_D], int ww, int wd,
                             Vector3 origin, Vector3 dir, float max_dist)
{
    RaycastResult res = {0};

    float len = sqrtf(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
    if (len < 0.0001f) return res;
    dir.x /= len; dir.y /= len; dir.z /= len;

    int bx = (int)floorf(origin.x);
    int by = (int)floorf(origin.y);
    int bz = (int)floorf(origin.z);

    int sx = dir.x > 0 ? 1 : -1;
    int sy = dir.y > 0 ? 1 : -1;
    int sz = dir.z > 0 ? 1 : -1;

    float tx = (dir.x != 0) ? fabsf(((dir.x > 0 ? bx+1 : bx) - origin.x) / dir.x) : 1e30f;
    float ty = (dir.y != 0) ? fabsf(((dir.y > 0 ? by+1 : by) - origin.y) / dir.y) : 1e30f;
    float tz = (dir.z != 0) ? fabsf(((dir.z > 0 ? bz+1 : bz) - origin.z) / dir.z) : 1e30f;

    float dtx = (dir.x != 0) ? fabsf(1.0f / dir.x) : 1e30f;
    float dty = (dir.y != 0) ? fabsf(1.0f / dir.y) : 1e30f;
    float dtz = (dir.z != 0) ? fabsf(1.0f / dir.z) : 1e30f;

    int last_nx=0, last_ny=0, last_nz=0;

    float t = 0;
    while (t < max_dist) {
        int cx = bx / CHUNK_W; int cz_idx = bz / CHUNK_D;
        int lx = bx % CHUNK_W; int lz = bz % CHUNK_D;

        if (cx >= 0 && cx < ww && cz_idx >= 0 && cz_idx < wd &&
            by >= 0 && by < CHUNK_H && lx >= 0 && lz >= 0)
        {
            if (world[cx][cz_idx].blocks[lx][by][lz] != BLOCK_AIR) {
                res.hit = true;
                res.bx = bx; res.by = by; res.bz = bz;
                res.nx = bx + last_nx;
                res.ny = by + last_ny;
                res.nz = bz + last_nz;
                return res;
            }
        }

        if (tx < ty && tx < tz) {
            bx += sx; t = tx; tx += dtx;
            last_nx = -sx; last_ny = 0; last_nz = 0;
        } else if (ty < tz) {
            by += sy; t = ty; ty += dty;
            last_nx = 0; last_ny = -sy; last_nz = 0;
        } else {
            bz += sz; t = tz; tz += dtz;
            last_nx = 0; last_ny = 0; last_nz = -sz;
        }
    }
    return res;
}

Vector3 camera_forward(Camera3D *cam) {
    Vector3 dir = Vector3Subtract(cam->target, cam->position);
    float len = sqrtf(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
    return (Vector3){dir.x/len, dir.y/len, dir.z/len};
}

#endif //VOXELCRAFT_RAYCAST_H
