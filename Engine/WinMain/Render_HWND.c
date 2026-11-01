/*

    Windows Handle.

*/

#include <windows.h>
#include <dwmapi.h>
#include "Render_HWND.h"
#include "Debug_Panel.h"
#include "..\Shader\Render_Bootstrap.hpp"

static MSG msg;
int frame_counter;                       //  Record the frames passing.
int *frame_counter_ptr = &frame_counter; //  Record the frames passing pointer, facilitates to change the frame counter and record the time.

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) // Define the event function.
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0); // Let GetMessage receive the WM_QUIT.
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
            DestroyWindow(hwnd);

        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_SIZING:
    {
        // Lock the window to a 16:9 aspect ratio (1920x1080) while dragging.
        RECT *rc = (RECT *)lParam;
        switch (wParam)
        {
        case WMSZ_LEFT:
            rc->left = rc->right - (rc->bottom - rc->top) * 16 / 9;
            break;
        case WMSZ_RIGHT:
            rc->right = rc->left + (rc->bottom - rc->top) * 16 / 9;
            break;
        case WMSZ_TOP:
            rc->top = rc->bottom - (rc->right - rc->left) * 9 / 16;
            break;
        case WMSZ_BOTTOM:
            rc->bottom = rc->top + (rc->right - rc->left) * 9 / 16;
            break;
        case WMSZ_TOPLEFT:
            rc->top = rc->bottom - (rc->right - rc->left) * 9 / 16;
            break;
        case WMSZ_TOPRIGHT:
            rc->top = rc->bottom - (rc->right - rc->left) * 9 / 16;
            break;
        case WMSZ_BOTTOMLEFT:
            rc->bottom = rc->top + (rc->right - rc->left) * 9 / 16;
            break;
        case WMSZ_BOTTOMRIGHT:
            rc->bottom = rc->top + (rc->right - rc->left) * 9 / 16;
            break;
        }
        return TRUE;
    }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// Register the HWND.
static const wchar_t CLASS_NAME[] = L"VerdheimEngine";

HWND RegisterWindow(HINSTANCE hInstance, int nCmdShow)
{
    // Clear the WNDCLASSEXW struct.
    WNDCLASSEXW wc = {0};

    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); // Default console arrow.
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = CLASS_NAME;

    RegisterClassExW(&wc);

    // Pin the window to the center of the primary monitor.
    const int win_w = 1280;
    const int win_h = 720;
    int screen_w = GetSystemMetrics(SM_CXSCREEN);
    int screen_h = GetSystemMetrics(SM_CYSCREEN);
    int x = (screen_w - win_w) / 2;
    int y = (screen_h - win_h) / 2;

    HWND hwnd = CreateWindowExW(
        0,
        CLASS_NAME,  // Register name.
        L"Verdheim", // Header's name.
        WS_OVERLAPPEDWINDOW,  // Standard window: title bar + border.
        x, y,
        win_w, win_h,
        NULL,
        NULL,
        hInstance,
        NULL);

    // Enable the modern dark title bar (DWM immersive dark mode).
    BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    create_debug_panel(hwnd);   // Embedded Debug panel on the right side.
    return hwnd;
}

void FrameLoop(void)
{
    BOOL running = TRUE; //  Record the program running state, if running, running == True.

    while (running) // When running, display the window and begin the frame.
    {
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) // WM_QUIT == Quit the hwnd windows.
            {
                running = FALSE; // Quit the frame and programme.
                break;
            }
            TranslateMessage(&msg); // Translate the message into WN_CHAR.
            DispatchMessageW(&msg); // Send to the WndProc.

            // listen_keyvalue(msg);   // File: KeyCode_Presser.c, Listening the key values, record them into a array.
        }

        // While a Debug command runs, skip rendering so the progress
        // window's messages (mouse, timer, cancel) are handled promptly.
        if (!debug_panel_busy())
            d3d12_frame();
        else
            Sleep(1);
    }
}
