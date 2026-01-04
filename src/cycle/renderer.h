#pragma once

#include "cycle/graphics/barrier_merger.h"
#include "cycle/types/camera.h"
#include "cycle/graphics/vulkan_types.h"
#include "cycle/graphics/render_device.h"
#include "cycle/types/id.h"

#define DEFAULT_TEXTURE_ID 0
#define DEFAULT_MATERIAL_ID 0
#define SAMPLER_LINEAR_ID 0
#define SAMPLER_NEAREST_ID 1

class Editor;
class Model;

class Renderer
{
public:
    void init(SDL_Window *window);
    void shutdown();

    void loadDynamicResources();
    void reloadShaders();
    void draw(Editor &editor);

    void setCamera(Camera *camera) { this->camera = camera; }

    RenderDevice &getRenderDevice() { return device; }

private:
    void drawModel(CommandEncoder &encoder, Model *model, mat4 worldMatrix);

    void resizeWindow();
    void createAttachmentImages();
    void destroyAttachmentImages();

    void compileShaders();
    void createPipelines();
    void destroyPipelines();

    void updateDynamicResources();

    RenderDevice device;
    Camera *camera;

    Image colorImage;
    Image depthImage;

    Buffer sceneInfoBuffer;
    Buffer materialsBuffer;
    Buffer lightsBuffer;

    Vector<EntityID> lightEntities;

    Sampler linearSampler;
    Sampler nearestSampler;

    PipelineLayout meshPipelineLayout;
    RenderPipeline meshPipeline;
    RenderPipeline skyboxPipeline;

    BarrierMerger barriers;
};