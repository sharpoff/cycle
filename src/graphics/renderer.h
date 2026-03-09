#pragma once

#include "graphics/barrier_merger.h"
#include "graphics/cache/cache_manager.h"

#include "types/camera.h"
#include "graphics/vulkan_types.h"
#include "graphics/render_device.h"
#include "types/gpu_light.h"
#include "types/shadowmap_cascade.h"
#include "core/constants.h"

class Renderer
{
public:
    Renderer(SDL_Window *window);

    void init();
    void shutdown();

    // should be called *after* loading entities/materials/textures/models/etc.
    void loadDynamicResources();

    void reloadShaders();
    void renderFrame(Camera &camera);

    vec2 getScreenSize() { return vec2(device.getSwapchainWidth(), device.getSwapchainHeight()); }

    CacheManager &getCacheManager() { return cacheManager; }
    RenderDevice &getRenderDevice() { return device; }

private:
    void drawMesh(VkCommandBuffer cmd, Mesh *mesh, mat4 worldMatrix);

    void resizeWindow();
    void createAttachmentImages();
    void destroyAttachmentImages();

    void compileShaders();
    void createPipelines();
    
    void destroyPipelines();

    void updateDynamicData(Camera &camera);
    void updateShadowmapCascades(Camera &camera, vec3 lightDir);
    void updateGPULights();

    RenderDevice device;
    SDL_Window *window;

    CacheManager cacheManager;

    Image colorImage;
    Image depthImage;

    Array<Buffer, FRAMES_IN_FLIGHT> sceneInfoBuffers;
    Buffer materialsBuffer;
    Buffer lightsBuffer;
    Buffer cascadesBuffer;

    Vector<GPULight> gpuLights;
    // Vector<EntityID> lightEntities;

    Sampler linearSampler;
    Sampler nearestSampler;

    RenderPipeline meshPipeline;
    RenderPipeline skyboxPipeline;
    RenderPipeline shadowmapPipeline;

    BarrierMerger barriers;

    Array<Cascade, SHADOWMAP_CASCADES> cascades;
    Array<Image, SHADOWMAP_CASCADES> shadowmapImages;

    float cascadeSplitLambda = 0.95f;
};