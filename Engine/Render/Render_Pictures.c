// Display.c —— 把内存池里的精灵图显示到窗口正中心（第一版：单帧静止）
#include <windows.h>
#include "SpriteDefinition.h"

#define SCREEN_WIDTH 1920 // 屏幕尺寸，对应你的换算桥（世界中心 = 960/540）
#define SCREEN_HEIGHT 1080

static unsigned char *framebuffer = NULL; // 画板：1920×1080×4 ≈ 8MB（你16MB池全给sprite了，这块自己malloc）
static HWND hwnd_main = NULL;

// 第 1 站：贴图函数 —— 把 sprite 像素铺进画板
void renderer_draw_sprite(struct Sprite *s)
{
    int start_x = s->position.x + SCREEN_WIDTH / 2; // 换算桥：世界坐标 → 屏幕坐标
    int start_y = s->position.y + SCREEN_HEIGHT / 2;

    for (int row = 0; row < s->height; row++)
    {
        for (int col = 0; col < s->width; col++)
        {
            int dst = (start_y + row) * SCREEN_WIDTH + (start_x + col); // 画板位置
            int src = ((s->height - 1 - row) * s->width + col) * 3;     // sprite像素，行反转（BMP自底向上）

            if (dst >= 0 && dst < SCREEN_WIDTH * SCREEN_HEIGHT) // 越界保护
            {
                framebuffer[dst * 4 + 0] = s->pixels[src + 0]; // B
                framebuffer[dst * 4 + 1] = s->pixels[src + 1]; // G
                framebuffer[dst * 4 + 2] = s->pixels[src + 2]; // R
                framebuffer[dst * 4 + 3] = 255;                // A = 不透明
            }
        }
    }
}

// 第 2 站：搬运工 —— 把整块画板端上窗口（GDI 的 StretchDIBits）
void renderer_present(HDC hdc)
{
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = SCREEN_WIDTH;
    bmi.bmiHeader.biHeight = -SCREEN_HEIGHT; // 负号 = 从上往下存（配合你的 y 轴向下）
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    StretchDIBits(hdc, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                  0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                  framebuffer, &bmi, DIB_RGB_COLORS, SRCCOPY);
}

// 第 3 站：窗口的消息处理（窗口要重画 → 端画板上去）
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        renderer_present(hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0); // 关窗口 → 退出程序
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// 第 4 站：建窗口 + 消息循环（窗口活着就一直转）
void create_window_and_loop(struct Sprite *s)
{
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.lpszClassName = "VerdheimWindow";
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
    RegisterClassA(&wc);

    hwnd_main = CreateWindowA("VerdheimWindow", "VerdheimEngine",
                              WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT,
                              SCREEN_WIDTH, SCREEN_HEIGHT,
                              NULL, NULL, wc.hInstance, NULL);

    renderer_draw_sprite(s); // 先把精灵铺上画板（这张图不动，铺一次）
    framebuffer_clear();     // 背景涂黑（见下面）

    ShowWindow(hwnd_main, SW_SHOW);
    UpdateWindow(hwnd_main);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

// 背景涂黑：整块画板清零，不然窗口是花的
void framebuffer_clear(void)
{
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++)
        framebuffer[i * 4 + 0] = framebuffer[i * 4 + 1] =
            framebuffer[i * 4 + 2] = 0; // RGB 全 0 = 黑
}
