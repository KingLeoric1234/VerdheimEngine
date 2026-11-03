/*

    Render_Pipeline.cpp.

*/

#define INITGUID
#include <d3d12.h>
#include <windows.h>

#include "Render_Bootstrap.hpp"

struct V // define the V's size in the memory.
{
    float x, y, z, u, v;
};

static ID3D12Resource *gpu_vertex_buffer = nullptr;

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
            (**void)&g_vb)))
        return false;
    return true;
}

// Copy the vertices from CPU into memories so that GPU will read the vertices correctly.
// example: copy_vertices(quad, 6, 0). Push the quad's 6 vertices into the place 0.
UINT copy_vertices(const V *verts, UINT vert_count, UINT offset_verts)
{
    void *memory_pointer;
    gpu_vertex_buffer->Map(0, nullptr, &memory_pointer);
    memcpy((V *)memory_pointer + offset_verts, verts, vert_count * sizeof(V));
    gpu_vertex_buffer->Unmap(0, nullptr);
    return vert_count;
}

// GPU read vertices.
// example: draw_vertices(0, g_vertCount); Read from 0 vertex to g_vertCount.
void draw_vertices(UINT offset_verts, UINT vert_count)
{
    D3D12_VERTEX_BUFFER_VIEW vertex_buffer =
        {
            gpu_vertex_buffer->GetGPUVirtualAddress() + offset_verts * sizeof(V), // Commence location.
            vert_count * sizeof(V),                                               // Memory costs.
            sizeof(V)                                                             // The pace per vertices. (20 bytes)
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