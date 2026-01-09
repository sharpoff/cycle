#pragma once

#include "SDL3/SDL_video.h"

#include "cycle/graphics/descriptor_set_writer.h"
#include "cycle/graphics/vulkan_types.h"

#include <math.h>

#include "cycle/constants.h"

#define VULKAN_API_VERSION VK_API_VERSION_1_3
#define ENABLE_LOG_DEVICE_LIMITS false

class RenderDevice
{
public:
    void init(SDL_Window *window);
    void shutdown();

    Buffer createBuffer(const BufferCreateInfo &createInfo, VmaMemoryUsage memoryUsage);
    Image createImage(const ImageCreateInfo &createInfo);
    Sampler createSampler(const SamplerCreateInfo &createInfo);
    RenderPipeline createRenderPipeline(const RenderPipelineCreateInfo &createInfo);
    ComputePipeline createComputePipeline(const ComputePipelineCreateInfo &createInfo);

    void destroyBuffer(Buffer &buffer);
    void destroyImage(Image &image);
    void destroySampler(Sampler &sampler);
    void destroyRenderPipeline(RenderPipeline &pipeline);
    void destroyComputePipeline(ComputePipeline &pipeline);

    void uploadBufferData(Buffer &buffer, void *data, uint64_t size);
    void uploadImage(Image &image, ImageLoadInfo &loadInfo);

    void generateMipmaps(Image &image);

    VkCommandBuffer beginCommandBuffer();
    void endCommandBuffer(VkCommandBuffer cmd);
    bool swapchainPresent();

    void immediateSubmit(Func<void(VkCommandBuffer cmd)> &&function);

    void writeDescriptor(uint32_t binding, Buffer &buffer, VkDescriptorType type, uint32_t dstArrayElement = 0);
    void writeDescriptor(uint32_t binding, Image &image, Sampler &sampler, VkDescriptorType type, uint32_t dstArrayElement = 0);
    void writeDescriptor(uint32_t binding, Image &image, VkDescriptorType type, uint32_t dstArrayElement = 0);
    void writeDescriptor(uint32_t binding, Sampler &sampler, VkDescriptorType type, uint32_t dstArrayElement = 0);
    void updateDescriptors();

    void waitIdle();

    uint32_t getSwapchainWidth() { return swapchainExtent.width; };
    uint32_t getSwapchainHeight() { return swapchainExtent.height; };
    VkImage getSwapchainImage() { return swapchainImages[imageIndex]; };
    VkImageView getSwapchainImageView() { return swapchainImageViews[imageIndex]; };
    VkSurfaceFormatKHR getSurfaceFormat() { return surfaceFormat; }
    VkDescriptorSet &getBindlessDescriptor() { return bindlessDescriptorSets[currentFrame]; }
    uint32_t getCurrentFrame() { return currentFrame; }

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

    SDL_Window *window = nullptr;
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    uint32_t graphicsQueueIndex = UINT32_MAX;
    uint32_t computeQueueIndex = UINT32_MAX;

    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue computeQueue = VK_NULL_HANDLE;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;

    VmaAllocator allocator = VK_NULL_HANDLE;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    Vector<VkImage> swapchainImages;
    Vector<VkImageView> swapchainImageViews;
    VkExtent2D swapchainExtent = {};
    VkPresentModeKHR presentMode;
    VkSurfaceFormatKHR surfaceFormat;

    VkCommandPool commandPool;
    Array<VkCommandBuffer, FRAMES_IN_FLIGHT> commandBuffers;
    Array<VkSemaphore, FRAMES_IN_FLIGHT> acquireSemaphores;
    Array<VkFence, FRAMES_IN_FLIGHT> finishRenderFences;
    Vector<VkSemaphore> submitSemaphores;

    // For immediateSubmit()
    VkCommandPool immediateCommandPool;
    VkCommandBuffer immediateCommandBuffer;
    VkFence immediateFence;

    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout descriptorSetLayout;
    Array<VkDescriptorSet, FRAMES_IN_FLIGHT> bindlessDescriptorSets;
    DescriptorSetWriter descriptorSetWriter;

    uint32_t currentFrame = 0;
    uint32_t imageIndex = 0;
    bool resizeRequested = false;
};