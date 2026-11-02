#ifndef CPUCALCULATION_H
#define CPUCALCULATION_H

#define MAX_NODES 10000

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

typedef struct List_Node // World position
{
    Sprite sprite;
    struct List_Node *next;
} List_Node;
/*

    Triangle Function: Externing.

*/

// Function Initialization.
float sine(float angle);
float cosine(float angle);
float Tangent(float angle);

// Coordinate Initialization.
Vec3 init_world_coordinate(Vec3 *World_Coordinate);
Vec3 *init_camera_world_coordinate(Vec3 *Camera_Coordinate);
Vec3 *Init_Camera_Active_Axis(Vec3 *Direction_Coordinate);

// Function of coordinates.
Vec3 Main_Camera_Active_Axis_Rotation(float UD_angle, float LR_angle);

#endif // CPUCALCULATION_H