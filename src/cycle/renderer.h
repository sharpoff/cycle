#pragma once

#include "cycle/camera.h"
#include "cycle/graphics/graphics_types.h"
#include "cycle/graphics/render_device.h"

class Renderer
{
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

public:
    Renderer() = default;
    ~Renderer() = default;

    void init(SDL_Window *window);
    void shutdown();

    void draw();

    void setCamera(Camera *camera) { this->camera = camera; }

    void updateDynamicData();

    uint32_t calculateMipLevels(uint32_t width, uint32_t height);
    Image   *loadImageFromFile(const char *file, ImageUsageFlags imageUsage);

private:
    RenderDevice device;
    Camera *camera;

    // Resources
    Image *colorTarget;
    Image *depthTarget;
    Image *testImage;

    Buffer *vertexBuffer;
    Buffer *globalDataBuffer;

    Sampler *linearSampler;
    Sampler *nearestSampler;

    GlobalData     globalData;
    Vector<Vertex> vertices;

    PipelineLayout *geometryPipelineLayout;
    RenderPipeline *geometryPipeline;
};