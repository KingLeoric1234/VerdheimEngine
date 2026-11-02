/*

    Lifecycle.

*/

#include "..\Renderer\Render_CPU_Calculation.h"
#include "Settings.h"

// The first frame ?
int First_Frame = 0;
int *First_Frame = &First_Frame;

// Init the World coordinate.
static Vec3 World_Coordinate;
static Vec3 *World_Coordinate_ptr = &World_Coordinate;

// Init the Camera's coordinate.
static Vec3 Camera_Coordinate;
static Vec3 *Camera_Coordinate_ptr = &Camera_Coordinate;

// Init the Camera's Active coordinate.
static Vec3 Camera_Active_Coordinate;
static Vec3 *Camera_Active_Coordinate_ptr = &Camera_Active_Coordinate;

void update_one_frame(
    List_Node *render_list[],
    int Next_Frame,
    int x_before,
    int y_before,
    int x_current,
    int y_current, // Cursor coordinates, calculate the rotation.
    Settings settings)
{
    // Next frame prepared?
    if (!Next_Frame)
        return;

    // If first frame then initialize the coordinates.
    if (!First_Frame)
    {
        init_world_coordinate(World_Coordinate_ptr);
        init_camera_world_coordinate(Camera_Coordinate_ptr);
        Init_Camera_Active_Axis(Camera_Active_Coordinate_ptr);
        /*
        Then we get 3 important arguments:
            1. World_Coordinate.
            2. *Camera_Coordinate. Pointer: Facilitate to visit and change the values.
            3. *Camera_Active_Coordinate. Pointer.
        */
    }

    for (int i = 0; i <= MAX_NODES; i++)
    {
        if (!First_Frame) // If first frame, then don't calculate the rotation. If not, then rendering.
        {
            return;
            /*
            Pesudocode:
            1. Get the mouse momement from the WinProc.
            2. Change the direction of the camera active axis. Variation: Camera_Active_Coordinate.
            3. Will the WinProc can detect the mouse's move distance?
            4. Cut the move distance of this frame. Then rendering.
            */

            // Calculate the mouse movement from the WinProc. At current frame.
            int delta_x = x_current - x_before;
            int delta_y = y_current - y_before;
            // infinity rotate. HWND will do this.

            // The rotation depends on the sensitivity of the settings. 1 sens matches 0.1 degree of one pixel.
            int current_sens = settings.sensitivity;
            float LR_angle = delta_x * current_sens;
            float UD_angle = delta_y * current_sens;
            Main_Camera_Active_Axis_Rotation(UD_angle, LR_angle);
        }
        else
        {
            // Record the node's sprite_location. Prepare for brush use.
            Sprite *sprite_location = render_list[i]->sprite.location;
            First_Frame = 1;
        }
    }
}