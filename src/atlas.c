#include "atlas.h"
#include "block.h"
#include <string.h>
#include <stdio.h>

Texture2D  g_atlas_texture    = {0};
AtlasEntry g_atlas_entries[MAX_ATLAS_ENTRIES] = {0};
int        g_atlas_entry_count = 0;


static void make_key(char *out, size_t out_sz,
                     const char *namespace, const char *filename) {
    snprintf(out, out_sz, "%s:%s", namespace, filename);
}

static AtlasEntry *atlas_find(const char *key) {
    for (int i = 0; i < g_atlas_entry_count; i++)
        if (strcmp(g_atlas_entries[i].full_key, key) == 0)
            return &g_atlas_entries[i];
    return NULL;
}

static void atlas_add_texture(Image *atlas_img, int *tile_idx,
                               const char *namespace, const char *filename,
                               const char *file_path)
{
    char key[128];
    make_key(key, sizeof(key), namespace, filename);

    if (atlas_find(key)) return;

    if (*tile_idx >= MAX_ATLAS_ENTRIES) {
        TraceLog(LOG_ERROR, "Atlas full! Max %d Tiles.", MAX_ATLAS_ENTRIES);
        return;
    }

    Image img = LoadImage(file_path);
    if (img.data == NULL) {
        TraceLog(LOG_WARNING, "Texture not found: %s", file_path);
        UnloadImage(img);
        img = GenImageColor(ATLAS_TILE_SIZE, ATLAS_TILE_SIZE, MAGENTA);
    }
    ImageResize(&img, ATLAS_TILE_SIZE, ATLAS_TILE_SIZE);

    int col = (*tile_idx) % ATLAS_TILES_ROW;
    int row = (*tile_idx) / ATLAS_TILES_ROW;
    int px  = col * ATLAS_TILE_SIZE;
    int py  = row * ATLAS_TILE_SIZE;

    Rectangle dst = { (float)px, (float)py,
                      (float)ATLAS_TILE_SIZE, (float)ATLAS_TILE_SIZE };
    ImageDraw(atlas_img, img, (Rectangle){0,0,ATLAS_TILE_SIZE,ATLAS_TILE_SIZE}, dst, WHITE);
    UnloadImage(img);

    AtlasEntry *e = &g_atlas_entries[g_atlas_entry_count++];
    strncpy(e->full_key, key, sizeof(e->full_key) - 1);
    e->tile_index = *tile_idx;
    e->uv = (Rectangle){
        .x      = (float)px / (float)ATLAS_SIZE,
        .y      = (float)py / (float)ATLAS_SIZE,
        .width  = (float)ATLAS_TILE_SIZE / (float)ATLAS_SIZE,
        .height = (float)ATLAS_TILE_SIZE / (float)ATLAS_SIZE,
    };

    TraceLog(LOG_INFO, "Atlas [%d] %s @ tile(%d,%d)", *tile_idx, key, col, row);
    (*tile_idx)++;
}

static void load_pack_textures(Image *atlas_img, int *tile_idx,
                                const char *pack_path,
                                const char *namespace)
{
    char tex_dir[256];
    snprintf(tex_dir, sizeof(tex_dir), "%s/textures", pack_path);

    FilePathList files = LoadDirectoryFilesEx(tex_dir, ".png", false);

    for (unsigned int i = 0; i < files.count; i++) {
        const char *full_path = files.paths[i];

        const char *filename = GetFileName(full_path);

        atlas_add_texture(atlas_img, tile_idx, namespace, filename, full_path);
    }

    UnloadDirectoryFiles(files);
}

void atlas_build(void) {
    TraceLog(LOG_INFO, "=== Atlas Build started ===");

    Image atlas_img = GenImageColor(ATLAS_SIZE, ATLAS_SIZE, (Color){0,0,0,0});
    int tile_idx = 0;

    Image fallback = GenImageColor(ATLAS_TILE_SIZE, ATLAS_TILE_SIZE, MAGENTA);
    ImageDraw(&atlas_img, fallback,
              (Rectangle){0,0,ATLAS_TILE_SIZE,ATLAS_TILE_SIZE},
              (Rectangle){0,0,ATLAS_TILE_SIZE,ATLAS_TILE_SIZE}, WHITE);
    UnloadImage(fallback);
    tile_idx++;

    load_pack_textures(&atlas_img, &tile_idx, "assets/base", "base");

    FilePathList mod_dirs = LoadDirectoryFiles("assets/mods");
    for (unsigned int i = 0; i < mod_dirs.count; i++) {
        const char *mod_path = mod_dirs.paths[i];
        if (!DirectoryExists(mod_path)) continue;

        char pack_json[256];
        snprintf(pack_json, sizeof(pack_json), "%s/pack.json", mod_path);
        if (!FileExists(pack_json)) continue;

        char *json_text = LoadFileText(pack_json);
        char ns[64] = {0};
        const char *ns_ptr = strstr(json_text, "\"namespace\"");
        if (ns_ptr) {
            ns_ptr = strchr(ns_ptr, ':');
            ns_ptr = strchr(ns_ptr, '"') + 1;
            int j = 0;
            while (*ns_ptr && *ns_ptr != '"' && j < 63)
                ns[j++] = *ns_ptr++;
        }
        UnloadFileText(json_text);

        if (ns[0] == '\0') {
            TraceLog(LOG_WARNING, "No Namespace in %s", pack_json);
            continue;
        }

        TraceLog(LOG_INFO, "Load Mod: %s (namespace: %s)", mod_path, ns);
        load_pack_textures(&atlas_img, &tile_idx, mod_path, ns);
    }
    UnloadDirectoryFiles(mod_dirs);

    if (g_atlas_texture.id != 0)
        UnloadTexture(g_atlas_texture);

    g_atlas_texture = LoadTextureFromImage(atlas_img);
    SetTextureFilter(g_atlas_texture, TEXTURE_FILTER_POINT);
    UnloadImage(atlas_img);

    for (int i = 0; i < g_block_count; i++) {
        BlockDef *def = &g_blocks[i];

        def->tex_top    = atlas_get_uv(def->tex_top_key);
        def->tex_side   = atlas_get_uv(def->tex_side_key);
        def->tex_bottom = atlas_get_uv(def->tex_bottom_key);
    }

    TraceLog(LOG_INFO, "=== Atlas finished: %d Tiles, %dx%d px ===",
             tile_idx, ATLAS_SIZE, ATLAS_SIZE);
}

Rectangle atlas_get_uv(const char *full_key) {
    if (!full_key || full_key[0] == '\0')
        return g_atlas_entries[0].uv;

    AtlasEntry *e = atlas_find(full_key);
    if (!e) {
        TraceLog(LOG_WARNING, "UV not found: %s", full_key);
        return g_atlas_entries[0].uv;
    }
    return e->uv;
}