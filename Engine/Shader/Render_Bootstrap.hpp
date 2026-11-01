#ifndef RENDERBOOTSTRAP_H
#define RENDERBOOTSTRAP_H

#include <windows.h>

// Forward-declared D3D12 types so this header can share state without
// pulling in <d3d12.h> (which would need INITGUID defined first).
struct ID3D12Device;
struct ID3D12CommandQueue;
struct IDXGIFactory4;
struct IDXGISwapChain3;
struct ID3D12DescriptorHeap;
struct ID3D12Resource;
struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList;
struct ID3D12Fence;

// Shared bootstrap state, owned by Render_Bootstrap.cpp, read by the
// render-pipeline module (Render_Pipeline.cpp).
extern ID3D12Device *g_device;
extern ID3D12CommandQueue *g_queue;
extern IDXGIFactory4 *g_factory;
extern IDXGISwapChain3 *g_sc;
extern ID3D12DescriptorHeap *g_rtvH;
extern ID3D12Resource *g_back[2];
extern UINT g_rtvSz;
extern ID3D12CommandAllocator *g_alloc;
extern ID3D12GraphicsCommandList *g_list;
extern ID3D12Fence *g_fence;
extern HANDLE g_event;
extern UINT64 g_fv;
extern int g_winW, g_winH;

// Entry points.
int  d3d12_bootstrap(HWND hwnd, int w, int h); // Registration only.
void d3d12_frame(void);                        // Per-frame rendering, in Render_Pipeline.cpp.

// Render-pipeline gates, called from d3d12_bootstrap after the core gates.
bool gate_pipeline(void);
bool gate_vbuffer(void);

#endif // RENDERBOOTSTRAP_H
