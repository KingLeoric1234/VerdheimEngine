#include "..\..\Tick\UpdateThings.h"
#include "..\..\Render\SpriteDefinition.h"
#include "..\..\EntryPoint\HWND\GPU_Rendering.h"

#define FRAME_COUNT 3

// The path of the animation's path
const char *frame_paths[FRAME_COUNT] =
    {
        "Resources/Monster/1.BMP",
        "Resources/Monster/2.BMP",
        "Resources/Monster/3.BMP"};

Sprite frames[FRAME_COUNT];
int current_frame = 0;
int frame_tick = 0;
#define FRAME_SWITCH_TICK 50

void update_things_init(void)
{
    for (int i = 0; i < FRAME_COUNT; i++)
    {
        load_bmp_to_sprite(frame_paths[i], &frames[i]);
        frames[i].position.x = 200;
        frames[i].position.y = 200;
    }
}

void update_things(void)
{
    frame_tick++;
    if (frame_tick >= FRAME_SWITCH_TICK)
    {
        frame_tick = 0;
        current_frame = (current_frame + 1) % FRAME_COUNT;
    }
    render_draw(&frames[current_frame]);
}

void Package()
{
    static bool initialized = false;
    if (!initialized)
    {
        update_things_init();
        initialized = true;
    }
    update_things();
}