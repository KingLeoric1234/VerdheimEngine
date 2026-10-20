#ifndef INITIALIZECHECK_H
#define INITIALIZECHECK_H

#include <stdbool.h>

#define RESOURCE_PATH "Resources\\"
// Engine init tag shared across files (defined in InitializeCheck.c).
extern bool engine_initialization;
extern unsigned char *sprite_memory;
extern unsigned char *screen_memory;

// Init stage: run once, before the frame loop.
bool allocate_memory(void);
bool initialize_resource_memory(void);
void mark_initialization_complete(void);
void free_memories(void);

// Allocate memory from sprite_memory (the big block).
void *sprite_memory_alloc(int size);

// Memory cursor: records how much sprite_memory has been used.
extern int sprite_memory_offset;

#endif // INITIALIZECHECK_H
