#include <windows.h>
#include <wincodec.h>
#include <objbase.h>

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