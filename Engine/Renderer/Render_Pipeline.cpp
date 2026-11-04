/*

    Render_Pipeline.cpp.

*/

#define INITGUID
#include <d3d12.h>
#include <windows.h>
#include <wincodec.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include "Render_Bootstrap.hpp"

extern ID3D12Device *g_device;
extern ID3D12CommandQueue *g_queue;
extern ID3D12CommandAllocator *g_alloc;
extern ID3D12GraphicsCommandList *g_list;
extern ID3D12Resource *g_back[2];
extern ID3D12DescriptorHeap *g_rtvH;
extern UINT g_rtvSz;
extern int g_winW, g_winH;
extern ID3D12Fence *g_fence;
extern UINT64 g_fv;
extern HANDLE g_event;
extern IDXGISwapChain3 *g_sc;

static ID3D12RootSignature *g_root = nullptr;
static ID3D12PipelineState *g_pso = nullptr;
static ID3D12Resource *g_vb = nullptr;
static ID3D12Resource *g_cb = nullptr;
static ID3D12Resource *g_tex = nullptr;
static ID3D12Resource *g_texUp = nullptr;
static ID3D12DescriptorHeap *g_srvH = nullptr;
static D3D12_VERTEX_BUFFER_VIEW g_vbView = {};
static UINT g_vertCount = 0;

struct V // define the V's size in the memory.
{
    float x, y, z, u, v;
};

// Constant buffer: per-frame globals, one float FOCUS padded to 16 bytes
// (D3D12 requires constant buffers to be 16-byte aligned).
struct CameraConst
{
    float FOCUS;
    float pad[3];
};

static unsigned char *wic_load(const wchar_t *path, unsigned *w, unsigned *h)
{
    IWICImagingFactory *fac = nullptr;
    IWICBitmapDecoder *dec = nullptr;
    IWICBitmapFrameDecode *frame = nullptr;
    IWICFormatConverter *conv = nullptr;

    CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PWD_ImagingFactory, (void **)&fac);

    fac->CreateDecoderFromFilename(path, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &dec);

    dec->GetFrame(0, &frame);

    fac->CreateFormatConverter(&conv);

    conv->Initialize(frame, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom);

    unsigned ww = 0, hh = 0;
    conv->GetSize(&ww, &hh);
    *w = ww;
    *h = hh;

    unsigned char *pixels = (unsigned char *)malloc(ww * hh * 4);
    conv->CopyPixels(nullptr, ww * 4, ww * hh * 4, pixels);

    conv->Release();
    frame->Release();
    dec->Release();
    fac->Release();

    return pixels;
}

// Build a texture: turn one image into a GPU-samplable texture
void make_texture(const wchar_t *path)
{
    // ── Step 1: Decode the image (pixels + size) ──
    unsigned w = 0, h = 0;
    unsigned char *pixels = wic_load(path, &w, &h);

    // ── Step 2: Create the GPU texture (DEFAULT heap; GPU-private) ──
    D3D12_HEAP_PROPERTIES hp = {};
    hp.Type = D3D12_HEAP_TYPE_DEFAULT; // GPU-only memory
    D3D12_RESOURCE_DESC rd = {};
    rd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // a 2D texture
    rd.Width = w;
    rd.Height = h; // pixel size
    rd.DepthOrArraySize = 1;
    rd.MipLevels = 1;                       // one layer, one mip
    rd.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // RGBA8 per pixel
    rd.SampleDesc.Count = 1;
    rd.SampleDesc.Quality = 0;
    g_device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd,
                                      D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_ID3D12Resource, (void **)&g_tex);

    // ── Step 3: Create the upload heap (UPLOAD; CPU-writable, GPU-readable) ──
    UINT rowPitch = ((w * 4 + 255) / 256) * 256; // row size, 256-byte aligned
    D3D12_HEAP_PROPERTIES hpUp = {};
    hpUp.Type = D3D12_HEAP_TYPE_UPLOAD; // CPU can write here
    D3D12_RESOURCE_DESC rdUp = {};
    rdUp.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; // a 1D buffer (transfer station)
    rdUp.Width = rowPitch * h;                        // total bytes = row * height
    rdUp.Height = 1;
    rdUp.DepthOrArraySize = 1;
    rdUp.MipLevels = 1;
    rdUp.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; // rows packed in order
    g_device->CreateCommittedResource(&hpUp, D3D12_HEAP_FLAG_NONE, &rdUp,
                                      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_ID3D12Resource, (void **)&g_texUp);

    // ── Step 4: Map + copy row by row (CPU writes pixels into upload heap) ──
    void *p = nullptr;
    g_texUp->Map(0, nullptr, &p); // open the door: get CPU ptr
    unsigned char *dst = (unsigned char *)p;
    for (UINT y = 0; y < h; y++)
        memcpy(dst + (size_t)y * rowPitch, // dest row (aligned pitch)
               pixels + (size_t)y * w * 4, // src row (compact)
               (size_t)w * 4);             // bytes per row
    g_texUp->Unmap(0, nullptr);            // close the door
    free(pixels);                          // pixels copied; release

    // ── Step 5: CopyTextureRegion (GPU moves pixels: upload heap -> texture) ──
    D3D12_TEXTURE_COPY_LOCATION src = {};
    src.pResource = g_texUp; // source: upload heap
    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    src.PlacedFootprint.Footprint.Width = w;
    src.PlacedFootprint.Footprint.Height = h;
    src.PlacedFootprint.Footprint.Depth = 1;
    src.PlacedFootprint.Footprint.RowPitch = rowPitch; // aligned row pitch
    D3D12_TEXTURE_COPY_LOCATION dst = {};
    dst.pResource = g_tex; // destination: texture
    dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst.SubresourceIndex = 0; // copy into layer 0
    g_list->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

    // ── Step 6: Barrier + fence (state transition + wait for GPU) ──
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = g_tex;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;            // was: copy target
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE; // now: samplable
    g_list->ResourceBarrier(1, &barrier);                                       // record barrier into list
    g_list->Close();                                                            // stop recording
    g_queue->ExecuteCommandLists(1, &g_list);                                   // submit to GPU queue
    g_queue->Signal(g_fence, ++g_fv);                                           // mark "reached frame N"
    if (g_fence->GetCompletedValue() < g_fv)                                    // GPU not there yet
    {
        g_fence->SetEventOnCompletion(g_fv, g_event); // fire event when reached
        WaitForSingleObject(g_event, INFINITE);       // CPU sleeps and waits
    }
    g_alloc->Reset(); // reset allocator for next use

    // ── Step 7: Create SRV (build the read-descriptor into the descriptor heap) ──
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;
    g_device->CreateShaderResourceView(g_tex, &srvDesc,
                                       g_srvH->GetCPUDescriptorHandleForHeapStart()); // place into the SRV heap
}

// Vertex Buffer: Memories allocated for GPU reading.
bool gate_vbuffer(void)
{
    const UINT max_Vertices = 512;
    UINT bytes = max_Vertices * sizeof(V);

    // The buffer place.
    D3D12_HEAP_PROPERTIES heap_properties = {};
    heap_properties.Type = D3D12_HEAP_TYPE_UPLOAD;

    // Shape of memory.
    D3D12_RESOURCE_DESC resource_dimension = {};
    resource_dimension.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resource_dimension.Width = bytes;
    resource_dimension.Height = 1;
    resource_dimension.DepthOrArraySize = 1;
    resource_dimension.MipLevels = 1;
    resource_dimension.SampleDesc.Count = 1;
    resource_dimension.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    if (FAILED(g_device->CreateCommittedResource(
            &heap_properties,
            D3D12_HEAP_FLAG_NONE,
            &resource_dimension,
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
            nullptr,
            IID_ID3D12Resource,
            (void **)&g_vb)))
        return false;
    return true;
}

// Build the constant buffer: allocate upload memory, fill FOCUS, let VS read it.
bool gate_cbuffer(void)
{
    D3D12_HEAP_PROPERTIES hp = {};
    hp.Type = D3D12_HEAP_TYPE_UPLOAD; // CPU can write here
    D3D12_RESOURCE_DESC rd = {};
    rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    rd.Width = sizeof(CameraConst); // 16 bytes: one float + pad
    rd.Height = 1;
    rd.DepthOrArraySize = 1;
    rd.MipLevels = 1;
    rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    if (FAILED(g_device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd,
                                                 D3D12_RESOURCE_STATE_GENERIC_READ,
                                                 nullptr, IID_ID3D12Resource, (void **)&g_cb)))
        return false;

    // Fill FOCUS (CPU writes the value into the upload memory).
    CameraConst *p = nullptr;
    g_cb->Map(0, nullptr, (void **)&p); // open the door
    p->FOCUS = 1.0f;                    // focal length; tweak to taste
    g_cb->Unmap(0, nullptr);            // close the door
    return true;
}

// Copy the vertices from CPU into memories so that GPU will read the vertices correctly.
// example: copy_vertices(quad, 6, 0). Push the quad's 6 vertices into the place 0.
UINT copy_vertices(const V *verts, UINT vert_count, UINT offset_verts)
{
    void *memory_pointer;
    g_vb->Map(0, nullptr, &memory_pointer);
    memcpy((V *)memory_pointer + offset_verts, verts, vert_count * sizeof(V));
    g_vb->Unmap(0, nullptr);
    return vert_count;
}

// GPU read vertices.
// example: draw_vertices(0, g_vertCount); Read from 0 vertex to g_vertCount.
void draw_vertices(UINT offset_verts, UINT vert_count)
{
    D3D12_VERTEX_BUFFER_VIEW vertex_buffer =
        {
            g_vb->GetGPUVirtualAddress() + offset_verts * sizeof(V), // Commence location.
            vert_count * sizeof(V),                                  // Memory costs.
            sizeof(V)                                                // The pace per vertices. (20 bytes)
    };

    g_list->IASetVertexBuffers(0, 1, &vertex_buffer);

    g_list->DrawInstanced(vert_count, 1, 0, 0);
}

// Build the root signature: declares which resources this pipeline uses,
// where they are bound, and which shader sees them.
// Three resources: b0 constant buffer (VS), t0 texture (PS), s0 sampler (PS).
bool gate_pipeline(void)
{
    // [1] Declare the "texture" resource: one SRV, bound at t0.
    D3D12_DESCRIPTOR_RANGE range = {};
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // it is a shader resource view
    range.NumDescriptors = 1;                          // one texture
    range.BaseShaderRegister = 0;                      // register t0

    // [2] Declare the two root parameters: b0 constant + t0 texture table.
    D3D12_ROOT_PARAMETER params[2] = {};
    // Parameter 0: constant buffer (FOCUS perspective params), VS only.
    params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    params[0].Descriptor.ShaderRegister = 0; // b0
    params[0].Descriptor.RegisterSpace = 0;
    params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    // Parameter 1: texture (descriptor table pointing at the SRV), PS only.
    params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[1].DescriptorTable.NumDescriptorRanges = 1;
    params[1].DescriptorTable.pDescriptorRanges = &range;
    params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // [3] Declare the sampler: s0, how to sample (linear filter, clamp), PS only.
    D3D12_STATIC_SAMPLER_DESC samp = {};
    samp.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;     // linear filtering
    samp.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP; // clamp coords outside [0,1]
    samp.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samp.MaxAnisotropy = 1;
    samp.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    samp.MinLOD = 0;
    samp.MaxLOD = 0;
    samp.ShaderRegister = 0; // s0
    samp.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // [4] Package into a root signature description: 2 params + 1 sampler.
    D3D12_ROOT_SIGNATURE_DESC rs = {};
    rs.NumParameters = 2;
    rs.pParameters = params;
    rs.NumStaticSamplers = 1;
    rs.pStaticSamplers = &samp;
    rs.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // [5] Finalize: serialize the description, then create the root signature.
    ID3DBlob *sig = nullptr, *err = nullptr;
    if (FAILED(D3D12SerializeRootSignature(&rs, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &err)))
        return false;
    if (FAILED(g_device->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(),
                                             IID_ID3D12RootSignature, (void **)&g_root)))
        return false;
    return true;
}

static ID3D12PipelineState *make_pso(const wchar_t *hlsl_path)
{
    ID3DBlob *vB = nullptr, *pB = nullptr, *error = nullptr;

    if (FAILED(D3DCompileFromFile(hlsl_path, nullptr, nullptr,
                                  "VS", "vs_5_0", 0, 0, &vB, &error)))
        return nullptr;

    if (FAILED(D3DCompileFromFile(hlsl_path, nullptr, nullptr,
                                  "PS", "ps_5_0", 0, 0, &pB, &error)))
        return nullptr;

    D3D12_INPUT_ELEMENT_DESC lay[2] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_description = {};
    pso_description.pRootSignature = g_root;                                        // Follow the "g_root" signiture to apply the rendering pipeline.
    pso_description.VS = {vB->GetBufferPointer(), vB->GetBufferSize()};             // Register the VS pipeline.
    pso_description.PS = {pB->GetBufferPointer(), pB->GetBufferSize()};             // Register the PS pipeline.
    pso_description.InputLayout = {lay, 2};                                         // Register the canvas.
    pso_description.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // Register the rasterize topology.
    pso_description.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;               // Rasterization settings registration.
    pso_description.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;                // Rasterization culling registration.
    pso_description.SampleMask = UINT_MAX;                                          // Occupy.
    pso_description.SampleDesc.Count = 1;                                           // Occupy.
    pso_description.NumRenderTargets = 1;                                           // Occupy.
    pso_description.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;                     // Canvas frame.
    pso_description.BlendState.RenderTarget[0].RenderTargetWriteMask =
        D3D12_COLOR_WRITE_ENABLE_ALL; // Mixing rule: Rendering on the screen.

    ID3D12PipelineState *pso = nullptr; // Create a registeration pointer to record the pso_description.
    if (FAILED(g_device->CreateGraphicsPipelineState(&pso_description, IID_ID3D12PipelineState, (void **)&pso)))
        return nullptr;
    return pso; // Submit the pso.
}

// Render one frame: take a canvas, draw, present, and sync with the GPU
void d3d12_frame()
{
    // ── Step 1: Grab the canvas + reset recording ──
    UINT back = g_sc->GetCurrentBackBufferIndex(); // which canvas to draw this frame
    g_alloc->Reset();                              // clear last frame's command memory
    g_list->Reset(g_alloc, nullptr);               // start recording this frame

    // ── Step 2: Turn the canvas from "displayable" to "drawable" ──
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = g_back[back]; // this canvas
    barrier.Transition.Subresource = 0;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;      // was: display
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET; // now: drawable
    g_list->ResourceBarrier(1, &barrier);

    // ── Step 3: Clear + bind output ──
    float clear[4] = {0.2f, 0.2f, 0.2f, 1.0f}; // clear color (gray)
    D3D12_CPU_DESCRIPTOR_HANDLE rtv =
        g_rtvH->GetCPUDescriptorHandleForHeapStart() + back * g_rtvSz; // this canvas' RTV
    g_list->ClearRenderTargetView(rtv, clear, 0, nullptr);             // wipe the canvas clean
    g_list->OMSetRenderTargets(1, &rtv, FALSE, nullptr);               // tell GPU to draw onto this canvas

    // ── Step 4: Set the viewport (draw region) ──
    D3D12_VIEWPORT vp = {};
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    vp.Width = (float)g_winW;
    vp.Height = (float)g_winH; // full screen
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    g_list->RSSetViewports(1, &vp);
    D3D12_RECT scissor = {0, 0, (LONG)g_winW, (LONG)g_winH}; // clip rect, full screen
    g_list->RSSetScissorRects(1, &scissor);

    // ── Step 5: Bind the pipeline's resources ──
    g_list->SetGraphicsRootSignature(g_root);                                                // root signature (resource entry)
    g_list->SetDescriptorHeaps(1, &g_srvH);                                                  // SRV heap (descriptor warehouse)
    g_list->SetGraphicsRootConstantBufferView(0, g_cb->GetGPUVirtualAddress());              // constant (b0, to VS)
    g_list->SetGraphicsRootDescriptorTable(1, g_srvH->GetGPUDescriptorHandleForHeapStart()); // texture (t0, to PS)

    // ── Step 6: Issue the Draw ──
    g_list->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // topology: triangles
    draw_vertices(0, g_vertCount);                                         // bind vertex buffer + draw (reuses the teaching piece)

    // ── Step 7: Finish + submit + sync ──
    D3D12_RESOURCE_BARRIER barrier2 = {};
    barrier2.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier2.Transition.pResource = g_back[back];
    barrier2.Transition.Subresource = 0;
    barrier2.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier2.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT; // back to displayable
    g_list->ResourceBarrier(1, &barrier2);

    g_list->Close();                          // stop recording
    g_queue->ExecuteCommandLists(1, &g_list); // submit to GPU queue
    g_sc->Present(1, 0);                      // show the canvas on screen

    g_queue->Signal(g_fence, ++g_fv);        // mark "reached frame N"
    if (g_fence->GetCompletedValue() < g_fv) // GPU not done with this frame yet
    {
        g_fence->SetEventOnCompletion(g_fv, g_event);
        WaitForSingleObject(g_event, INFINITE); // CPU sleeps until GPU finishes
    }
}
