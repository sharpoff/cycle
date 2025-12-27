#include "cycle/graphics/render_device.h"

#include "cycle/logger.h"

#include "cycle/graphics/graphics_types.h"
#include "cycle/graphics/vulkan_helpers.h"

#include "SDL3/SDL_vulkan.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"
#include <vulkan/vulkan_core.h>

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

    LOGI("Selected present mode: %s", vulkan::toString(presentMode));

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

#ifndef NDEBUG
    for (size_t i = 0; i < commandBuffers.size(); i++) {
        vulkan::setDebugName(device, (uint64_t)commandBuffers[i], VK_OBJECT_TYPE_COMMAND_BUFFER, "main VkCommandBuffer [" + std::to_string(i) + "]");
    }
#endif

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

    // descriptors
    Vector<VkDescriptorPoolSize> poolSizes = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1024},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024},
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

    setupImGui();
}

void RenderDevice::shutdown()
{
    vkDeviceWaitIdle(device);

    shutdownImGui();

    for (Image &image : swapchainImages) {
        vkDestroyImageView(device, image.view, nullptr);
    }
    vkDestroySwapchainKHR(device, swapchain, nullptr);

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

bool RenderDevice::createBuffer(Buffer &buffer, const BufferCreateInfo &createInfo, String debugName)
{
    assert(createInfo.size > 0);

    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = createInfo.size;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.usage = vulkan::getBufferUsageFlags(createInfo.usage);

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    allocInfo.priority = 1.0;

    buffer.size = createInfo.size;
    buffer.usage = createInfo.usage;
    VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation.handle, &buffer.allocation.info));

    if (bufferInfo.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        VkBufferDeviceAddressInfo deviceAddressInfo = {VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR};
        deviceAddressInfo.buffer = buffer.buffer;
        buffer.address = vkGetBufferDeviceAddress(device, &deviceAddressInfo);
    }

    if (!debugName.empty())
        vulkan::setDebugName(device, (uint64_t)buffer.buffer, VK_OBJECT_TYPE_BUFFER, debugName);

    return true;
}

bool RenderDevice::createImage(Image &image, const ImageCreateInfo &createInfo, String debugName)
{
    assert(createInfo.width != 0 && createInfo.height != 0);

    VkImageCreateInfo imageInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageInfo.imageType = vulkan::getImageType(createInfo.type);
    imageInfo.format = vulkan::getFormat(createInfo.format);
    imageInfo.extent.width = createInfo.width;
    imageInfo.extent.height = createInfo.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = createInfo.mipLevels;
    imageInfo.arrayLayers = createInfo.arrayLayers;
    imageInfo.samples = vulkan::getSampleCount(createInfo.sampleCount);
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = vulkan::getImageUsageFlags(createInfo.usage);
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    image.width = createInfo.width;
    image.height = createInfo.height;
    image.layerCount = createInfo.arrayLayers;
    image.levelCount = createInfo.mipLevels;
    image.sampleCount = createInfo.sampleCount;
    image.type = createInfo.type;
    image.usage = createInfo.usage;
    image.format = createInfo.format;
    image.isSwapchain = false;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    VK_CHECK(vmaCreateImage(allocator, &imageInfo, &allocInfo, &image.image, &image.allocation.handle, &image.allocation.info));
    assert(image.image != VK_NULL_HANDLE);

    // Create image view
    VkImageViewCreateInfo imageViewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    imageViewInfo.image = image.image;
    imageViewInfo.viewType = vulkan::getImageViewType(image.type);
    imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.format = vulkan::getFormat(image.format);
    imageViewInfo.subresourceRange = vulkan::getImageSubresourceRange(image);

    VK_CHECK(vkCreateImageView(device, &imageViewInfo, nullptr, &image.view));

    if (!debugName.empty()) {
        vulkan::setDebugName(device, (uint64_t)image.image, VK_OBJECT_TYPE_IMAGE, debugName + " (VkImage)");
        vulkan::setDebugName(device, (uint64_t)image.view, VK_OBJECT_TYPE_IMAGE_VIEW, debugName + " (VkImageView)");
    }

    return true;
}

bool RenderDevice::createSampler(Sampler &sampler, const SamplerCreateInfo &createInfo, String debugName)
{
    VkSamplerCreateInfo samplerCreateInfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    samplerCreateInfo.minFilter = vulkan::getFilter(createInfo.minFilter);
    samplerCreateInfo.magFilter = vulkan::getFilter(createInfo.magFilter);
    samplerCreateInfo.addressModeU = vulkan::getSamplerAddressMode(createInfo.addressModeU);
    samplerCreateInfo.addressModeV = vulkan::getSamplerAddressMode(createInfo.addressModeV);
    samplerCreateInfo.addressModeW = vulkan::getSamplerAddressMode(createInfo.addressModeW);
    samplerCreateInfo.mipmapMode = vulkan::getSamplerMipmapMode(createInfo.mipmapMode);
    samplerCreateInfo.compareOp = vulkan::getCompareOp(createInfo.compareOp);
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    samplerCreateInfo.maxLod = createInfo.maxLod;

    sampler.mipLodBias = createInfo.mipLodBias;
    sampler.minLod = createInfo.minLod;
    sampler.maxLod = createInfo.maxLod;
    sampler.maxAnisotropy = createInfo.maxAnisotropy;
    sampler.magFilter = createInfo.magFilter;
    sampler.minFilter = createInfo.minFilter;
    sampler.mipmapMode = createInfo.mipmapMode;
    sampler.addressModeU = createInfo.addressModeU;
    sampler.addressModeV = createInfo.addressModeV;
    sampler.addressModeW = createInfo.addressModeW;
    sampler.compareOp = createInfo.compareOp;
    VK_CHECK(vkCreateSampler(device, &samplerCreateInfo, nullptr, &sampler.sampler));

    if (!debugName.empty())
        vulkan::setDebugName(device, (uint64_t)sampler.sampler, VK_OBJECT_TYPE_SAMPLER, debugName);

    return true;
}

bool RenderDevice::createPipelineLayout(PipelineLayout &pipelineLayout, const PipelineLayoutCreateInfo &createInfo, String debugName)
{
    Vector<VkDescriptorSetLayout> vkDescriptorSetLayouts;
    Vector<VkDescriptorSet> vkDescriptorSets;
    Vector<VkPushConstantRange> vkPushConstantRanges;

    // Create descriptor set layouts
    if (!createInfo.descriptorSetLayouts.empty()) {
        vkDescriptorSetLayouts.resize(createInfo.descriptorSetLayouts.size());

        for (size_t i = 0; i < createInfo.descriptorSetLayouts.size(); i++) {
            const Vector<DescriptorSetLayoutBinding> &bindings = createInfo.descriptorSetLayouts[i].bindings;

            Vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings(bindings.size());

            for (size_t j = 0; j < bindings.size(); j++) {
                descriptorSetLayoutBindings[j].binding = bindings[j].binding;
                descriptorSetLayoutBindings[j].descriptorType = vulkan::getDescriptorType(bindings[j].descriptorType);
                descriptorSetLayoutBindings[j].descriptorCount = bindings[j].descriptorCount;
                descriptorSetLayoutBindings[j].stageFlags = vulkan::getShaderStageFlags(bindings[j].stageMask);
            }

            VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
            layoutCreateInfo.bindingCount = descriptorSetLayoutBindings.size();
            layoutCreateInfo.pBindings = descriptorSetLayoutBindings.data();

            VK_CHECK(vkCreateDescriptorSetLayout(device, &layoutCreateInfo, nullptr, &vkDescriptorSetLayouts[i]));
        }

        vkDescriptorSets.resize(vkDescriptorSetLayouts.size());

        // allocate descriptor sets
        VkDescriptorSetAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = vkDescriptorSets.size();
        allocInfo.pSetLayouts = vkDescriptorSetLayouts.data();
        VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, vkDescriptorSets.data()));
    }

    // Create push constants
    for (const PushConstantRange &range : createInfo.pushConstantRanges) {
        VkPushConstantRange &vkPushConstantRange = vkPushConstantRanges.emplace_back();
        vkPushConstantRange.stageFlags = vulkan::getShaderStageFlags(range.stageFlags);
        vkPushConstantRange.offset = range.offset;
        vkPushConstantRange.size = range.size;
    }

    VkPipelineLayoutCreateInfo vkLayoutCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    vkLayoutCreateInfo.setLayoutCount = vkDescriptorSetLayouts.size();
    vkLayoutCreateInfo.pSetLayouts = vkDescriptorSetLayouts.data();
    vkLayoutCreateInfo.pushConstantRangeCount = vkPushConstantRanges.size();
    vkLayoutCreateInfo.pPushConstantRanges = vkPushConstantRanges.data();

    pipelineLayout.descriptorSetLayouts = vkDescriptorSetLayouts;
    pipelineLayout.descriptorSets = vkDescriptorSets;
    VK_CHECK(vkCreatePipelineLayout(device, &vkLayoutCreateInfo, nullptr, &pipelineLayout.layout));

    if (!debugName.empty())
        vulkan::setDebugName(device, (uint64_t)pipelineLayout.layout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, debugName);

    return true;
}

bool RenderDevice::createRenderPipeline(RenderPipeline &renderPipeline, const RenderPipelineCreateInfo &createInfo, String debugName)
{
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

    Vector<VkVertexInputBindingDescription> vertexBindingDescriptions(createInfo.vertexBindings.size());
    for (size_t i = 0; i < createInfo.vertexBindings.size(); i++) {
        const VertexBinding &binding = createInfo.vertexBindings[i];
        vertexBindingDescriptions[i].binding = binding.binding;
        vertexBindingDescriptions[i].stride = binding.stride;
        vertexBindingDescriptions[i].inputRate = binding.inputRate == VERTEX_INPUT_RATE_VERTEX ? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE;
    }

    Vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions(createInfo.vertexAttributes.size());
    for (size_t i = 0; i < createInfo.vertexAttributes.size(); i++) {
        const VertexAttribute &attrib = createInfo.vertexAttributes[i];
        vertexAttributeDescriptions[i].location = attrib.location;
        vertexAttributeDescriptions[i].binding = attrib.binding;
        vertexAttributeDescriptions[i].format = vulkan::getFormat(attrib.format);
        vertexAttributeDescriptions[i].offset = attrib.offset;
    }

    VkPipelineVertexInputStateCreateInfo vertexInputState = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInputState.vertexAttributeDescriptionCount = vertexAttributeDescriptions.size();
    vertexInputState.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();
    vertexInputState.vertexBindingDescriptionCount = vertexBindingDescriptions.size();
    vertexInputState.pVertexBindingDescriptions = vertexBindingDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssemblyState.topology = vulkan::getPrimitiveTopology(createInfo.topology);

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
    rasterizationState.polygonMode = vulkan::getPolygonMode(createInfo.polygonMode);
    rasterizationState.cullMode = vulkan::getCullMode(createInfo.cullMode);
    rasterizationState.frontFace = vulkan::getFrontFace(createInfo.frontFace);
    rasterizationState.depthBiasEnable = VK_FALSE;
    rasterizationState.depthBiasConstantFactor = 0.0f;
    rasterizationState.depthBiasClamp = 0.0f;
    rasterizationState.depthBiasSlopeFactor = 0.0f;
    rasterizationState.lineWidth = 1.0;

    VkPipelineMultisampleStateCreateInfo multisampleState = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisampleState.rasterizationSamples = vulkan::getSampleCount(createInfo.sampleCount);
    if (deviceFeatures.sampleRateShading) {
        multisampleState.sampleShadingEnable = VK_TRUE;
        multisampleState.minSampleShading = 0.2f;
    } else {
        multisampleState.sampleShadingEnable = VK_FALSE;
    }

    const VkBool32 depthTestEnabled = createInfo.depthCompareOp != COMPARE_OP_ALWAYS;
    const VkBool32 depthWriteEnabled = createInfo.depthWriteEnable;

    VkPipelineDepthStencilStateCreateInfo depthStencilState = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depthStencilState.depthTestEnable = depthTestEnabled || depthWriteEnabled;
    depthStencilState.depthWriteEnable = depthWriteEnabled;
    depthStencilState.depthCompareOp = vulkan::getCompareOp(createInfo.depthCompareOp);
    depthStencilState.depthBoundsTestEnable = VK_FALSE;
    depthStencilState.stencilTestEnable = VK_FALSE; // TODO: stencil
    depthStencilState.front = {}; // TODO: stencil
    depthStencilState.back = {}; // TODO: stencil
    depthStencilState.minDepthBounds = 0.0f;
    depthStencilState.maxDepthBounds = 1.0f;

    const VkBool32 blendEnabled = createInfo.colorBlendOp != BLEND_OP_NONE;

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
    colorBlendAttachmentState.blendEnable = blendEnabled;
    colorBlendAttachmentState.srcColorBlendFactor = vulkan::getBlendFactor(createInfo.colorBlendFactorSrc);
    colorBlendAttachmentState.dstColorBlendFactor = vulkan::getBlendFactor(createInfo.colorBlendFactorDst);
    colorBlendAttachmentState.colorBlendOp = vulkan::getBlendOp(createInfo.colorBlendOp);
    colorBlendAttachmentState.srcAlphaBlendFactor = vulkan::getBlendFactor(createInfo.alphaBlendFactorSrc);
    colorBlendAttachmentState.dstAlphaBlendFactor = vulkan::getBlendFactor(createInfo.alphaBlendFactorDst);
    colorBlendAttachmentState.alphaBlendOp = vulkan::getBlendOp(createInfo.alphaBlendOp);
    colorBlendAttachmentState.colorWriteMask = vulkan::getColorComponentFlags(createInfo.colorWriteMask);

    Vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(createInfo.colorAttachmentFormats.size(), colorBlendAttachmentState);

    VkPipelineColorBlendStateCreateInfo colorBlendState = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    colorBlendState.logicOpEnable = VK_FALSE;
    colorBlendState.attachmentCount = colorBlendAttachments.size();
    colorBlendState.pAttachments = colorBlendAttachments.data();

    Vector<VkDynamicState> dynamicStates;
    if ((createInfo.dynamicState & DYNAMIC_STATE_VIEWPORT) == DYNAMIC_STATE_VIEWPORT) {
        dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
    }

    if ((createInfo.dynamicState & DYNAMIC_STATE_SCISSOR) == DYNAMIC_STATE_SCISSOR) {
        dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
    }

    VkPipelineDynamicStateCreateInfo dynamicState = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamicState.dynamicStateCount = dynamicStates.size();
    dynamicState.pDynamicStates = dynamicStates.data();

    Vector<VkFormat> colorAttachmentFormats(createInfo.colorAttachmentFormats.size());
    for (size_t i = 0; i < colorAttachmentFormats.size(); i++) {
        colorAttachmentFormats[i] = (vulkan::getFormat(createInfo.colorAttachmentFormats[i]));
    }

    VkPipelineRenderingCreateInfoKHR renderingInfo = {VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR};
    renderingInfo.colorAttachmentCount = colorAttachmentFormats.size();
    renderingInfo.pColorAttachmentFormats = colorAttachmentFormats.data();
    if (depthTestEnabled || depthWriteEnabled)
        renderingInfo.depthAttachmentFormat = vulkan::getFormat(createInfo.depthAttachmentFormat);

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
    pipelineCreateInfo.layout = createInfo.pipelineLayout->layout;

    renderPipeline.layout = createInfo.pipelineLayout;
    VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &renderPipeline.pipeline));

    if (vertexModule)
        vkDestroyShaderModule(device, vertexModule, nullptr);
    if (fragmentModule)
        vkDestroyShaderModule(device, fragmentModule, nullptr);
    if (tessellationControlModule)
        vkDestroyShaderModule(device, tessellationControlModule, nullptr);
    if (tessellationEvaluationModule)
        vkDestroyShaderModule(device, tessellationEvaluationModule, nullptr);

    if (!debugName.empty())
        vulkan::setDebugName(device, (uint64_t)renderPipeline.pipeline, VK_OBJECT_TYPE_PIPELINE, debugName);

    return true;
}

bool RenderDevice::createComputePipeline(ComputePipeline &computePipeline, const ComputePipelineCreateInfo &createInfo, String debugName)
{
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
    pipelineCreateInfo.layout = createInfo.pipelineLayout->layout;

    computePipeline.layout = createInfo.pipelineLayout;
    VK_CHECK(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &computePipeline.pipeline));

    if (!debugName.empty())
        vulkan::setDebugName(device, (uint64_t)computePipeline.pipeline, VK_OBJECT_TYPE_PIPELINE, debugName);

    return true;
}

void RenderDevice::destroyBuffer(Buffer &buffer)
{
    if (buffer.buffer != VK_NULL_HANDLE)
        vmaDestroyBuffer(allocator, buffer.buffer, buffer.allocation.handle);
}

void RenderDevice::destroyImage(Image &image)
{
    if (image.view != VK_NULL_HANDLE)
        vkDestroyImageView(device, image.view, nullptr);

    if (!image.isSwapchain && image.image != VK_NULL_HANDLE)
        vmaDestroyImage(allocator, image.image, image.allocation.handle);
}

void RenderDevice::destroySampler(Sampler &sampler)
{
    if (sampler.sampler != VK_NULL_HANDLE)
        vkDestroySampler(device, sampler.sampler, nullptr);
}

void RenderDevice::destroyPipelineLayout(PipelineLayout &layout)
{
    for (auto &descriptorSetLayout : layout.descriptorSetLayouts)
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

    if (layout.layout != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(device, layout.layout, nullptr);
}

void RenderDevice::destroyRenderPipeline(RenderPipeline &pipeline)
{
    if (pipeline.pipeline != VK_NULL_HANDLE)
        vkDestroyPipeline(device, pipeline.pipeline, nullptr);
}

void RenderDevice::destroyComputePipeline(ComputePipeline &pipeline)
{
    if (pipeline.pipeline != VK_NULL_HANDLE)
        vkDestroyPipeline(device, pipeline.pipeline, nullptr);
}

void RenderDevice::uploadBufferData(Buffer &buffer, void *data, uint64_t size, Buffer *stagingBuffer)
{
    assert(data && size > 0);

    if (stagingBuffer && stagingBuffer->size >= size) {
        memcpy(stagingBuffer->allocation.info.pMappedData, data, size);
        VK_CHECK(vmaFlushAllocation(allocator, stagingBuffer->allocation.handle, 0, VK_WHOLE_SIZE));

        immediateSubmit([size, stagingBuffer, &buffer](VkCommandBuffer cmd) -> void {
            VkBufferCopy copyRegion = {0, 0, size};
            vkCmdCopyBuffer(cmd, stagingBuffer->buffer, buffer.buffer, 1, &copyRegion);
        });
    } else {
        const BufferCreateInfo createInfo = {
            .size = size,
            .usage = BUFFER_USAGE_TRANSFER_SRC,
        };

        Buffer staging;
        createBuffer(staging, createInfo);
        memcpy(staging.allocation.info.pMappedData, data, size);

        VK_CHECK(vmaFlushAllocation(allocator, staging.allocation.handle, 0, VK_WHOLE_SIZE));

        immediateSubmit([size, &staging, &buffer](VkCommandBuffer cmd) -> void {
            VkBufferCopy copyRegion = {0, 0, size};
            vkCmdCopyBuffer(cmd, staging.buffer, buffer.buffer, 1, &copyRegion);
        });

        destroyBuffer(staging);
    }
}

void RenderDevice::uploadImageData(Image &image, void *data, uint64_t size)
{
    assert(data && size > 0);

    const BufferCreateInfo createInfo = {
        .size = size,
        .usage = BUFFER_USAGE_TRANSFER_SRC,
    };

    Buffer staging;
    createBuffer(staging, createInfo);
    memcpy(staging.allocation.info.pMappedData, data, size);

    VK_CHECK(vmaFlushAllocation(allocator, staging.allocation.handle, 0, VK_WHOLE_SIZE));

    immediateSubmit([size, &staging, &image](VkCommandBuffer cmd) -> void {
        // transition image to transfer
        VkImageMemoryBarrier transferBarrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        transferBarrier.srcAccessMask = 0;
        transferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        transferBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        transferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        transferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        transferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        transferBarrier.image = image.image;
        transferBarrier.subresourceRange = vulkan::getImageSubresourceRange(image);

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, // stages
            0,
            0, nullptr, // memory barriers
            0, nullptr, // buffer memory barriers
            1, &transferBarrier // image memory barriers
        );

        // copy
        VkBufferImageCopy copyRegion = {};
        copyRegion.imageSubresource = vulkan::getImageSubresourceLayers(image);
        copyRegion.imageExtent = {image.width, image.height, 1};

        vkCmdCopyBufferToImage(cmd, staging.buffer, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

        // transition image to fragment shader
        VkImageMemoryBarrier fragmentBarrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        fragmentBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        fragmentBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        fragmentBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        fragmentBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        fragmentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        fragmentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        fragmentBarrier.image = image.image;
        fragmentBarrier.subresourceRange = vulkan::getImageSubresourceRange(image);

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

bool RenderDevice::beginCommandEncoding(CommandEncoder &encoder)
{
    VK_CHECK(vkWaitForFences(device, 1, &finishRenderFences[currentFrame], VK_TRUE, ~0ull));
    VK_CHECK(vkResetFences(device, 1, &finishRenderFences[currentFrame]));

    VkResult result = vkAcquireNextImageKHR(device, swapchain, ~0ull, acquireSemaphores[currentFrame], nullptr, &imageIndex);
    if (resizeRequested || result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return false;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        LOGE("%s", "Failed to acquire swapchain image.");
        exit(EXIT_FAILURE);
    }

    encoder.cmd = commandBuffers[currentFrame];
    VK_CHECK(vkResetCommandBuffer(encoder.cmd, 0));

    VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK(vkBeginCommandBuffer(encoder.cmd, &beginInfo));

    return true;
}

void RenderDevice::endCommandEncoding(CommandEncoder &encoder)
{
    VK_CHECK(vkEndCommandBuffer(encoder.cmd));

    VkPipelineStageFlags stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submit = {};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &acquireSemaphores[currentFrame];
    submit.pWaitDstStageMask = stages;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &encoder.cmd;
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

void RenderDevice::writeDescriptor(uint32_t binding, Buffer &buffer, DescriptorType type, uint32_t dstArrayElement)
{
    descriptorSetWriter.write(binding, buffer.buffer, buffer.size, vulkan::getDescriptorType(type), dstArrayElement);
}

void RenderDevice::writeDescriptor(uint32_t binding, Image &image, Sampler &sampler, DescriptorType type, uint32_t dstArrayElement)
{
    descriptorSetWriter.write(binding, image.view, sampler.sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, vulkan::getDescriptorType(type), dstArrayElement);
}

void RenderDevice::updateDescriptors(PipelineLayout &layout, uint32_t set)
{
    assert(set >= 0 && set < layout.descriptorSets.size()); // bounds check
    descriptorSetWriter.update(device, layout.descriptorSets[set]);
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
    uint32_t imageCount = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);

    Vector<VkImage> vkSwapchainImages(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, vkSwapchainImages.data());

    // create swapchain image views from images
    for (uint32_t i = 0; i < vkSwapchainImages.size(); i++) {
        VkImageViewCreateInfo imageViewCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        imageViewCreateInfo.image = vkSwapchainImages[i];
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

        Image &image = swapchainImages.emplace_back();
        image.image = vkSwapchainImages[i];
        image.format = IMAGE_FORMAT_B8G8R8A8_SRGB;
        image.usage = IMAGE_USAGE_COLOR_ATTACHMENT;
        image.type = IMAGE_TYPE_2D;
        image.sampleCount = 1;
        image.isSwapchain = true;
        image.width = swapchainExtent.width;
        image.height = swapchainExtent.height;

        VK_CHECK(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &image.view));
        // vulkan::setDebugName(device, (uint64_t)image.view, VK_OBJECT_TYPE_IMAGE_VIEW, "swapchain image view [" + std::to_string(i) + "]");
    }
}

void RenderDevice::createSyncObjects()
{
    submitSemaphores.resize(swapchainImages.size());
    for (size_t i = 0; i < submitSemaphores.size(); i++) {
        VkSemaphoreCreateInfo semaphoreCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &submitSemaphores[i]));
        vulkan::setDebugName(device, (uint64_t)submitSemaphores[i], VK_OBJECT_TYPE_SEMAPHORE, "submit VkSemaphore [" + std::to_string(i) + "]");
    }

    for (unsigned int i = 0; i < FRAMES_IN_FLIGHT; i++) {
        VkSemaphoreCreateInfo semaphoreCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &acquireSemaphores[i]));
        vulkan::setDebugName(device, (uint64_t)acquireSemaphores[i], VK_OBJECT_TYPE_SEMAPHORE, "acquire VkSemaphore [" + std::to_string(i) + "]");

        VkFenceCreateInfo fenceCreateInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &finishRenderFences[i]));
        vulkan::setDebugName(device, (uint64_t)finishRenderFences[i], VK_OBJECT_TYPE_FENCE, "finish render VkFence [" + std::to_string(i) + "]");
    }
}

void RenderDevice::recreateSwapchain()
{
    waitIdle();

    for (Image &image : swapchainImages) {
        vkDestroyImageView(device, image.view, nullptr);
    }
    swapchainImages.clear();
    vkDestroySwapchainKHR(device, swapchain, nullptr);

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
    io.IniFilename = "resources/config/imgui.ini";

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
    initInfo.MinImageCount = FRAMES_IN_FLIGHT;
    initInfo.ImageCount = FRAMES_IN_FLIGHT;
    initInfo.Allocator = nullptr;
    initInfo.CheckVkResultFn = nullptr;
    initInfo.UseDynamicRendering = true;
    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = renderingCreateInfo;
    initInfo.PipelineInfoMain.RenderPass = nullptr;
    initInfo.PipelineInfoMain.Subpass = 0;
    initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    ImGui_ImplVulkan_Init(&initInfo);
}

void RenderDevice::shutdownImGui()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}