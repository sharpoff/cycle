#pragma once

#include "graphics/barrier_merger.h"

#include "core/camera.h"
#include "core/constants.h"
#include "graphics/cascade.h"
#include "graphics/gpu_light.h"
#include "graphics/mesh.h"
#include "graphics/model.h"
#include "graphics/render_device.h"
// #include "graphics/vulkan_types.h"

using ModelPtr = std::shared_ptr<Model>;

class Renderer
{
    friend class Application;

public:
    void Init(SDL_Window *window);
    void Shutdown();

    // should be called *after* loading entities/materials/textures/models/etc.
    void LoadDynamicResources();

    void ReloadShaders();
    void DrawFrame();

    vec2 GetScreenSize();
    float GetAspectRatio();

    RenderDevice &GetDevice() { return device; }

    void SetCamera(Camera &camera) { camera_ = &camera; } // should be set

private:
    Renderer() {};
    Renderer(const Renderer &) = delete;
    Renderer(Renderer &&) = delete;
    Renderer &operator=(const Renderer &) = delete;
    Renderer &operator=(Renderer &&) = delete;

    void DrawModel(VkCommandBuffer cmd, ModelPtr model, mat4 worldMatrix);
    void DrawMesh(VkCommandBuffer cmd, Mesh &mesh, mat4 worldMatrix);

    void ResizeWindow();
    void CreateAttachmentImages();
    void DestroyAttachmentImages();

    void CompileShaders();
    void CreatePipelines();

    void DestroyPipelines();

    void UpdateDynamicData(Camera &camera);
    void UpdateShadowmapCascades(Camera &camera, vec3 lightDir);
    void UpdateGpuLights();

    RenderDevice device;
    SDL_Window *window;

    Camera *camera_ = nullptr;
    // CacheManager cacheManager;

    TexturePtr colorImage;
    TexturePtr depthImage;

    Array<BufferPtr, FRAMES_IN_FLIGHT> sceneInfoBuffers;
    BufferPtr materialsBuffer;
    BufferPtr lightsBuffer;
    BufferPtr cascadesBuffer;

    Vector<GPULight> gpuLights;
    // Vector<EntityID> lightEntities;

    SamplerPtr linearSampler;
    SamplerPtr nearestSampler;

    RenderPipelinePtr meshPipeline;
    RenderPipelinePtr skyboxPipeline;
    RenderPipelinePtr shadowmapPipeline;

    BarrierMerger barriers;

    Array<Cascade, SHADOWMAP_CASCADES> cascades;
    Array<TexturePtr, SHADOWMAP_CASCADES> shadowmapImages;

    float cascadeSplitLambda = 0.95f;
};

static Renderer *gRenderer = nullptr;