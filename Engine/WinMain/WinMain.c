#include <windows.h>
#include "Render_HWND.h"

// Engine's public vertex type, mirrored here so the C side can build geometry.
struct V
{
    float x, y, z, u, v;
};

// Engine's public draw API, implemented in Render_Pipeline.cpp.
void brush(const wchar_t *hlsl_file, const wchar_t *img_file,
           const V *verts, unsigned int vert_count);

int d3d12_bootstrap(HWND hwnd, int w, int h);
void d3d12_frame(void);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow)
{
    HWND hwnd = RegisterWindow(hInstance, nCmdShow);
    int err = d3d12_bootstrap(hwnd, 1280, 720);
    if (err != 0) // Non-zero exit code = the failing bootstrap step.
        return err;

    // One brush call sets everything the first frame draws: rule, image, geometry.
    V quad[6] = {
        {-0.25f, 0.25f, 0.5f, 0.0f, 0.0f},
        {0.25f, 0.25f, 0.1f, 1.0f, 0.0f},
        {-0.25f, -0.25f, 0.5f, 0.0f, 1.0f},
        {-0.25f, -0.25f, 0.5f, 0.0f, 1.0f},
        {0.25f, 0.25f, 0.1f, 1.0f, 0.0f},
        {0.25f, -0.25f, 0.1f, 1.0f, 1.0f},
    };
    brush(L"Engine\\Shader\\Render_shader.hlsl",
          L"Resources\\tile_0001.bmp", quad, 6);

    FrameLoop();
    return 0;
}
