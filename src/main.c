// ============================================================
// VerdheimEngine — minimal engine skeleton
//
// The whole game is one loop:
//     while (window open) { read input; update world; draw screen; }
// This file is the first pillar: a character-frame loop that shows
// "update state, then draw" frame by frame.
// ============================================================

#include <stdio.h>

#define FRAME_COUNT 5

int main(void)
{
    // ---- Init ----
    int frame = 0;
    int x = 0;            // world state: the dot's x position

    // ---- Loop ----
    while (frame < FRAME_COUNT)
    {
        // Update: world state advances by one step
        x += 1;

        // Draw: print the state to the character screen
        printf("frame %d: ", frame);
        for (int i = 0; i < x; i++) printf(".");
        printf("o\n");

        frame++;
    }

    // ---- Close ----
    return 0;
}
