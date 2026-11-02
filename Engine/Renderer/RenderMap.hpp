#ifndef RENDERMAP_HPP
#define RENDERMAP_HPP

class RenderObject
{
public:
    RenderObject()
    {
        m_vertices = nullptr;
        m_imgH = m_imgW = 0;
    }
    ~RenderObject() { delete[] m_vertices; }

    void decode_picture(const char *imgPath);
    void parse_shader(const char *hlslPath);
    void draw();

private:
    float *m_vertices; // Read vertices.
    int m_vertexCount;
    void *m_pixels;                // Parse the pixels.
    ID3D12PipelineState *m_shader; // Parse the shader.
    unsigned int m_imgW;
    unsigned int m_imgH;
    wchar_t *m_shaderPath;
    wchar_t *m_imgPath;
};

#endif // RENDERMAP_HPP