#include <stdio.h>
#include <math.h>
#include "..\Render\SpriteDefinition.h"
#include "..\Tick\Update.h"

Vec3 Normalize(Vec3 dvector); // Declearify the Normalize function facilitate to use it.

// The coordinate changes instantly, without acceleration and deceleration.
Vec3 transform_movement(Sprite sprite, float speed, Vec3 dvector) // "dvector": diretion vector, calculate the direction speed.
{
    Vec3 sprite_position;
    sprite_position = sprite.position;
    float frame_rendering_time_ms_float = 1.0f / *p_frame; // (s/ms). We get the 0.005ms/frame.

    if (dvector.x || dvector.y || dvector.z)
        Normalize(dvector); // If dvector != 0, then Normalize the vector.

    // Speed calculation, transform the position instantly.
    sprite_position.x -= frame_rendering_time_ms_float * speed * dvector.x;
    sprite_position.y -= frame_rendering_time_ms_float * speed * dvector.y;
    sprite_position.z -= frame_rendering_time_ms_float * speed * dvector.z;

    // Output the instant position, print the instant image on the screen.
    return sprite_position;
}

// The coordinate changing follows differential equation, simulating the actual movement.
Vec3 simulate_movement(Sprite sprite, Vec3 input, float acceleration, float deceleration, float dt)
{
    Vec3 sprite_position;
    sprite_position = sprite.position;
    float frame_rendering_time_ms_float = 1.0f / *p_frame; // (s/ms). We get the 0.005ms/frame.

    // Check the acceleration and deceleration, facilitates to add the acceleration and deceleration.
    if (!deceleration)
        deceleration = acceleration;
    else if (!acceleration)
        acceleration = deceleration;
    else
        acceleration = deceleration = 0;

    // Acceleration. "dt" = frame_rendering_time_ms_float.
    sprite.position.x += input.x * acceleration * dt;
    sprite.position.y += input.y * acceleration * dt;
    sprite.position.z += input.z * acceleration * dt;
}

Vec3 Normalize(Vec3 dvector) // Normalize the direction vector.
{
    float length = sqrtf(dvector.x * dvector.x + dvector.y * dvector.y + dvector.z * dvector.z);

    // Prevent to divide 0.
    if (length > 0.0001f)
    {
        dvector.x /= length;
        dvector.y /= length;
        dvector.z /= length;
    }
    return dvector;
}
