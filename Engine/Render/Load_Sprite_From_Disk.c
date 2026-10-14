#include <stdio.h>
#include "SpriteDefinition.h"
#include "..\InitializeCheck\InitializeCheck.h"

struct Sprite load_sprite_from_disk(unsigned char *engine_memory, const char *filename)
{
    struct Sprite sprite;
    char path[256];
    snprintf(path, sizeof(path), "%s%s", RESOURCE_PATH, filename);

    FILE *f = fopen(path, "rb");
    if (!f)
    {
        printf("Do not found the tile");
    }

    unsigned char header[54];
    fread(header, 1, 54, f); // BMP's header won't get into the array.
    sprite.width = *(int *)(header + 18);
    sprite.height = *(int *)(header + 22);
    int offset = *(int *)(header + 10);

    sprite.pixels = engine_memory;
    fseek(f, offset, SEEK_SET);
    fread(sprite.pixels, 1, sprite.width * sprite.height * 3, f);
    fclose(f);
    return sprite;
}