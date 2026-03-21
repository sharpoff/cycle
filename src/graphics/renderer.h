#pragma once

#include "core/world.h"
#include "graphics/barrier_merger.h"
#include "graphics/cache/cache_manager.h"

#include "core/camera.h"
#include "core/constants.h"
#include "graphics/render_device.h"
#include "graphics/vulkan_types.h"
#include "graphics/gpu_light.h"
#include "graphics/cascade.h"

class Renderer
{
public:
    Renderer(SDL_Window *window);

    void init();
    void shutdown();

    // should be called *after* loading entities/materials/textures/models/etc.
    void loadDynamicResources();

    void reloadShaders();
    void drawFrame(World &world, Camera &camera);

    vec2 getScreenSize() { return vec2(device.getSwapchainWidth(), device.getSwapchainHeight()); }

    CacheManager &getCacheManager() { return cacheManager; }
    RenderDevice &getRenderDevice() { return device; }

private:
    void drawModel(VkCommandBuffer cmd, Model *model, mat4 worldMatrix);
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