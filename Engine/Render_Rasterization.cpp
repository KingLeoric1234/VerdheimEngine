/*
Pesudocode 1: Overall prgramme
Rasterization ended, GPU put out the Array P.
Read
    Array P[1920 x 1080]
for every elements in Array P
    if current pixel gets Rasterization?
        No, read the next pixel;
        Yes, Z-buffer and Next Step;
    P[elements]:
        UV coloring
=============================================
Pesudocode 2: Data Structure: Linked List.
Initialize:
    original linked list which nodes' contains nothing and the "next" pointer contains next node's location.
        We get Pixel1-Pixel2-....-Pixel2073600 long linked list.
Begin Rasterization.
    Input:
        Tools: 1. create nodes and write nodes.
               2. add nodes into the correct place.
        Container: 1. Initialized linked list.
    Rasterization:
        Gets the initialized empty linked list.
        for (i = 0; i < prepared to rasterized triangles; i++)
            // Rasterizing one triangle.
            Create new nodes and write in nodes for every rasterized pixels, every pixels contains xyz, uv.
            We get lots of new_nodes.
            // Question: How to record the unsorted new_nodes with pointers.
            // Link the nodes.
            for (i = 0, i < length(new_nodes), i++) // get a new_node and insert into the initialized list.
                nodes_x = new_nodes.x;
                nodes_y = new_nodes.y;
                // Traverse the linked list.
                for (i = 0; i < List.elements; i++)
                    if(i == 0)  // Get the beginner pointer at the first of the loop.
                        *current_node = &beginner;
                    if (nodes_x == current_node->x && nodes_y == current_node->y && current_node->z == NULL)   // Find the correct place. And the place is NULL.
                        current_node->x = new_node.x;
                        current_node->y = new_node.y;
                        current_node->z = new_node.z;
                        current_node->u = new_node.u;
                        current_node->v = new_node.v;
                        // Pointer will not be changed, cause we use the initialized list.
                    else if (nodes_x == current_node->x && nodes_y == current_node->y && current_node->z != NULL)   // The initialization filled with pixels.
                        current_node->next = &new_node;
                        *current_node = current_node->next; // Transfer into next node.
    Output: The linked list with severals of pixels information.
            beginner-[pixels 1: (A - B)]-[pixels 2: (A - B - C - D)]-[pixels 3: (A)]-......-[pixels 2073600: (A - B)]-end.

*/

/*

    Warning: "-1" will be "NULL" return for integer and float.
            See the "initialize_pixels_array" function.

*/
#include "Render_Vertex_Shader.hpp"
#include "Render_Pixel_Shader.hpp"

// Initialize the nodes.
void initialize_pixels_array(PAElement Pixels_Array[])
{
    for (int i = 0; i < SCREEN_PIXELS; i++)
    {
        Pixels_Array[i].Sposition.x = -1;
        Pixels_Array[i].Sposition.y = -1;
        Pixels_Array[i].Sposition.z = -1;

        Pixels_Array[i].u = -1;
        Pixels_Array[i].v = -1;
    }
}

// fill the node
// connect the following nodes when node filled.
void fill_the_node(
    PAElement listed_array[],
    PAElement *element)
{
    // Hash searching.
    int screen_x = element->Sposition.x; // Begin with 0. Must be integer.
    int screen_y = element->Sposition.y; // Begin with 0. Must be integer.

    // Searching the correct element.
    if (listed_array[screen_y * SCREEN_WIDTH + screen_x].u != -1 &&
        listed_array[screen_y * SCREEN_WIDTH + screen_x].Sposition.x != -1 &&
        listed_array[screen_y * SCREEN_WIDTH + screen_x].pointer == NULL) // If filled the parent node and the pointer NULL. Follow the parent node.
        listed_array[screen_y * SCREEN_WIDTH + screen_x].pointer = element;
    else if (listed_array[screen_y * SCREEN_WIDTH + screen_x].u == -1 &&
             listed_array[screen_y * SCREEN_WIDTH + screen_x].Sposition.x == -1) // If parent node NULL, fill the parent node.
    {
        listed_array[screen_y * SCREEN_WIDTH + screen_x].Sposition = element->Sposition;
        listed_array[screen_y * SCREEN_WIDTH + screen_x].u = element->u;
        listed_array[screen_y * SCREEN_WIDTH + screen_x].v = element->v;
    }
    else if (listed_array[screen_y * SCREEN_WIDTH + screen_x].pointer != NULL)
    // If parent node not NULL, and the pointer not NULL,
    // then transfer the parent pointer to point the new node,
    // the new node points old node at the same time.
    {
        element->pointer = listed_array[screen_y * SCREEN_WIDTH + screen_x].pointer;
        listed_array[screen_y * SCREEN_WIDTH + screen_x].pointer = element;
    }
}

PAElement create_new_node(
    Vec3 Current_Screen_Position,
    float Current_u,
    float Current_v)
{
    PAElement current_element;
    PAElement *current_element_ptr;
    current_element_ptr = &current_element;

    current_element_ptr->Sposition = Current_Screen_Position;
    current_element_ptr->u = Current_u;
    current_element_ptr->v = Current_v;
    return current_element;
}
// Create new nodes when array filled.