/*

Pesudocode:
1. Initialize:
    Data structure from: Render_Rasterization.hpp.
    Read the data structure and paint the pixels on the screen.
*/

#include "Render_Rasterization.hpp"
#include "Render_Pixel_shader.hpp"

// Read the data structure.
PAElement paint_on_screen(PAElement paelement[], int pixel_number)
{
    PAElement temporary_element;
    PAElement *tePtr = &temporary_element;
    tePtr->Sposition = paelement[pixel_number].Sposition;
    tePtr->u = paelement[pixel_number].u;
    tePtr->v = paelement[pixel_number].v;
    return temporary_element;
}