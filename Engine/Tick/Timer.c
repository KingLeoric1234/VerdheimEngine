/*

Timer: Simulate the continuous frame: 200 frames/s, calculate the frame's passing time.

*/

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "Timer.h"
#include "Update.h"
#include "..\EntryPoint\HWND\HWND.h"

int64_t game_total_time_calculation()
{
    int frame_summary = *frame_counter_ptr;
    int minimum_time = ceil(1000 / *p_frame);      // dimension: ms, ceil the time prevent to get zero.
    int total_time = frame_summary * minimum_time; // dimension: ms.
    return total_time;                             // dimension: ms.
}

float minimum_time()
{
    return ceil(1000 / *p_frame);
}