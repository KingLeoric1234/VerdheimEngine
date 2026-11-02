#include <windows.h>
#include <wincodec.h>
#include <objbase.h>
#include <stdlib.h>

unsigned char *wic_load(const wchar_t *path, unsigned int *outW, unsigned int *outH)
{
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    IWICImagingFactory *factory = nullptr;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                                CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))))
        return nullptr;

    IWICBitmapDecoder *decoder = nullptr;
    if (FAILED(factory->CreateDecoderFromFilename(path, nullptr, GENERIC_READ,
                                                  WICDecodeMetadataCacheOnLoad, &decoder)))
    {
        factory->Release();
        return nullptr;
    }

    IWICBitmapFrameDecode *frame = nullptr;
    if (FAILED(decoder->GetFrame(0, &frame)))
    {
        decoder->Release();
        factory->Release();
        return nullptr;
    }

    UINT w = 0, h = 0;
    frame->GetSize(&w, &h);

    IWICFormatConverter *conv = nullptr;
    if (FAILED(factory->CreateFormatConverter(&conv)))
    {
        frame->Release();
        decoder->Release();
        factory->Release();
        return nullptr;
    }

    if (FAILED(conv->Initialize(frame, GUID_WICPixelFormat32bppRGBA,
                                WICBitmapDitherTypeNone, nullptr, 0,
                                WICBitmapPaletteTypeCustom)))
    {
        conv->Release();
        frame->Release();
        decoder->Release();
        factory->Release();
        return nullptr;
    }

    UINT stride = w * 4;
    UINT size = stride * h;
    unsigned char *pixels = (unsigned char *)malloc(size);
    if (!pixels)
    {
        conv->Release();
        frame->Release();
        decoder->Release();
        factory->Release();
        return nullptr;
    }

    if (FAILED(conv->CopyPixels(nullptr, stride, size, pixels)))
    {
        free(pixels);
        conv->Release();
        frame->Release();
        decoder->Release();
        factory->Release();
        return nullptr;
    }

    conv->Release();
    frame->Release();
    decoder->Release();
    factory->Release();

    *outW = w;
    *outH = h;
    return pixels;
}

// Create a window icon (title bar + taskbar) from an image file.
HICON wic_load_icon(const wchar_t *path)
{
    unsigned int w = 0, h = 0;
    unsigned char *pixels = wic_load(path, &w, &h);
    if (!pixels)
        return NULL;

    // A top-down 32-bit DIB whose alpha is premultiplied (required for icons).
    BITMAPV5HEADER bi = {0};
    bi.bV5Size = sizeof(bi);
    bi.bV5Width = (LONG)w;
    bi.bV5Height = -(LONG)h;
    bi.bV5Planes = 1;
    bi.bV5BitCount = 32;
    bi.bV5Compression = BI_BITFIELDS;
    bi.bV5RedMask = 0x00FF0000;
    bi.bV5GreenMask = 0x0000FF00;
    bi.bV5BlueMask = 0x000000FF;
    bi.bV5AlphaMask = 0xFF000000;

    void *bits = NULL;
    HBITMAP hbm = CreateDIBSection(NULL, (BITMAPINFO *)&bi, DIB_RGB_COLORS, &bits, NULL, 0);
    if (!hbm)
    {
        free(pixels);
        return NULL;
    }

    // Straight RGBA -> premultiplied BGRA.
    unsigned char *dst = (unsigned char *)bits;
    for (unsigned int i = 0; i < w * h; i++)
    {
        unsigned char r = pixels[i * 4 + 0];
        unsigned char g = pixels[i * 4 + 1];
        unsigned char b = pixels[i * 4 + 2];
        unsigned char a = pixels[i * 4 + 3];
        dst[i * 4 + 0] = (unsigned char)((unsigned int)b * a / 255);
        dst[i * 4 + 1] = (unsigned char)((unsigned int)g * a / 255);
        dst[i * 4 + 2] = (unsigned char)((unsigned int)r * a / 255);
        dst[i * 4 + 3] = a;
    }

    // CreateIconIndirect requires a mask the same size as the color bitmap.
    size_t nbytes = ((size_t)w * h + 7) / 8;
    unsigned char *zbuf = (unsigned char *)calloc(1, nbytes);
    HBITMAP mask = CreateBitmap((int)w, (int)h, 1, 1, zbuf);
    free(zbuf);

    ICONINFO ii = {0};
    ii.fIcon = TRUE;
    ii.hbmColor = hbm;
    ii.hbmMask = mask;
    HICON hicon = CreateIconIndirect(&ii);

    if (mask)
        DeleteObject(mask);
    DeleteObject(hbm);
    free(pixels);
    return hicon;
}