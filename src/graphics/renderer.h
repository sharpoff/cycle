#pragma once

#include "graphics/barrier_merger.h"

#include "core/camera.h"
#include "core/constants.h"
#include "graphics/cascade.h"
#include "graphics/gpu_light.h"
#include "graphics/material.h"
#include "graphics/mesh.h"
#include "graphics/model.h"
#include "graphics/render_device.h"
// #include "graphics/vulkan_types.h"

class Renderer
{
    friend class Application;

public:
    void Init(SDL_Window *window);
    void Shutdown();

    // add texture to be used in descriptors (bindless access)
    void AddTextureToDescriptor(Texture *texture);

    // similar to texture variant, but you should add texture to descriptor *before* adding material, so that material's textures have ids
    void AddMaterialToDescriptor(Material *material);

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

    void DrawImGuiDebug();
    void DrawModel(VkCommandBuffer cmd, Model *model, mat4 worldMatrix);
    void DrawMesh(VkCommandBuffer cmd, Mesh &mesh, mat4 worldMatrix);

    void ResizeWindow();
    void CreateAttachmentImages();
    void DestroyAttachmentImages();

    void CompileShaders();
    void CreatePipelines();

    void DestroyPipelines();

    void UpdateDynamicData();
    void UpdateShadowmapCascades(Camera &camera, vec3 lightDir);
    void UpdateGpuLights();

    RenderDevice device;
    SDL_Window *window;

    Camera *camera_ = nullptr;
    // CacheManager cacheManager;

    Vector<Texture *> descriptorTextures;
    Vector<Material *> descriptorMaterials;

    Texture *colorImage;
    Texture *depthImage;

    Array<Buffer *, FRAMES_IN_FLIGHT> sceneInfoBuffers;
    Buffer *materialsBuffer;
    Buffer *lightsBuffer;
    Buffer *cascadesBuffer;

    Vector<GPULight> gpuLights;
    // Vector<EntityID> lightEntities;

    Sampler * linearSampler;
    Sampler * nearestSampler;

    RenderPipeline * meshPipeline;
    RenderPipeline * skyboxPipeline;
    RenderPipeline * shadowmapPipeline;

    BarrierMerger barriers;

    // shadows
    Array<Cascade, SHADOWMAP_CASCADES> cascades;
    Array<Texture *, SHADOWMAP_CASCADES> shadowmapImages;

    float cascadeSplitLambda = 0.95f;
};

inline Renderer *gRenderer = nullptr;