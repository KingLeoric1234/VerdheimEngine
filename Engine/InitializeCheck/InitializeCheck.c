#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>  // malloc / exit
#include <windows.h> // Sleep / MessageBoxW
#include "InitializeCheck.h"

#define INITIALIZE_MEMORY_SIZE (16 * 1024 * 1024) // Allocate the memory size: 16MB.
#define RESOURCE_PATH "Resources\\"               // Clarify the resources path, the engine needs to find resources along this path.

// Engine Memory initializing.
unsigned char *engine_memory = NULL;
// Engine init tag: if the engine is not initialized, raise an error and refuse to run.
bool engine_initialization = false;
void reallocate_memory(void); // forward declaration: retry allocation after a short wait.

// When programme open, allocate memories for engine.
void allocate_memory(void)
{
    // Step 1: allocate RAM for engine data (not disk storage).
    engine_memory = malloc(INITIALIZE_MEMORY_SIZE); // allocate space for engine_memory.
    if (engine_memory == NULL)
    {
        reallocate_memory(); // When failed, reallocate the memory.
        if (engine_memory == NULL)
        {
            // Raise the unsolved problems.
            MessageBoxA(NULL, "We have occurred the unsolvable problems，please cancel the progresses, \n in order to release the space，then reopen the programme.", "VerdheimEngine", MB_OK | MB_ICONERROR);
        }
    }
}

// Reallocate the memories when allocate memory failed.
void reallocate_memory()
{
    Sleep(15000);                                   // sleep for 15s wait for system recovering.
    engine_memory = malloc(INITIALIZE_MEMORY_SIZE); // reallocate memories.
}

// Set a pointer to the resource disk so that finding resources quickly.
void initialize_resource_memory(void)
{
    // Check the resource folder exists (fopen cannot open a directory, so use the Windows API).
    DWORD attr = GetFileAttributesA(RESOURCE_PATH);
    if (attr == INVALID_FILE_ATTRIBUTES) // Folder not found: raise the error messagebox.
    {
        MessageBoxA(NULL, "We have occurred the unsolvable problems, please check the resources path.", "VerdheimEngine", MB_OK | MB_ICONERROR);
    }
}

void mark_initialization_complete(void)
{
    // Confirm both the resource folder and the engine memory are ready before starting.
    DWORD attr = GetFileAttributesA(RESOURCE_PATH);
    if (attr != INVALID_FILE_ATTRIBUTES && engine_memory)
    {
        engine_initialization = true;
    }
}

void free_memories()
{
    free(engine_memory);
}

// Check whether the memory outrange. If space enough, return True, otherwise false.
bool memory_check(int offset, int size)
{
    return offset + size <= INITIALIZE_MEMORY_SIZE;
}
