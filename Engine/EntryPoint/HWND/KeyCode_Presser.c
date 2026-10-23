#include "KeyCode_Presser.h"
#include <stdio.h>

#define KEYDOWN 6 // Max key codes we record per frame.(0-5)

static BYTE keyState[256]; // Key pressed array
static BYTE pressedArray[KEYDOWN];
static int pressedCount;
short array_length = 0; // initialize the array_length == 0. Simplify the calculation.

// Define an array to store the key pressed data.
BYTE insert_keyvalue(MSG msg)
{
    if (array_length >= KEYDOWN) // More than KEYDOWN calculator then return -1.
        return 0;                // The array full.

    for (short i = 0; i <= KEYDOWN; i++)
    {
        if (!pressedArray[i])
            pressedArray[i] = msg.wParam; // insert the key_state.
        array_length += 1;
    }
    return pressedArray[KEYDOWN];
}

BYTE delete_keyvalue()
{
    for (short i = 0; i <= KEYDOWN; i++)
    {
        if (pressedArray[i])
        {
            pressedArray[i] = NULL;
        }
    }
}

void listen_keyvalue(MSG msg)
{
    if (msg.message == WM_KEYDOWN && msg.wParam)
    {
        insert_keyvalue(msg);
    }
}

int search_keystates(BYTE target)
{
    for (int i = 0; i < array_length; i++)
    {
        if (pressedArray[i] == target)
            return 1; // We get the target and return 1 indicates that target in the array.
    }
}