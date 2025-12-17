#pragma once

#include "cycle/graphics/graphics_types.h"
#include "cycle/graphics/render_device.h"

namespace Renderer
{
    void Init(SDL_Window *window);
    void Shutdown();

    void Draw();

    uint32_t CalculateMipLevels(uint32_t width, uint32_t height);
    Image   *LoadImageFromFile(const char *file, ImageUsageFlags imageUsage);

    struct Vertex
    {
        vec3  position;
        float uv_x;
        vec3  color;
        float uv_y;
    };

    struct GlobalData
    {
        mat4 viewProjection;
    };

    inline RenderDevice device;

    // Resources
    inline Image *colorTarget;
    inline Image *depthTarget;
    inline Image *testImage;

    inline Buffer *vertexBuffer;
    inline Buffer *globalDataBuffer;

    inline Sampler *linearSampler;
    inline Sampler *nearestSampler;

    inline GlobalData     globalData;
    inline Vector<Vertex> vertices;

    inline PipelineLayout *geometryPipelineLayout;
    inline RenderPipeline *geometryPipeline;
} // namespace Renderer