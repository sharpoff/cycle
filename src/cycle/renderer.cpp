#include "cycle/renderer.h"

#include "cycle/filesystem.h"

#include "cycle/graphics/render_device.h"
#include "stb_image.h"

void Renderer::init(SDL_Window *window)
{
    device.init(window);

    vec2 windowSize = device.getWindowSize();

    { // color target
        ImageCreateInfo createInfo = {
            .width = (uint32_t)windowSize.x,
            .height = (uint32_t)windowSize.y,
            .mipLevels = calculateMipLevels(windowSize.x, windowSize.y),
            .sampleCount = 1,
            .usage = IMAGE_USAGE_COLOR_ATTACHMENT,
            .format = IMAGE_FORMAT_R8G8B8A8_SRGB,
        };

        colorTarget = device.createImage(createInfo);
    }

    { // depth target
        ImageCreateInfo createInfo = {
            .width = (uint32_t)windowSize.x,
            .height = (uint32_t)windowSize.y,
            .mipLevels = 1,
            .sampleCount = 1,
            .usage = IMAGE_USAGE_DEPTH_ATTACHMENT,
            .format = IMAGE_FORMAT_D32_SFLOAT,
        };

        depthTarget = device.createImage(createInfo);
    }

    // create pipelines
    {
        const Vector<DescriptorSetLayoutBinding> bindings0 = {
            {0, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024, SHADER_STAGE_FRAGMENT}, // textures
            {1, DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, SHADER_STAGE_VERTEX}, // vertices
            {2, DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, SHADER_STAGE_VERTEX}, // global data
        };

        const Vector<DescriptorSetLayout> descriptorSetLayouts = {
            {bindings0}, // set 0
        };

        PipelineLayoutCreateInfo layoutCreateInfo = {};
        layoutCreateInfo.descriptorSetLayouts = descriptorSetLayouts;

        geometryPipelineLayout = device.createPipelineLayout(layoutCreateInfo);

        const RenderPipelineCreateInfo createInfo = {
            .depthCompareOp = COMPARE_OP_ALWAYS,
            .depthWriteEnable = true,
            .colorAttachmentFormats = {IMAGE_FORMAT_B8G8R8A8_SRGB},
            .depthAttachmentFormat = IMAGE_FORMAT_D32_SFLOAT,
            .pipelineLayout = geometryPipelineLayout,
            .vertexCode = filesystem::readBinaryFile("resources/shaders/bin/triangle.vert.spv"),
            .fragmentCode = filesystem::readBinaryFile("resources/shaders/bin/triangle.frag.spv"),
        };

        geometryPipeline = device.createRenderPipeline(createInfo);
    }

    // create vertex buffer
    {
        vertices = {
            {{-1.0f, -1.0f, -1.0f}, 0.0f, {1.0f, 0.0f, 0.0f}, 1.0f},
            {{1.0f, -1.0f, -1.0f}, 1.0f, {0.0f, 1.0f, 0.0f}, 1.0f},
            {{0.0f, 1.0f, -1.0f}, 0.5f, {0.0f, 0.0f, 1.0f}, 0.0f},
        };

        const BufferCreateInfo createInfo = {
            .size = vertices.size() * sizeof(Vertex),
            .usage = BUFFER_USAGE_STORAGE | BUFFER_USAGE_TRANSFER_DST,
        };
        vertexBuffer = device.createBuffer(createInfo);

        device.uploadBufferData(vertexBuffer, vertices.data(), createInfo.size);
    }

    // create global data buffer
    {
        const BufferCreateInfo createInfo = {
            .size = vertices.size() * sizeof(Vertex),
            .usage = BUFFER_USAGE_UNIFORM,
        };
        globalDataBuffer = device.createBuffer(createInfo);
    }

    // create samplers
    {
        SamplerCreateInfo samplerCreateInfo = {};
        samplerCreateInfo.magFilter = SAMPLER_FILTER_LINEAR;
        samplerCreateInfo.minFilter = SAMPLER_FILTER_LINEAR;
        samplerCreateInfo.addressModeU = SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerCreateInfo.addressModeV = SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerCreateInfo.addressModeW = SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;

        linearSampler = device.createSampler(samplerCreateInfo);

        samplerCreateInfo.magFilter = SAMPLER_FILTER_NEAREST;
        samplerCreateInfo.minFilter = SAMPLER_FILTER_NEAREST;
        nearestSampler = device.createSampler(samplerCreateInfo);
    }

    // load image
    {
        testImage = loadImageFromFile("resources/textures/checkerboard.png", IMAGE_USAGE_SAMPLED | IMAGE_USAGE_TRANSFER_DST);
    }

    // write descriptor set
    {
        uint32_t set = 0;
        device.writeDescriptor(0, testImage, linearSampler, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        device.writeDescriptor(1, vertexBuffer, DESCRIPTOR_TYPE_STORAGE_BUFFER);
        device.writeDescriptor(2, globalDataBuffer, DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        device.updateDescriptors(geometryPipeline->layout, set);
    }
}

void Renderer::shutdown()
{
    device.waitIdle();

    device.destroyImage(&colorTarget);
    device.destroyImage(&depthTarget);
    device.destroyImage(&testImage);

    device.destroyBuffer(&vertexBuffer);
    device.destroyBuffer(&globalDataBuffer);

    device.destroySampler(&linearSampler);
    device.destroySampler(&nearestSampler);

    device.destroyPipelineLayout(&geometryPipelineLayout);
    device.destroyPipeline(&geometryPipeline);

    device.shutdown();
}

void Renderer::draw()
{
    CommandBuffer *cmd = device.beginCommandBuffer();
    if (cmd == nullptr) {
        return;
    }

    updateDynamicData();

    AttachmentResource swapchainAttachment = {
        .image = device.getSwapchainImage(),
        .load = false,
        .store = true,
    };

    AttachmentResource depthAttachment = {
        .image = depthTarget,
        .load = false,
        .store = true,
    };

    Vector<AttachmentResource> colorAttachments = {swapchainAttachment};

    RenderingInfo renderInfo = {};
    renderInfo.colorAttachments = colorAttachments;
    renderInfo.depthAttachment = &depthAttachment;

    device.beginRendering(cmd, renderInfo);

    device.bindPipeline(cmd, geometryPipeline);
    device.draw(cmd, vertices.size(), 1, 0, 0);

    device.endRendering(cmd);

    device.endCommandBuffer(cmd);
    device.submitCommandBuffer(cmd);

    device.destroyCommandBuffer(&cmd);
}

void Renderer::updateDynamicData()
{
    assert(camera);
    if (void *mapped = device.getMappedData(globalDataBuffer); mapped != nullptr) {
        globalData.viewProjection = camera->getProjection() * camera->getView();
        memcpy(mapped, &globalData, sizeof(GlobalData));
    }
}

uint32_t Renderer::calculateMipLevels(uint32_t width, uint32_t height)
{
    return floor(log2(std::max(width, height))) + 1;
}

Image *Renderer::loadImageFromFile(const char *file, ImageUsageFlags imageUsage)
{
    uint32_t width, height, channels;
    unsigned char *pixels = stbi_load(file, (int *)&width, (int *)&height, (int *)&channels, STBI_rgb_alpha);
    if (!pixels) {
        LOGE("Failed to load image from file '%s'!", file);
        return nullptr;
    }

    const ImageCreateInfo createInfo = {
        .width = width,
        .height = height,
        .usage = imageUsage,
        .format = IMAGE_FORMAT_R8G8B8A8_UNORM,
    };

    // for uploads
    if ((imageUsage & IMAGE_USAGE_TRANSFER_DST) != IMAGE_USAGE_TRANSFER_DST)
        imageUsage |= IMAGE_USAGE_TRANSFER_DST;

    Image *image = device.createImage(createInfo);
    device.uploadImageData(image, pixels, width * height * STBI_rgb_alpha);

    return image;
}