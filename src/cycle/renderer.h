#pragma once

#include "cycle/graphics/barrier_merger.h"
#include "cycle/types/camera.h"
#include "cycle/graphics/vulkan_types.h"
#include "cycle/graphics/render_device.h"
#include "cycle/types/gpu_light.h"
#include "cycle/types/id.h"
#include "cycle/types/model.h"
#include "cycle/constants.h"

class Renderer
{
public:
    static void init(SDL_Window *window);
    void shutdown();

    // should be called *after* loading entities/materials/textures/models/etc.
    void loadDynamicResources();

    void reloadShaders();
    void draw();

    void setCamera(Camera *camera) { this->camera = camera; }

    vec2 getScreenSize() { return vec2(device.getSwapchainWidth(), device.getSwapchainHeight()); }
    RenderDevice &getRenderDevice() { return device; }

private:
    Renderer() {}
    Renderer(Renderer &) = delete;
    void operator=(Renderer const &) = delete;

    void initInternal(SDL_Window *window);

    void drawModel(VkCommandBuffer cmd, Model *model, mat4 worldMatrix);

    void resizeWindow();
    void createAttachmentImages();
    void destroyAttachmentImages();

    void compileShaders();
    void createPipelines();
    
    void destroyPipelines();

    void update();
    void updateShadowmapCascades(vec3 lightDir);
    void updateGPULights();

    RenderDevice device;
    SDL_Window *window;
    Camera *camera;

    Image colorImage;
    Image depthImage;
    Image shadowmapImage;

    Array<Buffer, FRAMES_IN_FLIGHT> sceneInfoBuffers;
    Buffer materialsBuffer;
    Buffer lightsBuffer;
    Buffer cascadesBuffer;

    Vector<GPULight> gpuLights;
    Vector<EntityID> lightEntities;

    Sampler linearSampler;
    Sampler nearestSampler;

    RenderPipeline meshPipeline;
    RenderPipeline skyboxPipeline;
    RenderPipeline shadowmapPipeline;

    BarrierMerger barriers;

    Array<mat4, SHADOWMAP_CASCADES> cascadeMatrices;
    Array<float, SHADOWMAP_CASCADES> cascadeDepths;
    float cascadeSplitLambda = 0.95f;
};