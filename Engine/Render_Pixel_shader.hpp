#ifndef RENDER_PIXEL_SHADER_HPP
#define RENDER_PIXEL_SHADER_HPP

#include "Render_CPU_Calculation.h"
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define SCREEN_PIXELS (SCREEN_WIDTH * SCREEN_HEIGHT)
#define NULL 0x00

typedef struct Pixels_Array_Element
{
    Vec3 Sposition;
    float u, v;
    struct Pixels_Array_Element *pointer;
} PAElement;

#endif // RENDER_PIXEL_SHADER_HPP