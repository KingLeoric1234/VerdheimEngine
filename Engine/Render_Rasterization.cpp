/*
Rasterization ended, GPU put out the Array P.
Read
    Array P[1920 x 1080]
for every elements in Array P
    if current pixel gets Rasterization?
        No, read the next pixel;
        Yes, Z-buffer and Next Step;
    P[elements]:
        UV coloring
*/

#include "Render_Vertex_Shader.hpp"

#define MIN_NODES 2073600
#define MAX_NODES 10000000
#define NULL 0x00

typedef struct RenderList_Node
{
    Vec3 Sposition; // Screen positions.
    float u, v;
    struct RenderList_Node *next_node;
} RNode;

RNode List[MAX_NODES];
RNode *List = &List[0];

// Data Structure, Special Linked List.
static void init_list()
{
    for (int i = 0; i < MIN_NODES; i++)
    {
        (List + i)->next_node = (List + i + 1);
        if (i == MIN_NODES)
            (List + MIN_NODES)->next_node = NULL;
    }
}

static void add_nodes(
    RNode *new_node,
    RNode *target_node // The new node follows the target node.
)
{
    RNode *tnodeptr = &target_node;
    RNode *nnodeptr = &new_node;

    nnodeptr->next_node = (tnodeptr + 1); // New.next_node points the target_node's next node.
    tnodeptr->next_node = nnodeptr;       // target_node.next_node points the new_node's location.
}

static void write_in_nodes(
    RNode *node,          // Target node.
    Vec3 Screen_Position, // Screen Coordinates: Cross, Height, Depth
    float u,
    float v // Texture Coordinates.
)
{
    node->Sposition = Screen_Position;
    node->u = u;
    node->v = v;
}
// This place stores rasterization function.
