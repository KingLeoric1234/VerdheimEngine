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

#include "Render_Vertex_shader.hpp" // Itself.
#include "Render_CPU_Calculation.h" // One-Way to recieve CPU's calculation.

/*

    Vertex Calculation.

*/
static Triangle Projection(
    Triangle Origin_triangle,
    int focus)
{
    // Origin Triangle.
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
