#include <windows.h>
#include "HWND.h"

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
    const int win_h = 780;
    int screen_w = GetSystemMetrics(SM_CXSCREEN);
    int screen_h = GetSystemMetrics(SM_CYSCREEN);
    int x = (screen_w - win_w) / 2;
    int y = (screen_h - win_h) / 2;

    HWND hwnd = CreateWindowExW(
        0,
        CLASS_NAME,   // Register name.
        L"Veridheim", // Header's name.
        WS_POPUP,     // Windows style: No titles.
        x, y,
        win_w, win_h,
        NULL,
        NULL,
        hInstance,
        NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
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
            listen_keyvalue(msg);   // File: KeyCode_Presser.c, Listening the key values, record them into a array.
        }
    }
}