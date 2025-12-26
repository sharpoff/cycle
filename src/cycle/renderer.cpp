#include "cycle/renderer.h"

#include "cycle/filesystem.h"
#include "cycle/graphics/graphics_types.h"
#include "cycle/graphics/render_device.h"
#include "cycle/id_types.h"

#include "cycle/globals.h"

void Renderer::init(SDL_Window *window)
{
    device.init(window);
}

void Renderer::shutdown()
{
    device.waitIdle();

    destroyAttachmentImages();

    ResourceManager *resourceManager = g_engine->getResourceManager();
    resourceManager->destroyAllResources();

    device.destroyBuffer(sceneInfoBuffer);
    device.destroyBuffer(vertexBuffer);

    device.destroySampler(linearSampler);
    device.destroySampler(nearestSampler);

    device.destroyPipelineLayout(geometryPipelineLayout);
    device.destroyRenderPipeline(geometryPipeline);

    device.shutdown();
}

void Renderer::loadResources()
{
    createAttachmentImages();

    // create global data buffer
    {
        const BufferCreateInfo createInfo = {
            .size = sizeof(SceneInfo),
            .usage = BUFFER_USAGE_UNIFORM,
        };

        bool created = device.createBuffer(sceneInfoBuffer, createInfo);
        assert(created);
    }

    // create samplers
    {
        { // linear
            SamplerCreateInfo samplerCreateInfo = {};
            samplerCreateInfo.magFilter = SAMPLER_FILTER_LINEAR;
            samplerCreateInfo.minFilter = SAMPLER_FILTER_LINEAR;
            samplerCreateInfo.addressModeU = SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            samplerCreateInfo.addressModeV = SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            samplerCreateInfo.addressModeW = SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            device.createSampler(linearSampler, samplerCreateInfo);
        }

        { // nearest
            SamplerCreateInfo samplerCreateInfo = {};
            samplerCreateInfo.magFilter = SAMPLER_FILTER_NEAREST;
            samplerCreateInfo.minFilter = SAMPLER_FILTER_NEAREST;
            samplerCreateInfo.addressModeU = SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            samplerCreateInfo.addressModeV = SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            samplerCreateInfo.addressModeW = SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            device.createSampler(nearestSampler, samplerCreateInfo);
        }
    }

    ResourceManager *resourceManager = g_engine->getResourceManager();
    assert(resourceManager);

    // load image
    {
        auto id = resourceManager->loadTextureFromFile("checkerboard", "resources/textures/checkerboard.png");
        if (id == TextureID::Invalid) {
            LOGE("%s", "Failed to load a texture!");
            exit(-1);
        }
    }

    // load model
    {
        auto id = resourceManager->loadModelFromFile("monkey", "resources/models/monkey.gltf"); 
        if (id == ModelID::Invalid) {
            LOGE("%s", "Failed to load a model!");
            exit(-1);
        }
    }

    // create pipelines
    {
        const Vector<DescriptorSetLayoutBinding> bindings0 = {
            {0, DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, SHADER_STAGE_VERTEX}, // scene data
            {1, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024, SHADER_STAGE_FRAGMENT}, // textures
        };

        const Vector<DescriptorSetLayout> descriptorSetLayouts = {
            {bindings0}, // set 0
        };

        // const Vector<PushConstantRange> pushConstantRanges = {
        //     {SHADER_STAGE_VERTEX, 0, sizeof(MeshDrawInfo)}
        // };

        PipelineLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.descriptorSetLayouts = descriptorSetLayouts;
        // layoutCreateInfo.pushConstantRanges = pushConstantRanges;
        device.createPipelineLayout(geometryPipelineLayout, layoutCreateInfo);

        const RenderPipelineCreateInfo createInfo = {
            .vertexBindings = {
                {0, sizeof(Vertex), VERTEX_INPUT_RATE_VERTEX}
            },
            .vertexAttributes = {
                {0, 0, VERTEX_FORMAT_R32G32B32_SFLOAT,  offsetof(Vertex, position)},
                {1, 0, VERTEX_FORMAT_R32_SFLOAT,        offsetof(Vertex, uv_x)},
                {2, 0, VERTEX_FORMAT_R32G32B32_SFLOAT,  offsetof(Vertex, normal)},
                {3, 0, VERTEX_FORMAT_R32_SFLOAT,        offsetof(Vertex, uv_y)},
                {4, 0, VERTEX_FORMAT_R32G32B32_SFLOAT,  offsetof(Vertex, color)},
            },
            .cullMode = CULL_MODE_BACK,
            .frontFace = FRONT_FACE_COUNTER_CLOCKWISE,
            .depthCompareOp = COMPARE_OP_GREATER,
            .depthWriteEnable = true,
            .colorAttachmentFormats = {IMAGE_FORMAT_B8G8R8A8_SRGB},
            .depthAttachmentFormat = IMAGE_FORMAT_D32_SFLOAT,
            .pipelineLayout = &geometryPipelineLayout,
            .vertexCode = filesystem::readBinaryFile("resources/shaders/bin/triangle.vert.spv"),
            .fragmentCode = filesystem::readBinaryFile("resources/shaders/bin/triangle.frag.spv"),
        };
        device.createRenderPipeline(geometryPipeline, createInfo);
    }

    // write descriptor set
    {
        device.writeDescriptor(0, sceneInfoBuffer, DESCRIPTOR_TYPE_UNIFORM_BUFFER);

        if (Image *texture = resourceManager->getTextureByName("checkerboard"); texture != nullptr) {
            device.writeDescriptor(1, *texture, linearSampler, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        }

        uint32_t set = 0;
        device.updateDescriptors(geometryPipelineLayout, set);
    }
}

void Renderer::draw()
{
    CommandEncoder cmdEncoder = {};
    if (!device.beginCommandEncoding(cmdEncoder)) {
        // recreate all swapchain dependant resources
        destroyAttachmentImages();
        createAttachmentImages();
        return;
    }

    updateDynamicData();

    AttachmentInfo swapchainAttachment = {
        .image = &device.getSwapchainImage(),
        .load = false,
        .store = true,
    };

    AttachmentInfo depthAttachment = {
        .image = &depthImage,
        .load = false,
        .store = true,
    };

    cmdEncoder.beginRendering({
        .renderAreaExtent = device.getWindowSize(),
        .colorAttachments = {swapchainAttachment},
        .depthAttachment = &depthAttachment,
    });

    ResourceManager *resourceManager = g_engine->getResourceManager();
    assert(resourceManager);

    cmdEncoder.bindPipeline(geometryPipeline);
    if (Model *model = resourceManager->getModelByName("monkey"); model != nullptr) {
        for (MeshID meshID : model->meshes) {
            if (meshID == MeshID::Invalid)
                continue;

            Mesh *mesh = resourceManager->getMeshByID(meshID);
            if (!mesh)
                continue;

            // MeshDrawInfo push = {};
            // push.vertexBufferAddress = mesh->vertexBuffer.address;
            // push.indexBufferAddress = mesh->indexBuffer.address;

            // cmdEncoder.pushConstants(geometryPipelineLayout, SHADER_STAGE_VERTEX, &push, sizeof(push));
            cmdEncoder.bindVertexBuffer(mesh->vertexBuffer);
            cmdEncoder.bindIndexBuffer(mesh->indexBuffer);

            cmdEncoder.drawIndexed(mesh->indices.size(), 1, 0, 0, 0);
        }
    }

    // cmdEncoder.bindPipeline(geometryPipeline);

    // cmdEncoder.bindVertexBuffer(vertexBuffer);
    // cmdEncoder.draw(vertices.size(), 1, 0, 0);

    cmdEncoder.endRendering();
    device.endCommandEncoding(cmdEncoder);

    if (!device.swapchainPresent()) {
        // recreate all swapchain dependant resources
        destroyAttachmentImages();
        createAttachmentImages();
        return;
    }
}

void Renderer::createAttachmentImages()
{
    vec2 windowSize = device.getWindowSize();

    { // color image
        ImageCreateInfo createInfo = {
            .width = (uint32_t)windowSize.x,
            .height = (uint32_t)windowSize.y,
            .mipLevels = calculateMipLevels(windowSize.x, windowSize.y),
            .sampleCount = 1,
            .usage = IMAGE_USAGE_COLOR_ATTACHMENT,
            .format = IMAGE_FORMAT_R8G8B8A8_SRGB,
        };

        bool created = device.createImage(colorImage, createInfo, "color image");
        assert(created);
    }

    { // depth image
        ImageCreateInfo createInfo = {
            .width = (uint32_t)windowSize.x,
            .height = (uint32_t)windowSize.y,
            .mipLevels = 1,
            .sampleCount = 1,
            .usage = IMAGE_USAGE_DEPTH_ATTACHMENT,
            .format = IMAGE_FORMAT_D32_SFLOAT,
        };

        bool created = device.createImage(depthImage, createInfo, "depth image");
        assert(created);
    }
}

void Renderer::destroyAttachmentImages()
{
    device.destroyImage(colorImage);
    device.destroyImage(depthImage);
}

void Renderer::updateDynamicData()
{
    assert(camera);
    if (void *mapped = sceneInfoBuffer.allocation.info.pMappedData; mapped != nullptr) {
        SceneInfo sceneInfo = {};
        sceneInfo.viewProjection = camera->getProjection() * camera->getView();
        memcpy(mapped, &sceneInfo, sizeof(sceneInfo));
    }
}

uint32_t Renderer::calculateMipLevels(uint32_t width, uint32_t height)
{
    return floor(log2(std::max(width, height))) + 1;
}