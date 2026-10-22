#include "KeyCode_Presser.h"
#include <stdio.h>

#define KEYDOWN 8 // Max key codes we record per frame.

static BYTE keyState[256];
static BYTE pressedList[KEYDOWN];
static int pressedCount;

void KeyCOde_Presser_OnKeyDown(BYTE vkey)
{
    keyState[vkey] = 1;
    if (pressedCount < KEYDOWN)
        pressedList[pressedCount++] = vkey;
}

void KeyCode_Presser_OnKeyUP(BYTE vkey)
{
    keyState[vkey] = 0;
}

void KeyCode_Presser_FrameReset(void)
{
    pressedCount = 0;
}

const BYTE *KeyCode_Presser_GetList(int *count)
{
    *count = pressedCount;
    return pressedList
}

// Pseudo code
Begin : One frame.

        void get_key_pressed[KEYDOWN]()
{
    add keyState[1];
    add keyState[2];
    ... add keyState[8];
}

void *get_key_pressed = &get_key_pressed;

End One Frame.

    void
    get_key_released(void *get_key_pressed)
{
    *get_key_pressed[KEYDOWN] = 0; // Release the key code, prepare for next frame.
}

// Player:
if get_key_pressed
[KEYDOWN] exists keyState[1]
{
    execute some codes;
}