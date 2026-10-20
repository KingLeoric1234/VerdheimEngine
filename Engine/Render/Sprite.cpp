#include "SpriteDefinition.h"                 // include <sprite.h>: C file use the Sprite struct.
#include "..\EntryPoint\HWND\GPU_Rendering.h" // Needs rendering function.
#include "Sprite.h"                           // Needs class declearation "ASprite".
#include <cstdlib>                            // Needs "free" function.

ASprite::ASprite(const char *path, int swidth, int sheight, float posX, float posY)
{
    // Call the variation "sprite" from "Sprite.h".
    // Set the BMP image's scales.
    sprite.width = swidth;
    sprite.height = sheight;

    // Set the image's positions.
    sprite.position.x = posX;
    sprite.position.y = posY;
    sprite.position.z = 0.0f; // This variation will be the "height" variation. Decide the rendering levels.

    // Execute the written C function to load the BMP images.
    // Load the BMP image frames and image's functions.
    load_bmp_to_sprite(path, &sprite);
};

// Destructor.
ASprite::~ASprite()
{
    // Release the memory automatically.
    if (sprite.pixels != nullptr)
    {
        free(sprite.pixels); // Free the pointer of the pixels.
        sprite.pixels = nullptr;
    }
}

// Draw itself.
void ASprite::draw()
{
    render_draw(&sprite); //
}

// Set the sprite display location.
void ASprite::setPosition(float newX, float newY)
{
    sprite.position.x = newX;
    sprite.position.y = newY;
}

// Get the sprite display position x.
float ASprite::getX()
{
    return sprite.position.x;
}

// Get the sprite display position y.
float ASprite::getY()
{
    return sprite.position.y;
}