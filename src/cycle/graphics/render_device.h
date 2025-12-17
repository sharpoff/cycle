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

    void Init(SDL_Window *window);
    void Shutdown();

    Buffer          *CreateBuffer(const BufferCreateInfo &createInfo);
    Image           *CreateImage(const ImageCreateInfo &createInfo);
    Sampler         *CreateSampler(const SamplerCreateInfo &createInfo);
    PipelineLayout  *CreatePipelineLayout(const PipelineLayoutCreateInfo &createInfo);
    RenderPipeline  *CreateRenderPipeline(const RenderPipelineCreateInfo &createInfo);
    ComputePipeline *CreateComputePipeline(const ComputePipelineCreateInfo &createInfo);

    void DestroyBuffer(Buffer **buffer);
    void DestroyImage(Image **image);
    void DestroySampler(Sampler **sampler);
    void DestroyPipelineLayout(PipelineLayout **layout);
    void DestroyPipeline(RenderPipeline **pipeline);
    void DestroyPipeline(ComputePipeline **pipeline);
    void DestroyCommandBuffer(CommandBuffer **cmd);

    void  UploadBufferData(Buffer *buffer, void *data, size_t size);
    void  UploadImageData(Image *image, void *data, size_t size);
    void *GetMappedData(Buffer *buffer);

    CommandBuffer *BeginCommandBuffer();
    void           EndCommandBuffer(CommandBuffer *commandBuffer);
    void           SubmitCommandBuffer(CommandBuffer *commandBuffer);
    void           Draw(CommandBuffer *commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
    void           DrawIndexed(CommandBuffer *commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance);
    void           BindPipeline(CommandBuffer *commandBuffer, RenderPipeline *pipeline);
    void           BindPipeline(CommandBuffer *commandBuffer, ComputePipeline *pipeline);
    void           BindVertexBuffer(CommandBuffer *commandBuffer, Buffer *vertexBuffer);
    void           BeginRendering(CommandBuffer *commandBuffer, const RenderingInfo &renderInfo);
    void           EndRendering(CommandBuffer *commandBuffer);
    void           SetViewport(CommandBuffer *commandBuffer, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
    void           SetScissor(CommandBuffer *commandBuffer, uint32_t x, uint32_t y, uint32_t width, uint32_t height);

    void WriteDescriptor(uint32_t binding, Buffer *buffer, DescriptorType type, uint32_t dstArrayElement = 0);
    void WriteDescriptor(uint32_t binding, Image *image, Sampler *sampler, DescriptorType type, uint32_t dstArrayElement = 0);
    void UpdateDescriptors(PipelineLayout *layout, uint32_t set);

    void DeviceWaitIdle();

    vec2   GetWindowSize();
    Image *GetSwapchainImage();

private:
    void CreateInstance();
    void CreateDevice();
    void CreateAllocator();
    void CreateSwapchain();

    void RecreateSwapchain();

    VkCommandBuffer CreateCommandBuffer(VkCommandBufferLevel level, bool start);
    void            FlushCommandBuffer(VkCommandBuffer cmd, VkQueue queue, VkCommandPool pool, bool free);

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