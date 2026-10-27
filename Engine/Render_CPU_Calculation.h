#ifndef CPUCALCULATION_H
#define CPUCALCULATION_H

/*

    Variation:
    The variation we will use. Externing.

*/
typedef struct Vec3
{
    float x;
    float y;
    float z;
} Vec3;

typedef struct Sprite
{
    unsigned char *location; // Sprite's location.
    Vec3 position;
} Sprite;

/*

    Triangle Function: Externing.

*/
float sine();
float cosine();
float tangent();

#endif // CPUCALCULATION_H