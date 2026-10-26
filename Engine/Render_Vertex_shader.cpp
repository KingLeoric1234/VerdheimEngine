// Vertex_shader.cpp: Ensure the every vertexs' location.
// Pesudocode:
/*
Initialize Camera_location;
Initialize Map_List;
Map_List = [Object1, Object2, Object3];

// In this Step, We need to get the Object's location.
Vec3 Object1;   // Define the Object1's location in the 3D space.
Object1.x = 2, Object1.y = 3, Object1.z = 4;    // Vec3 Object1 = (1, 1, 1)
Vec3 Object2;   
Object2.x = 5, Object2.y = 2, Object2.z = 1;    // Vec3 Object1 = (1, 1, 1)
Vec3 Object3;
Object3.x = 5, Object3.y = 2, Object3.z = 1;    // Vec3 Object1 = (1, 1, 1)

// In this Step, We need to get the Camera's location.
Vec3 Camera_location;
Camera_location = (0, 0, 0);

// In this Step, we need to define the focal length.
int focal_length = 5;

// In this Step, we need to ensure the final length in the screen.
typedef struct ScreenVec3 Actual_location
{
    float screenx;
    float screeny;
}

int Object[Infinity]; // Infinity Array.
for (int i = 0, i <= len(Map_List), i++)
{
    ScreenVec3 Object[i];
}

for (int i = 0; i <= len(Map_List), i++)
{
    Object[i].screenx = focal_length * Object[i].x / Object[i].z;
    Object[i].screeny = focal_length * Object[i].y / Object[i].z;
}
*/