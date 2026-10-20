#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <cstdint>
#include <d2d1.h>                                  // Direct2D.
#include <wrl/client.h>                            // ComPtr.
#include "GPU_Rendering.h"                         // Link the originial header file.
#include "..\..\Render\SpriteDefinition.h"         // Needs to use the Sprite struct.
#include "..\..\Render\CPU_Calculation.h"          // Needs to use the CPU-calculated vertex and scales to render the sprites.
#include "..\..\InitializeCheck\InitializeCheck.h" // Needs to use RESOURCE_PATH #define.
#include "HWND_Proxy.h"                            // Needs window scales (RSface) to render.

// Function: Load the BMP file into the "Sprite".
// Return 0 == succeed, 1 == failed.
extern "C" int load_bmp_to_sprite(const char *filepath, Sprite *out)
{
    // Read the file in binary. filepath == sprite path.
    FILE *f = fopen(filepath, "rb");
    if (!f)
        return -1;

    // Read the file's header.
    BITMAPFILEHEADER fh;
    fread(&fh, sizeof(fh), 1, f);
    if (fh.bfType != 0x4D42)
    {
        fclose(f);
        return -1;
    } // If not BMP, then return 0 to report the file frame error.

    // Read the file's header.
    BITMAPINFOHEADER ih;
    fread(&ih, sizeof(ih), 1, f);

    // Jumop to the pixels location and read them.
    fseek(f, fh.bfOffBits, SEEK_SET);

    // Allocate memories for BMP image from the big memory pool.
    int w = ih.biWidth;
    int h = ih.biHeight;
    out->width = w;
    out->height = h;

    // Allocate memorys for image in the allocated memories in the initialization range.
    out->pixels = (unsigned char *)sprite_memory_alloc(w * h * 3);
    if (out->pixels == NULL)
    {
        fclose(f);
        return -1; // Out of memory.
    }

    // Read the pixels's data.
    unsigned char *row = (unsigned char *)malloc(w * 3);
    for (int y = h - 1; y >= 0; y--)
    {
        fread(row, w * 3, 1, f);
        memcpy(out->pixels + y * w * 3, row, w * 3);
    }
    free(row);
    fclose(f);

    return 0;
}
//////////////////////////////

// Intelligent Pointer.
using Microsoft::WRL::ComPtr;

// Global Intelligent Pointer.
static ComPtr<ID2D1Factory> g_factory;     // D2D Factory: Only one.
static ComPtr<ID2D1HwndRenderTarget> g_rt; // Render Target: Bind the HWND graphic. Let the GPU know where needs to render.

extern "C" int render_init(void *hwnd)
{
    // Create the Factory.
    HRESULT hr = D2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED,
        g_factory.GetAddressOf());

    // Check whether the factory set.
    if (FAILED(hr))
        return 1;

    // Get the scales of the HWND windows: HWND's height and HWND's width.
    RSface surface = render_query_surface();

    // Package which window fits and the scales of the graphic.
    D2D1_HWND_RENDER_TARGET_PROPERTIES hwnd_props =
        D2D1::HwndRenderTargetProperties((HWND)hwnd,
                                         D2D1::SizeU((UINT32)surface.width, (UINT32)surface.height));

    // Check out whether the graphic renderer completed.
    hr = g_factory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        hwnd_props,
        g_rt.ReleaseAndGetAddressOf());

    if (FAILED(hr))
        return 1; // Graphic renderer failed to bind the renderer into the HWND.

    return 0; // Exit Normally.
}

extern "C" void render_begin(void)
{
    if (g_rt)
        g_rt->BeginDraw(); // Begin to draw the sprites into the window.
}

extern "C" void render_draw(const Sprite *s)
{
    // Check out whether the g_rt, Sprite, Sprite address empty. If empty, then return nothing.
    if (!g_rt || !s || !s->pixels)
        return;

    std::vector<std::uint8_t> bgra(s->width * s->height * 4);
    for (int i = 0; i < s->width * s->height; ++i)
    {
        bgra[i * 4 + 0] = s->pixels[i * 3 + 0]; // B
        bgra[i * 4 + 1] = s->pixels[i * 3 + 1]; // G
        bgra[i * 4 + 2] = s->pixels[i * 3 + 2]; // R
        bgra[i * 4 + 3] = 255;                  // A
    }

    D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE));

    ComPtr<ID2D1Bitmap> bmp;
    HRESULT hr = g_rt->CreateBitmap(
        D2D1::SizeU((UINT32)s->width,
                    (UINT32)s->height),
        bgra.data(),
        s->width * 4,
        props,
        bmp.GetAddressOf());

    if (FAILED(hr))
        return;

    D2D1_RECT_F dst = D2D1::RectF(
        (FLOAT)s->position.x, (FLOAT)s->position.y,
        (FLOAT)s->position.x + s->width,
        (FLOAT)s->position.y + s->height);

    g_rt->DrawBitmap(
        bmp.Get(),
        dst,
        1.0f,
        D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
        D2D1::RectF(0.0f, 0.0f, 1.0f, 1.0f));
}

extern "C" void render_end(void)
{
    if (g_rt)
    {
        g_rt->EndDraw(); // EndDraw finishes the presentation (D2D has no Present method).
    }
}

extern "C" void render_shutdown(void)
{
    g_rt.Reset();
    g_factory.Reset();
}