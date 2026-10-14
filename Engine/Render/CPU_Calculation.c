#include "SpriteDefinition.h"
#include "..\3DSpaceDefinition\Space.h" // Needs to use Vec3
#include <stdio.h>
#include <stdbool.h>

// Computes the pixel-drawing start point (top-left corner) for a sprite.
// position is the anchor's world coordinate; width/height push it to the start point.
struct Vec3 anchor_definition(struct Sprite sprite, bool left, bool upper, bool center)
{
    struct Vec3 Vector; // Working copy; the original sprite is left untouched.
    Vector = sprite.position;
    if (center) // Center anchor: pull back half the width and height.
    {
        Vector.x -= sprite.width / 2;
        Vector.y -= sprite.height / 2;
        return (Vector); // Top-left start point for drawing.
    }
    else if (left && upper) // Upper-left: no offset; position is already the start point.
    {
        return (Vector);
    }
    else if (!left && upper) // Upper-right: shift left by the full width.
    {
        Vector.x -= sprite.width;
        return (Vector);
    }
    else if (left && !upper) // Bottom-left: shift up by the full height.
    {
        Vector.y -= sprite.height;
        return (Vector);
    }
    else if (!left && !upper) // Bottom-right: shift left by the width, up by the height.
    {
        Vector.x -= sprite.width;
        Vector.y -= sprite.height;
        return (Vector);
    }
    else
    {
        return (Vector);
    }
}