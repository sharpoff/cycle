#pragma once

#include "SDL3/SDL_video.h"

#include "cycle/graphics/command_encoder.h"

#include "cycle/graphics/descriptor_set_writer.h"
#include "cycle/graphics/vulkan_types.h"

#include <math.h>

#define VULKAN_API_VERSION VK_API_VERSION_1_3
#define ENABLE_LOG_DEVICE_LIMITS false

#define FRAMES_IN_FLIGHT 2

class RenderDevice
{
public:
    void init(SDL_Window *window);
    void shutdown();

    bool createBuffer(Buffer &buffer, const BufferCreateInfo &createInfo);
    bool createImage(Image &image, const ImageCreateInfo &createInfo);
    bool createSampler(Sampler &sampler, const SamplerCreateInfo &createInfo);
    bool createPipelineLayout(PipelineLayout &pipelineLayout, const PipelineLayoutCreateInfo &createInfo);
    bool createRenderPipeline(RenderPipeline &renderPipeline, const RenderPipelineCreateInfo &createInfo);
    bool createComputePipeline(ComputePipeline &computePipeline, const ComputePipelineCreateInfo &createInfo);

    void destroyBuffer(Buffer &buffer);
    void destroyImage(Image &image);
    void destroySampler(Sampler &sampler);
    void destroyPipelineLayout(PipelineLayout &layout);
    void destroyRenderPipeline(RenderPipeline &pipeline);
    void destroyComputePipeline(ComputePipeline &pipeline);

    void uploadBufferData(Buffer &buffer, void *data, uint64_t size, Buffer *stagingBuffer = nullptr);
    void uploadImageData(Image &image, void *data, uint64_t size);

    void generateMipmaps(Image &image);

    bool beginCommandBuffer(CommandEncoder &encoder);
    void endCommandBuffer(CommandEncoder &encoder);
    bool swapchainPresent();

    void immediateSubmit(Func<void(VkCommandBuffer cmd)> &&function);

    void writeDescriptor(uint32_t binding, Buffer &buffer, VkDescriptorType type, uint32_t dstArrayElement = 0);
    void writeDescriptor(uint32_t binding, Image &image, Sampler &sampler, VkDescriptorType type, uint32_t dstArrayElement = 0);
    void writeDescriptor(uint32_t binding, Image &image, VkDescriptorType type, uint32_t dstArrayElement = 0);
    void writeDescriptor(uint32_t binding, Sampler &sampler, VkDescriptorType type, uint32_t dstArrayElement = 0);
    void updateDescriptors(PipelineLayout &layout);

    void waitIdle();

    uint32_t    getSwapchainWidth() { return swapchainExtent.width; };
    uint32_t    getSwapchainHeight() { return swapchainExtent.height; };
    VkImage     getSwapchainImage() { return swapchainImages[imageIndex]; };
    VkImageView getSwapchainImageView() { return swapchainImageViews[imageIndex]; };

    uint32_t calculateMipLevels(uint32_t width, uint32_t height) { return floor(log2(std::max(width, height))) + 1; }

    VkSampleCountFlagBits maxSampleCount = VK_SAMPLE_COUNT_1_BIT;

private:
    void createInstance();
    void createDevice();
    void createAllocator();
    void createSwapchain();
    void destroySwapchain();
    void createSyncObjects();

    void recreateSwapchain();

    void setupImGui();
    void shutdownImGui();

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

    VkSwapchainKHR      swapchain = VK_NULL_HANDLE;
    Vector<VkImage>     swapchainImages;
    Vector<VkImageView> swapchainImageViews;
    VkExtent2D          swapchainExtent = {};
    VkPresentModeKHR    presentMode;
    VkSurfaceFormatKHR  surfaceFormat;

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

    uint32_t currentFrame = 0;
    uint32_t imageIndex = 0;
    bool     resizeRequested = false;
};