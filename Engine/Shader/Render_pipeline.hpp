#ifndef RENDERPIPELINE_HPP
#define RENDERPIPELINE_HPP

#include "Render_Pipeline.cpp"

void brush(
    const wchar_t *hlsl_file,
    const wchar_t *img_file,
    const V *verts,
    unsigned int vert_count);

#endif // RENDERPIPELINE_HPP