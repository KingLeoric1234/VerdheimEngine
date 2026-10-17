#ifndef SPACE_H
#define SPACE_H

typedef struct Vec3
{
    float x, y, z;
} Vec3;

float euclidean_distance(struct Vec3 a, struct Vec3 b);

#endif