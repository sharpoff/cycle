#pragma once

#include "SDL3/SDL_video.h"

#include "cycle/graphics/command_encoder.h"
#include "cycle/math.h"

#include "cycle/graphics/descriptor_set_writer.h"
#include "cycle/graphics/graphics_types.h"

#define VULKAN_API_VERSION VK_API_VERSION_1_3

#define FRAMES_IN_FLIGHT 2

class RenderDevice
{
public:
    void init(SDL_Window *window);
    void shutdown();

    bool createBuffer(Buffer &buffer, const BufferCreateInfo &createInfo, String debugName = "");
    bool createImage(Image &image, const ImageCreateInfo &createInfo, String debugName = "");
    bool createSampler(Sampler &sampler, const SamplerCreateInfo &createInfo, String debugName = "");
    bool createPipelineLayout(PipelineLayout &pipelineLayout, const PipelineLayoutCreateInfo &createInfo, String debugName = "");
    bool createRenderPipeline(RenderPipeline &renderPipeline, const RenderPipelineCreateInfo &createInfo, String debugName = "");
    bool createComputePipeline(ComputePipeline &computePipeline, const ComputePipelineCreateInfo &createInfo, String debugName = "");

    void destroyBuffer(Buffer &buffer);
    void destroyImage(Image &image);
    void destroySampler(Sampler &sampler);
    void destroyPipelineLayout(PipelineLayout &layout);
    void destroyRenderPipeline(RenderPipeline &pipeline);
    void destroyComputePipeline(ComputePipeline &pipeline);

    void uploadBufferData(Buffer &buffer, void *data, uint64_t size, Buffer *stagingBuffer = nullptr);
    void uploadImageData(Image &image, void *data, uint64_t size);

    bool beginCommandEncoding(CommandEncoder &encoder);
    void endCommandEncoding(CommandEncoder &encoder);
    bool swapchainPresent();

    void immediateSubmit(Func<void(VkCommandBuffer cmd)> &&function);

    void writeDescriptor(uint32_t binding, Buffer &buffer, DescriptorType type, uint32_t dstArrayElement = 0);
    void writeDescriptor(uint32_t binding, Image &image, Sampler &sampler, DescriptorType type, uint32_t dstArrayElement = 0);
    void updateDescriptors(PipelineLayout &layout, uint32_t set);

    void waitIdle();

    vec2   getWindowSize();
    Image &getSwapchainImage();

private:
    void createInstance();
    void createDevice();
    void createAllocator();
    void createSwapchain();

    void recreateSwapchain();

    VkInstance instance = VK_NULL_HANDLE;

    SDL_Window  *window = nullptr;
    VkSurfaceKHR surface = VK_NULL_HANDLE;

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
    Vector<Image>    swapchainImages;
    VkExtent2D         swapchainExtent = {};
    VkPresentModeKHR   presentMode;
    VkSurfaceFormatKHR surfaceFormat;

    VkCommandPool                            commandPool;
    Array<VkCommandBuffer, FRAMES_IN_FLIGHT> commandBuffers;
    Array<VkSemaphore, FRAMES_IN_FLIGHT>     acquireSemaphores;
    Array<VkFence, FRAMES_IN_FLIGHT>         finishRenderFences;
    Vector<VkSemaphore>                      submitSemaphores;

    // For immediateSubmit()
    VkCommandPool   immediateCommandPool;
    VkCommandBuffer immediateCommandBuffer;
    VkFence         immediateFence;

    VkDescriptorPool    descriptorPool;
    DescriptorSetWriter descriptorSetWriter;

    uint32_t maxSampleCount = VK_SAMPLE_COUNT_1_BIT;
    uint32_t currentFrame = 0;
    uint32_t imageIndex = 0;
    bool     resizeRequested = false;
};