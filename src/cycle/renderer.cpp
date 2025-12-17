#include "cycle/renderer.h"

#include "cycle/filesystem.h"

#include "cycle/graphics/render_device.h"
#include "stb_image.h"

namespace Renderer
{
    void Init(SDL_Window *window)
    {
        device.Init(window);

        vec2 windowSize = device.GetWindowSize();

        { // color target
            ImageCreateInfo createInfo = {
                .width = (uint32_t)windowSize.x,
                .height = (uint32_t)windowSize.y,
                .mipLevels = CalculateMipLevels(windowSize.x, windowSize.y),
                .sampleCount = 1,
                .usage = IMAGE_USAGE_COLOR_ATTACHMENT,
                .format = IMAGE_FORMAT_R8G8B8A8_SRGB,
            };

            colorTarget = device.CreateImage(createInfo);
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

            depthTarget = device.CreateImage(createInfo);
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

            geometryPipelineLayout = device.CreatePipelineLayout(layoutCreateInfo);

            const RenderPipelineCreateInfo createInfo = {
                .depthCompareOp = COMPARE_OP_ALWAYS,
                .depthWriteEnable = true,
                .colorAttachmentFormats = {IMAGE_FORMAT_B8G8R8A8_SRGB},
                .depthAttachmentFormat = IMAGE_FORMAT_D32_SFLOAT,
                .pipelineLayout = geometryPipelineLayout,
                .vertexCode = filesystem::readBinaryFile("shaders/bin/triangle.vert.spv"),
                .fragmentCode = filesystem::readBinaryFile("shaders/bin/triangle.frag.spv"),
            };

            geometryPipeline = device.CreateRenderPipeline(createInfo);
        }

        // create vertex buffer
        {
            vertices = {
                {{-1.0f, 1.0f, -1.0f}, 0.0f, {1.0f, 0.0f, 0.0f}, 1.0f},
                {{1.0f, 1.0f, -1.0f}, 1.0f, {0.0f, 1.0f, 0.0f}, 1.0f},
                {{0.0f, -1.0f, -1.0f}, 0.5f, {0.0f, 0.0f, 1.0f}, 0.0f},
            };

            const BufferCreateInfo createInfo = {
                .size = vertices.size() * sizeof(Vertex),
                .usage = BUFFER_USAGE_STORAGE | BUFFER_USAGE_TRANSFER_DST,
            };
            vertexBuffer = device.CreateBuffer(createInfo);

            device.UploadBufferData(vertexBuffer, vertices.data(), createInfo.size);
        }

        // create global data buffer
        {
            const BufferCreateInfo createInfo = {
                .size = vertices.size() * sizeof(Vertex),
                .usage = BUFFER_USAGE_UNIFORM,
            };
            globalDataBuffer = device.CreateBuffer(createInfo);
        }

        // create samplers
        {
            SamplerCreateInfo samplerCreateInfo = {};
            samplerCreateInfo.magFilter = SAMPLER_FILTER_LINEAR;
            samplerCreateInfo.minFilter = SAMPLER_FILTER_LINEAR;
            samplerCreateInfo.addressModeU = SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            samplerCreateInfo.addressModeV = SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            samplerCreateInfo.addressModeW = SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;

            linearSampler = device.CreateSampler(samplerCreateInfo);

            samplerCreateInfo.magFilter = SAMPLER_FILTER_NEAREST;
            samplerCreateInfo.minFilter = SAMPLER_FILTER_NEAREST;
            nearestSampler = device.CreateSampler(samplerCreateInfo);
        }

        // load image
        {
            testImage = LoadImageFromFile("assets/textures/checkerboard.png", IMAGE_USAGE_SAMPLED | IMAGE_USAGE_TRANSFER_DST);
        }

        // write descriptor set
        {
            uint32_t set = 0;
            device.WriteDescriptor(0, testImage, linearSampler, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            device.WriteDescriptor(1, vertexBuffer, DESCRIPTOR_TYPE_STORAGE_BUFFER);
            device.WriteDescriptor(2, globalDataBuffer, DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            device.UpdateDescriptors(geometryPipeline->layout, set);
        }
    }

    void Shutdown()
    {
        device.Shutdown();
    }

    void Draw()
    {
    }

    uint32_t CalculateMipLevels(uint32_t width, uint32_t height)
    {
        return floor(log2(std::max(width, height))) + 1;
    }

    Image *LoadImageFromFile(const char *file, ImageUsageFlags imageUsage)
    {
        uint32_t       width, height, channels;
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

        Image *image = device.CreateImage(createInfo);
        device.UploadImageData(image, pixels, width * height * STBI_rgb_alpha);

        return image;
    }
} // namespace Renderer