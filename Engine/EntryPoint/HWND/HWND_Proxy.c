#include "HWND_Proxy.h"
#include <windows.h>

static HWND g_hwnd = NULL;

void platform_set_window(void *hwnd) // Tell the proxy which the HWND is.
{
    g_hwnd = (HWND)hwnd; // Get the
}

RSface render_query_surface(void) // Return the screen scales to CPU_Calculation.c.
{
    RSface surface = {0, 0}; // If HWND not created, then return 0 and raise an error.
    if (g_hwnd != NULL)
    {
        RECT client;
        if (GetClientRect(g_hwnd, &client))
        {
            surface.width = client.right - client.left;
            surface.height = client.bottom - client.top;
        }
    }
    else
    {
        return surface; // At this time, surface = False.
    }
    return surface; // At this time, surface = clinet size.
}