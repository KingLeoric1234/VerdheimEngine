#ifndef SPRITE_H
#define SPRITE_H

#include "..\Render\SpriteDefinition.h"
#include <cstdlib>

class ASprite
{
public:
    // Constructor: Execute automatically.
    // Arguments: BMP file path (width, height, posX, posY).
    ASprite(const char *path, int w, int h, float posX, float posY);

    // Destructor: Release the data.
    ~ASprite();

    // Member method.
    void draw();                              // Render the sprite in the screen.
    void setPosition(float newX, float newY); // Set the position.
    float getX();                             // Get the position X to the rendering hardware.
    float getY();                             // Get the position Y to the rendering hardware.

private:
    // Combination: include the struct definition "Sprite" of the C file "SpriteDefinition.h".
    Sprite sprite;
};

#endif // SPRITE_H