#ifndef HWNDPROXY_H
#define HWNDPROXY_H

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct Render_Surface
    {
        int width;  // HWND's Screen width.
        int height; // HWND's Screen height.
    } RSface;       // Render Surface.

    void platform_set_window(void *hwnd); // Tell the proxy which the hwnd winodw is.

    RSface render_query_surface(void);

#ifdef __cplusplus
}
#endif

#endif