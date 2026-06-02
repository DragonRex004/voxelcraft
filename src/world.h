//
// Created by drago on 01.06.2026.
//

#ifndef VOXELCRAFT_WORLD_H
#define VOXELCRAFT_WORLD_H

#define WORLD_W 5
#define WORLD_D 5

void world_link_neighbors(Chunk world[][WORLD_D], int ww, int wd) {
    for (int cx = 0; cx < ww; cx++) {
        for (int cz = 0; cz < wd; cz++) {
            Chunk *c = &world[cx][cz];

            c->neighbor_px = (cx + 1 < ww)  ? &world[cx+1][cz] : NULL;
            c->neighbor_nx = (cx - 1 >= 0)   ? &world[cx-1][cz] : NULL;
            c->neighbor_pz = (cz + 1 < wd)  ? &world[cx][cz+1] : NULL;
            c->neighbor_nz = (cz - 1 >= 0)   ? &world[cx][cz-1] : NULL;
        }
    }
}

void world_set_block(Chunk world[][WORLD_D], int ww, int wd,
                     int wx, int wy, int wz, uint8_t type)
{
    if (wy < 0 || wy >= CHUNK_H) return;
    int cx = wx / CHUNK_W, cz = wz / CHUNK_D;
    int lx = wx % CHUNK_W, lz = wz % CHUNK_D;
    if (cx < 0 || cx >= ww || cz < 0 || cz >= wd) return;

    world[cx][cz].blocks[lx][wy][lz] = type;
    chunk_build_mesh(&world[cx][cz]);

    // Nachbar-Chunk neu bauen wenn Block an der Grenze liegt
    if (lx == 0          && world[cx][cz].neighbor_nx)
        chunk_build_mesh(world[cx][cz].neighbor_nx);
    if (lx == CHUNK_W-1  && world[cx][cz].neighbor_px)
        chunk_build_mesh(world[cx][cz].neighbor_px);
    if (lz == 0          && world[cx][cz].neighbor_nz)
        chunk_build_mesh(world[cx][cz].neighbor_nz);
    if (lz == CHUNK_D-1  && world[cx][cz].neighbor_pz)
        chunk_build_mesh(world[cx][cz].neighbor_pz);
}

#endif //VOXELCRAFT_WORLD_H
