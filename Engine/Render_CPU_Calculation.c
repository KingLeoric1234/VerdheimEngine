/*
    Const Definition:
    Const Value Definition, usually used in the project.

*/

#include "Render_CPU_Calculation.h"

#define NULL 0x00
#define MAX_NODES 10000
#define PI 3.1415926

/*

    This data structure is only-used in this file.

*/
typedef struct List_Node // World position
{
    Sprite sprite;
    struct List_Node *next;
} List_Node;

/*

    Null Value Definition:
    Define the null value of the variation.

*/

static Vec3 Null_position()
{
    Vec3 Null_position;
    Null_position.x = NULL;
    Null_position.y = NULL;
    Null_position.z = NULL;
    return Null_position;
}

static Sprite Null_Sprite()
{
    Sprite Null_Sprite;
    Null_Sprite.location = NULL;
    Null_Sprite.position = Null_position();
    return Null_Sprite;
}

static List_Node Null_list()
{
    List_Node zero_list;
    List_Node *zero_list_ptr = &zero_list;
    zero_list_ptr->next = NULL;
    zero_list_ptr->sprite = Null_Sprite();
    return zero_list;
}

/*
    Data Structure and Manuplate(Map):
    Definition: Map List.
    CPU: Manuplate the list framely.
    GPU: Read the list framely.

*/

static List_Node MapListPool[MAX_NODES];
static List_Node *freeList = NULL;
List_Node BeginNode, EndNode;

static void init_list()
{
    for (int i = 0; i < MAX_NODES; i++)
    {
        MapListPool[i].next = freeList;
        freeList = &MapListPool[i];
    }
    BeginNode.next = &EndNode;
    EndNode.next = NULL;
}

static List_Node *node_new(void) // Get a new Node.
{
    if (freeList == NULL)
        return NULL; // The node empty.
    List_Node *n = freeList;
    freeList = freeList->next;
    return n;
}

static void insert_the_node(List_Node *added_node)
{
    List_Node *n = node_new();
    if (n == NULL)
        return;
    n->next = added_node->next;
    added_node->next = n;
}

static void fill_the_node(List_Node *node, Sprite callsprite)
{
    node->sprite = callsprite;
    node->next = NULL;
}

// TODO: Design the map's list and read the map.
static void delete_the_node(List_Node *node)
{
    // Step 1: Found the previous node.
    List_Node **pp = &BeginNode.next;  // Traverse from the first one.
    while (*pp != NULL && *pp != node) // Traverse the node from the first one.
    {
        pp = &(*pp)->next; // Point the next one.
    }
    if (*pp == NULL)   // If the loop break but the pointer return NULL, then do not found the node.
        return;        // Not found.
    *pp = (*pp)->next; // Then change the correct node to the next.

    // Step 2: Clear up the instant node.
    node->next = NULL;
    node->sprite = Null_Sprite();
}

/*

    Triangle Function Definition:

*/

float sine(float angle)
{
    // Check the input.
    if (angle >= 180)
        angle -= 360;
    else if (angle <= -180)
        angle += 360;
    float Triangle_angle = (angle / 180) * PI;

    // Taylor expansion.
    float T3 = Triangle_angle * Triangle_angle * Triangle_angle;
    float T5 = Triangle_angle * Triangle_angle * Triangle_angle * Triangle_angle * Triangle_angle;
    float T7 = Triangle_angle * Triangle_angle * Triangle_angle * Triangle_angle * Triangle_angle * Triangle_angle * Triangle_angle;
    float Answer = Triangle_angle - (T3 / 6) + (T5 / 120) - (T7 / 5040);
    return Answer;
}

float cosine(float angle)
{
    if (angle >= 180)
        angle -= 360;
    else if (angle <= -180)
        angle += 360;
    float Triangle_angle = (angle / 180) * PI;
    float T2 = Triangle_angle * Triangle_angle;
    float T4 = Triangle_angle * Triangle_angle * Triangle_angle * Triangle_angle;
    float T6 = Triangle_angle * Triangle_angle * Triangle_angle * Triangle_angle * Triangle_angle * Triangle_angle;
    float T8 = Triangle_angle * Triangle_angle * Triangle_angle * Triangle_angle * Triangle_angle * Triangle_angle * Triangle_angle * Triangle_angle;
    float Answer = 1 - (T2 / 2) + (T4 / 24) - (T6 / 720) + (T8 / 40320);
    return Answer;
}

float Tangent(float angle)
{
    float Answer = sine(angle) / cosine(angle);
    return Answer;
}

/*
    World:
    Define: World Coordinate

    This is a Static Axis: The world coordinate is a static axis, which means the world coordinate will not change with the camera's movement.
    The world coordinate is the origin of the world, and the camera's position is relative to the world coordinate.

*/

static Vec3 Init_World_Coordinate;
static Vec3 *World_Coordinate_ptr = &Init_World_Coordinate;

static Vec3 init_world_coordinate(Vec3 *World_Coordinate)
{
    World_Coordinate->x = 0;
    World_Coordinate->y = 0;
    World_Coordinate->z = 0;
}

/*
    Camera.
*/

/*
    Camera:
    Define: Camera Coordinate

            Warning:
    The camera coordinate initialization will link the save file? Maybe.

    This is an Actual coordinate: ralate to the world coordinate. Facilitates the distance calculation between the camera and the sprite.

*/

static Vec3 Main_Camera_World_Coordinate;
static Vec3 *Main_Camera_World_ptr = &Main_Camera_World_Coordinate;

static Vec3 init_camera_world_coordinate(Vec3 *Camera_Coordinate)
{
    Camera_Coordinate->x = 0;
    Camera_Coordinate->y = 0;
    Camera_Coordinate->z = 0;
}

/*

    Camera:
    Camera Active Axis.

    This is an Active Axis: The camera's active axis is the axis that the camera is currently facing.
    The camera's active axis is determined by the camera's rotation.

*/

static Vec3 Main_Camera_Active_Axis;
static Vec3 *Main_Camera_Active_Axis_ptr = &Main_Camera_Active_Axis;

static Vec3 Init_Camera_Active_Axis(Vec3 *Direction_Coordinate)
{
    Direction_Coordinate->x = Main_Camera_World_Coordinate.x;
    Direction_Coordinate->y = Main_Camera_World_Coordinate.y;
    Direction_Coordinate->z = Main_Camera_World_Coordinate.z;
}

static Vec3 Main_Camera_Active_Axis_Rotation(float UD_angle, float LR_angle) // Up Down angle, Left Right angle.
{
    // Coordinates: upper(u), right(r), front(f).
    // phi: Up Down angle, theta: Left Right angle.
    float sin_phi = sine(UD_angle);
    float cos_phi = cosine(UD_angle);
    // ===
    float sin_theta = sine(LR_angle);
    float cos_theta = cosine(LR_angle);
    // ===
    Vec3 u, r, f;

    f.x = cos_phi * cos_theta;
    f.y = sin_phi;
    f.z = cos_phi * sin_theta;

    u.x = -sin_phi * cos_theta;
    u.y = cos_phi;
    u.z = -sin_phi * sin_theta;

    r.x = sin_theta;
    r.y = 0;
    r.z = -cos_theta;
}

/*
    Camera:
    FOV (Field of view).

    How to calculate the field of view of the camera's active axis.
    How far , how near the camera can see.

*/

/*

    Output:
    The output of the camera. Into the GPU.

*/

Vec3 Camera_Output[2];
Vec3 *Camera_Output_ptr = Camera_Output;

void pack_camera_output(
    Vec3 *Camera_output,          // Camera's output. (GPU read)
    Vec3 Main_Camera_WCoordinate, // Camera's World Coordinate.
    Vec3 Main_Camera_AAxis        // Camera's Active Axis.
)
{
    // Line 1: Camera's World Coordinate.
    Camera_output[0].x = Main_Camera_WCoordinate.x;
    Camera_output[0].y = Main_Camera_WCoordinate.y;
    Camera_output[0].z = Main_Camera_WCoordinate.z;

    // Line 2: Camera's Active Axis.
    Camera_output[1].x = Main_Camera_AAxis.x;
    Camera_output[1].y = Main_Camera_AAxis.y;
    Camera_output[1].z = Main_Camera_AAxis.z;
}