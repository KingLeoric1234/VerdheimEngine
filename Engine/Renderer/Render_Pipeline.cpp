/*
    Render_Pipeline.cpp

    The rendering side of the engine: root signature, pipeline state (built
    from a shader file via make_pso), a textured vertex buffer, the GPU texture
    that carries the decoded image, and the per-frame draw.

    Decoding is a separate concern (Decode_pictures.cpp exposes wic_load);
    registration is a separate concern (Render_Bootstrap.cpp). This module
    only owns "turn geometry + a texture into screen pixels".
*/

#define INITGUID
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <windows.h>
#include <cstring>
#include "Render_Bootstrap.hpp"
#include "Decode_pictures.hpp" // wic_load: image file -> RGBA8 pixels.

// Rendering state, private to this module.
static ID3D12RootSignature *g_root = nullptr;
static ID3D12PipelineState *g_pso = nullptr;
static ID3D12Resource *g_vb = nullptr;         // Vertex buffer (pos + uv).
static ID3D12Resource *g_tex = nullptr;        // GPU texture (default heap).
static ID3D12Resource *g_texUp = nullptr;      // Upload staging for the texture.
static ID3D12Resource *g_cb = nullptr;         // Constant buffer: FOCUS (b0).
static ID3D12DescriptorHeap *g_srvH = nullptr; // SRV heap (shader-visible).
static UINT g_srvSz = 0;
static UINT g_vertCount = 0; // Vertex count set by brush; read by the frame.

// One vertex: NDC position + texture coordinate.
struct V
{
    float x, y, z, u, v;
};

struct RenderItem
{
    ID3D12PipelineState *pso;
    ID3D12Resource *tex;
    D3D12_GPU_DESCRIPTOR_HANDLE srv;
    UINT vbOffset;
    UINT vertCount;
};

#define MAX_ITEMS 16
static RenderItem g_items[MAX_ITEMS]; // How much items per frame contains.
static int g_itemCount = 0;
static UINT g_vbFilled = 0;

// Compile a shader file and build a pipeline state object (PSO) from it.
// One shader -> one PSO. Reuses the single root signature g_root.
// Returns the new PSO, or nullptr if the shader fails to compile or the
// pipeline state cannot be created. The caller keeps the returned PSO.
static ID3D12PipelineState *make_pso(const wchar_t *hlsl_path)
{
    ID3DBlob *vB = nullptr, *pB = nullptr, *err = nullptr;
    if (FAILED(D3DCompileFromFile(hlsl_path, nullptr, nullptr,
                                  "VS", "vs_5_0", 0, 0, &vB, &err)))
        return nullptr;
    if (FAILED(D3DCompileFromFile(hlsl_path, nullptr, nullptr,
                                  "PS", "ps_5_0", 0, 0, &pB, &err)))
        return nullptr;

    // Input layout: POSITION (xyz, offset 0) then TEXCOORD (uv, offset 12).
    D3D12_INPUT_ELEMENT_DESC lay[2] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
    D3D12_GRAPHICS_PIPELINE_STATE_DESC p = {};
    p.pRootSignature = g_root;
    p.VS = {vB->GetBufferPointer(), vB->GetBufferSize()};
    p.PS = {pB->GetBufferPointer(), pB->GetBufferSize()};
    p.InputLayout = {lay, 2};
    p.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    p.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    p.SampleMask = UINT_MAX;
    p.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    p.SampleDesc.Count = 1;
    p.NumRenderTargets = 1;
    p.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    // p = {} zeroes the blend state, so RenderTargetWriteMask becomes 0, which
    // makes the output-merger stage reject every PS write. Enable all writes.
    p.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    ID3D12PipelineState *pso = nullptr;
    if (FAILED(g_device->CreateGraphicsPipelineState(&p, IID_ID3D12PipelineState, (void **)&pso)))
        return nullptr;
    return pso;
}

bool gate_pipeline(void)
{
    // Root signature: a CBV for FOCUS (b0, used in VS), a descriptor table
    // for the texture (t0, sampled in PS), and a static sampler (s0).
    D3D12_DESCRIPTOR_RANGE range = {};
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    range.NumDescriptors = 1;
    range.BaseShaderRegister = 0; // t0

    D3D12_ROOT_PARAMETER params[2] = {};
    params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[0].Descriptor.ShaderRegister = 0; // b0
    params[0].Descriptor.RegisterSpace = 0;
    params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[1].DescriptorTable.NumDescriptorRanges = 1;
    params[1].DescriptorTable.pDescriptorRanges = &range;
    params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_STATIC_SAMPLER_DESC samp = {};
    samp.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samp.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samp.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samp.MaxAnisotropy = 1;
    samp.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    samp.MinLOD = 0;
    samp.MaxLOD = 0;
    samp.ShaderRegister = 0; // s0
    samp.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC rs = {};
    rs.NumParameters = 2;
    rs.pParameters = params;
    rs.NumStaticSamplers = 1;
    rs.pStaticSamplers = &samp;
    rs.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    ID3DBlob *sig = nullptr, *err = nullptr;
    if (FAILED(D3D12SerializeRootSignature(&rs, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &err)))
        return false;
    if (FAILED(g_device->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(),
                                             IID_ID3D12RootSignature, (void **)&g_root)))
        return false;

    // Root signature only. No default PSO here: the shader rule is chosen by
    // brush -> make_pso, so gate_pipeline holds no hard-coded content.
    return true;
}

// Create a GPU texture from an image file, upload it, make it shader-visible,
// and allocate the FOCUS constant buffer. Reusable: pass any image path.
static bool make_texture(RenderItem *it, const wchar_t *path)
{
    UINT tw = 0, th = 0;
    unsigned char *px = wic_load(path, &tw, &th);
    if (!px)
        return false;

    // 1) GPU texture (default heap), initially COPY_DEST.
    D3D12_HEAP_PROPERTIES hp = {};
    hp.Type = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC td = {};
    td.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    td.Width = tw;
    td.Height = th;
    td.DepthOrArraySize = 1;
    td.MipLevels = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    if (FAILED(g_device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &td,
                                                 D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_ID3D12Resource, (void **)&g_tex)))
    {
        free(px);
        return false;
    }

    // 2) Upload staging buffer (UPLOAD heap), rows aligned to 256 bytes.
    UINT rowPitch = (tw * 4 + 255) & ~255u;
    D3D12_HEAP_PROPERTIES hpUp = {};
    hpUp.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC ud = {};
    ud.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    ud.Width = rowPitch * th;
    ud.Height = 1;
    ud.DepthOrArraySize = 1;
    ud.MipLevels = 1;
    ud.SampleDesc.Count = 1;
    ud.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    if (FAILED(g_device->CreateCommittedResource(&hpUp, D3D12_HEAP_FLAG_NONE, &ud,
                                                 D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_ID3D12Resource, (void **)&g_texUp)))
    {
        free(px);
        return false;
    }

    // 3) Copy the decoded rows into the staging buffer.
    void *dst = nullptr;
    g_texUp->Map(0, nullptr, &dst);
    for (UINT r = 0; r < th; r++)
        memcpy((unsigned char *)dst + r * rowPitch, px + r * tw * 4, tw * 4);
    g_texUp->Unmap(0, nullptr);
    free(px);

    // 4) Copy staging -> texture, then transition to shader-visible, on a
    //    one-shot command list; wait on a private fence so g_alloc/g_list
    //    are free again before the frame loop starts.
    g_alloc->Reset();
    g_list->Reset(g_alloc, nullptr);
    D3D12_TEXTURE_COPY_LOCATION dstLoc;
    dstLoc.pResource = g_tex;
    dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLoc.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION srcLoc;
    srcLoc.pResource = g_texUp;
    srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcLoc.PlacedFootprint.Offset = 0;
    srcLoc.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srcLoc.PlacedFootprint.Footprint.Width = tw;
    srcLoc.PlacedFootprint.Footprint.Height = th;
    srcLoc.PlacedFootprint.Footprint.Depth = 1;
    srcLoc.PlacedFootprint.Footprint.RowPitch = rowPitch;

    D3D12_BOX box = {0, 0, 0, tw, th, 1};
    g_list->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, &box);
    D3D12_RESOURCE_BARRIER bar = {};
    bar.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    bar.Transition.pResource = g_tex;
    bar.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    bar.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    g_list->ResourceBarrier(1, &bar);
    g_list->Close();
    ID3D12CommandList *ll[] = {g_list};
    g_queue->ExecuteCommandLists(1, ll);

    ID3D12Fence *fence = nullptr;
    if (FAILED(g_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_ID3D12Fence, (void **)&fence)))
        return false;
    HANDLE ev = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    g_queue->Signal(fence, 1);
    fence->SetEventOnCompletion(1, ev);
    WaitForSingleObject(ev, INFINITE);
    fence->Release();
    CloseHandle(ev);

    // 5) Shader-visible SRV heap + SRV for the texture.
    D3D12_DESCRIPTOR_HEAP_DESC sh = {};
    sh.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    sh.NumDescriptors = 1;
    sh.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (FAILED(g_device->CreateDescriptorHeap(&sh, IID_ID3D12DescriptorHeap, (void **)&g_srvH)))
        return false;
    g_srvSz = g_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_SHADER_RESOURCE_VIEW_DESC sv = {};
    sv.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    sv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    sv.Texture2D.MipLevels = 1;
    g_device->CreateShaderResourceView(g_tex, &sv, g_srvH->GetCPUDescriptorHandleForHeapStart());

    // 6) Constant buffer (b0): FOCUS. One float, padded to 256 bytes.
    D3D12_HEAP_PROPERTIES cbHp = {};
    cbHp.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC cbd = {};
    cbd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    cbd.Width = 256;
    cbd.Height = 1;
    cbd.DepthOrArraySize = 1;
    cbd.MipLevels = 1;
    cbd.SampleDesc.Count = 1;
    cbd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    if (FAILED(g_device->CreateCommittedResource(&cbHp, D3D12_HEAP_FLAG_NONE, &cbd,
                                                 D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_ID3D12Resource, (void **)&g_cb)))
        return false;
    void *cbDst = nullptr;
    g_cb->Map(0, nullptr, &cbDst);
    memset(cbDst, 0, 256);
    *(float *)cbDst = 1.0f; // FOCUS: perspective strength (near big, far small).
    g_cb->Unmap(0, nullptr);
    return true;
}

bool gate_vbuffer(void)
{
    // Reserve a vertex buffer that can hold up to MAX_VERTS vertices. No
    // geometry is baked in here: brush fills the actual vertices each call.
    const UINT maxVerts = 512;
    UINT bytes = maxVerts * sizeof(V);

    D3D12_HEAP_PROPERTIES hp = {};
    hp.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC rd = {};
    rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    rd.Width = bytes;
    rd.Height = 1;
    rd.DepthOrArraySize = 1;
    rd.MipLevels = 1;
    rd.SampleDesc.Count = 1;
    rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    if (FAILED(g_device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd,
                                                 D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr, IID_ID3D12Resource, (void **)&g_vb)))
        return false;
    return true;
}

void d3d12_frame(void)
{
    UINT idx = g_sc->GetCurrentBackBufferIndex();
    g_alloc->Reset();
    g_list->Reset(g_alloc, g_pso);
    g_list->SetGraphicsRootSignature(g_root);

    // Bind the texture SRV heap, the FOCUS constant buffer (b0) and the
    // texture descriptor table (t0) before drawing.
    ID3D12DescriptorHeap *heaps[] = {g_srvH};
    g_list->SetDescriptorHeaps(1, heaps);
    g_list->SetGraphicsRootConstantBufferView(0, g_cb->GetGPUVirtualAddress());
    g_list->SetGraphicsRootDescriptorTable(1, g_srvH->GetGPUDescriptorHandleForHeapStart());

    D3D12_RESOURCE_BARRIER toRT = {};
    toRT.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    toRT.Transition.pResource = g_back[idx];
    toRT.Transition.Subresource = 0;
    toRT.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    toRT.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    g_list->ResourceBarrier(1, &toRT);

    float clear[4] = {0.0f, 0.0f, 0.0f, 1.0f}; // Black background.
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = g_rtvH->GetCPUDescriptorHandleForHeapStart();
    rtv.ptr += idx * g_rtvSz;
    g_list->ClearRenderTargetView(rtv, clear, 0, nullptr);
    g_list->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

    D3D12_VIEWPORT vp = {0, 0, (float)g_winW, (float)g_winH, 0.0f, 1.0f};
    g_list->RSSetViewports(1, &vp);
    D3D12_RECT sc = {0, 0, g_winW, g_winH};
    g_list->RSSetScissorRects(1, &sc);
    g_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    D3D12_VERTEX_BUFFER_VIEW vb = {g_vb->GetGPUVirtualAddress(), (UINT)(g_vertCount * sizeof(V)), sizeof(V)};
    g_list->IASetVertexBuffers(0, 1, &vb);
    g_list->DrawInstanced(g_vertCount, 1, 0, 0);

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

// brush: the engine's single public draw API. Hand an image, its vertices and
// its shader rule; the image is decoded/uploaded, the vertices copied into the
// vertex buffer, and d3d12_frame draws it every frame. Call it once at startup
// (before the frame loop) to set what the first frame draws.
// NOTE: single-instance for now - a later brush call replaces the previous one.
void brush(
    const wchar_t *hlsl_file,
    const wchar_t *img_file,
    const V *verts,
    unsigned int vert_count)
{
    // 1) shader rule -> a matching PSO. Reject if it failed to build.
    ID3D12PipelineState *pso = make_pso(hlsl_file);
    if (!pso)
        return;
    g_pso = pso;

    // 2) image -> GPU texture (fills g_tex / g_srvH).
    if (!make_texture(img_file))
        return;

    // 3) vertices -> GPU vertex buffer, and record how many we drew.
    void *p;
    g_vb->Map(0, nullptr, &p);
    memcpy(p, verts, vert_count * sizeof(V));
    g_vb->Unmap(0, nullptr);
    g_vertCount = vert_count;
}
