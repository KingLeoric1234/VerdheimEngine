/*

    RenderMap.cpp: An object needs to be rendered. Abstract the specific realization.

*/

#include <windows.h>
#include "Decode_pictures.hpp"
#include "Render_pipeline.hpp"
#include "RenderMap.hpp"

void RenderObject::decode_picture(const char *imgPath)
{
    // Translate path type from char into wide char.
    int len = MultiByteToWideChar(CP_UTF8, 0, imgPath, -1, NULL, 0);
    wchar_t *wpath = (wchar_t *)malloc(len * sizeof(wchar_t)); // wpath = wide_path.
    MultiByteToWideChar(CP_UTF8, 0, imgPath, -1, wpath, len);

    unsigned int w = 0, h = 0;
    unsigned char *px = wic_load(wpath, &w, &h);
    free(wpath);

    // Fail check.
    if (!px)
    {
        m_pixels = nullptr;
        m_imgW = m_imgH = 0;
        return;
    }

    m_pixels = px;
    m_imgW = w;
    m_imgH = h;
}

void RenderObject::parse_shader(const char *hlslPath)
{
    // Translate path from char into wide char.
    int len = MultiByteToWideChar(CP_UTF8, 0, hlslPath, -1, NULL, 0);
    wchar_t *wpath = (wchar_t *)malloc(len * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, hlslPath, -1, wpath, len);

    // Use make_pso, compiling the hlsl.
    ID3D12PipelineState *pso = make_pso(wpath);
    free(wpath);

    if (!pso)
    {
        m_shader = nullptr;
        return;
    }
    m_shader = pso;
}

void RenderObject::draw()
{
    // brush: from the Render_Pipeline.cpp.
    brush(m_shaderPath, m_imgPath, (V *)m_vertices, (unsigned)m_vertexCount);
}
