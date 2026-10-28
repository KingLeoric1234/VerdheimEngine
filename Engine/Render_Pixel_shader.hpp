#ifndef RENDER_PIXEL_SHADER_HPP
#define RENDER_PIXEL_SHADER_HPP

typedef struct Pixels_Array_Element
{
    Vec3 Sposition;
    float u, v;
    struct Pixels_Array_Element *pointer;
} PAElement;

#endif // RENDER_PIXEL_SHADER_HPP