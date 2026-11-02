/*
    Render_Bootstrap.cpp

    Registration only : create the device, the command queue, the
    swapchain, the render-target view heap, the command allocator/list, and the
    fence. Nothing here draws anything; all rendering lives in Render_Pipeline.cpp.

    The render-pipeline gates (gate_pipeline / gate_vbuffer) are created by
    d3d12_bootstrap after the core gates, so the whole chain is one deterministic
    path before the first frame.
*/

#define INITGUID
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <windows.h>
#include <cstring>
#include "Render_Bootstrap.hpp"

// Bootstrap state. Non-static so the render-pipeline module can read it via
// the extern declarations in Render_Bootstrap.hpp.

// Device and command queue.
// Device, Create a device to receive the command.
// Queue: Send the command into the GPU pipeline.
ID3D12Device *g_device = nullptr;

// Create a swap chain and a buffer.
// DXGI factory, create the swap chain.
// g_sc: The swap chain which created. sc: Swap Chain.
// g_back[2]: 2 buffer resources behind the device, draw per frame.
// g_rtvH
IDXGIFactory4 *g_factory = nullptr;
IDXGISwapChain3 *g_sc = nullptr;
ID3D12DescriptorHeap *g_rtvH = nullptr;
ID3D12Resource *g_back[2] = {};
UINT g_rtvSz = 0;

// Command record.
ID3D12CommandAllocator *g_alloc = nullptr;   // Where to write.
ID3D12GraphicsCommandList *g_list = nullptr; // What to write.
ID3D12CommandQueue *g_queue = nullptr;       // Who to execute.

ID3D12Fence *g_fence = nullptr;
HANDLE g_event = nullptr;
UINT64 g_fv = 0;
int g_winW = 0, g_winH = 0;

static bool gate_device_queue(void)
{
    // Enable the D3D12 debug layer so silent runtime errors surface instead
    // of being swallowed; break on any ERROR-severity message.
    ID3D12Debug *dbg = nullptr;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_ID3D12Debug, (void **)&dbg)))
    {
        dbg->EnableDebugLayer();
        dbg->Release();
    }
    if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0,
                                 IID_ID3D12Device, (void **)&g_device)))
        return false;
    // Fail fast on any D3D12 ERROR so silent issues never hide again.
    ID3D12InfoQueue *iq = nullptr;
    if (SUCCEEDED(g_device->QueryInterface(IID_ID3D12InfoQueue, (void **)&iq)))
    {
        iq->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        iq->Release();
    }
    D3D12_COMMAND_QUEUE_DESC q = {};
    q.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    q.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    if (FAILED(g_device->CreateCommandQueue(&q, IID_ID3D12CommandQueue, (void **)&g_queue)))
        return false;
    return true;
}

static int gate_swapchain(HWND hwnd, int w, int h)
{
    if (FAILED(CreateDXGIFactory(IID_IDXGIFactory4, (void **)&g_factory)))
        return 4011;
    DXGI_SWAP_CHAIN_DESC sc = {};
    sc.BufferCount = 2;
    sc.BufferDesc.Width = (UINT)w;
    sc.BufferDesc.Height = (UINT)h;
    sc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sc.OutputWindow = hwnd;
    sc.SampleDesc.Count = 1;
    sc.Windowed = TRUE;
    sc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // Recommended D3D12 mode.
    IDXGISwapChain *base = nullptr;
    if (FAILED(g_factory->CreateSwapChain(g_queue, &sc, &base)))
        return 4012;
    if (FAILED(base->QueryInterface(IID_IDXGISwapChain3, (void **)&g_sc)))
        return 4013;
    return 0;
}

static bool gate_rtv(void)
{
    D3D12_DESCRIPTOR_HEAP_DESC hd = {};
    hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    hd.NumDescriptors = 2;
    if (FAILED(g_device->CreateDescriptorHeap(&hd, IID_ID3D12DescriptorHeap, (void **)&g_rtvH)))
        return false;
    g_rtvSz = g_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE cpu = g_rtvH->GetCPUDescriptorHandleForHeapStart();
    for (UINT i = 0; i < 2; i++)
    {
        if (FAILED(g_sc->GetBuffer(i, IID_ID3D12Resource, (void **)&g_back[i])))
            return false;
        g_device->CreateRenderTargetView(g_back[i], nullptr, cpu);
        cpu.ptr += g_rtvSz;
    }
    return true;
}

static bool gate_commands(void)
{
    if (FAILED(g_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                IID_ID3D12CommandAllocator, (void **)&g_alloc)))
        return false;
    if (FAILED(g_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_alloc,
                                           nullptr, IID_ID3D12GraphicsCommandList, (void **)&g_list)))
        return false;
    g_list->Close();
    return true;
}

static bool gate_fence(void)
{
    if (FAILED(g_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_ID3D12Fence, (void **)&g_fence)))
        return false;
    g_event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    return true;
}

int d3d12_bootstrap(HWND hwnd, int w, int h)
{
    // Remember the window size; the viewport needs it in d3d12_frame.
    g_winW = w;
    g_winH = h;
    // Each gate returns false on failure; return a distinct error code
    // so the caller can tell exactly which step failed.
    if (!gate_device_queue())
        return 401;
    int sc = gate_swapchain(hwnd, w, h); // Returns 0 or 4011/4012/4013.
    if (sc != 0)
        return sc;
    if (!gate_rtv())
        return 403;
    if (!gate_commands())
        return 404;
    if (!gate_pipeline()) // Render_Pipeline.cpp
        return 405;
    if (!gate_vbuffer()) // Render_Pipeline.cpp
        return 406;
    if (!gate_fence())
        return 407;
    return 0;
}
