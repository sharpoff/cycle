#pragma once

#include "core/camera.h"
#include "core/constants.h"
#include "graphics/cascade.h"
#include "graphics/types.h"
#include "graphics/mesh.h"
#include "graphics/model.h"
#include "graphics/render_device.h"

class Renderer
{
    friend class Engine;

public:
    void Initialize(Window *window);
    void Shutdown();

    // add texture to be used in descriptors (bindless access)
    void AddTextureToDescriptor(uint32_t textureIndex);

    // similar to texture variant, but you should add texture to descriptor *before* adding material, so that material's textures have ids
    void AddMaterialToDescriptor(uint32_t materialIndex);

    // should be called *after* loading entities/materials/textures/models/etc.
    void LoadResources();

    void RenderFrame();
    void ReloadShaders();

    vec2 GetScreenSize();
    float GetAspectRatio();

    RenderDevice &GetDevice() { return m_device; }

    void SetCamera(Camera &camera) { m_camera = &camera; } // should be set

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

    bool firstRun = true;
    RenderDevice m_device;

    Camera *m_camera = nullptr;

    Vector<uint32_t> m_descriptorTextureIndices;
    Vector<uint32_t> m_descriptorMaterialIndices;

    Texture m_colorImage;
    Texture m_depthImage;

    Array<Buffer, FRAMES_IN_FLIGHT> m_sceneInfoBuffers;
    Buffer m_materialsBuffer;
    Buffer m_lightsBuffer;
    Buffer m_cascadesBuffer;

    Vector<GPULight> m_gpuLights;

    Sampler m_linearSampler;
    Sampler m_nearestSampler;

    RenderPipeline m_meshPipeline;
    RenderPipeline m_skyboxPipeline;
    RenderPipeline m_shadowmapPipeline;

    // Shadows
    Array<Cascade, SHADOWMAP_CASCADES> m_cascades;
    Array<Texture, SHADOWMAP_CASCADES> m_shadowmapImages;

    float cascadeSplitLambda = 0.95f;
};

inline Renderer *gRenderer = nullptr;