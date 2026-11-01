/*

    HLSL.

*/
struct VS_INPUT
{
    float3 Wposition : POSITION;
    float2 uv : TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

cbuffer Camera : register(b0)
{
    float FOCUS;
};

VS_OUTPUT VS(VS_INPUT v)
{
    VS_OUTPUT o;

    if (v.Wposition.z == 0)
        v.Wposition.z = 0.1;

    o.pos.x = v.Wposition.x * FOCUS;
    o.pos.y = v.Wposition.y * FOCUS;
    o.pos.z = v.Wposition.z;
    o.pos.w = v.Wposition.z;

    o.uv = v.uv;
    return o;
}

// Pixel shader input.
struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

Texture2D myTexture : register(t0);
SamplerState mySampler : register(s0);

float4 PS(PS_INPUT v) : SV_TARGET
{
    return myTexture.Sample(mySampler, v.uv);
}

// Vertex_shader.cpp: Ensure the every vertexs' location.
// Pesudocode:
/*
Pesudocode:
    Set Alpha, Beta;
    Alpha: Vertical FOV, Beta: Horizontal FOV;
    Set Pixel = Vec3 (x, y, z);
    Use Pixel.y and Pixel.z to calculate the Tangent of the angle, and then to compare with the Beta;
    If the Tangent of the angle is greater than the Beta, then the Pixel is out of the FOV;
    else, goto the next step;
    If the Pixel is in the FOV, then use Pixel.x and Pixel.z to calculate the Tangent of the angle, and then to compare with the Alpha;
    If the Tangent of the angle is greater than the Alpha, then the Pixel is out.
    else, the Pixel is in the FOV;
*/

/*

#include "Render_Vertex_shader.hpp" // Itself.
#include "Render_CPU_Calculation.h" // One-Way to recieve CPU's calculation.
#include "Render_Bootstrap.cpp"

// ======= header =======

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

*/

/*

    C Language: Vertex Calculation.

*/

/*

static Triangle Projection(
    Triangle Origin_triangle,
    int focus)
{
    // Origin Triangle, and Pointer.
    Triangle Projection_Triangle;
    Triangle *PTriPtr = &Projection_Triangle;

    // World Coordinate -> Screen Coordinate.
    // Copy the Coordinates from the Origin Triangle.
    float Depth_OV1 = Origin_triangle.v1.Wposition.z;
    float Depth_OV2 = Origin_triangle.v2.Wposition.z;
    float Depth_OV3 = Origin_triangle.v3.Wposition.z;

    // Calculate the Screen Coordinates.
    float Screen_V1x = Origin_triangle.v1.Wposition.x * focus / Depth_OV1;
    float Screen_V1y = Origin_triangle.v1.Wposition.y * focus / Depth_OV1;

    float Screen_V2x = Origin_triangle.v2.Wposition.x * focus / Depth_OV2;
    float Screen_V2y = Origin_triangle.v2.Wposition.y * focus / Depth_OV2;

    float Screen_V3x = Origin_triangle.v3.Wposition.x * focus / Depth_OV3;
    float Screen_V3y = Origin_triangle.v3.Wposition.y * focus / Depth_OV3;

    // Transform the Screen Coordinates from the Results into Triangle's pointer.
    PTriPtr->v1.Wposition.x = Screen_V1x;
    PTriPtr->v1.Wposition.y = Screen_V1y;
    PTriPtr->v1.Wposition.z = Depth_OV1;

    PTriPtr->v2.Wposition.x = Screen_V2x;
    PTriPtr->v2.Wposition.y = Screen_V2y;
    PTriPtr->v2.Wposition.z = Depth_OV2;

    PTriPtr->v3.Wposition.x = Screen_V3x;
    PTriPtr->v3.Wposition.y = Screen_V3y;
    PTriPtr->v3.Wposition.z = Depth_OV3;

    // Record the Texture Coordinates.
    PTriPtr->v1.u = Origin_triangle.v1.u;
    PTriPtr->v1.v = Origin_triangle.v1.v;

    PTriPtr->v2.u = Origin_triangle.v2.u;
    PTriPtr->v2.v = Origin_triangle.v2.v;

    PTriPtr->v3.u = Origin_triangle.v3.u;
    PTriPtr->v3.v = Origin_triangle.v3.v;

    return Projection_Triangle;
}

*/

/*
Pesudocode 1: Overall prgramme
Rasterization ended, GPU put out the Array P.
Read
    Array P[1920 x 1080]
for every elements in Array P
    if current pixel gets Rasterization?
        No, read the next pixel;
        Yes, Z-buffer and Next Step;
    P[elements]:
        UV coloring
=============================================
Pesudocode 2: Data Structure: Linked List.
Initialize:
    original linked list which nodes' contains nothing and the "next" pointer contains next node's location.
        We get Pixel1-Pixel2-....-Pixel2073600 long linked list.
Begin Rasterization.
    Input:
        Tools: 1. create nodes and write nodes.
               2. add nodes into the correct place.
        Container: 1. Initialized linked list.
    Rasterization:
        Gets the initialized empty linked list.
        for (i = 0; i < prepared to rasterized triangles; i++)
            // Rasterizing one triangle.
            Create new nodes and write in nodes for every rasterized pixels, every pixels contains xyz, uv.
            We get lots of new_nodes.
            // Question: How to record the unsorted new_nodes with pointers.
            // Link the nodes.
            for (i = 0, i < length(new_nodes), i++) // get a new_node and insert into the initialized list.
                nodes_x = new_nodes.x;
                nodes_y = new_nodes.y;
                // Traverse the linked list.
                for (i = 0; i < List.elements; i++)
                    if(i == 0)  // Get the beginner pointer at the first of the loop.
                        *current_node = &beginner;
                    if (nodes_x == current_node->x && nodes_y == current_node->y && current_node->z == NULL)   // Find the correct place. And the place is NULL.
                        current_node->x = new_node.x;
                        current_node->y = new_node.y;
                        current_node->z = new_node.z;
                        current_node->u = new_node.u;
                        current_node->v = new_node.v;
                        // Pointer will not be changed, cause we use the initialized list.
                    else if (nodes_x == current_node->x && nodes_y == current_node->y && current_node->z != NULL)   // The initialization filled with pixels.
                        current_node->next = &new_node;
                        *current_node = current_node->next; // Transfer into next node.
    Output: The linked list with severals of pixels information.
            beginner-[pixels 1: (A - B)]-[pixels 2: (A - B - C - D)]-[pixels 3: (A)]-......-[pixels 2073600: (A - B)]-end.

*/

/*

    Warning: "-1" will be "NULL" return for integer and float.
            See the "initialize_pixels_array" function.

*/

/*

#include "Render_Vertex_Shader.hpp"
#include "Render_Pixel_Shader.hpp"

// ======= header =======
// No header, no cpp file.

*/

/*

// Initialize the nodes.
void initialize_pixels_array(PAElement Pixels_Array[])
{
    for (int i = 0; i < SCREEN_PIXELS; i++)
    {
        Pixels_Array[i].Sposition.x = -1;
        Pixels_Array[i].Sposition.y = -1;
        Pixels_Array[i].Sposition.z = -1;

        Pixels_Array[i].u = -1;
        Pixels_Array[i].v = -1;
    }
}

// fill the node
// connect the following nodes when node filled.
void fill_the_node(
    PAElement listed_array[],
    PAElement *element)
{
    // Hash searching.
    int screen_x = element->Sposition.x; // Begin with 0. Must be integer.
    int screen_y = element->Sposition.y; // Begin with 0. Must be integer.

    // Searching the correct element.
    if (listed_array[screen_y * SCREEN_WIDTH + screen_x].u != -1 &&
        listed_array[screen_y * SCREEN_WIDTH + screen_x].Sposition.x != -1 &&
        listed_array[screen_y * SCREEN_WIDTH + screen_x].pointer == NULL) // If filled the parent node and the pointer NULL. Follow the parent node.
        listed_array[screen_y * SCREEN_WIDTH + screen_x].pointer = element;
    else if (
        listed_array[screen_y * SCREEN_WIDTH + screen_x].u == -1 &&
        listed_array[screen_y * SCREEN_WIDTH + screen_x].Sposition.x == -1) // If parent node NULL, fill the parent node.
    {
        listed_array[screen_y * SCREEN_WIDTH + screen_x].Sposition = element->Sposition;
        listed_array[screen_y * SCREEN_WIDTH + screen_x].u = element->u;
        listed_array[screen_y * SCREEN_WIDTH + screen_x].v = element->v;
    }
    else if (listed_array[screen_y * SCREEN_WIDTH + screen_x].pointer != NULL)
    // If parent node not NULL, and the pointer not NULL,
    // then transfer the parent pointer to point the new node,
    // the new node points old node at the same time.
    {
        element->pointer = listed_array[screen_y * SCREEN_WIDTH + screen_x].pointer;
        listed_array[screen_y * SCREEN_WIDTH + screen_x].pointer = element;
    }
}

PAElement create_new_node(
    Vec3 Current_Screen_Position,
    float Current_u,
    float Current_v)
{
    PAElement current_element;
    PAElement *current_element_ptr;
    current_element_ptr = &current_element;

    current_element_ptr->Sposition = Current_Screen_Position;
    current_element_ptr->u = Current_u;
    current_element_ptr->v = Current_v;
    return current_element;
}

*/

/*

Pesudocode:
1. Initialize:
    Data structure from: Render_Rasterization.hpp.
    Read the data structure and paint the pixels on the screen.
*/

/*

#include "Render_Rasterization.hpp"
#include "Render_Pixel_shader.hpp"

// ======= header =======

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
*/

// Read the data structure.
/*
PAElement paint_on_screen(PAElement paelement[], int pixel_number)
{
    PAElement temporary_element;
    PAElement *tePtr = &temporary_element;
    tePtr->Sposition = paelement[pixel_number].Sposition;
    tePtr->u = paelement[pixel_number].u;
    tePtr->v = paelement[pixel_number].v;
    return temporary_element;
}
*/

/*

    HLSL.

*/