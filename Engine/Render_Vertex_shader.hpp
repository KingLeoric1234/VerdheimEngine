
#ifndef RENDERVERTEXSHADER_CPP
#define RENDERVERTEXSHADER_CPP

#include "Render_CPU_Calculation.h"

typedef struct Vertex
{
    Vec3 Wposition;
    float u, v;
} Vertex;

typedef struct Triangle
{
    Vertex v1;
    Vertex v2;
    Vertex v3;
} Triangle;

#endif RENDERVERTEXSHADER_CPP