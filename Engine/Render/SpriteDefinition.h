#ifndef SPRITEDEFINITION_H
#define SPRITEDEFINITION_H

#include "..\3DSpaceDefinition\Space.h"
struct Sprite
{
    struct Vec3 position;  // position.
    int width, height;     // scales.
    unsigned char *pixels; // image_paths.
};

#endif // SPRITEDEFINITION_H
