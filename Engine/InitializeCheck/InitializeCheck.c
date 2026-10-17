#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>  // malloc / exit
#include <windows.h> // Sleep / MessageBoxW
#include "InitializeCheck.h"

#define INITIALIZE_MEMORY_SIZE (16 * 1024 * 1024) // Allocate the memory size: 16MB.
#define RESOURCE_PATH "Resources\\"               // Clarify the resources path, the engine needs to find resources along this path.

// Engine Memory initializing.
unsigned char *sprite_memory = NULL;
unsigned char *screen_memory = NULL;

// Engine init tag: if the engine is not initialized, raise an error and refuse to run.
bool engine_initialization = false;
void reallocate_memory(void); // forward declaration: retry allocation after a short wait.

// When programme open, allocate memories for engine.
bool allocate_memory(void)
{
    // Step 1: allocate RAM for engine data (not disk storage).
    sprite_memory = malloc(INITIALIZE_MEMORY_SIZE); // Allocate space for engine_memory.
    screen_memory = malloc(INITIALIZE_MEMORY_SIZE); // Allocate the same memory size for screen display.

    if (sprite_memory == NULL || screen_memory == NULL)
    {
        reallocate_memory(); // When failed, reallocate the memory.
        if (sprite_memory == NULL || screen_memory == NULL)
        {
            return false;
        }
    }
    return true;
}

// Reallocate the memories when allocate memory failed.
void reallocate_memory()
{
    Sleep(15000);                                   // sleep for 15s wait for system recovering.
    screen_memory = malloc(INITIALIZE_MEMORY_SIZE); // reallocate memories.
    sprite_memory = malloc(INITIALIZE_MEMORY_SIZE);
}

// Set a pointer to the resource disk so that finding resources quickly.
bool initialize_resource_memory(void)
{
    // Check the resource folder exists (fopen cannot open a directory, so use the Windows API).
    DWORD attr = GetFileAttributesA(RESOURCE_PATH);
    return attr;
}

void mark_initialization_complete(void)
{
    // Confirm both the resource folder and the engine memory are ready before starting.
    DWORD attr = GetFileAttributesA(RESOURCE_PATH);
    if (attr != INVALID_FILE_ATTRIBUTES && sprite_memory)
    {
        engine_initialization = true;
    }
}

void free_memories()
{
    free(screen_memory);
    free(sprite_memory);
}

// Check whether the memory outrange. If space enough, return True, otherwise false.
bool memory_check(int offset, int size)
{
    return offset + size <= INITIALIZE_MEMORY_SIZE;
}
