#define INITGUID
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <windows.h>
#include <cstring>

static ID3D12Device *g_device = nullptr;
static ID3D12CommandQueue *g_queue = nullptr;
static IDXGIFactory4 *g_factory = nullptr;
static IDXGISwapChain3 *g_sc = nullptr;
static ID3D12DescriptorHeap *g_rtvH = nullptr;
static ID3D12Resource *g_back[2] = {};
static UINT g_rtvSz = 0;
static ID3D12CommandAllocator *g_alloc = nullptr;
static ID3D12GraphicsCommandList *g_list = nullptr;
static ID3D12Fence *g_fence = nullptr;
static HANDLE g_event = nullptr;
static UINT64 g_fv = 0;
static ID3D12RootSignature *g_root = nullptr;
static ID3D12PipelineState *g_pso = nullptr;
static ID3D12Resource *g_vb = nullptr;

struct V
{
    float x, y, z;
};

static bool gate_device_queue(void)
{
    if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0,
                                 IID_ID3D12Device, (void **)&g_device)))
        return false;
    D3D12_COMMAND_QUEUE_DESC q = {};
    q.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    q.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    if (FAILED(g_device->CreateCommandQueue(&q, IID_ID3D12CommandQueue, (void **)&g_queue)))
        return false;
    return true;
}

static bool gate_swapchain(HWND hwnd, int w, int h)
{
    if (FAILED(CreateDXGIFactory(IID_IDXGIFactory4, (void **)&g_factory)))
        return false;
    DXGI_SWAP_CHAIN_DESC sc = {};
    sc.BufferCount = 2;
    sc.BufferDesc.Width = (UINT)w;
    sc.BufferDesc.Height = (UINT)h;
    sc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sc.OutputWindow = hwnd;
    sc.SampleDesc.Count = 1;
    sc.Windowed = TRUE;
    IDXGISwapChain *base = nullptr;
    if (FAILED(g_factory->CreateSwapChain(g_queue, &sc, &base)))
        return false;
    if (FAILED(base->QueryInterface(IID_IDXGISwapChain3, (void **)&g_sc)))
        return false;
    return true;
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

static bool gate_pipeline(void)
{
    D3D12_ROOT_SIGNATURE_DESC rs = {};
    rs.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    ID3DBlob *sig = nullptr, *err = nullptr;
    if (FAILED(D3D12SerializeRootSignature(&rs, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &err)))
        return false;
    if (FAILED(g_device->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(),
                                             IID_ID3D12RootSignature, (void **)&g_root)))
        return false;

    const char *vs = "void VS(float3 p:POSITION, out float4 o:SV_POSITION){o=float4(p,1);}";
    const char *ps = "float4 PS():SV_TARGET{return float4(1,0,0,1);}";
    ID3DBlob *vB = nullptr, *pB = nullptr;
    D3DCompile(vs, strlen(vs), "vs", 0, 0, "VS", "vs_5_0", 0, 0, &vB, &err);
    D3DCompile(ps, strlen(ps), "ps", 0, 0, "PS", "ps_5_0", 0, 0, &pB, &err);

    D3D12_INPUT_ELEMENT_DESC lay = {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
                                    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};
    D3D12_GRAPHICS_PIPELINE_STATE_DESC p = {};
    p.pRootSignature = g_root;
    p.VS = {vB->GetBufferPointer(), vB->GetBufferSize()};
    p.PS = {pB->GetBufferPointer(), pB->GetBufferSize()};
    p.InputLayout = {&lay, 1};
    p.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    p.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    p.SampleMask = UINT_MAX;
    p.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    p.SampleDesc.Count = 1;
    p.NumRenderTargets = 1;
    p.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    if (FAILED(g_device->CreateGraphicsPipelineState(&p, IID_ID3D12PipelineState, (void **)&g_pso)))
        return false;
    return true;
}

static bool gate_vbuffer(void)
{
    V tri[3] = {{0, 0.5f, 0}, {0.5f, -0.5f, 0}, {-0.5f, -0.5f, 0}};
    D3D12_HEAP_PROPERTIES hp = {};
    hp.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC rd = {};
    rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    rd.Width = sizeof(tri);
    rd.Height = 1;
    rd.DepthOrArraySize = 1;
    rd.MipLevels = 1;
    rd.SampleDesc.Count = 1;
    rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    if (FAILED(g_device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd,
                                                 D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_ID3D12Resource, (void **)&g_vb)))
        return false;
    void *p;
    g_vb->Map(0, nullptr, &p);
    memcpy(p, tri, sizeof(tri));
    g_vb->Unmap(0, nullptr);
    return true;
}

static bool gate_fence(void)
{
    if (FAILED(g_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_ID3D12Fence, (void **)&g_fence)))
        return false;
    g_event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    return true;
}

bool d3d12_bootstrap(HWND hwnd, int w, int h)
{
    return gate_device_queue() && gate_swapchain(hwnd, w, h) && gate_rtv() && gate_commands() && gate_pipeline() && gate_vbuffer() && gate_fence();
}

void d3d12_frame(void)
{
    UINT idx = g_sc->GetCurrentBackBufferIndex();
    g_alloc->Reset();
    g_list->Reset(g_alloc, g_pso);

    D3D12_RESOURCE_BARRIER toRT = {};
    toRT.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    toRT.Transition.pResource = g_back[idx];
    toRT.Transition.Subresource = 0;
    toRT.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    toRT.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    g_list->ResourceBarrier(1, &toRT);

    float clear[4] = {0.1f, 0.2f, 0.4f, 1.0f};
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = g_rtvH->GetCPUDescriptorHandleForHeapStart();
    rtv.ptr += idx * g_rtvSz;
    g_list->ClearRenderTargetView(rtv, clear, 0, nullptr);

    g_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    D3D12_VERTEX_BUFFER_VIEW vb = {g_vb->GetGPUVirtualAddress(), 3 * sizeof(V), sizeof(V)};
    g_list->IASetVertexBuffers(0, 1, &vb);
    g_list->DrawInstanced(3, 1, 0, 0);

    D3D12_RESOURCE_BARRIER toPres = toRT;
    toPres.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    toPres.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    g_list->ResourceBarrier(1, &toPres);
    g_list->Close();

    ID3D12CommandList *lists[] = {g_list};
    g_queue->ExecuteCommandLists(1, lists);
    g_sc->Present(1, 0);

    g_fv++;
    g_queue->Signal(g_fence, g_fv);
    if (g_fence->GetCompletedValue() < g_fv)
    {
        g_fence->SetEventOnCompletion(g_fv, g_event);
        WaitForSingleObject(g_event, INFINITE);
    }
}