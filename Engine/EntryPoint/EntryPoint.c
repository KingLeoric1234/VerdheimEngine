#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "HWND\HWND_Proxy.h"                    // Seperate the system's API to core programme, faciliating transplant.
#include "..\InitializeCheck\InitializeCheck.h" // Allocate memories.
#include "HWND\HWND.h"                          // Handle the Window.
#include "HWND\GPU_Rendering.h"              // Rendering Pipeline.

// Engine on/off switch: when false, the engine stays shut down and skips the loop.
bool programme_execution = true;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    // Initialize the engine first, so the programme runs correctly.
    if (programme_execution) // InitializeCheck.c File
    {
        allocate_memory(); // Allocate memorys for sprites and screen.

        initialize_resource_memory(); // Initialize the resources path for engine, facilitate to find.

        mark_initialization_complete(); // If initialization succeed, then mark the "True" for next step.
    }

    // Now bring up the window and enter the frame loop.
    HWND hwnd = RegisterWindow(hInstance, nCmdShow);
    if (hwnd == NULL)
    {
        MessageBoxA(NULL, "Failed to create window.", "VerdheimEngine", MB_OK | MB_ICONERROR);
        return 1; // The programme runs into error.
    }

    platform_set_window(hwnd); // Let the Proxy know what the main window is. Filename: "HWND_Proxy.h".

    FrameLoop(); // Loop the window, the WinProc get the message and send it to sprite loop (Game sprite loop).
    return 0;
}
