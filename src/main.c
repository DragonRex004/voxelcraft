#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "block.h"
#include "chunk.h"
#include "world.h"
#include "atlas.h"
#include "raycast.h"
#include "block_loader.h"
#include <stdio.h>

Shader g_chunk_shader = {0};

#define WORLD_W 5
#define WORLD_D 5

static Chunk world[WORLD_W][WORLD_D];

int main(void) {
    InitWindow(1280, 720, "VoxelCraft");
    SetTargetFPS(60);
    DisableCursor();

    load_all_packs();

    atlas_build();

    Image dbg_atlas = LoadImageFromTexture(g_atlas_texture);
    ExportImage(dbg_atlas, "atlas_debug.png");
    UnloadImage(dbg_atlas);


    g_chunk_shader = LoadShaderFromMemory(
        // Vertex Shader
        "#version 330\n"
        "in vec3 vertexPosition;\n"
        "in vec2 vertexTexCoord;\n"
        "in vec3 vertexNormal;\n"
        "uniform mat4 mvp;\n"
        "out vec2 fragTexCoord;\n"
        "out vec3 fragNormal;\n"
        "void main() {\n"
        "    fragTexCoord = vertexTexCoord;\n"
        "    fragNormal   = vertexNormal;\n"
        "    gl_Position  = mvp * vec4(vertexPosition, 1.0);\n"
        "}\n",

        // Fragment Shader
        "#version 330\n"
        "in vec2 fragTexCoord;\n"
        "in vec3 fragNormal;\n"
        "uniform sampler2D texture0;\n"
        "out vec4 finalColor;\n"
        "void main() {\n"
        "    vec3 lightDir = normalize(vec3(0.6, 1.0, 0.4));\n"
        "    float diff    = max(dot(fragNormal, lightDir), 0.0);\n"
        "    float ambient = 0.4;\n"
        "    float light   = ambient + diff * 0.6;\n"
        "    vec4 texColor = texture(texture0, fragTexCoord);\n"
        "    if (texColor.a < 0.1) discard;\n"
        "    finalColor    = vec4(texColor.rgb * light, texColor.a);\n"
        "}\n"
    );

    for (int cx = 0; cx < WORLD_W; cx++) {
        for (int cz = 0; cz < WORLD_D; cz++) {
            world[cx][cz] = (Chunk){0};
            world[cx][cz].cx = cx;
            world[cx][cz].cz = cz;
            chunk_generate(&world[cx][cz]);
        }
    }

    world_link_neighbors(world, WORLD_W, WORLD_D);

    for (int cx = 0; cx < WORLD_W; cx++)
        for (int cz = 0; cz < WORLD_D; cz++)
            chunk_build_mesh(&world[cx][cz]);

    Camera3D cam = {
        .position   = { (float)(WORLD_W * CHUNK_W) / 2.0f, 40.0f,
                        (float)(WORLD_D * CHUNK_D) / 2.0f - 10.0f },
        .target     = { (float)(WORLD_W * CHUNK_W) / 2.0f, 20.0f,
                        (float)(WORLD_D * CHUNK_D) / 2.0f },
        .up         = { 0.0f, 1.0f, 0.0f },
        .fovy       = 70.0f,
        .projection = CAMERA_PERSPECTIVE,
    };

    uint16_t selected_block = block_get_id("base:grass");

    // Block-Namen for HUD
    const char *block_names[] = { "Grass", "Dirt", "Stone" };
    uint16_t    block_ids[3];
    block_ids[0] = block_get_id("base:grass");
    block_ids[1] = block_get_id("base:dirt");
    block_ids[2] = block_get_id("base:stone");
    int selected_idx = 0;

    while (!WindowShouldClose()) {

        UpdateCamera(&cam, CAMERA_FREE);

        if (IsKeyPressed(KEY_ONE))   { selected_idx = 0; selected_block = block_ids[0]; }
        if (IsKeyPressed(KEY_TWO))   { selected_idx = 1; selected_block = block_ids[1]; }
        if (IsKeyPressed(KEY_THREE)) { selected_idx = 2; selected_block = block_ids[2]; }

        Vector3 fwd = camera_forward(&cam);
        RaycastResult rc = world_raycast(
            world, WORLD_W, WORLD_D,
            cam.position, fwd, 8.0f
        );

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && rc.hit) {
            world_set_block(world, WORLD_W, WORLD_D,
                            rc.bx, rc.by, rc.bz,
                            block_get_id("base:air"));
        }

        if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && rc.hit) {
            world_set_block(world, WORLD_W, WORLD_D,
                            rc.nx, rc.ny, rc.nz,
                            selected_block);
        }

        BeginDrawing();
            ClearBackground((Color){135, 206, 235, 255});

            BeginMode3D(cam);

                for (int cx = 0; cx < WORLD_W; cx++)
                    for (int cz = 0; cz < WORLD_D; cz++)
                        chunk_draw(&world[cx][cz]);

                if (rc.hit) {
                    DrawCubeWires(
                        (Vector3){rc.bx + 0.5f, rc.by + 0.5f, rc.bz + 0.5f},
                        1.02f, 1.02f, 1.02f,
                        (Color){0, 0, 0, 180}
                    );
                }

            EndMode3D();

            int sw = GetScreenWidth()  / 2;
            int sh = GetScreenHeight() / 2;
            DrawLine(sw - 12, sh,      sw + 12, sh,      WHITE);
            DrawLine(sw,      sh - 12, sw,      sh + 12, WHITE);
            DrawLine(sw - 12, sh,      sw + 12, sh,      (Color){0,0,0,120});
            DrawLine(sw,      sh - 12, sw,      sh + 12, (Color){0,0,0,120});

            DrawFPS(10, 10);

            int total_tris = 0;
            for (int cx = 0; cx < WORLD_W; cx++)
                for (int cz = 0; cz < WORLD_D; cz++)
                    total_tris += world[cx][cz].mesh.triangleCount;
            DrawText(TextFormat("Triangles: %d", total_tris), 10, 35, 18, WHITE);

            DrawText(TextFormat("Pos: %.1f / %.1f / %.1f",
                cam.position.x, cam.position.y, cam.position.z),
                10, 58, 18, WHITE);

            DrawText(TextFormat("Block [%d]: %s",
                selected_idx + 1, block_names[selected_idx]),
                10, 81, 18, WHITE);

            DrawText("LMB=Break  RMB=Place  1/2/3=Block  Q/E=Up/Down",
                10, GetScreenHeight() - 30, 16, (Color){255,255,255,180});

        EndDrawing();
    }

    for (int cx = 0; cx < WORLD_W; cx++)
        for (int cz = 0; cz < WORLD_D; cz++)
            if (world[cx][cz].mesh.vertexCount > 0)
                UnloadModel(world[cx][cz].model);

    UnloadShader(g_chunk_shader);
    UnloadTexture(g_atlas_texture);
    CloseWindow();
    return 0;
}