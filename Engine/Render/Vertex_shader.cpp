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

