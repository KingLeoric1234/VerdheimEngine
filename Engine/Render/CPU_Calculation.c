#include "SpriteDefinition.h"              // Needs to use Sprite variation
#include "..\3DSpaceDefinition\Space.h"    // Needs to use Vec3 variation.
#include "..\EntryPoint\HWND\HWND_Proxy.h" // This is the main proxy of the system, which facilitates to transplant the game into the other systems.
#include <stdio.h>
#include <stdbool.h>

// Only-Read Sprites, prevent to write in the sprite settings. We need to change the sprite settings at sprite's properties (Struct Sprite).
// Define the struct to visit the sprite's width and height.

typedef struct Sprite_Dimention
{
    int width;
    int height;

} SDim;

// Return the sprite's pointer (Path). CPU only-read it too.
unsigned char *sprite_path(Sprite sprite)
{
    return sprite.pixels;
}

// This function is the only way that CPU can read the scale of the sprites.
SDim sprite_scales(Sprite sprite)
{
    SDim d;
    d.width = sprite.width;
    d.height = sprite.height;
    return d;
}

// Return Sprite's Position to CPU. CPU only-read it.
Vec3 sprite_position(Sprite sprite)
{
    return sprite.position;
}

// Computes the pixel-drawing start point (top-left corner) for a sprite.
// position is the anchor's world coordinate; width/height push it to the start point.
// TODO: Facilitate the renderer.
Vec3 anchor_definition(struct Sprite sprite, bool left, bool upper, bool center)
{
    Vec3 Vector; // Working copy; the original sprite is left untouched.
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

// CCTGPU: CPU Calculation to GPU.
Sprite CCTGPU(Sprite sprite)
{
    // Send the Screen scales into variation names "screen", facilitate to find the sprite rendering locations.
    RSface screen = render_query_surface();
    sprite.height = sprite_scales(sprite).height;
    sprite.width = sprite_scales(sprite).width;
    sprite.pixels = sprite_path(sprite);
    sprite.position = sprite_position(sprite);
    return sprite;
}