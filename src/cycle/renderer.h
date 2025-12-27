#pragma once

#include "cycle/types/camera.h"
#include "cycle/graphics/graphics_types.h"
#include "cycle/graphics/render_device.h"

class Renderer
{
public:
    void init(SDL_Window *window);
    void shutdown();
    void loadResources();

    void draw();

    void setCamera(Camera *camera) { this->camera = camera; }

    RenderDevice &getRenderDevice() { return device; }

private:
    void createAttachmentImages();
    void destroyAttachmentImages();

    void     updateDynamicData();
    uint32_t calculateMipLevels(uint32_t width, uint32_t height);

    RenderDevice device;
    Camera      *camera;

    Image depthImage;

    Buffer sceneInfoBuffer;

    Sampler linearSampler;
    Sampler nearestSampler;

    PipelineLayout geometryPipelineLayout;
    RenderPipeline geometryPipeline;
};