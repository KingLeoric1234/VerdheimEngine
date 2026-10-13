#include <stdbool.h>
#include <stdio.h>

// Engine on/off switch: when false, the engine stays shut down and skips the loop.
bool programme_execution = false;

// Engine init tag: if the engine is not initialized, raise an error and refuse to run.
bool initialization = false;

// Set to true when the current frame has finished rendering.
bool frame_finished = false;

// Set to true when the frame time budget has been reached.
bool time_scale_finished = false;

// ---- Init stage: run once, before the frame loop ----
static void allocate_memory(void)
{
    // Step 1: allocate RAM for engine data (not disk storage).
}

static void initialize_resource_memory(void)
{
    // Step 2: reserve memory for game resources.
}

static void release_cpu_cache(void)
{
    // Step 3: release CPU caches and allocate fixed, fast memory for resources.
}

static void build_resource_index(void)
{
    // Step 4: use pointers (needles) to reference resources for quick lookup.
}

static void mark_initialization_complete(void)
{
    // Step 5: give the init tag once initialization has finished.
    initialization = true;
}

// ---- Frame stage: run every frame ----
static void load_sprite_from_disk(void)
{
    // Step 1: use pointers (needles) to load the sprite resources from disk into memory.
}

static void cpu_compute_shape(void)
{
    // Step 2: let the CPU compute the correct shape of the resources (scale & anchor).
}

static void gpu_load_sprite(void)
{
    // Step 3: ask the GPU to load the sprite.
}

static void transmit_and_render(void)
{
    // Step 4: CPU sends scale and anchor to the GPU, which renders the sprite to screen.
}

static void check_frame_complete(void)
{
    // Step 5: mark frame_finished = true, then check whether the frame time is up.
}

// TODO: frame timing — waits until the target frame time is reached.
static void Timer(void)
{
    // not written yet
}

int main(void)
{
    // Initialize the engine first, so the programme runs correctly.
    if (programme_execution)
    {
        allocate_memory();
        initialize_resource_memory();
        release_cpu_cache();
        build_resource_index();
        mark_initialization_complete();
    }

    // Keep looping until the programme is cancelled.
    while (programme_execution || initialization)
    {
        load_sprite_from_disk();
        cpu_compute_shape();
        gpu_load_sprite();
        transmit_and_render();
        check_frame_complete();

        if (frame_finished && time_scale_finished)
        {
            // Frame done within budget: push the frame to the screen, then continue.
            continue;
        }
        else if (!frame_finished)
        {
            printf("Error: Frame Rendering Overtime, please decrease the frame time\n");
        }
        else
        {
            // Frame rendered, but the frame time is not up yet: wait for the timer.
            Timer();
        }
    }

    return 0;
}
