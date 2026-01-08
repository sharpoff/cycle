#pragma once

#include "cycle/graphics/barrier_merger.h"
#include "cycle/types/camera.h"
#include "cycle/graphics/vulkan_types.h"
#include "cycle/graphics/render_device.h"
#include "cycle/types/gpu_light.h"
#include "cycle/types/id.h"
#include <filesystem>

const std::filesystem::path shadersDir = "resources/shaders/";
const std::filesystem::path shadersBinaryDir = "resources/shaders/bin/";
const std::filesystem::path texturesDir = "resources/textures/";
const std::filesystem::path modelsDir = "resources/models/";

#define DEFAULT_TEXTURE_ID 0
#define DEFAULT_MATERIAL_ID 0
#define SAMPLER_LINEAR_ID 0
#define SAMPLER_NEAREST_ID 1

class Model;

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

    void drawModel(CommandEncoder &encoder, Model *model, mat4 worldMatrix);

    void resizeWindow();
    void createAttachmentImages();
    void destroyAttachmentImages();

    void compileShaders();
    void createPipelines();
    void destroyPipelines();

    void updateDynamicResources();
    void updateGPULights();

    RenderDevice device;
    SDL_Window *window;
    Camera *camera;

    Image colorImage;
    Image depthImage;

    Buffer sceneInfoBuffer;
    Buffer materialsBuffer;
    Buffer lightsBuffer;

    Vector<GPULight> gpuLights;
    Vector<EntityID> lightEntities;

    Sampler linearSampler;
    Sampler nearestSampler;

    PipelineLayout meshPipelineLayout;
    RenderPipeline meshPipeline;
    RenderPipeline skyboxPipeline;

    BarrierMerger barriers;
};