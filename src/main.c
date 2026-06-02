#include "raylib.h"
#include "chunk.h"
#include "raycast.h"
#include "world.h"

uint8_t selected_block = BLOCK_GRASS;

int main(void) {
    InitWindow(1280, 720, "VoxelCraft — ALPHA");
    SetTargetFPS(60);
    DisableCursor();

    Camera3D cam = {
        .position   = { 8.0f, 30.0f, -10.0f },
        .target     = { 24.0f, 15.0f, 24.0f },
        .up         = { 0.0f, 1.0f, 0.0f },
        .fovy       = 70.0f,
        .projection = CAMERA_PERSPECTIVE,
    };

    Chunk world[WORLD_W][WORLD_D];

    for (int cx = 0; cx < WORLD_W; cx++)
        for (int cz = 0; cz < WORLD_D; cz++) {
            world[cx][cz] = (Chunk){0};
            world[cx][cz].cx = cx;
            world[cx][cz].cz = cz;
            chunk_generate(&world[cx][cz]);
        }

    world_link_neighbors(world, WORLD_W, WORLD_D);

    for (int cx = 0; cx < WORLD_W; cx++)
        for (int cz = 0; cz < WORLD_D; cz++)
            chunk_build_mesh(&world[cx][cz]);

    while (!WindowShouldClose()) {
        UpdateCamera(&cam, CAMERA_FREE);

        Vector3 fwd = camera_forward(&cam);
        RaycastResult rc = world_raycast(world, WORLD_W, WORLD_D,
                                         cam.position, fwd, 8.0f);

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && rc.hit) {
            world_set_block(world, WORLD_W, WORLD_D,
                            rc.bx, rc.by, rc.bz, BLOCK_AIR);
        }

        if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && rc.hit) {
            world_set_block(world, WORLD_W, WORLD_D,
                            rc.nx, rc.ny, rc.nz, selected_block);
        }

        if (IsKeyPressed(KEY_ONE))   selected_block = BLOCK_GRASS;
        if (IsKeyPressed(KEY_TWO))   selected_block = BLOCK_DIRT;
        if (IsKeyPressed(KEY_THREE)) selected_block = BLOCK_STONE;


        BeginDrawing();
        ClearBackground((Color){135, 206, 235, 255});

        BeginMode3D(cam);
        for (int cx = 0; cx < WORLD_W; cx++)
            for (int cz = 0; cz < WORLD_D; cz++)
                chunk_draw(&world[cx][cz]);


        if (rc.hit) {
            DrawCubeWires(
                (Vector3){rc.bx + 0.5f, rc.by + 0.5f, rc.bz + 0.5f},
                1.01f, 1.01f, 1.01f,
                (Color){255, 255, 255, 200});
        }

        EndMode3D();

        int cx2 = GetScreenWidth()  / 2;
        int cy2 = GetScreenHeight() / 2;
        DrawLine(cx2-10, cy2, cx2+10, cy2, WHITE);
        DrawLine(cx2, cy2-10, cx2, cy2+10, WHITE);

        DrawFPS(10, 10);
        int total_tris = 0;
        for (int x=0;x<WORLD_W;x++)
            for (int z=0;z<WORLD_D;z++)
                total_tris += world[x][z].mesh.triangleCount;
        DrawText(TextFormat("Triangle: %d", total_tris), 10, 35, 18, WHITE);
        DrawText(TextFormat("Block: %s",
            selected_block==BLOCK_GRASS ? "Grass" :
            selected_block==BLOCK_DIRT  ? "Dirt"  : "Stone"), 10, 58, 18, WHITE);
        DrawText("LMB=Break  RMB=Place  1/2/3=BlockType  Q/E=Up/Down", 10, 81, 18, WHITE);
        EndDrawing();
    }

    for (int cx=0;cx<WORLD_W;cx++)
        for (int cz=0;cz<WORLD_D;cz++)
            if (world[cx][cz].mesh.vertexCount > 0)
                UnloadModel(world[cx][cz].model);

    CloseWindow();
    return 0;
}