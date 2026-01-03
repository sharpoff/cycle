#pragma once

#include <limits>
#include <stdint.h>

// clang-format off
#include <volk.h>
#include <vk_mem_alloc.h>
// clang-format on

#include "cycle/containers.h"

//========================
//        General
//========================

struct Allocation
{
    VmaAllocation     handle = VK_NULL_HANDLE;
    VmaAllocationInfo info = {};
};

//========================
//         Image
//========================

struct ImageLoadInfo
{
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t channels = 4; // rgba
    uint32_t size = 0;
    void *pixels = nullptr;
};

struct ImageCreateInfo
{
    uint32_t              width = 0;
    uint32_t              height = 0;
    uint32_t              arrayLayers = 1;
    uint32_t              mipLevels = 1;
    VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;
    VkImageViewType       type = VK_IMAGE_VIEW_TYPE_2D;
    VkImageUsageFlags     usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    VkFormat              format = VK_FORMAT_R8G8B8A8_SRGB;
    VkImageAspectFlags    aspect = VK_IMAGE_ASPECT_COLOR_BIT;
};

struct Image
{
    uint32_t              width = 0;
    uint32_t              height = 0;
    uint32_t              arrayLayers = 1;
    uint32_t              mipLevels = 1;
    VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;
    VkImageViewType       type = VK_IMAGE_VIEW_TYPE_2D;
    VkImageUsageFlags     usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    VkFormat              format = VK_FORMAT_R8G8B8A8_SRGB;
    VkImageAspectFlags    aspect = VK_IMAGE_ASPECT_COLOR_BIT;

    VkImage       image = VK_NULL_HANDLE;
    VkImageView   view = VK_NULL_HANDLE;
    Allocation    allocation = {};
};

//========================
//         Buffer
//========================

struct BufferCreateInfo
{
    uint64_t           size;
    VkBufferUsageFlags usage;
};

struct Buffer
{
    uint64_t           size = 0;
    VkBufferUsageFlags usage;

    VkBuffer        buffer = VK_NULL_HANDLE;
    Allocation      allocation;
    VkDeviceAddress address;
};

//========================
//        Sampler
//========================

struct SamplerCreateInfo
{
    float                mipLodBias = 0.0f;
    float                minLod = 0.0f;
    float                maxLod = std::numeric_limits<float>::max();
    float                maxAnisotropy = 0.0f;
    VkFilter             magFilter = VK_FILTER_LINEAR;
    VkFilter             minFilter = VK_FILTER_LINEAR;
    VkSamplerMipmapMode  mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    VkSamplerAddressMode addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    VkCompareOp          compareOp = VK_COMPARE_OP_NEVER;
};

struct Sampler
{
    float                mipLodBias = 0.0f;
    float                minLod = 0.0f;
    float                maxLod = std::numeric_limits<float>::max();
    float                maxAnisotropy = 0.0f;
    VkFilter             magFilter = VK_FILTER_LINEAR;
    VkFilter             minFilter = VK_FILTER_LINEAR;
    VkSamplerMipmapMode  mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    VkSamplerAddressMode addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    VkCompareOp          compareOp = VK_COMPARE_OP_NEVER;

    VkSampler sampler;
};

//========================
//    Pipeline layout
//========================

struct PipelineLayoutCreateInfo
{
    Vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
    Vector<VkPushConstantRange>          pushConstantRanges;
};

struct PipelineLayout
{
    VkDescriptorSetLayout descriptorSetLayout;
    VkPushConstantRange   pushConstantRange;
    VkDescriptorSet       descriptorSet;
    VkPipelineLayout      layout;
};

//========================
//    Render pipeline
//========================

struct RenderPipelineCreateInfo
{
    Vector<VkVertexInputBindingDescription>   vertexBindings;
    Vector<VkVertexInputAttributeDescription> vertexAttributes;

    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPolygonMode   polygonMode = VK_POLYGON_MODE_FILL;
    VkCullModeFlags cullMode = VK_CULL_MODE_NONE;
    VkFrontFace     frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    uint32_t patchControlPoints = 0;

    VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;

    VkCompareOp depthCompareOp = VK_COMPARE_OP_ALWAYS;
    bool        depthWriteEnable = false;

    Vector<VkFormat> colorAttachmentFormats;
    VkFormat         depthAttachmentFormat = VK_FORMAT_UNDEFINED;

    VkBlendOp             colorBlendOp = VK_BLEND_OP_MAX_ENUM;
    VkBlendFactor         colorBlendFactorSrc = VK_BLEND_FACTOR_ZERO;
    VkBlendFactor         colorBlendFactorDst = VK_BLEND_FACTOR_ZERO;
    VkBlendFactor         alphaBlendFactorSrc = VK_BLEND_FACTOR_ZERO;
    VkBlendFactor         alphaBlendFactorDst = VK_BLEND_FACTOR_ZERO;
    VkBlendOp             alphaBlendOp = VK_BLEND_OP_MAX_ENUM;
    VkColorComponentFlags colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    Vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    PipelineLayout *pipelineLayout = nullptr;

    Vector<char> vertexCode;
    Vector<char> fragmentCode;
    Vector<char> tessellationControlCode;
    Vector<char> tessellationEvaluationCode;
};

struct RenderPipeline
{
    PipelineLayout *layout = nullptr;
    VkPipeline      pipeline;
};

//========================
//    Compute pipeline
//========================

struct ComputePipelineCreateInfo
{
    PipelineLayout *pipelineLayout = nullptr;
    Vector<char>    computeCode;
};

struct ComputePipeline
{
    PipelineLayout *layout = nullptr;
    VkPipeline      pipeline;
};