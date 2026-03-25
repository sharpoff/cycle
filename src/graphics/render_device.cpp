#include "graphics/render_device.h"

#include "core/constants.h"
#include "core/logger.h"
#include "glm/common.hpp"
#include "graphics/vulkan_helpers.h"

#include "SDL3/SDL_vulkan.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"
#include "ktx.h"

static const char *imguiConfigFile = "assets/config/imgui.ini";
static const char *imguiLogFile = "assets/config/imgui_log.txt";

void RenderDevice::init(SDL_Window *window)
{
    this->window = window;
    createInstance();

    SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface);

    createDevice();

    VkSampleCountFlags supportedSampleCount = std::min(deviceProperties.limits.framebufferColorSampleCounts, deviceProperties.limits.framebufferDepthSampleCounts);
    if (supportedSampleCount & VK_SAMPLE_COUNT_64_BIT)
        maxSampleCount = VK_SAMPLE_COUNT_64_BIT;
    if (supportedSampleCount & VK_SAMPLE_COUNT_32_BIT)
        maxSampleCount = VK_SAMPLE_COUNT_32_BIT;
    if (supportedSampleCount & VK_SAMPLE_COUNT_16_BIT)
        maxSampleCount = VK_SAMPLE_COUNT_16_BIT;
    if (supportedSampleCount & VK_SAMPLE_COUNT_8_BIT)
        maxSampleCount = VK_SAMPLE_COUNT_8_BIT;
    if (supportedSampleCount & VK_SAMPLE_COUNT_4_BIT)
        maxSampleCount = VK_SAMPLE_COUNT_4_BIT;

    createAllocator();

    createSwapchain();

    if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB)
        LOGI("%s", "Swapchain format: VK_FORMAT_B8G8R8A8_SRGB");
    LOGI("Present mode: %s", vulkan::toString(presentMode));

    createSyncObjects();

    // create command pool
    VkCommandPoolCreateInfo commandPoolCreateInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    commandPoolCreateInfo.queueFamilyIndex = graphicsQueueIndex;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool));
    vulkan::setDebugName(device, (uint64_t)commandPool, VK_OBJECT_TYPE_COMMAND_POOL, "main VkCommandPool");

    // create command buffers
    VkCommandBufferAllocateInfo bufferAllocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    bufferAllocInfo.commandPool = commandPool;
    bufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    bufferAllocInfo.commandBufferCount = commandBuffers.size();
    VK_CHECK(vkAllocateCommandBuffers(device, &bufferAllocInfo, commandBuffers.data()));

    // create immediate submit objects
    {
        VkCommandPoolCreateInfo commandPoolCreateInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        commandPoolCreateInfo.queueFamilyIndex = graphicsQueueIndex;
        commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        VK_CHECK(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &immediateCommandPool));
        vulkan::setDebugName(device, (uint64_t)immediateCommandPool, VK_OBJECT_TYPE_COMMAND_POOL, "immediate VkCommandPool");

        VkCommandBufferAllocateInfo bufferAllocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        bufferAllocInfo.commandPool = immediateCommandPool;
        bufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        bufferAllocInfo.commandBufferCount = 1;
        VK_CHECK(vkAllocateCommandBuffers(device, &bufferAllocInfo, &immediateCommandBuffer));
        vulkan::setDebugName(device, (uint64_t)immediateCommandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER, "immediate VkCommandBuffer");

        VkFenceCreateInfo fenceCreateInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &immediateFence));
        vulkan::setDebugName(device, (uint64_t)immediateFence, VK_OBJECT_TYPE_FENCE, "immediate VkFence");
    }

    // create descriptor pool
    {
        Vector<VkDescriptorPoolSize> poolSizes = {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1024},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1024},
            {VK_DESCRIPTOR_TYPE_SAMPLER, 10},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1024},
        };

        VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        descriptorPoolCreateInfo.maxSets = 0;
        for (auto &poolSize : poolSizes) {
            descriptorPoolCreateInfo.maxSets += poolSize.descriptorCount;
        }
        descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();
        VK_CHECK(vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &descriptorPool));
        vulkan::setDebugName(device, (uint64_t)descriptorPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL, "main VkDescriptorPool");
    }

    // create bindless descriptor set
    {
        const Vector<VkDescriptorSetLayoutBinding> bindings = {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT}, // scene data
            {1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1024, VK_SHADER_STAGE_FRAGMENT_BIT}, // textures
            {2, VK_DESCRIPTOR_TYPE_SAMPLER, 2, VK_SHADER_STAGE_FRAGMENT_BIT},          // samplers
            {3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT},   // materials
            {4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT},   // lights
            {5, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT},   // shadowmap cascade matrices
        };
        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        descriptorSetLayoutCreateInfo.bindingCount = bindings.size();
        descriptorSetLayoutCreateInfo.pBindings = bindings.data();

        VK_CHECK(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout));

        // allocate descriptor set
        VkDescriptorSetAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descriptorSetLayout;

        for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
            VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &bindlessDescriptorSets[i]));
        }
    }

    setupImGui();
}

void RenderDevice::shutdown()
{
    vkDeviceWaitIdle(device);

    shutdownImGui();

    destroySwapchain();

    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);

    vkDestroyCommandPool(device, commandPool, nullptr);

    vkDestroyCommandPool(device, immediateCommandPool, nullptr);
    vkDestroyFence(device, immediateFence, nullptr);

    for (VkSemaphore &semaphore : submitSemaphores) {
        vkDestroySemaphore(device, semaphore, nullptr);
    }

    for (unsigned int i = 0; i < FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, acquireSemaphores[i], nullptr);
        vkDestroyFence(device, finishRenderFences[i], nullptr);
    }

    vmaDestroyAllocator(allocator);

    vkDestroyDevice(device, nullptr);

    vkDestroySurfaceKHR(instance, surface, nullptr);

    vkDestroyInstance(instance, nullptr);
}

BufferPtr RenderDevice::createBuffer(const BufferCreateInfo &createInfo, VmaMemoryUsage memoryUsage)
{
    assert(createInfo.size > 0);

    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = createInfo.size;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.usage = createInfo.usage;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = memoryUsage;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    allocInfo.priority = 1.0;

    BufferPtr buffer = std::make_shared<Buffer>();
    buffer->size = createInfo.size;
    buffer->usage = createInfo.usage;
    VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer->buffer, &buffer->allocation.handle, &buffer->allocation.info));

    if (bufferInfo.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        VkBufferDeviceAddressInfo deviceAddressInfo = {VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR};
        deviceAddressInfo.buffer = buffer->buffer;
        buffer->address = vkGetBufferDeviceAddress(device, &deviceAddressInfo);
    }

    return buffer;
}

TexturePtr RenderDevice::createTexture(const TextureCreateInfo &createInfo)
{
    assert(createInfo.width != 0 && createInfo.height != 0);

    VkImageCreateInfo imageInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = createInfo.format;
    imageInfo.extent.width = createInfo.width;
    imageInfo.extent.height = createInfo.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = createInfo.mipLevels;
    imageInfo.arrayLayers = createInfo.arrayLayers;
    imageInfo.samples = createInfo.sampleCount;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = createInfo.usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.flags = (imageInfo.arrayLayers == 6) ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0; // cubemap

    TexturePtr texture = std::make_shared<Texture>();
    texture->width = createInfo.width;
    texture->height = createInfo.height;
    texture->arrayLayers = createInfo.arrayLayers;
    texture->mipLevels = createInfo.mipLevels;
    texture->sampleCount = createInfo.sampleCount;
    texture->type = createInfo.type;
    texture->usage = createInfo.usage;
    texture->format = createInfo.format;
    texture->aspect = createInfo.aspect;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    VK_CHECK(vmaCreateImage(allocator, &imageInfo, &allocInfo, &texture->image, &texture->allocation.handle, &texture->allocation.info));
    assert(texture->image != VK_NULL_HANDLE);

    // Create image view
    VkImageViewCreateInfo imageViewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    imageViewInfo.image = texture->image;
    imageViewInfo.viewType = texture->type;
    imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.format = texture->format;
    imageViewInfo.subresourceRange = {texture->aspect, 0, texture->mipLevels, 0, texture->arrayLayers};

    VkImageView view;
    VK_CHECK(vkCreateImageView(device, &imageViewInfo, nullptr, &texture->view));

    if (createInfo.debugName)
        vulkan::setDebugName(device, uint64_t(texture->image), VK_OBJECT_TYPE_IMAGE, createInfo.debugName);

    return texture;
}

SamplerPtr RenderDevice::createSampler(const SamplerCreateInfo &createInfo)
{
    VkSamplerCreateInfo samplerCreateInfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    samplerCreateInfo.minFilter = createInfo.minFilter;
    samplerCreateInfo.magFilter = createInfo.magFilter;
    samplerCreateInfo.addressModeU = createInfo.addressModeU;
    samplerCreateInfo.addressModeV = createInfo.addressModeV;
    samplerCreateInfo.addressModeW = createInfo.addressModeW;
    samplerCreateInfo.mipmapMode = createInfo.mipmapMode;
    samplerCreateInfo.compareOp = createInfo.compareOp;
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    samplerCreateInfo.maxLod = createInfo.maxLod;

    SamplerPtr sampler = std::make_shared<Sampler>();
    sampler->mipLodBias = createInfo.mipLodBias;
    sampler->minLod = createInfo.minLod;
    sampler->maxLod = createInfo.maxLod;
    sampler->maxAnisotropy = createInfo.maxAnisotropy;
    sampler->magFilter = createInfo.magFilter;
    sampler->minFilter = createInfo.minFilter;
    sampler->mipmapMode = createInfo.mipmapMode;
    sampler->addressModeU = createInfo.addressModeU;
    sampler->addressModeV = createInfo.addressModeV;
    sampler->addressModeW = createInfo.addressModeW;
    sampler->compareOp = createInfo.compareOp;
    VK_CHECK(vkCreateSampler(device, &samplerCreateInfo, nullptr, &sampler->sampler));

    return sampler;
}

RenderPipelinePtr RenderDevice::createRenderPipeline(const RenderPipelineCreateInfo &createInfo)
{
    VkPipelineLayoutCreateInfo vkLayoutCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    vkLayoutCreateInfo.setLayoutCount = 1;
    vkLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
    vkLayoutCreateInfo.pushConstantRangeCount = createInfo.pushConstantRanges.size();
    vkLayoutCreateInfo.pPushConstantRanges = createInfo.pushConstantRanges.data();

    VkPipelineLayout vkPipelineLayout;
    VK_CHECK(vkCreatePipelineLayout(device, &vkLayoutCreateInfo, nullptr, &vkPipelineLayout));

    Vector<VkPipelineShaderStageCreateInfo> stages;

    VkShaderModule vertexModule = VK_NULL_HANDLE;
    VkShaderModule fragmentModule = VK_NULL_HANDLE;
    VkShaderModule tessellationControlModule = VK_NULL_HANDLE;
    VkShaderModule tessellationEvaluationModule = VK_NULL_HANDLE;

    if (!createInfo.vertexCode.empty()) {
        VkShaderModuleCreateInfo shaderModuleCreateInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        shaderModuleCreateInfo.codeSize = createInfo.vertexCode.size();
        shaderModuleCreateInfo.pCode = (uint32_t*)createInfo.vertexCode.data();
        VK_CHECK(vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &vertexModule));

        VkPipelineShaderStageCreateInfo &stage = stages.emplace_back();
        stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stage.module = vertexModule;
        stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        stage.pName = "main";
    }

    if (!createInfo.fragmentCode.empty()) {
        VkShaderModuleCreateInfo shaderModuleCreateInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        shaderModuleCreateInfo.codeSize = createInfo.fragmentCode.size();
        shaderModuleCreateInfo.pCode = (uint32_t*)createInfo.fragmentCode.data();
        VK_CHECK(vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &fragmentModule));

        VkPipelineShaderStageCreateInfo &stage = stages.emplace_back();
        stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stage.module = fragmentModule;
        stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stage.pName = "main";
    }

    if (!createInfo.tessellationControlCode.empty()) {
        VkShaderModuleCreateInfo shaderModuleCreateInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        shaderModuleCreateInfo.codeSize = createInfo.tessellationControlCode.size();
        shaderModuleCreateInfo.pCode = (uint32_t*)createInfo.tessellationControlCode.data();
        VK_CHECK(vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &tessellationControlModule));

        VkPipelineShaderStageCreateInfo &stage = stages.emplace_back();
        stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stage.module = tessellationControlModule;
        stage.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        stage.pName = "main";
    }

    if (!createInfo.tessellationEvaluationCode.empty()) {
        VkShaderModuleCreateInfo shaderModuleCreateInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        shaderModuleCreateInfo.codeSize = createInfo.tessellationEvaluationCode.size();
        shaderModuleCreateInfo.pCode = (uint32_t*)createInfo.tessellationEvaluationCode.data();
        VK_CHECK(vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &tessellationEvaluationModule));

        VkPipelineShaderStageCreateInfo &stage = stages.emplace_back();
        stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stage.module = tessellationEvaluationModule;
        stage.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        stage.pName = "main";
    }

    VkPipelineVertexInputStateCreateInfo vertexInputState = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInputState.vertexAttributeDescriptionCount = createInfo.vertexAttributes.size();
    vertexInputState.pVertexAttributeDescriptions = createInfo.vertexAttributes.data();
    vertexInputState.vertexBindingDescriptionCount = createInfo.vertexBindings.size();
    vertexInputState.pVertexBindingDescriptions = createInfo.vertexBindings.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssemblyState.topology = createInfo.topology;

    VkPipelineTessellationStateCreateInfo tessellationState = {VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO};
    tessellationState.patchControlPoints = createInfo.patchControlPoints;

    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = getSwapchainWidth();
    viewport.height = getSwapchainHeight();
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = getSwapchainWidth();
    scissor.extent.height = getSwapchainHeight();

    VkPipelineViewportStateCreateInfo viewportState = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizationState = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterizationState.depthClampEnable = VK_FALSE;
    rasterizationState.rasterizerDiscardEnable = VK_FALSE;
    rasterizationState.polygonMode = createInfo.polygonMode;
    rasterizationState.cullMode = createInfo.cullMode;
    rasterizationState.frontFace = createInfo.frontFace;
    rasterizationState.depthBiasEnable = VK_FALSE;
    rasterizationState.depthBiasConstantFactor = 0.0f;
    rasterizationState.depthBiasClamp = 0.0f;
    rasterizationState.depthBiasSlopeFactor = 0.0f;
    rasterizationState.lineWidth = 1.0;

    VkPipelineMultisampleStateCreateInfo multisampleState = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisampleState.rasterizationSamples = createInfo.sampleCount;
    if (deviceFeatures.sampleRateShading) {
        multisampleState.sampleShadingEnable = VK_TRUE;
        multisampleState.minSampleShading = 0.2f;
    } else {
        multisampleState.sampleShadingEnable = VK_FALSE;
    }

    const VkBool32 depthTestEnabled = createInfo.depthCompareOp != VK_COMPARE_OP_ALWAYS;
    const VkBool32 depthWriteEnabled = createInfo.depthWriteEnable;

    VkPipelineDepthStencilStateCreateInfo depthStencilState = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depthStencilState.depthTestEnable = depthTestEnabled || depthWriteEnabled;
    depthStencilState.depthWriteEnable = depthWriteEnabled;
    depthStencilState.depthCompareOp = createInfo.depthCompareOp;
    depthStencilState.depthBoundsTestEnable = VK_FALSE;
    depthStencilState.stencilTestEnable = VK_FALSE; // TODO: stencil
    depthStencilState.front = {}; // TODO: stencil
    depthStencilState.back = {}; // TODO: stencil
    depthStencilState.minDepthBounds = 0.0f;
    depthStencilState.maxDepthBounds = 1.0f;

    const VkBool32 blendEnabled = createInfo.colorBlendOp != VK_BLEND_OP_MAX_ENUM;

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
    colorBlendAttachmentState.blendEnable = blendEnabled;
    colorBlendAttachmentState.srcColorBlendFactor = createInfo.colorBlendFactorSrc;
    colorBlendAttachmentState.dstColorBlendFactor = createInfo.colorBlendFactorDst;
    colorBlendAttachmentState.colorBlendOp = createInfo.colorBlendOp;
    colorBlendAttachmentState.srcAlphaBlendFactor = createInfo.alphaBlendFactorSrc;
    colorBlendAttachmentState.dstAlphaBlendFactor = createInfo.alphaBlendFactorDst;
    colorBlendAttachmentState.alphaBlendOp = createInfo.alphaBlendOp;
    colorBlendAttachmentState.colorWriteMask = createInfo.colorWriteMask;

    Vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(createInfo.colorAttachmentFormats.size(), colorBlendAttachmentState);

    VkPipelineColorBlendStateCreateInfo colorBlendState = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    colorBlendState.logicOpEnable = VK_FALSE;
    colorBlendState.attachmentCount = colorBlendAttachments.size();
    colorBlendState.pAttachments = colorBlendAttachments.data();

    VkPipelineDynamicStateCreateInfo dynamicState = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamicState.dynamicStateCount = createInfo.dynamicStates.size();
    dynamicState.pDynamicStates = createInfo.dynamicStates.data();

    VkPipelineRenderingCreateInfoKHR renderingInfo = {VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR};
    renderingInfo.colorAttachmentCount = createInfo.colorAttachmentFormats.size();
    renderingInfo.pColorAttachmentFormats = createInfo.colorAttachmentFormats.data();
    if (depthTestEnabled || depthWriteEnabled)
        renderingInfo.depthAttachmentFormat = createInfo.depthAttachmentFormat;

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineCreateInfo.pNext = &renderingInfo;
    pipelineCreateInfo.stageCount = stages.size();
    pipelineCreateInfo.pStages = stages.data();
    pipelineCreateInfo.pVertexInputState = &vertexInputState;
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
    pipelineCreateInfo.pTessellationState = &tessellationState;
    pipelineCreateInfo.pViewportState = &viewportState;
    pipelineCreateInfo.pRasterizationState = &rasterizationState;
    pipelineCreateInfo.pMultisampleState = &multisampleState;
    pipelineCreateInfo.pDepthStencilState = &depthStencilState;
    pipelineCreateInfo.pColorBlendState = &colorBlendState;
    pipelineCreateInfo.pDynamicState = &dynamicState;
    pipelineCreateInfo.layout = vkPipelineLayout;

    VkPipeline vkPipeline;
    VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &vkPipeline));

    if (vertexModule)
        vkDestroyShaderModule(device, vertexModule, nullptr);
    if (fragmentModule)
        vkDestroyShaderModule(device, fragmentModule, nullptr);
    if (tessellationControlModule)
        vkDestroyShaderModule(device, tessellationControlModule, nullptr);
    if (tessellationEvaluationModule)
        vkDestroyShaderModule(device, tessellationEvaluationModule, nullptr);

    RenderPipelinePtr renderPipeline = std::make_shared<RenderPipeline>();
    renderPipeline->layout = vkPipelineLayout;
    renderPipeline->pipeline = vkPipeline;

    return renderPipeline;
}

ComputePipelinePtr RenderDevice::createComputePipeline(const ComputePipelineCreateInfo &createInfo)
{
    VkPipelineLayoutCreateInfo vkLayoutCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    vkLayoutCreateInfo.setLayoutCount = 1;
    vkLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
    vkLayoutCreateInfo.pushConstantRangeCount = createInfo.pushConstantRanges.size();
    vkLayoutCreateInfo.pPushConstantRanges = createInfo.pushConstantRanges.data();

    VkPipelineLayout vkPipelineLayout;
    VK_CHECK(vkCreatePipelineLayout(device, &vkLayoutCreateInfo, nullptr, &vkPipelineLayout));

    VkPipelineShaderStageCreateInfo computeStage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    VkShaderModule computeModule = VK_NULL_HANDLE;

    if (!createInfo.computeCode.empty()) {
        VkShaderModuleCreateInfo shaderModuleCreateInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        shaderModuleCreateInfo.codeSize = createInfo.computeCode.size();
        shaderModuleCreateInfo.pCode = (uint32_t*)createInfo.computeCode.data();
        VK_CHECK(vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &computeModule));

        computeStage.module = computeModule;
        computeStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        computeStage.pName = "main";
    }

    assert(computeModule != VK_NULL_HANDLE);

    VkComputePipelineCreateInfo pipelineCreateInfo = {VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    pipelineCreateInfo.stage = computeStage;
    pipelineCreateInfo.layout = vkPipelineLayout;

    VkPipeline vkPipeline;
    VK_CHECK(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &vkPipeline));

    ComputePipelinePtr computePipeline = std::make_shared<ComputePipeline>();
    computePipeline->layout = vkPipelineLayout;
    computePipeline->pipeline = vkPipeline;

    return computePipeline;
}

void RenderDevice::destroyBuffer(BufferPtr buffer)
{
    if (buffer && buffer->buffer != VK_NULL_HANDLE)
        vmaDestroyBuffer(allocator, buffer->buffer, buffer->allocation.handle);
}

void RenderDevice::destroyTexture(TexturePtr texture)
{
    if (!texture)
        return;

    if (texture->view != VK_NULL_HANDLE)
        vkDestroyImageView(device, texture->view, nullptr);

    if (texture->image != VK_NULL_HANDLE)
        vmaDestroyImage(allocator, texture->image, texture->allocation.handle);
}

void RenderDevice::destroySampler(SamplerPtr sampler)
{
    if (sampler && sampler->sampler != VK_NULL_HANDLE)
        vkDestroySampler(device, sampler->sampler, nullptr);
}

void RenderDevice::destroyRenderPipeline(RenderPipelinePtr pipeline)
{
    if (!pipeline)
        return;

    if (pipeline->layout != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(device, pipeline->layout, nullptr);

    if (pipeline->pipeline != VK_NULL_HANDLE)
        vkDestroyPipeline(device, pipeline->pipeline, nullptr);
}

void RenderDevice::destroyComputePipeline(ComputePipelinePtr pipeline)
{
    if (!pipeline)
        return;

    if (pipeline->layout != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(device, pipeline->layout, nullptr);

    if (pipeline->pipeline != VK_NULL_HANDLE)
        vkDestroyPipeline(device, pipeline->pipeline, nullptr);
}

void RenderDevice::uploadBufferData(BufferPtr buffer, void *data, uint64_t size)
{
    assert(buffer && data && size > 0);

    // using mapped data
    if (buffer->allocation.info.pMappedData) {
        memcpy(buffer->allocation.info.pMappedData, data, size);
        return;
    }

    // using created staging buffer
    const BufferCreateInfo createInfo = {
        .size = size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    };

    BufferPtr staging = createBuffer(createInfo, VMA_MEMORY_USAGE_CPU_ONLY);
    memcpy(staging->allocation.info.pMappedData, data, size);

    VK_CHECK(vmaFlushAllocation(allocator, staging->allocation.handle, 0, VK_WHOLE_SIZE));

    immediateSubmit([size, &staging, &buffer](VkCommandBuffer cmd) -> void {
        VkBufferCopy copyRegion = {0, 0, size};
        vkCmdCopyBuffer(cmd, staging->buffer, buffer->buffer, 1, &copyRegion);
    });

    destroyBuffer(staging);
}

void RenderDevice::uploadTexture(TexturePtr texture, TextureLoadInfo &info)
{
    const uint32_t bufsize = info.textureKTX ? info.size : info.size * texture->arrayLayers;

    const BufferCreateInfo createInfo = {
        .size = bufsize,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    };
    BufferPtr staging = createBuffer(createInfo, VMA_MEMORY_USAGE_CPU_ONLY);
    uploadBufferData(staging, info.data, info.size);

    immediateSubmit([&staging, &texture, &info](VkCommandBuffer cmd) -> void {
        // transition image to transfer
        VkImageMemoryBarrier transferBarrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        transferBarrier.srcAccessMask = 0;
        transferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        transferBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        transferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        transferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        transferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        transferBarrier.image = texture->image;
        transferBarrier.subresourceRange = {texture->aspect, 0, texture->mipLevels, 0, texture->arrayLayers};

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, // stages
            0,
            0, nullptr, // memory barriers
            0, nullptr, // buffer memory barriers
            1, &transferBarrier // image memory barriers
        );

        // copy
        Vector<VkBufferImageCopy> copyRegions{};
        if (info.textureKTX) { // copy image and all faces and mip levels
            for (uint32_t j = 0; j < texture->mipLevels; j++) {
                ktx_size_t     offset = 0;
                KTX_error_code ret = ktxTexture_GetImageOffset(info.textureKTX, j, 0, 0, &offset);
                assert(ret == KTX_SUCCESS);

                VkBufferImageCopy copyRegion = {};
                copyRegion.imageSubresource = {texture->aspect, j, 0, texture->arrayLayers};
                copyRegion.imageExtent = {texture->width >> j, texture->height >> j, 1};
                copyRegion.bufferOffset = offset;
                copyRegions.push_back(copyRegion);
            }
        } else { // generate mip levels later if needed.
            VkBufferImageCopy copyRegion = {};
            copyRegion.imageSubresource = {texture->aspect, 0, 0, 1};
            copyRegion.imageExtent = {texture->width, texture->height, 1};
            copyRegions.push_back(copyRegion);
        }

        vkCmdCopyBufferToImage(cmd, staging->buffer, texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, copyRegions.size(), copyRegions.data());

        // transition image to fragment shader
        VkImageMemoryBarrier fragmentBarrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        fragmentBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        fragmentBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        fragmentBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        fragmentBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        fragmentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        fragmentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        fragmentBarrier.image = texture->image;
        fragmentBarrier.subresourceRange = {texture->aspect, 0, texture->mipLevels, 0, texture->arrayLayers};

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, // stages
            0,
            0, nullptr, // memory barriers
            0, nullptr, // buffer memory barriers
            1, &fragmentBarrier // image memory barriers
        );
    });

    destroyBuffer(staging);
}

void RenderDevice::uploadMeshGPUData(BufferPtr vertexBuffer, Vector<Vertex> &vertices, BufferPtr indexBuffer, Vector<uint32_t> &indices)
{
    // vertex buffer
    {
        BufferCreateInfo createInfo = {
            .size = vertices.size() * sizeof(Vertex),
            .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        };

        vertexBuffer = createBuffer(createInfo, VMA_MEMORY_USAGE_GPU_ONLY);
        uploadBufferData(vertexBuffer, vertices.data(), createInfo.size);
    }

    // index buffer
    {
        BufferCreateInfo createInfo = {
            .size = indices.size() * sizeof(uint32_t),
            .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        };

        indexBuffer = createBuffer(createInfo, VMA_MEMORY_USAGE_GPU_ONLY);
        uploadBufferData(indexBuffer, indices.data(), createInfo.size);
    }
}

void RenderDevice::generateMipmaps(TexturePtr texture)
{
    if (texture->mipLevels <= 1) {
        LOGW("%s", "Failed to generate mipmaps. image->mipLevels <= 1");
        return;
    }

    immediateSubmit([&texture](VkCommandBuffer cmd) -> void {
        // transition first mip
        {
            VkImageMemoryBarrier barrier = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
                .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = texture->image,
                .subresourceRange = {texture->aspect, 0, 1, 0, texture->arrayLayers}
            };

            vkCmdPipelineBarrier(cmd,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );
        }

        // copy mips from n-1 to n
        for (uint32_t i = 1; i < texture->mipLevels; i++) {
            VkImageBlit blit{};

            // src
            blit.srcSubresource = {texture->aspect, i - 1, 0, texture->arrayLayers};
            blit.srcOffsets[1].x = static_cast<int32_t>(texture->width >> (i - 1));
            blit.srcOffsets[1].y = static_cast<int32_t>(texture->height >> (i - 1));
            blit.srcOffsets[1].z = 1;

            // dst
            blit.dstSubresource = {texture->aspect, i, 0, texture->arrayLayers};
            blit.dstOffsets[1].x = static_cast<int32_t>(texture->width >> i);
            blit.dstOffsets[1].y = static_cast<int32_t>(texture->height >> i);
            blit.dstOffsets[1].z = 1;

            VkImageSubresourceRange subresourceRange = {texture->aspect, i, 1, 0, texture->arrayLayers};

            // transition mip level to transfer dst
            VkImageMemoryBarrier barrier0 = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
                .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = texture->image,
                .subresourceRange = subresourceRange
            };

            vkCmdPipelineBarrier(cmd,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier0
            );

            // blit from previous mip level
            vkCmdBlitImage(cmd,
                texture->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &blit,
                VK_FILTER_LINEAR
            );

            // transition mip level to transfer src
            VkImageMemoryBarrier barrier1 = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = texture->image,
                .subresourceRange = subresourceRange
            };

            vkCmdPipelineBarrier(cmd,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier1
            );
        }

        // all mip layers are in TRANSFER_SRC, so transition all to SHADER_READ
        VkImageMemoryBarrier barrier1 = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = texture->image,
            .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, texture->mipLevels, 0, texture->arrayLayers}
        };

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier1
        );
    });
}

void RenderDevice::immediateSubmit(Func<void(VkCommandBuffer cmd)> &&function)
{
    VK_CHECK(vkResetFences(device, 1, &immediateFence));
    VK_CHECK(vkResetCommandBuffer(immediateCommandBuffer, 0));

    VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(immediateCommandBuffer, &beginInfo));
    function(immediateCommandBuffer);
    VK_CHECK(vkEndCommandBuffer(immediateCommandBuffer));

    VkSubmitInfo submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &immediateCommandBuffer;

    VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submit, immediateFence));
    VK_CHECK(vkWaitForFences(device, 1, &immediateFence, VK_TRUE, ~0L));
}

VkCommandBuffer RenderDevice::beginCommandBuffer()
{
    VK_CHECK(vkWaitForFences(device, 1, &finishRenderFences[currentFrame], VK_TRUE, ~0ull));
    VK_CHECK(vkResetFences(device, 1, &finishRenderFences[currentFrame]));

    VkResult result = vkAcquireNextImageKHR(device, swapchain, ~0ull, acquireSemaphores[currentFrame], nullptr, &imageIndex);
    if (resizeRequested || result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return VK_NULL_HANDLE;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        LOGE("%s", "Failed to acquire swapchain image.");
        exit(EXIT_FAILURE);
    }

    VkCommandBuffer cmd = commandBuffers[currentFrame];
    VK_CHECK(vkResetCommandBuffer(cmd, 0));

    VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));

    return cmd;
}

void RenderDevice::endCommandBuffer(VkCommandBuffer cmd)
{
    VK_CHECK(vkEndCommandBuffer(cmd));

    VkPipelineStageFlags stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submit = {};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &acquireSemaphores[currentFrame];
    submit.pWaitDstStageMask = stages;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &submitSemaphores[imageIndex];

    VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submit, finishRenderFences[currentFrame]));
}

bool RenderDevice::swapchainPresent()
{
    // Present
    VkPresentInfoKHR presentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &submitSemaphores[imageIndex];

    VkResult result =  vkQueuePresentKHR(graphicsQueue, &presentInfo);
    if (resizeRequested || result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreateSwapchain();
        return false;
    }

    currentFrame = (currentFrame + 1) % FRAMES_IN_FLIGHT;
    return true;
}

void RenderDevice::writeDescriptor(uint32_t binding, BufferPtr buffer, VkDescriptorType type, uint32_t dstArrayElement)
{
    descriptorSetWriter.write(binding, buffer->buffer, buffer->size, type, dstArrayElement);
}

void RenderDevice::writeDescriptor(uint32_t binding, TexturePtr texture, Sampler &sampler, VkDescriptorType type, uint32_t dstArrayElement)
{
    descriptorSetWriter.write(binding, texture->view, sampler.sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, type, dstArrayElement);
}

void RenderDevice::writeDescriptor(uint32_t binding, TexturePtr texture, VkDescriptorType type, uint32_t dstArrayElement)
{
    descriptorSetWriter.write(binding, texture->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, type, dstArrayElement);
}

void RenderDevice::writeDescriptor(uint32_t binding, SamplerPtr sampler, VkDescriptorType type, uint32_t dstArrayElement)
{
    descriptorSetWriter.write(binding, sampler->sampler, type, dstArrayElement);
}

void RenderDevice::updateDescriptors()
{
    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        descriptorSetWriter.update(device, bindlessDescriptorSets[i]);
    }
    descriptorSetWriter.clear();
}

void RenderDevice::waitIdle()
{
    vkDeviceWaitIdle(device);
}

void RenderDevice::createInstance()
{
    VK_CHECK(volkInitialize());

    VkApplicationInfo appCI = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appCI.apiVersion = VULKAN_API_VERSION;
    appCI.pEngineName = "Engine";
    appCI.pApplicationName = "Application";
    appCI.engineVersion = 0;
    appCI.applicationVersion = 0;

    uint32_t extensionCount = 0;
    const char *const *extensionNames = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
    assert(extensionNames && extensionCount > 0);

    Vector<const char *> instanceExtensions(extensionNames, extensionNames + extensionCount);
#ifndef NDEBUG
    instanceExtensions.push_back("VK_EXT_debug_utils");
#endif

    VkInstanceCreateInfo instanceInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instanceInfo.pApplicationInfo = &appCI;
    instanceInfo.enabledLayerCount = 0;
    instanceInfo.ppEnabledLayerNames = nullptr;
    instanceInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
    instanceInfo.ppEnabledExtensionNames = instanceExtensions.data();
    VK_CHECK(vkCreateInstance(&instanceInfo, nullptr, &instance));

    volkLoadInstance(instance);
}

void RenderDevice::createDevice()
{
    uint32_t physicalDeviceCount = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr));
    assert(physicalDeviceCount > 0);

    Vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data()));

    // FIXME: find appropriate device
    physicalDevice = physicalDevices[0];

    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

    printf("Selected GPU: %s\n", deviceProperties.deviceName);

    // Log GPU limits
    if (ENABLE_LOG_DEVICE_LIMITS)
    {
        printf("---- deviceProperties.limits ----\n");
        printf("\tmaxImageDimension1D: %u\n", deviceProperties.limits.maxImageDimension1D);
        printf("\tmaxImageDimension2D: %u\n", deviceProperties.limits.maxImageDimension2D);
        printf("\tmaxImageDimension3D: %u\n", deviceProperties.limits.maxImageDimension3D);
        printf("\tmaxImageDimensionCube: %u\n", deviceProperties.limits.maxImageDimensionCube);
        printf("\tmaxImageArrayLayers: %u\n", deviceProperties.limits.maxImageArrayLayers);
        printf("\tmaxTexelBufferElements: %u\n", deviceProperties.limits.maxTexelBufferElements);
        printf("\tmaxUniformBufferRange: %u\n", deviceProperties.limits.maxUniformBufferRange);
        printf("\tmaxStorageBufferRange: %u\n", deviceProperties.limits.maxStorageBufferRange);
        printf("\tmaxPushConstantsSize: %u\n", deviceProperties.limits.maxPushConstantsSize);
        printf("\tmaxMemoryAllocationCount: %u\n", deviceProperties.limits.maxMemoryAllocationCount);
        printf("\tmaxSamplerAllocationCount: %u\n", deviceProperties.limits.maxSamplerAllocationCount);
        printf("\tbufferImageGranularity: %lu\n", deviceProperties.limits.bufferImageGranularity);
        printf("\tsparseAddressSpaceSize: %lu\n", deviceProperties.limits.sparseAddressSpaceSize);
        printf("\tmaxBoundDescriptorSets: %u\n", deviceProperties.limits.maxBoundDescriptorSets);
        printf("\tmaxPerStageDescriptorSamplers: %u\n", deviceProperties.limits.maxPerStageDescriptorSamplers);
        printf("\tmaxPerStageDescriptorUniformBuffers: %u\n", deviceProperties.limits.maxPerStageDescriptorUniformBuffers);
        printf("\tmaxPerStageDescriptorStorageBuffers: %u\n", deviceProperties.limits.maxPerStageDescriptorStorageBuffers);
        printf("\tmaxPerStageDescriptorSampledImages: %u\n", deviceProperties.limits.maxPerStageDescriptorSampledImages);
        printf("\tmaxPerStageDescriptorStorageImages: %u\n", deviceProperties.limits.maxPerStageDescriptorStorageImages);
        printf("\tmaxPerStageDescriptorInputAttachments: %u\n", deviceProperties.limits.maxPerStageDescriptorInputAttachments);
        printf("\tmaxPerStageResources: %u\n", deviceProperties.limits.maxPerStageResources);
        printf("\tmaxDescriptorSetSamplers: %u\n", deviceProperties.limits.maxDescriptorSetSamplers);
        printf("\tmaxDescriptorSetUniformBuffers: %u\n", deviceProperties.limits.maxDescriptorSetUniformBuffers);
        printf("\tmaxDescriptorSetUniformBuffersDynamic: %u\n", deviceProperties.limits.maxDescriptorSetUniformBuffersDynamic);
        printf("\tmaxDescriptorSetStorageBuffers: %u\n", deviceProperties.limits.maxDescriptorSetStorageBuffers);
        printf("\tmaxDescriptorSetStorageBuffersDynamic: %u\n", deviceProperties.limits.maxDescriptorSetStorageBuffersDynamic);
        printf("\tmaxDescriptorSetSampledImages: %u\n", deviceProperties.limits.maxDescriptorSetSampledImages);
        printf("\tmaxDescriptorSetStorageImages: %u\n", deviceProperties.limits.maxDescriptorSetStorageImages);
        printf("\tmaxDescriptorSetInputAttachments: %u\n", deviceProperties.limits.maxDescriptorSetInputAttachments);
        printf("\tmaxVertexInputAttributes: %u\n", deviceProperties.limits.maxVertexInputAttributes);
        printf("\tmaxVertexInputBindings: %u\n", deviceProperties.limits.maxVertexInputBindings);
        printf("\tmaxVertexInputAttributeOffset: %u\n", deviceProperties.limits.maxVertexInputAttributeOffset);
        printf("\tmaxVertexInputBindingStride: %u\n", deviceProperties.limits.maxVertexInputBindingStride);
        printf("\tmaxVertexOutputComponents: %u\n", deviceProperties.limits.maxVertexOutputComponents);
        printf("\tmaxTessellationGenerationLevel: %u\n", deviceProperties.limits.maxTessellationGenerationLevel);
        printf("\tmaxTessellationPatchSize: %u\n", deviceProperties.limits.maxTessellationPatchSize);
        printf("\tmaxTessellationControlPerVertexInputComponents: %u\n", deviceProperties.limits.maxTessellationControlPerVertexInputComponents);
        printf("\tmaxTessellationControlPerVertexOutputComponents: %u\n", deviceProperties.limits.maxTessellationControlPerVertexOutputComponents);
        printf("\tmaxTessellationControlPerPatchOutputComponents: %u\n", deviceProperties.limits.maxTessellationControlPerPatchOutputComponents);
        printf("\tmaxTessellationControlTotalOutputComponents: %u\n", deviceProperties.limits.maxTessellationControlTotalOutputComponents);
        printf("\tmaxTessellationEvaluationInputComponents: %u\n", deviceProperties.limits.maxTessellationEvaluationInputComponents);
        printf("\tmaxTessellationEvaluationOutputComponents: %u\n", deviceProperties.limits.maxTessellationEvaluationOutputComponents);
        printf("\tmaxGeometryShaderInvocations: %u\n", deviceProperties.limits.maxGeometryShaderInvocations);
        printf("\tmaxGeometryInputComponents: %u\n", deviceProperties.limits.maxGeometryInputComponents);
        printf("\tmaxGeometryOutputComponents: %u\n", deviceProperties.limits.maxGeometryOutputComponents);
        printf("\tmaxGeometryOutputVertices: %u\n", deviceProperties.limits.maxGeometryOutputVertices);
        printf("\tmaxGeometryTotalOutputComponents: %u\n", deviceProperties.limits.maxGeometryTotalOutputComponents);
        printf("\tmaxFragmentInputComponents: %u\n", deviceProperties.limits.maxFragmentInputComponents);
        printf("\tmaxFragmentOutputAttachments: %u\n", deviceProperties.limits.maxFragmentOutputAttachments);
        printf("\tmaxFragmentDualSrcAttachments: %u\n", deviceProperties.limits.maxFragmentDualSrcAttachments);
        printf("\tmaxFragmentCombinedOutputResources: %u\n", deviceProperties.limits.maxFragmentCombinedOutputResources);
        printf("\tmaxComputeSharedMemorySize: %u\n", deviceProperties.limits.maxComputeSharedMemorySize);
        printf("\tmaxComputeWorkGroupCount[0]: %u\n", deviceProperties.limits.maxComputeWorkGroupCount[0]);
        printf("\tmaxComputeWorkGroupCount[1]: %u\n", deviceProperties.limits.maxComputeWorkGroupCount[1]);
        printf("\tmaxComputeWorkGroupCount[2]: %u\n", deviceProperties.limits.maxComputeWorkGroupCount[2]);
        printf("\tmaxComputeWorkGroupInvocations: %u\n", deviceProperties.limits.maxComputeWorkGroupInvocations);
        printf("\tmaxComputeWorkGroupSize[0]: %u\n", deviceProperties.limits.maxComputeWorkGroupSize[0]);
        printf("\tmaxComputeWorkGroupSize[1]: %u\n", deviceProperties.limits.maxComputeWorkGroupSize[1]);
        printf("\tmaxComputeWorkGroupSize[2]: %u\n", deviceProperties.limits.maxComputeWorkGroupSize[2]);
        printf("\tsubPixelPrecisionBits: %u\n", deviceProperties.limits.subPixelPrecisionBits);
        printf("\tsubTexelPrecisionBits: %u\n", deviceProperties.limits.subTexelPrecisionBits);
        printf("\tmipmapPrecisionBits: %u\n", deviceProperties.limits.mipmapPrecisionBits);
        printf("\tmaxDrawIndexedIndexValue: %u\n", deviceProperties.limits.maxDrawIndexedIndexValue);
        printf("\tmaxDrawIndirectCount: %u\n", deviceProperties.limits.maxDrawIndirectCount);
        printf("\tmaxSamplerLodBias: %f\n", deviceProperties.limits.maxSamplerLodBias);
        printf("\tmaxSamplerAnisotropy: %f\n", deviceProperties.limits.maxSamplerAnisotropy);
        printf("\tmaxViewports: %u\n", deviceProperties.limits.maxViewports);
        printf("\tmaxViewportDimensions[0]: %u\n", deviceProperties.limits.maxViewportDimensions[0]);
        printf("\tmaxViewportDimensions[1]: %u\n", deviceProperties.limits.maxViewportDimensions[1]);
        printf("\tviewportBoundsRange[0]: %f\n", deviceProperties.limits.viewportBoundsRange[0]);
        printf("\tviewportBoundsRange[1]: %f\n", deviceProperties.limits.viewportBoundsRange[1]);
        printf("\tviewportSubPixelBits: %u\n", deviceProperties.limits.viewportSubPixelBits);
        printf("\tminMemoryMapAlignment: %lu\n", deviceProperties.limits.minMemoryMapAlignment);
        printf("\tminTexelBufferOffsetAlignment: %lu\n", deviceProperties.limits.minTexelBufferOffsetAlignment);
        printf("\tminUniformBufferOffsetAlignment: %lu\n", deviceProperties.limits.minUniformBufferOffsetAlignment);
        printf("\tminStorageBufferOffsetAlignment: %lu\n", deviceProperties.limits.minStorageBufferOffsetAlignment);
        printf("\tminTexelOffset: %u\n", deviceProperties.limits.minTexelOffset);
        printf("\tmaxTexelOffset: %u\n", deviceProperties.limits.maxTexelOffset);
        printf("\tminTexelGatherOffset: %u\n", deviceProperties.limits.minTexelGatherOffset);
        printf("\tmaxTexelGatherOffset: %u\n", deviceProperties.limits.maxTexelGatherOffset);
        printf("\tminInterpolationOffset: %f\n", deviceProperties.limits.minInterpolationOffset);
        printf("\tmaxInterpolationOffset: %f\n", deviceProperties.limits.maxInterpolationOffset);
        printf("\tsubPixelInterpolationOffsetBits: %u\n", deviceProperties.limits.subPixelInterpolationOffsetBits);
        printf("\tmaxFramebufferWidth: %u\n", deviceProperties.limits.maxFramebufferWidth);
        printf("\tmaxFramebufferHeight: %u\n", deviceProperties.limits.maxFramebufferHeight);
        printf("\tmaxFramebufferLayers: %u\n", deviceProperties.limits.maxFramebufferLayers);
        printf("\tframebufferColorSampleCounts: %u\n", deviceProperties.limits.framebufferColorSampleCounts);
        printf("\tframebufferDepthSampleCounts: %u\n", deviceProperties.limits.framebufferDepthSampleCounts);
        printf("\tframebufferStencilSampleCounts: %u\n", deviceProperties.limits.framebufferStencilSampleCounts);
        printf("\tframebufferNoAttachmentsSampleCounts: %u\n", deviceProperties.limits.framebufferNoAttachmentsSampleCounts);
        printf("\tmaxColorAttachments: %u\n", deviceProperties.limits.maxColorAttachments);
        printf("\tsampledImageColorSampleCounts: %u\n", deviceProperties.limits.sampledImageColorSampleCounts);
        printf("\tsampledImageIntegerSampleCounts: %u\n", deviceProperties.limits.sampledImageIntegerSampleCounts);
        printf("\tsampledImageDepthSampleCounts: %u\n", deviceProperties.limits.sampledImageDepthSampleCounts);
        printf("\tsampledImageStencilSampleCounts: %u\n", deviceProperties.limits.sampledImageStencilSampleCounts);
        printf("\tstorageImageSampleCounts: %u\n", deviceProperties.limits.storageImageSampleCounts);
        printf("\tmaxSampleMaskWords: %u\n", deviceProperties.limits.maxSampleMaskWords);
        printf("\ttimestampComputeAndGraphics: %s\n", deviceProperties.limits.timestampComputeAndGraphics ? "true" : "false");
        printf("\ttimestampPeriod: %f\n", deviceProperties.limits.timestampPeriod);
        printf("\tmaxClipDistances: %u\n", deviceProperties.limits.maxClipDistances);
        printf("\tmaxCullDistances: %u\n", deviceProperties.limits.maxCullDistances);
        printf("\tmaxCombinedClipAndCullDistances: %u\n", deviceProperties.limits.maxCombinedClipAndCullDistances);
        printf("\tdiscreteQueuePriorities: %u\n", deviceProperties.limits.discreteQueuePriorities);
        printf("\tpointSizeRange[0]: %f\n", deviceProperties.limits.pointSizeRange[0]);
        printf("\tpointSizeRange[1]: %f\n", deviceProperties.limits.pointSizeRange[1]);
        printf("\tlineWidthRange[0]: %f\n", deviceProperties.limits.lineWidthRange[0]);
        printf("\tlineWidthRange[1]: %f\n", deviceProperties.limits.lineWidthRange[1]);
        printf("\tpointSizeGranularity: %f\n", deviceProperties.limits.pointSizeGranularity);
        printf("\tlineWidthGranularity: %f\n", deviceProperties.limits.lineWidthGranularity);
        printf("\tstrictLines: %s\n", deviceProperties.limits.strictLines ? "true" : "false");
        printf("\tstandardSampleLocations: %s\n", deviceProperties.limits.standardSampleLocations ? "true" : "false");
        printf("\toptimalBufferCopyOffsetAlignment: %lu\n", deviceProperties.limits.optimalBufferCopyOffsetAlignment);
        printf("\toptimalBufferCopyRowPitchAlignment: %lu\n", deviceProperties.limits.optimalBufferCopyRowPitchAlignment);
        printf("\tnonCoherentAtomSize: %lu\n", deviceProperties.limits.nonCoherentAtomSize);
        printf("---- deviceProperties.limits ----\n");
    }

    // find queue indices
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    Vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    VkBool32 presentSupport = VK_FALSE;

    uint32_t i = 0;
    for (VkQueueFamilyProperties queueFamily : queueFamilies) {
        if (queueFamily.queueCount <= 0)
            continue;

        if (graphicsQueueIndex == UINT32_MAX && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport));
            if (presentSupport)
                graphicsQueueIndex = i;
        }

        if (computeQueueIndex == UINT32_MAX && queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
            computeQueueIndex = i;
        i++;
    }

    assert(graphicsQueueIndex != UINT32_MAX && computeQueueIndex != UINT32_MAX);
    Set<uint32_t> uniqueQueueIndices = {graphicsQueueIndex, computeQueueIndex};

    Vector<VkDeviceQueueCreateInfo> deviceQueueCI;

    float queuePriority = 1.0f;
    for (auto &index : uniqueQueueIndices) {
        VkDeviceQueueCreateInfo queueCI = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queueCI.queueCount = 1;
        queueCI.pQueuePriorities = &queuePriority;
        queueCI.queueFamilyIndex = index;
        deviceQueueCI.push_back(queueCI);
    }

    // VK 1.0 features
    deviceFeatures = {};
    deviceFeatures.fillModeNonSolid = VK_TRUE;
    deviceFeatures.shaderSampledImageArrayDynamicIndexing = VK_TRUE;

    // VK 1.2 features
    VkPhysicalDeviceVulkan12Features features12 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    features12.runtimeDescriptorArray = VK_TRUE;
    features12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    features12.bufferDeviceAddress = VK_TRUE;

    // Dynamic rendering features
    VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES};
    dynamicRenderingFeatures.dynamicRendering = VK_TRUE;
    dynamicRenderingFeatures.pNext = &features12;

    // sync2
    VkPhysicalDeviceSynchronization2Features sync2Features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES};
    sync2Features.synchronization2 = VK_TRUE;
    sync2Features.pNext = &dynamicRenderingFeatures;

    const char *deviceExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME
    };

    // create device
    VkDeviceCreateInfo deviceCI = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    deviceCI.pNext = &sync2Features;
    deviceCI.ppEnabledExtensionNames = deviceExtensions;
    deviceCI.enabledExtensionCount = sizeof(deviceExtensions) / sizeof(deviceExtensions[0]);
    deviceCI.pEnabledFeatures = &deviceFeatures;
    deviceCI.queueCreateInfoCount = deviceQueueCI.size();
    deviceCI.pQueueCreateInfos = deviceQueueCI.data();

    VK_CHECK(vkCreateDevice(physicalDevice, &deviceCI, nullptr, &device));
    volkLoadDevice(device);

    vulkan::setDebugName(device, (uint64_t)instance, VK_OBJECT_TYPE_INSTANCE, "main VkInstance");
    vulkan::setDebugName(device, (uint64_t)physicalDevice, VK_OBJECT_TYPE_PHYSICAL_DEVICE, "main VkPhysicalDevice");
    vulkan::setDebugName(device, (uint64_t)device, VK_OBJECT_TYPE_DEVICE, "main VkDevice");

    // get queues
    vkGetDeviceQueue(device, graphicsQueueIndex, 0, &graphicsQueue);
    vkGetDeviceQueue(device, computeQueueIndex, 0, &computeQueue);

    if (graphicsQueueIndex == computeQueueIndex) {
        vulkan::setDebugName(device, (uint64_t)graphicsQueue, VK_OBJECT_TYPE_QUEUE, "graphics/compute queue");
    } else {
        vulkan::setDebugName(device, (uint64_t)graphicsQueue, VK_OBJECT_TYPE_QUEUE, "graphics queue");
        vulkan::setDebugName(device, (uint64_t)computeQueue, VK_OBJECT_TYPE_QUEUE, "compute queue");
    }
}

void RenderDevice::createAllocator()
{
    VmaVulkanFunctions vmaFunctions = {};
    vmaFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vmaFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
    vmaFunctions.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
    vmaFunctions.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
    vmaFunctions.vkAllocateMemory = vkAllocateMemory;
    vmaFunctions.vkFreeMemory = vkFreeMemory;
    vmaFunctions.vkMapMemory = vkMapMemory;
    vmaFunctions.vkUnmapMemory = vkUnmapMemory;
    vmaFunctions.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
    vmaFunctions.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
    vmaFunctions.vkBindBufferMemory = vkBindBufferMemory;
    vmaFunctions.vkBindImageMemory = vkBindImageMemory;
    vmaFunctions.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
    vmaFunctions.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
    vmaFunctions.vkCreateBuffer = vkCreateBuffer;
    vmaFunctions.vkDestroyBuffer = vkDestroyBuffer;
    vmaFunctions.vkCreateImage = vkCreateImage;
    vmaFunctions.vkDestroyImage = vkDestroyImage;
    vmaFunctions.vkCmdCopyBuffer = vkCmdCopyBuffer;
    vmaFunctions.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2;
    vmaFunctions.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2;
    vmaFunctions.vkBindBufferMemory2KHR = vkBindBufferMemory2;
    vmaFunctions.vkBindImageMemory2KHR = vkBindImageMemory2;
    vmaFunctions.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2;

    VmaAllocatorCreateInfo createInfo = {};
    createInfo.instance = instance;
    createInfo.device = device;
    createInfo.physicalDevice = physicalDevice;
    createInfo.pVulkanFunctions = &vmaFunctions;
    createInfo.vulkanApiVersion = VULKAN_API_VERSION;
    createInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    VK_CHECK(vmaCreateAllocator(&createInfo, &allocator));
}

void RenderDevice::createSwapchain()
{
    VkSurfaceCapabilitiesKHR capabilities;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities));

    uint32_t width = 0;
    uint32_t height = 0;
    SDL_GetWindowSize(window, (int*)&width, (int*)&height);

    swapchainExtent.width = glm::clamp(static_cast<uint32_t>(width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    swapchainExtent.height = glm::clamp(static_cast<uint32_t>(height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    // Find best surface format
    {
        uint32_t surfaceFormatCount;
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, nullptr));
        Vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, surfaceFormats.data()));

        bool found = false;
        for (auto &format : surfaceFormats) {
            if (format.format == VK_FORMAT_B8G8R8A8_SRGB) {
                surfaceFormat = format;
                found = true;
            }

            if (found) break;
        }

        if (!found) // fallback
            surfaceFormat = surfaceFormats[0];
    }

    // Find best present mode
    {
        uint32_t presentModeCount;
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr));
        Vector<VkPresentModeKHR> presentModes(presentModeCount);
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data()));

        bool found = false;
        for (auto &mode : presentModes) {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                found = true;
                presentMode = mode;
            }

            if (found) break;
        }

        if (!found) // fallback
            presentMode = VK_PRESENT_MODE_FIFO_KHR;
    }

    VkSwapchainCreateInfoKHR swapchainCI{};
    swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCI.surface = surface;
    swapchainCI.minImageCount = capabilities.minImageCount;
    swapchainCI.imageFormat = surfaceFormat.format;
    swapchainCI.imageColorSpace = surfaceFormat.colorSpace;
    swapchainCI.imageExtent = swapchainExtent;
    swapchainCI.imageArrayLayers = 1;
    swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCI.preTransform = capabilities.currentTransform;
    swapchainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCI.presentMode = presentMode;

    VK_CHECK(vkCreateSwapchainKHR(device, &swapchainCI, nullptr, &swapchain));
    vulkan::setDebugName(device, (uint64_t)swapchain, VK_OBJECT_TYPE_SWAPCHAIN_KHR, "swapchain");

    // get swapchain images
    uint32_t swapchainImageCount = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr);

    swapchainImages.resize(swapchainImageCount);
    swapchainImageViews.resize(swapchainImageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages.data());

    // create swapchain image views from images
    for (uint32_t i = 0; i < swapchainImages.size(); i++) {
        VkImageViewCreateInfo imageViewCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        imageViewCreateInfo.image = swapchainImages[i];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = surfaceFormat.format;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        VK_CHECK(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &swapchainImageViews[i]));
    }
}

void RenderDevice::destroySwapchain()
{
    for (VkImageView &view : swapchainImageViews)
        vkDestroyImageView(device, view, nullptr);

    swapchainImages.clear();
    swapchainImageViews.clear();
    vkDestroySwapchainKHR(device, swapchain, nullptr);
}

void RenderDevice::createSyncObjects()
{
    submitSemaphores.resize(swapchainImages.size());
    for (size_t i = 0; i < submitSemaphores.size(); i++) {
        VkSemaphoreCreateInfo semaphoreCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &submitSemaphores[i]));
    }

    for (unsigned int i = 0; i < FRAMES_IN_FLIGHT; i++) {
        VkSemaphoreCreateInfo semaphoreCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &acquireSemaphores[i]));

        VkFenceCreateInfo fenceCreateInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &finishRenderFences[i]));
    }
}

void RenderDevice::recreateSwapchain()
{
    waitIdle();

    destroySwapchain();

    for (VkSemaphore &semaphore : submitSemaphores) {
        vkDestroySemaphore(device, semaphore, nullptr);
    }
    submitSemaphores.clear();

    for (unsigned int i = 0; i < FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, acquireSemaphores[i], nullptr);
        vkDestroyFence(device, finishRenderFences[i], nullptr);
    }

    createSwapchain();
    createSyncObjects();
}

void RenderDevice::setupImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.IniFilename = imguiConfigFile;
    io.LogFilename = imguiLogFile;

    VkFormat colorFormat = VK_FORMAT_B8G8R8A8_SRGB;

    VkPipelineRenderingCreateInfoKHR renderingCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR};
    renderingCreateInfo.colorAttachmentCount = 1;
    renderingCreateInfo.pColorAttachmentFormats = &colorFormat;
    renderingCreateInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;

    ImGui_ImplSDL3_InitForVulkan(window);
    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.ApiVersion = VULKAN_API_VERSION;
    initInfo.Instance = instance;
    initInfo.PhysicalDevice = physicalDevice;
    initInfo.Device = device;
    initInfo.QueueFamily = graphicsQueueIndex;
    initInfo.Queue = graphicsQueue;
    initInfo.PipelineCache = nullptr;
    initInfo.DescriptorPool = descriptorPool;
    initInfo.MinImageCount = 2;
    initInfo.ImageCount = 2;
    initInfo.Allocator = nullptr;
    initInfo.CheckVkResultFn = nullptr;
    initInfo.UseDynamicRendering = true;
    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = renderingCreateInfo;
    initInfo.PipelineInfoMain.RenderPass = nullptr;
    initInfo.PipelineInfoMain.Subpass = 0;
    initInfo.PipelineInfoMain.MSAASamples = maxSampleCount;
    ImGui_ImplVulkan_Init(&initInfo);
}

void RenderDevice::shutdownImGui()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}