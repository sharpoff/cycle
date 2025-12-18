#pragma once

#include "SDL3/SDL_video.h"

#include "cycle/math.h"

#include "cycle/graphics/descriptor_set_writer.h"
#include "cycle/graphics/graphics_types.h"

#ifndef NDEBUG
#define ENABLE_VULKAN_DEBUG
#endif

#define VULKAN_API_VERSION VK_API_VERSION_1_4

#define FRAMES_IN_FLIGHT 2

class RenderDevice
{
public:
    RenderDevice() = default;
    ~RenderDevice() = default;

    void init(SDL_Window *window);
    void shutdown();

    Buffer          *createBuffer(const BufferCreateInfo &createInfo);
    Image           *createImage(const ImageCreateInfo &createInfo);
    Sampler         *createSampler(const SamplerCreateInfo &createInfo);
    PipelineLayout  *createPipelineLayout(const PipelineLayoutCreateInfo &createInfo);
    RenderPipeline  *createRenderPipeline(const RenderPipelineCreateInfo &createInfo);
    ComputePipeline *createComputePipeline(const ComputePipelineCreateInfo &createInfo);

    void destroyBuffer(Buffer **buffer);
    void destroyImage(Image **image);
    void destroySampler(Sampler **sampler);
    void destroyPipelineLayout(PipelineLayout **layout);
    void destroyPipeline(RenderPipeline **pipeline);
    void destroyPipeline(ComputePipeline **pipeline);
    void destroyCommandBuffer(CommandBuffer **cmd);

    void  uploadBufferData(Buffer *buffer, void *data, size_t size);
    void  uploadImageData(Image *image, void *data, size_t size);
    void *getMappedData(Buffer *buffer);

    CommandBuffer *beginCommandBuffer();
    void           endCommandBuffer(CommandBuffer *commandBuffer);
    void           submitCommandBuffer(CommandBuffer *commandBuffer);
    void           draw(CommandBuffer *commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
    void           drawIndexed(CommandBuffer *commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance);
    void           bindPipeline(CommandBuffer *commandBuffer, RenderPipeline *pipeline);
    void           bindPipeline(CommandBuffer *commandBuffer, ComputePipeline *pipeline);
    void           bindVertexBuffer(CommandBuffer *commandBuffer, Buffer *vertexBuffer);
    void           beginRendering(CommandBuffer *commandBuffer, const RenderingInfo &renderInfo);
    void           endRendering(CommandBuffer *commandBuffer);
    void           setViewport(CommandBuffer *commandBuffer, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
    void           setScissor(CommandBuffer *commandBuffer, uint32_t x, uint32_t y, uint32_t width, uint32_t height);

    void writeDescriptor(uint32_t binding, Buffer *buffer, DescriptorType type, uint32_t dstArrayElement = 0);
    void writeDescriptor(uint32_t binding, Image *image, Sampler *sampler, DescriptorType type, uint32_t dstArrayElement = 0);
    void updateDescriptors(PipelineLayout *layout, uint32_t set);

    void waitIdle();

    vec2   getWindowSize();
    Image *getSwapchainImage();

private:
    void createInstance();
    void createDevice();
    void createAllocator();
    void createSwapchain();

    void recreateSwapchain();

    VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level, bool start);
    void            flushCommandBuffer(VkCommandBuffer cmd, VkQueue queue, VkCommandPool pool, bool free);

    VkInstance instance = VK_NULL_HANDLE;

    SDL_Window  *window = nullptr;
    VkSurfaceKHR surface = VK_NULL_HANDLE;

#ifdef ENABLE_VULKAN_DEBUG
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
#endif

    uint32_t graphicsQueueIndex = UINT32_MAX;
    uint32_t computeQueueIndex = UINT32_MAX;

    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue computeQueue = VK_NULL_HANDLE;

    VkPhysicalDevice           physicalDevice = VK_NULL_HANDLE;
    VkDevice                   device = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures   deviceFeatures;

    VmaAllocator allocator = VK_NULL_HANDLE;

    VkSwapchainKHR     swapchain = VK_NULL_HANDLE;
    Vector<Image *>    swapchainImages;
    VkExtent2D         swapchainExtent = {};
    VkPresentModeKHR   presentMode;
    VkSurfaceFormatKHR surfaceFormat;

    VkCommandPool                            commandPool;
    Array<VkCommandBuffer, FRAMES_IN_FLIGHT> commandBuffers;
    Array<VkSemaphore, FRAMES_IN_FLIGHT>     acquireSemaphores;
    Array<VkFence, FRAMES_IN_FLIGHT>         finishRenderFences;
    Vector<VkSemaphore>                      submitSemaphores;

    VkDescriptorPool    descriptorPool;
    DescriptorSetWriter descriptorSetWriter;

    uint32_t maxSampleCount = VK_SAMPLE_COUNT_1_BIT;
    uint32_t currentFrame = 0;
    uint32_t imageIndex = 0;
    bool     resizeRequested = false;
};