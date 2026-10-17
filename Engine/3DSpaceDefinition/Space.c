#include <stdio.h>
#include <math.h>

// Define the vector in the world to facilitate the calculating.
typedef struct Vec3
{
    float x, y, z;
} Vec3;

// Define the distance in the world.
float euclidean_distance(struct Vec3 a, struct Vec3 b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return sqrt(dx * dx + dy * dy + dz * dz);
}
