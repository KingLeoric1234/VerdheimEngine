#include <stdio.h>
#include "..\Render\Load_Sprite_From_Disk.h"
#include "..\InitializeCheck\InitializeCheck.h"
#include "..\Render\Render_Pictures.h"

// Engine on/off switch: when false, the engine stays shut down and skips the loop.
bool programme_execution = true;

int main(void)
{
    // Initialize the engine first, so the programme runs correctly.
    if (programme_execution) // InitializeCheck.c File
    {
        allocate_memory();
        initialize_resource_memory();
        mark_initialization_complete();
    }

    // Keep looping until the programme is cancelled.
    while (programme_execution || engine_initialization)
    {
        struct Sprite sprite = load_sprite_from_disk(engine_memory, RESOURCE_PATH, "tile_0001.bmp");
        framebuffer = malloc(SCREEN_WIDTH * SCREEN_HEIGHT * 4); // 画板自己 malloc（池满了）
        if (!framebuffer)
            return 1;

        create_window_and_loop(&sprite); // ← 窗口出现，图在正中心，关窗口 = 退出
    }

    free(framebuffer);
    free_memories(); // Free all memories that engine_memory possessed.
    return 0;
}
