#include <stdbool.h>
#include <stdio.h>
#include "SpriteDefinition.h"
#include "..\3DSpaceDefinition\Space.h"

// Define a sprite with position, scales, image_paths.
struct Sprite
{
    struct Vec3 position;  // position.
    int width, height;     // scales.
    unsigned char *pixels; // image_paths. Point the resources path.
};
