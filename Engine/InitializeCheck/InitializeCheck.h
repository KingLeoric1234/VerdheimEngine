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

#endif // INITIALIZECHECK_H
