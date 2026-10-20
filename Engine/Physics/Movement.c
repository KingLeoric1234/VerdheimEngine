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
Vec3 simulate_movement(Sprite sprite, float Force, float Mass, float k, float dt, Vec3 dvector, float *velocity, float origin_velocity)
{
    // Prevent origin_velocity, velocity, director vector is NULL.
    if (!origin_velocity)
        origin_velocity = 0;

    if (!dvector.x && !dvector.y && !dvector.z)
    {
        dvector.x = 0;
        dvector.y = 0;
        dvector.z = 0;
    }

    if (!velocity)
        velocity = 0;

    if (!sprite.pixels && !Force && !Mass && !k && !dt)
    {
        Vec3 zero = {0, 0, 0};
        return zero; // Preventing to execute the code behind.
    }

    // TODO: This will write the frame counter and link the keycode press detecter.
    // The frame counter must link the keycode press detecter that works.
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
