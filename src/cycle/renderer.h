#pragma once

#include "cycle/graphics/barrier_merger.h"
#include "cycle/types/camera.h"
#include "cycle/graphics/vulkan_types.h"
#include "cycle/graphics/render_device.h"

#define DEFAULT_TEXTURE_ID 0
#define DEFAULT_MATERIAL_ID 0
#define SAMPLER_LINEAR_ID 0
#define SAMPLER_NEAREST_ID 1

class World;
class Editor;

class Renderer
{
public:
    void init(SDL_Window *window);
    void shutdown();

    void loadResources();
    void reloadShaders();
    void draw(World &world, Editor &editor);

    void setCamera(Camera *camera) { this->camera = camera; }

    RenderDevice &getRenderDevice() { return device; }

private:
    void resizeWindow();
    void createAttachmentImages();
    void destroyAttachmentImages();

    void compileShaders();
    void createPipelines();

    void updateDynamicData();

    RenderDevice device;
    Camera *camera;

    Image colorImage;
    Image depthImage;

    Buffer sceneInfoBuffer;
    Buffer materialsBuffer;

    Sampler linearSampler;
    Sampler nearestSampler;

    PipelineLayout geometryPipelineLayout;
    RenderPipeline geometryPipeline;

    BarrierMerger barriers;
};