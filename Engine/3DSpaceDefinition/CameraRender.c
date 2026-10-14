#include <stdio.h>
#include <stdbool.h>

// Clamp the screen's size. If needs to upload the telephone, this needs to be elasticly.
#define SCREEN_WIDTH 1080
#define SCREEN_HEIGHT 1920

// Define the elements on the screen. Fits 1920x1080 (16:9).
#define DRAW_X 32
#define DRAW_Y 18
#define DRAW_Z 5

// Define the actual render elements on the screen, prevent missing render objects.
#define ACTUAL_DRAW_X DRAW_X + 2
#define ACTUAL_DRAW_Y DRAW_Y + 2

int space_elements[ACTUAL_DRAW_X * ACTUAL_DRAW_Y * DRAW_Z]; // Define the
