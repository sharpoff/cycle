#pragma once

#include "cycle/types/camera.h"
#include "cycle/graphics/graphics_types.h"
#include "cycle/graphics/render_device.h"

#define DEFAULT_TEXTURE_ID 0
#define DEFAULT_MATERIAL_ID 0
#define SAMPLER_LINEAR_ID 0
#define SAMPLER_NEAREST_ID 1

class ResourceManager;

class Renderer
{
public:
    void init(SDL_Window *window);
    void shutdown();

    void loadResources();
    void recreatePipelines();
    void draw();

    void setCamera(Camera *camera) { this->camera = camera; }

private:
    void createAttachmentImages();
    void destroyAttachmentImages();

    void compileShaders();
    void createPipelines();

    void     updateDynamicData();
    uint32_t calculateMipLevels(uint32_t width, uint32_t height);

    RenderDevice device;
    Camera      *camera;

    uint32_t bindlessDescriptorSet = 0;

    Image depthImage;

    Buffer sceneInfoBuffer;
    Buffer materialsBuffer;

    Sampler linearSampler;
    Sampler nearestSampler;

    PipelineLayout geometryPipelineLayout;
    RenderPipeline geometryPipeline;
};