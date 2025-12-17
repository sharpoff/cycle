#include "cycle/graphics/render_device.h"

#include "cycle/logger.h"

#include "cycle/graphics/graphics_types.h"
#include "cycle/graphics/vulkan_helpers.h"

#include "SDL3/SDL_vulkan.h"

#include <algorithm>

// TODO: add anisotropy feature for sampler
// TODO: add stencil testing when creating render pipeline

#ifdef ENABLE_VULKAN_DEBUG
VkBool32 VKAPI_PTR vulkanDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void                                       *pUserData)
{
    const char *type = "UNDEFINED";
    if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
        type = "GENERAL";
    else if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
        type = "VALIDATION";
    else if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
        type = "PERFORMANCE";

    const char *severity = "UNDEFINED";
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        severity = "INFO";
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        severity = "ERROR";
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
        severity = "VERBOSE";
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        severity = "WARNING";

    printf("[%s][VULKAN][%s / %s] %s\n", __TIME__, type, severity, pCallbackData->pMessage);

    return VK_FALSE;
}
#endif

void RenderDevice::Init(SDL_Window *window)
{
    this->window = window;

    CreateInstance();

    SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface);

    CreateDevice();

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

    CreateAllocator();

    CreateSwapchain();

    // create command pool
    VkCommandPoolCreateInfo commandPoolCI = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    commandPoolCI.queueFamilyIndex = graphicsQueueIndex;
    commandPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK(vkCreateCommandPool(device, &commandPoolCI, nullptr, &commandPool));

    // create command buffers
    VkCommandBufferAllocateInfo bufferAllocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    bufferAllocInfo.commandPool = commandPool;
    bufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    bufferAllocInfo.commandBufferCount = commandBuffers.size();
    VK_CHECK(vkAllocateCommandBuffers(device, &bufferAllocInfo, commandBuffers.data()));

    // create syncronizaiton objects
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

    // profiler context
#ifdef ENABLE_VULKAN_PROFILE
    for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
        tracyVkCtx[i] = TracyVkContext(physicalDevice, device, graphicsQueue, commandBuffers[i]);
    }
#endif

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
}

void RenderDevice::Shutdown()
{
    vkDeviceWaitIdle(device);

    for (Image *image : swapchainImages) {
        vkDestroyImageView(device, image->view, nullptr);
        delete image;
    }
    vkDestroySwapchainKHR(device, swapchain, nullptr);

    vkDestroyDescriptorPool(device, descriptorPool, nullptr);

    vkDestroyCommandPool(device, commandPool, nullptr);

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

#ifdef ENABLE_VULKAN_DEBUG
    vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
#endif

    vkDestroyInstance(instance, nullptr);
}

Buffer *RenderDevice::CreateBuffer(const BufferCreateInfo &createInfo)
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

    Buffer *buffer = new Buffer();
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

Image *RenderDevice::CreateImage(const ImageCreateInfo &createInfo)
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

    Image *image = new Image();
    image->width = createInfo.width;
    image->height = createInfo.height;
    image->layerCount = createInfo.arrayLayers;
    image->levelCount = createInfo.mipLevels;
    image->sampleCount = createInfo.sampleCount;
    image->type = createInfo.type;
    image->usage = createInfo.usage;
    image->format = createInfo.format;
    image->isSwapchain = false;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    VK_CHECK(vmaCreateImage(allocator, &imageInfo, &allocInfo, &image->image, &image->allocation.handle, &image->allocation.info));
    assert(image->image != VK_NULL_HANDLE);

    // Create image view
    VkImageViewCreateInfo imageViewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    imageViewInfo.image = image->image;
    imageViewInfo.viewType = vulkan::getImageViewType(image->type);
    imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.format = vulkan::getFormat(image->format);
    imageViewInfo.subresourceRange = vulkan::getImageSubresourceRange(image);

    VK_CHECK(vkCreateImageView(device, &imageViewInfo, nullptr, &image->view));

    return image;
}

Sampler *RenderDevice::CreateSampler(const SamplerCreateInfo &createInfo)
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

    Sampler *sampler = new Sampler();
    assert(sampler);
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

PipelineLayout *RenderDevice::CreatePipelineLayout(const PipelineLayoutCreateInfo &createInfo)
{
    Vector<VkDescriptorSetLayout> descriptorSetLayouts;
    Vector<VkDescriptorSet> descriptorSets;

    if (!createInfo.descriptorSetLayouts.empty()) {
        descriptorSetLayouts.resize(createInfo.descriptorSetLayouts.size());

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

            VK_CHECK(vkCreateDescriptorSetLayout(device, &layoutCreateInfo, nullptr, &descriptorSetLayouts[i]));
        }

        descriptorSets.resize(descriptorSetLayouts.size());

        // allocate descriptor sets
        VkDescriptorSetAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = descriptorSets.size();
        allocInfo.pSetLayouts = descriptorSetLayouts.data();
        VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()));
    }

    VkPipelineLayoutCreateInfo layoutCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layoutCreateInfo.setLayoutCount = descriptorSetLayouts.size();
    layoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();

    PipelineLayout *pipelineLayout = new PipelineLayout();
    pipelineLayout->descriptorSetLayouts = descriptorSetLayouts;
    pipelineLayout->descriptorSets = descriptorSets;
    VK_CHECK(vkCreatePipelineLayout(device, &layoutCreateInfo, nullptr, &pipelineLayout->layout));

    return pipelineLayout;
}

RenderPipeline *RenderDevice::CreateRenderPipeline(const RenderPipelineCreateInfo &createInfo)
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

    vec2 windowSize = GetWindowSize();
    uint32_t width = (uint32_t)windowSize.x;
    uint32_t height = (uint32_t)windowSize.y;

    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = width;
    viewport.height = height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = width;
    scissor.extent.height = height;

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

    RenderPipeline *renderPipeline = new RenderPipeline();
    renderPipeline->layout = createInfo.pipelineLayout;
    VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &renderPipeline->pipeline));

    if (vertexModule)
        vkDestroyShaderModule(device, vertexModule, nullptr);
    if (fragmentModule)
        vkDestroyShaderModule(device, fragmentModule, nullptr);
    if (tessellationControlModule)
        vkDestroyShaderModule(device, tessellationControlModule, nullptr);
    if (tessellationEvaluationModule)
        vkDestroyShaderModule(device, tessellationEvaluationModule, nullptr);

    return renderPipeline;
}

ComputePipeline *RenderDevice::CreateComputePipeline(const ComputePipelineCreateInfo &createInfo)
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

    ComputePipeline *computePipeline = new ComputePipeline();
    computePipeline->layout = createInfo.pipelineLayout;
    VK_CHECK(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &computePipeline->pipeline));

    return computePipeline;
}

void RenderDevice::DestroyBuffer(Buffer **buffer)
{
    if (buffer) {
        if ((*buffer)->buffer != VK_NULL_HANDLE)
            vmaDestroyBuffer(allocator, (*buffer)->buffer, (*buffer)->allocation.handle);

        delete *buffer;
        *buffer = nullptr;
    }
}

void RenderDevice::DestroyImage(Image **image)
{
    if (image) {
        if ((*image)->view != VK_NULL_HANDLE)
            vkDestroyImageView(device, (*image)->view, nullptr);

        if ((*image)->image != VK_NULL_HANDLE)
            vmaDestroyImage(allocator, (*image)->image, (*image)->allocation.handle);

        delete *image;
        *image = nullptr;
    }
}

void RenderDevice::DestroySampler(Sampler **sampler)
{
    if (sampler) {
        if ((*sampler)->sampler != VK_NULL_HANDLE)
            vkDestroySampler(device, (*sampler)->sampler, nullptr);

        delete *sampler;
        *sampler = nullptr;
    }
}

void RenderDevice::DestroyPipelineLayout(PipelineLayout **layout)
{
    if (layout) {
        for (auto &descriptorSetLayout : (*layout)->descriptorSetLayouts)
            vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

        if ((*layout)->layout != VK_NULL_HANDLE)
            vkDestroyPipelineLayout(device, (*layout)->layout, nullptr);

        delete *layout;
        *layout = nullptr;
    }
}

void RenderDevice::DestroyPipeline(RenderPipeline **pipeline)
{
    if (pipeline) {
        if ((*pipeline)->pipeline != VK_NULL_HANDLE)
            vkDestroyPipeline(device, (*pipeline)->pipeline, nullptr);

        delete *pipeline;
        *pipeline = nullptr;
    }
}

void RenderDevice::DestroyPipeline(ComputePipeline **pipeline)
{
    if (pipeline) {
        if ((*pipeline)->pipeline != VK_NULL_HANDLE)
            vkDestroyPipeline(device, (*pipeline)->pipeline, nullptr);

        delete *pipeline;
        *pipeline = nullptr;
    }
}

void RenderDevice::DestroyCommandBuffer(CommandBuffer **cmd)
{
    if (cmd) {
        delete *cmd;
        *cmd = nullptr;
    }
}

void RenderDevice::UploadBufferData(Buffer *buffer, void *data, size_t size)
{
    assert(data && size > 0);

    const BufferCreateInfo createInfo = {
        .size = size,
        .usage = BUFFER_USAGE_TRANSFER_SRC,
    };

    Buffer *staging = CreateBuffer(createInfo);
    memcpy(staging->allocation.info.pMappedData, data, size);

    VK_CHECK(vmaFlushAllocation(allocator, staging->allocation.handle, 0, VK_WHOLE_SIZE));

    VkCommandBuffer copyCmd = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

    VkBufferCopy copyRegion = {0, 0, size};
    vkCmdCopyBuffer(copyCmd, staging->buffer, buffer->buffer, 1, &copyRegion);

    FlushCommandBuffer(copyCmd, graphicsQueue, commandPool, true);

    DestroyBuffer(&staging);
}

void RenderDevice::UploadImageData(Image *image, void *data, size_t size)
{
    assert(data && size > 0);

    const BufferCreateInfo createInfo = {
        .size = size,
        .usage = BUFFER_USAGE_TRANSFER_SRC,
    };

    Buffer *staging = CreateBuffer(createInfo);
    memcpy(staging->allocation.info.pMappedData, data, size);

    VK_CHECK(vmaFlushAllocation(allocator, staging->allocation.handle, 0, VK_WHOLE_SIZE));

    VkCommandBuffer copyCmd = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

    // transition image to transfer
    VkImageMemoryBarrier transferBarrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    transferBarrier.srcAccessMask = 0;
    transferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    transferBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    transferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    transferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    transferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    transferBarrier.image = image->image;
    transferBarrier.subresourceRange = vulkan::getImageSubresourceRange(image);

    vkCmdPipelineBarrier(copyCmd,
        VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, // stages
        0,
        0, nullptr, // memory barriers
        0, nullptr, // buffer memory barriers
        1, &transferBarrier // image memory barriers
    );

    // copy
    VkBufferImageCopy copyRegion = {};
    copyRegion.imageSubresource = vulkan::getImageSubresourceLayers(image);
    copyRegion.imageExtent = {image->width, image->height, 1};

    vkCmdCopyBufferToImage(copyCmd, staging->buffer, image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

    // transition image to fragment shader
    VkImageMemoryBarrier fragmentBarrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    fragmentBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    fragmentBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    fragmentBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    fragmentBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    fragmentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    fragmentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    fragmentBarrier.image = image->image;
    fragmentBarrier.subresourceRange = vulkan::getImageSubresourceRange(image);

    vkCmdPipelineBarrier(copyCmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, // stages
        0,
        0, nullptr, // memory barriers
        0, nullptr, // buffer memory barriers
        1, &fragmentBarrier // image memory barriers
    );

    FlushCommandBuffer(copyCmd, graphicsQueue, commandPool, true);

    DestroyBuffer(&staging);
}

void *RenderDevice::GetMappedData(Buffer *buffer)
{
    return buffer->allocation.info.pMappedData;
}

CommandBuffer *RenderDevice::BeginCommandBuffer()
{
    CommandBuffer *commandBuffer = new CommandBuffer();

    VK_CHECK(vkWaitForFences(device, 1, &finishRenderFences[currentFrame], VK_TRUE, ~0ull));
    VK_CHECK(vkResetFences(device, 1, &finishRenderFences[currentFrame]));

    VkResult result = vkAcquireNextImageKHR(device, swapchain, ~0ull, acquireSemaphores[currentFrame], nullptr, &imageIndex);
    if (resizeRequested || result == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapchain();
        return nullptr;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        LOGE("%s", "Failed to acquire swapchain image.");
        exit(EXIT_FAILURE);
    }

    commandBuffer->cmd = commandBuffers[currentFrame];
    VK_CHECK(vkResetCommandBuffer(commandBuffer->cmd, 0));

    VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK(vkBeginCommandBuffer(commandBuffer->cmd, &beginInfo));

    return commandBuffer;
}

void RenderDevice::EndCommandBuffer(CommandBuffer *commandBuffer)
{
    assert(commandBuffer);
    VK_CHECK(vkEndCommandBuffer(commandBuffer->cmd));
}

void RenderDevice::SubmitCommandBuffer(CommandBuffer *commandBuffer)
{
    assert(commandBuffer);

    // Submit
    VkSubmitInfo submit = {};
    VkPipelineStageFlags stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &acquireSemaphores[currentFrame];
    submit.pWaitDstStageMask = stages;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &commandBuffer->cmd;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &submitSemaphores[imageIndex];
    VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submit, finishRenderFences[currentFrame]));

    // Present
    VkPresentInfoKHR presentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &submitSemaphores[imageIndex];

    VkResult result =  vkQueuePresentKHR(graphicsQueue, &presentInfo);
    if (resizeRequested || result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        RecreateSwapchain();
    }

    currentFrame = (currentFrame + 1) % FRAMES_IN_FLIGHT;
}

void RenderDevice::Draw(CommandBuffer *commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    assert(commandBuffer);
    vkCmdDraw(commandBuffer->cmd, vertexCount, instanceCount, firstVertex, firstInstance);
}

void RenderDevice::DrawIndexed(CommandBuffer *commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
    assert(commandBuffer);
    vkCmdDrawIndexed(commandBuffer->cmd, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void RenderDevice::BindPipeline(CommandBuffer *commandBuffer, RenderPipeline *pipeline)
{
    assert(commandBuffer && pipeline && pipeline->layout);
    PipelineLayout *pipelineLayout = pipeline->layout;

    vkCmdBindPipeline(commandBuffer->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
    if (!pipelineLayout->descriptorSets.empty())
        vkCmdBindDescriptorSets(commandBuffer->cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout->layout, 0, pipelineLayout->descriptorSets.size(), pipelineLayout->descriptorSets.data(), 0, nullptr);
}

void RenderDevice::BindPipeline(CommandBuffer *commandBuffer, ComputePipeline *pipeline)
{
    assert(commandBuffer && pipeline && pipeline->layout);
    PipelineLayout *pipelineLayout = pipeline->layout;

    vkCmdBindPipeline(commandBuffer->cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->pipeline);
    if (!pipelineLayout->descriptorSets.empty())
        vkCmdBindDescriptorSets(commandBuffer->cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout->layout, 0, pipelineLayout->descriptorSets.size(), pipelineLayout->descriptorSets.data(), 0, nullptr);
}

void RenderDevice::BindVertexBuffer(CommandBuffer *commandBuffer, Buffer *vertexBuffer)
{
    assert(commandBuffer && vertexBuffer);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer->cmd, 0, 1, &vertexBuffer->buffer, &offset);
}

void RenderDevice::BeginRendering(CommandBuffer *commandBuffer, const RenderingInfo &renderInfo)
{
    vec2 windowSize = GetWindowSize();
    uint32_t width = (uint32_t)windowSize.x;
    uint32_t height = (uint32_t)windowSize.y;

    VkRenderingAttachmentInfo depthInfo = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    if (renderInfo.depthAttachment) {
        depthInfo.clearValue.depthStencil = {0.0f, 0};
        depthInfo.loadOp = renderInfo.depthAttachment->load ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthInfo.storeOp = renderInfo.depthAttachment->store ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthInfo.imageView = renderInfo.depthAttachment->image->view;
        depthInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    }

    Vector<VkImageMemoryBarrier> imageMemoryBarriers;
    VkPipelineStageFlags srcPipelineStage = 0;
    VkPipelineStageFlags dstPipelineStage = 0;

    Vector<VkRenderingAttachmentInfo> colorAttachments(renderInfo.colorAttachments.size());
    for (size_t i = 0; i < renderInfo.colorAttachments.size(); i++) {
        const AttachmentResource &attachment = renderInfo.colorAttachments[i];

        VkRenderingAttachmentInfo &info = colorAttachments[i];
        info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        info.clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        info.imageView = attachment.image->view;
        info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        info.loadOp = attachment.load ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
        info.storeOp = attachment.store ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;

        VkImageMemoryBarrier &barrier = imageMemoryBarriers.emplace_back();
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = attachment.image->image;
        barrier.subresourceRange = vulkan::getImageSubresourceRange(attachment.image);
        srcPipelineStage |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dstPipelineStage |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }

    if (renderInfo.depthAttachment) {
        VkImageMemoryBarrier &depthBarrier = imageMemoryBarriers.emplace_back();
        depthBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        depthBarrier.srcAccessMask = 0;
        depthBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        depthBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthBarrier.newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        depthBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        depthBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        depthBarrier.image = renderInfo.depthAttachment->image->image;
        depthBarrier.subresourceRange = vulkan::getImageSubresourceRange(renderInfo.depthAttachment->image);
        srcPipelineStage |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dstPipelineStage |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    }

    // insert barriers
    vkCmdPipelineBarrier(commandBuffer->cmd,
        srcPipelineStage, dstPipelineStage, // stages
        0,
        0, nullptr, // memory barriers
        0, nullptr, // buffer memory barriers
        imageMemoryBarriers.size(), imageMemoryBarriers.data() // image memory barriers
    );

    // begin rendering
    VkRenderingInfo vulkanRenderingInfo = {VK_STRUCTURE_TYPE_RENDERING_INFO};
    vulkanRenderingInfo.renderArea = {};
    vulkanRenderingInfo.renderArea.extent = {width, height};
    vulkanRenderingInfo.pDepthAttachment = &depthInfo;
    vulkanRenderingInfo.pColorAttachments = colorAttachments.data();
    vulkanRenderingInfo.colorAttachmentCount = colorAttachments.size();
    vulkanRenderingInfo.layerCount = 1;

    vkCmdBeginRendering(commandBuffer->cmd, &vulkanRenderingInfo);

    SetViewport(commandBuffer, 0.0f, 0.0f, width, height);
    SetScissor(commandBuffer, 0.0f, 0.0f, width, height);

    commandBuffer->renderInfos.push(renderInfo);
}

void RenderDevice::EndRendering(CommandBuffer *commandBuffer)
{
    vkCmdEndRendering(commandBuffer->cmd);

    auto &colorAttachments = commandBuffer->renderInfos.front().colorAttachments;
    for (auto &attachment : colorAttachments) {
        if (!attachment.image->isSwapchain) { // find swapchain image
            continue;
        }

        VkImageMemoryBarrier swapchainBarrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        swapchainBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        swapchainBarrier.dstAccessMask = 0;
        swapchainBarrier.oldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        swapchainBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        swapchainBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        swapchainBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        swapchainBarrier.image = attachment.image->image;
        swapchainBarrier.subresourceRange.levelCount = 1;
        swapchainBarrier.subresourceRange.baseMipLevel = 0;
        swapchainBarrier.subresourceRange.baseArrayLayer = 0;
        swapchainBarrier.subresourceRange.layerCount = 1;
        swapchainBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        vkCmdPipelineBarrier(commandBuffer->cmd,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_NONE, // stages
            0,
            0, nullptr, // memory barriers
            0, nullptr, // buffer memory barriers
            1, &swapchainBarrier // image memory barriers
        );

        break;
    }

    commandBuffer->renderInfos.pop();
}

void RenderDevice::SetViewport(CommandBuffer *commandBuffer, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    VkViewport viewport = {};
    viewport.x = x;
    viewport.y = y;
    viewport.width = width;
    viewport.height = height;
    vkCmdSetViewport(commandBuffer->cmd, 0, 1, &viewport);
}

void RenderDevice::SetScissor(CommandBuffer *commandBuffer, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    VkRect2D scissor = {};
    scissor.offset.x = x;
    scissor.offset.y = y;
    scissor.extent.width = width;
    scissor.extent.height = height;
    vkCmdSetScissor(commandBuffer->cmd, 0, 1, &scissor);
}

void RenderDevice::WriteDescriptor(uint32_t binding, Buffer *buffer, DescriptorType type, uint32_t dstArrayElement)
{
    descriptorSetWriter.write(binding, buffer->buffer, buffer->size, vulkan::getDescriptorType(type), dstArrayElement);
}

void RenderDevice::WriteDescriptor(uint32_t binding, Image *image, Sampler *sampler, DescriptorType type, uint32_t dstArrayElement)
{
    descriptorSetWriter.write(binding, image->view, sampler->sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, vulkan::getDescriptorType(type), dstArrayElement);
}

void RenderDevice::UpdateDescriptors(PipelineLayout *layout, uint32_t set)
{
    assert(set >= 0 && set < layout->descriptorSets.size()); // bounds check
    descriptorSetWriter.update(device, layout->descriptorSets[set]);
    descriptorSetWriter.clear();
}

void RenderDevice::DeviceWaitIdle()
{
    vkDeviceWaitIdle(device);
}

vec2 RenderDevice::GetWindowSize()
{
    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    return vec2(width, height);
}

Image *RenderDevice::GetSwapchainImage()
{
    return swapchainImages[imageIndex];
}

void RenderDevice::CreateInstance()
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

#ifdef ENABLE_VULKAN_DEBUG
    instanceExtensions.push_back("VK_EXT_debug_utils");
#endif

    Vector<const char *> instanceLayers;
#ifdef ENABLE_VULKAN_DEBUG
    instanceLayers.push_back("VK_LAYER_KHRONOS_validation");
#endif

    VkInstanceCreateInfo instanceInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instanceInfo.pApplicationInfo = &appCI;
    instanceInfo.enabledLayerCount = static_cast<uint32_t>(instanceLayers.size());
    instanceInfo.ppEnabledLayerNames = instanceLayers.data();
    instanceInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
    instanceInfo.ppEnabledExtensionNames = instanceExtensions.data();

#ifdef ENABLE_VULKAN_DEBUG
    VkDebugUtilsMessengerCreateInfoEXT messengerInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = vulkanDebugCallback,
        .pUserData = nullptr,
    };
    instanceInfo.pNext = &messengerInfo;

    VK_CHECK(vkCreateInstance(&instanceInfo, nullptr, &instance));
#else
    VK_CHECK(vkCreateInstance(&instanceInfo, nullptr, &instance));
#endif

    volkLoadInstance(instance);

#ifdef ENABLE_VULKAN_DEBUG
    VK_CHECK(vkCreateDebugUtilsMessengerEXT(instance, &messengerInfo, nullptr, &debugMessenger));
#endif
}

void RenderDevice::CreateDevice()
{
    uint32_t physicalDeviceCount = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr));
    assert(physicalDeviceCount > 0);

    Vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data()));

    // FIXME: find appropriate device
    physicalDevice = physicalDevices[0];
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

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

    const char *deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME};

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

    // get queues
    vkGetDeviceQueue(device, graphicsQueueIndex, 0, &graphicsQueue);
    vkGetDeviceQueue(device, computeQueueIndex, 0, &computeQueue);
}

void RenderDevice::CreateAllocator()
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

void RenderDevice::CreateSwapchain()
{
    VkSurfaceCapabilitiesKHR capabilities;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities));

    vec2 windowSize = GetWindowSize();
    swapchainExtent.width = std::clamp(static_cast<uint32_t>(windowSize.x), capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    swapchainExtent.height = std::clamp(static_cast<uint32_t>(windowSize.y), capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

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

    // NOTE: uncomment if VK_SHARING_MODE_CONCURRENT used
    // swapchainCI.queueFamilyIndexCount = 1;
    // swapchainCI.pQueueFamilyIndices = &presentQueueIndex;

    VK_CHECK(vkCreateSwapchainKHR(device, &swapchainCI, nullptr, &swapchain));

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

        Image *image = new Image();
        image->image = vkSwapchainImages[i];
        image->format = IMAGE_FORMAT_B8G8R8A8_SRGB;
        image->usage = IMAGE_USAGE_COLOR_ATTACHMENT;
        image->type = IMAGE_TYPE_2D;
        image->sampleCount = 1;
        image->isSwapchain = true;
        image->width = swapchainExtent.width;
        image->height = swapchainExtent.height;

        vkCreateImageView(device, &imageViewCreateInfo, nullptr, &image->view);
        swapchainImages.push_back(image);
    }
}

void RenderDevice::RecreateSwapchain()
{
}

VkCommandBuffer RenderDevice::CreateCommandBuffer(VkCommandBufferLevel level, bool start)
{
    VkCommandBufferAllocateInfo bufferAllocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    bufferAllocInfo.commandPool = commandPool;
    bufferAllocInfo.level = level;
    bufferAllocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    VK_CHECK(vkAllocateCommandBuffers(device, &bufferAllocInfo, &commandBuffer));

    if (start) {
        VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));
    }

    return commandBuffer;
}

void RenderDevice::FlushCommandBuffer(VkCommandBuffer cmd, VkQueue queue, VkCommandPool pool, bool free)
{
    if (cmd == VK_NULL_HANDLE)
        return;

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkSubmitInfo submit = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.pCommandBuffers = &cmd;
    submit.commandBufferCount = 1;

    VkFenceCreateInfo fenceCreateInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};

    VkFence fence = VK_NULL_HANDLE;
    VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence));

    VK_CHECK(vkQueueSubmit(queue, 1, &submit, fence));
    VK_CHECK(vkWaitForFences(device, 1, &fence, VK_TRUE, ~0L));
    vkDestroyFence(device, fence, nullptr);

    if (free)
        vkFreeCommandBuffers(device, pool, 1, &cmd);
}