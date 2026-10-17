#ifndef RENDERPIPELINE_H
#define RENDERPIPELINE_H

#include "..\..\Render\SpriteDefinition.h"

#ifdef __cplusplus
extern "C"
{
#endif

    int render_init(void *hwnd);       // Create the factory and bind the HWND.
    void render_begin(void);           // Lock the canvas.
    void render_draw(const Sprite *s); // Draw sprites on canvas. (Rasterizer + Colour + Mix)
    void render_end(void);             // EndDraw + Present onto the screen.
    void render_shutdown(void);        // Release the canvas.

#ifdef __cplusplus
}
#endif

#endif // RENDERPIPELINE_H