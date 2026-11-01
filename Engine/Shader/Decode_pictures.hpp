#ifndef RENDER_IMAGE_LOADER_H
#define RENDER_IMAGE_LOADER_H

// Decode an image file into raw RGBA8 pixels (w*h*4 bytes).
// Returns a malloc'd buffer on success, or NULL on failure.
unsigned char *wic_load(const wchar_t *path, unsigned int *outW, unsigned int *outH);

#endif