#ifndef SPACE_H
#define SPACE_H

struct Vec3
{
    float x, y, z;
};

float euclidean_distance(struct Vec3 a, struct Vec3 b);

#endif