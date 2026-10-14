#ifndef INITIALIZECHECK_H
#define INITIALIZECHECK_H

#include <stdbool.h>

#define RESOURCE_PATH "Resources\\"
// Engine init tag shared across files (defined in InitializeCheck.c).
extern bool engine_initialization;
extern unsigned char *engine_memory;

// Init stage: run once, before the frame loop.
void allocate_memory(void);
void initialize_resource_memory(void);
void mark_initialization_complete(void);
void free_memories(void);

#endif // INITIALIZECHECK_H
