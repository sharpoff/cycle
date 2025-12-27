#pragma once

#include <assert.h>

#include "cycle/graphics/graphics_types.h"

#include <volk.h>

#define VK_CHECK(code)                 \
    do {                               \
        VkResult res = (code);         \
        if (res != VK_SUCCESS) {       \
            assert(res != VK_SUCCESS); \
        }                              \
    } while (0)

namespace vulkan
{
    VkImageType              getImageType(ImageType type);
    VkImageViewType          getImageViewType(ImageType type);
    VkImageSubresourceRange  getImageSubresourceRange(Image &image);
    VkImageSubresourceLayers getImageSubresourceLayers(Image &image);
    VkImageUsageFlags        getImageUsageFlags(ImageUsageFlags usage);
    VkFormat                 getFormat(ImageFormat format);
    VkFormat                 getFormat(VertexFormat format);
    VkBufferUsageFlags       getBufferUsageFlags(BufferUsageFlags usage);
    VkFilter                 getFilter(SamplerFilter filter);
    VkSamplerMipmapMode      getSamplerMipmapMode(SamplerFilter mode);
    VkSamplerAddressMode     getSamplerAddressMode(SamplerAddressMode samplerAddressMode);
    VkSampleCountFlagBits    getSampleCount(uint8_t sampleCount);
    VkCompareOp              getCompareOp(CompareOp compareOp);
    VkPrimitiveTopology      getPrimitiveTopology(PrimitiveTopology topology);
    VkPolygonMode            getPolygonMode(PolygonMode polygonMode);
    VkCullModeFlags          getCullMode(CullMode cullMode);
    VkFrontFace              getFrontFace(FrontFace frontFace);
    VkBlendOp                getBlendOp(BlendOp blendOp);
    VkBlendFactor            getBlendFactor(BlendFactor blendFactor);
    VkColorComponentFlags    getColorComponentFlags(ColorComponentFlags colorComponentMask);
    VkDescriptorType         getDescriptorType(DescriptorType type);
    VkShaderStageFlags       getShaderStageFlags(ShaderStageFlags stage);

    const char *toString(VkPresentModeKHR presentMode);
    const char *toString(VkFormat format);

    void setDebugName(VkDevice device, uint64_t objectHandle, VkObjectType objectType, String name);
    void beginDebugLabel(VkCommandBuffer cmd, const char *name, float color[4]);
    void endDebugLabel(VkCommandBuffer cmd);
} // namespace vulkan