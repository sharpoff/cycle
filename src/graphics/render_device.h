#pragma once

#include "SDL3/SDL_video.h"

#include "graphics/descriptor_set_writer.h"
#include "graphics/vertex.h"
#include "graphics/vulkan_types.h"

#include <math.h>

#include "core/constants.h"

#define VULKAN_API_VERSION VK_API_VERSION_1_3
#define ENABLE_LOG_DEVICE_LIMITS false

class RenderDevice
{
public:
    void Init(SDL_Window *window);
    void Shutdown();

    Buffer *CreateBuffer(const BufferCreateInfo &createInfo, VmaMemoryUsage memoryUsage);
    Texture *CreateTexture(const TextureCreateInfo &createInfo);
    Sampler *CreateSampler(const SamplerCreateInfo &createInfo);
    RenderPipeline *CreateRenderPipeline(const RenderPipelineCreateInfo &createInfo);
    ComputePipeline *CreateComputePipeline(const ComputePipelineCreateInfo &createInfo);

    void DestroyBuffer(Buffer *buffer);
    void DestroyTexture(Texture *texture);
    void DestroySampler(Sampler *sampler);
    void DestroyRenderPipeline(RenderPipeline *pipeline);
    void DestroyComputePipeline(ComputePipeline *pipeline);

    void UploadBufferData(Buffer *buffer, void *data, uint64_t size);
    void UploadTexture(Texture *texture, TextureLoadInfo &loadInfo);
    void UploadMeshGpuData(Buffer *&vertexBuffer, Vector<Vertex> &vertices, Buffer *&indexBuffer, Vector<uint32_t> &indices);

    void GenerateMipmaps(Texture *texture);

    VkCommandBuffer BeginCommandBuffer();
    void EndCommandBuffer(VkCommandBuffer cmd);
    bool SwapchainPresent();

    void ImmediateSubmit(Func<void(VkCommandBuffer cmd)> &&function);

    void WriteDescriptor(uint32_t binding, Buffer *buffer, VkDescriptorType type, uint32_t dstArrayElement = 0);
    void WriteDescriptor(uint32_t binding, Texture *texture, Sampler &sampler, VkDescriptorType type, uint32_t dstArrayElement = 0);
    void WriteDescriptor(uint32_t binding, Texture *texture, VkDescriptorType type, uint32_t dstArrayElement = 0);
    void WriteDescriptor(uint32_t binding, Sampler *sampler, VkDescriptorType type, uint32_t dstArrayElement = 0);
    void UpdateDescriptors();

    void WaitIdle();

    uint32_t GetSwapchainWidth() { return swapchainExtent.width; };
    uint32_t GetSwapchainHeight() { return swapchainExtent.height; };
    VkImage GetSwapchainImage() { return swapchainImages[imageIndex]; };
    VkImageView GetSwapchainImageView() { return swapchainImageViews[imageIndex]; };
    VkSurfaceFormatKHR GetSurfaceFormat() { return surfaceFormat; }
    VkDescriptorSet &GetBindlessDescriptor() { return bindlessDescriptorSets[currentFrame]; }
    uint32_t GetCurrentFrame() { return currentFrame; }

    uint32_t CalculateMipLevels(uint32_t width, uint32_t height) { return floor(log2(std::max(width, height))) + 1; }

    VkSampleCountFlagBits maxSampleCount = VK_SAMPLE_COUNT_1_BIT;

private:
    void CreateInstance();
    void CreateDevice();
    void CreateAllocator();
    void CreateSwapchain();
    void DestroySwapchain();
    void CreateSyncObjects();

    void RecreateSwapchain();

    void SetupImGui();
    void ShutdownImGui();

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