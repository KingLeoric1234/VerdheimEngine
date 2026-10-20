#include <stdio.h>
#include <stdbool.h>
#include <windows.h>
#include "..\EntryPoint\HWND\GPU_Rendering.h" // We need the rendering steps.
#include "..\Render\SpriteDefinition.h"       // We need the Sprite variation

// Extern the load bmp function into the file.
extern int load_bmp_to_sprite(const char *filepath, Sprite *out);

// The frame's first aid, I will update these function to help frames.
int down_the_frame(int *p_frame)
{
    *p_frame -= 10; // Decrease 10 frames.
}

void Update(bool Program_State)
{
    // Initialize the BMP tile.
    static Sprite tile;
    load_bmp_to_sprite("Resources/tile_0001.bmp", &tile);
    tile.position.x = 200;
    tile.position.y = 200;

    // Frame Initialization.
    int frame = 200;       // Define the frame's refreshment.
    int *p_frame = &frame; // Get the location of the "int frame".
    float frame_rendering_time_ms_float = 1000.0f / *p_frame;

    if (Program_State) // If programe running, then loop the frame.
    {
        // float ms
        double frame_rendering_time_ms_double = frame_rendering_time_ms_float; // use this: double ms.
        LARGE_INTEGER freq, t0, t1;

        QueryPerformanceFrequency(&freq); // Record the tick.
        QueryPerformanceCounter(&t0);     // Record the begin time.

        // Begin to render, File: GPU_Rendering.
        render_begin();
        render_draw(&tile);
        render_end();

        QueryPerformanceCounter(&t1); // Record the rendering end time.

        double seconds = (double)(t1.QuadPart - t0.QuadPart) / (double)freq.QuadPart; // Record the rendering time.
        double ms = seconds * 1000.0f;                                                // Record the rendering time ~ ms(double)

        if (ms <= frame_rendering_time_ms_double) // If the rendering time delays.
        {
            Sleep(frame_rendering_time_ms_double - ms); // Waiting for the next frame.
        }
        else
        {
            down_the_frame(p_frame); // Decrease the frame's refreshment.
        }
    }
}
