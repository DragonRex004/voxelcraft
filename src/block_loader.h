//
// Created by drago on 02.06.2026.
//

#ifndef VOXELCRAFT_BLOCK_LOADER_H
#define VOXELCRAFT_BLOCK_LOADER_H

#include "block.h"
#include "cJson/cJSON.h"
#include "raylib.h"
#include <string.h>
#include <stdio.h>

static void resolve_tex_key(char *out, size_t out_sz,
                             const char *namespace, const char *value)
{
    if (strchr(value, ':')) {
        strncpy(out, value, out_sz - 1);
    } else {
        snprintf(out, out_sz, "%s:%s", namespace, value);
    }
}

void load_blocks_json(const char *path, const char *namespace) {
    char *text = LoadFileText(path);
    if (!text) {
        TraceLog(LOG_ERROR, "blocks.json not found: %s", path);
        return;
    }

    cJSON *root   = cJSON_Parse(text);
    cJSON *blocks = cJSON_GetObjectItem(root, "blocks");
    UnloadFileText(text);

    if (!blocks) {
        TraceLog(LOG_ERROR, "No 'blocks' Array in %s", path);
        cJSON_Delete(root);
        return;
    }

    cJSON *b;
    cJSON_ArrayForEach(b, blocks) {
        const char *name = cJSON_GetObjectItem(b, "name")->valuestring;
        uint16_t id = block_register(namespace, name);
        BlockDef *def = block_get_by_id(id);

        cJSON *solid = cJSON_GetObjectItem(b, "solid");
        cJSON *trans = cJSON_GetObjectItem(b, "transparent");
        cJSON *light = cJSON_GetObjectItem(b, "light_emission");

        def->solid          = solid ? cJSON_IsTrue(solid) : true;
        def->transparent    = trans ? cJSON_IsTrue(trans) : false;
        def->light_emission = light ? (uint8_t)light->valueint : 0;

        cJSON *textures = cJSON_GetObjectItem(b, "textures");
        if (textures) {
            cJSON *all    = cJSON_GetObjectItem(textures, "all");
            cJSON *top    = cJSON_GetObjectItem(textures, "top");
            cJSON *bottom = cJSON_GetObjectItem(textures, "bottom");
            cJSON *sides  = cJSON_GetObjectItem(textures, "sides");

            const char *top_val    = top    ? top->valuestring    :
                                     all    ? all->valuestring    : "";
            const char *bottom_val = bottom ? bottom->valuestring :
                                     all    ? all->valuestring    : "";
            const char *side_val   = sides  ? sides->valuestring  :
                                     all    ? all->valuestring    : "";

            resolve_tex_key(def->tex_top_key,    sizeof(def->tex_top_key),
                            namespace, top_val);
            resolve_tex_key(def->tex_bottom_key, sizeof(def->tex_bottom_key),
                            namespace, bottom_val);
            resolve_tex_key(def->tex_side_key,   sizeof(def->tex_side_key),
                            namespace, side_val);
        }

        TraceLog(LOG_INFO, "Block loaded: %s (id=%d)", def->full_name, id);
    }

    cJSON_Delete(root);
}

void load_all_packs(void) {
    load_blocks_json("assets/base/blocks.json", "base");

    FilePathList mod_dirs = LoadDirectoryFiles("assets/mods");
    for (unsigned int i = 0; i < mod_dirs.count; i++) {
        const char *mod_path = mod_dirs.paths[i];
        if (!DirectoryExists(mod_path)) continue;

        char blocks_json[256];
        snprintf(blocks_json, sizeof(blocks_json), "%s/blocks.json", mod_path);
        if (!FileExists(blocks_json)) continue;

        char pack_json[256];
        snprintf(pack_json, sizeof(pack_json), "%s/pack.json", mod_path);
        char *text = LoadFileText(pack_json);
        char ns[64] = {0};
        const char *p = strstr(text, "\"namespace\"");
        if (p) {
            p = strchr(p, ':');
            p = strchr(p, '"') + 1;
            int j = 0;
            while (*p && *p != '"' && j < 63) ns[j++] = *p++;
        }
        UnloadFileText(text);

        if (ns[0]) load_blocks_json(blocks_json, ns);
    }
    UnloadDirectoryFiles(mod_dirs);
}

#endif //VOXELCRAFT_BLOCK_LOADER_H
