#include "cycle/graphics/vulkan_helpers.h"

#include "cycle/logger.h"

namespace
{
    inline bool isBitSet(uint64_t flag, uint64_t bit)
    {
        return ((flag & bit) == bit);
    }
}

namespace vulkan
{
    VkImageType getImageType(ImageType type)
    {
        switch (type) {
            case IMAGE_TYPE_1D:
                return VK_IMAGE_TYPE_1D;
            case IMAGE_TYPE_2D:
                return VK_IMAGE_TYPE_2D;
            case IMAGE_TYPE_3D:
                return VK_IMAGE_TYPE_3D;
            case IMAGE_TYPE_CUBE:
                return VK_IMAGE_TYPE_2D;
        }

        LOGE("Invalid image type %d!\n", type);
        return VK_IMAGE_TYPE_MAX_ENUM;
    }

    VkImageViewType getImageViewType(ImageType type)
    {
        switch (type) {
            case IMAGE_TYPE_1D:
                return VK_IMAGE_VIEW_TYPE_1D;
            case IMAGE_TYPE_2D:
                return VK_IMAGE_VIEW_TYPE_2D;
            case IMAGE_TYPE_3D:
                return VK_IMAGE_VIEW_TYPE_3D;
            case IMAGE_TYPE_CUBE:
                return VK_IMAGE_VIEW_TYPE_CUBE;
        }

        LOGE("Invalid image type %d!\n", type);
        return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
    }

    VkImageSubresourceRange getImageSubresourceRange(Image &image)
    {
        VkImageSubresourceRange subresourceRange = {};
        if (isBitSet(image.usage, IMAGE_USAGE_DEPTH_ATTACHMENT)) {
            subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        } else if (isBitSet(image.usage, IMAGE_USAGE_STENCIL_ATTACHMENT)) {
            subresourceRange.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
        } else {
            subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        subresourceRange.levelCount = image.levelCount;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = image.layerCount;

        return subresourceRange;
    }

    VkImageSubresourceLayers getImageSubresourceLayers(Image &image)
    {
        VkImageSubresourceLayers subresourceRange = {};
        if (isBitSet(image.usage, IMAGE_USAGE_DEPTH_ATTACHMENT)) {
            subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        } else if (isBitSet(image.usage, IMAGE_USAGE_STENCIL_ATTACHMENT)) {
            subresourceRange.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
        } else {
            subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        subresourceRange.mipLevel = 0;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = image.layerCount;

        return subresourceRange;
    }

    VkImageUsageFlags getImageUsageFlags(ImageUsageFlags usage)
    {
        VkImageUsageFlags result = 0;
        if (isBitSet(usage, IMAGE_USAGE_TRANSFER_SRC)) {
            result |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }

        if (isBitSet(usage, IMAGE_USAGE_TRANSFER_DST)) {
            result |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }

        if (isBitSet(usage, IMAGE_USAGE_SAMPLED)) {
            result |= VK_IMAGE_USAGE_SAMPLED_BIT;
        }

        if (isBitSet(usage, IMAGE_USAGE_STORAGE)) {
            result |= VK_IMAGE_USAGE_STORAGE_BIT;
        }

        if (isBitSet(usage, IMAGE_USAGE_COLOR_ATTACHMENT)) {
            result |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }

        if (isBitSet(usage, IMAGE_USAGE_DEPTH_ATTACHMENT) || isBitSet(usage, IMAGE_USAGE_STENCIL_ATTACHMENT)) {
            result |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }

        if (isBitSet(usage, IMAGE_USAGE_TRANSIENT_ATTACHMENT)) {
            result |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
        }

        if (isBitSet(usage, IMAGE_USAGE_INPUT_ATTACHMENT)) {
            result |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        }

        if (isBitSet(usage, IMAGE_USAGE_HOST_TRANSFER)) {
            result |= VK_IMAGE_USAGE_HOST_TRANSFER_BIT;
        }

        return result;
    }

    VkFormat getFormat(ImageFormat format)
    {
        switch (format) {
            case IMAGE_FORMAT_UNDEFINED:
                return VK_FORMAT_UNDEFINED;
            case IMAGE_FORMAT_R8G8B8A8_SRGB:
                return VK_FORMAT_R8G8B8A8_SRGB;
            case IMAGE_FORMAT_B8G8R8A8_UNORM:
                return VK_FORMAT_B8G8R8A8_UNORM;
            case IMAGE_FORMAT_D32_SFLOAT:
                return VK_FORMAT_D32_SFLOAT;
            case IMAGE_FORMAT_B8G8R8A8_SRGB:
                return VK_FORMAT_B8G8R8A8_SRGB;
            case IMAGE_FORMAT_R8G8B8A8_UNORM:
                return VK_FORMAT_R8G8B8A8_UNORM;
        }

        LOGE("Invalid format %d!\n", format);
        return VK_FORMAT_MAX_ENUM;
    }

    VkFormat getFormat(VertexFormat format)
    {
        switch (format) {
            case VERTEX_FORMAT_R32_SFLOAT:
                return VK_FORMAT_R32_SFLOAT;
            case VERTEX_FORMAT_R32G32_SFLOAT:
                return VK_FORMAT_R32G32_SFLOAT;
            case VERTEX_FORMAT_R32G32B32_SFLOAT:
                return VK_FORMAT_R32G32B32_SFLOAT;
            case VERTEX_FORMAT_R32G32B32A32_SFLOAT:
                return VK_FORMAT_R32G32B32A32_SFLOAT;
        }

        LOGE("Invalid format %d!\n", format);
        return VK_FORMAT_MAX_ENUM;
    }

    VkBufferUsageFlags getBufferUsageFlags(BufferUsageFlags usage)
    {
        VkBufferUsageFlags result = 0;
        if (isBitSet(usage, BUFFER_USAGE_TRANSFER_SRC)) {
            result |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        }

        if (isBitSet(usage, BUFFER_USAGE_TRANSFER_DST)) {
            result |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        }

        if (isBitSet(usage, BUFFER_USAGE_UNIFORM)) {
            result |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        }

        if (isBitSet(usage, BUFFER_USAGE_STORAGE)) {
            result |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        }

        if (isBitSet(usage, BUFFER_USAGE_INDEX)) {
            result |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        }

        if (isBitSet(usage, BUFFER_USAGE_VERTEX)) {
            result |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        }

        if (isBitSet(usage, BUFFER_USAGE_INDIRECT)) {
            result |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
        }

        if (isBitSet(usage, BUFFER_USAGE_DEVICE_ADDRESS)) {
            result |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        }

        return result;
    }

    VkFilter getFilter(SamplerFilter filter)
    {
        switch (filter) {
            case SAMPLER_FILTER_LINEAR:
                return VK_FILTER_LINEAR;
            case SAMPLER_FILTER_NEAREST:
                return VK_FILTER_NEAREST;
        }

        LOGE("Invalid sampler filter %d!\n", filter);
        return VK_FILTER_MAX_ENUM;
    }

    VkSamplerMipmapMode getSamplerMipmapMode(SamplerFilter mode)
    {
        switch (mode) {
            case SAMPLER_FILTER_LINEAR:
                return VK_SAMPLER_MIPMAP_MODE_LINEAR;
            case SAMPLER_FILTER_NEAREST:
                return VK_SAMPLER_MIPMAP_MODE_NEAREST;
        }

        LOGE("Invalid sampler mipmap mode %d!\n", mode);
        return VK_SAMPLER_MIPMAP_MODE_MAX_ENUM;
    }

    VkSamplerAddressMode getSamplerAddressMode(SamplerAddressMode samplerAddressMode)
    {
        switch (samplerAddressMode) {
            case SAMPLER_ADDRESS_MODE_REPEAT:
                return VK_SAMPLER_ADDRESS_MODE_REPEAT;
            case SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT:
                return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            case SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:
                return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            case SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER:
                return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            case SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE:
                return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
        }

        LOGE("Invalid sampler address mode %d!\n", samplerAddressMode);
        return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
    }

    VkSampleCountFlagBits getSampleCount(uint8_t sampleCount)
    {
        switch (sampleCount) {
            case 1:
                return VK_SAMPLE_COUNT_1_BIT;
            case 2:
                return VK_SAMPLE_COUNT_2_BIT;
            case 4:
                return VK_SAMPLE_COUNT_4_BIT;
            case 8:
                return VK_SAMPLE_COUNT_8_BIT;
            case 16:
                return VK_SAMPLE_COUNT_16_BIT;
            case 32:
                return VK_SAMPLE_COUNT_32_BIT;
            case 64:
                return VK_SAMPLE_COUNT_64_BIT;
        }

        LOGE("Invalid sample count %d!\n", sampleCount);
        return VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM;
    }

    VkCompareOp getCompareOp(CompareOp compareOp)
    {
        switch (compareOp) {
            case COMPARE_OP_NEVER:
                return VK_COMPARE_OP_NEVER;
            case COMPARE_OP_LESS:
                return VK_COMPARE_OP_LESS;
            case COMPARE_OP_EQUAL:
                return VK_COMPARE_OP_EQUAL;
            case COMPARE_OP_LESS_OR_EQUAL:
                return VK_COMPARE_OP_LESS_OR_EQUAL;
            case COMPARE_OP_GREATER:
                return VK_COMPARE_OP_GREATER;
            case COMPARE_OP_NOT_EQUAL:
                return VK_COMPARE_OP_NOT_EQUAL;
            case COMPARE_OP_GREATER_OR_EQUAL:
                return VK_COMPARE_OP_GREATER_OR_EQUAL;
            case COMPARE_OP_ALWAYS:
                return VK_COMPARE_OP_ALWAYS;
        }

        LOGE("Invalid compare operator %d!\n", compareOp);
        return VK_COMPARE_OP_MAX_ENUM;
    }

    VkPrimitiveTopology getPrimitiveTopology(PrimitiveTopology topology)
    {
        switch (topology) {
            case PRIMITIVE_TOPOLOGY_POINT_LIST:
                return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
            case PRIMITIVE_TOPOLOGY_LINE_LIST:
                return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            case PRIMITIVE_TOPOLOGY_LINE_STRIP:
                return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
            case PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            case PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            case PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
        }

        LOGE("Invalid primitive topology %d!\n", topology);
        return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
    }

    VkPolygonMode getPolygonMode(PolygonMode polygonMode)
    {
        switch (polygonMode) {
            case POLYGON_MODE_FILL:
                return VK_POLYGON_MODE_FILL;
            case POLYGON_MODE_LINE:
                return VK_POLYGON_MODE_LINE;
            case POLYGON_MODE_POINT:
                return VK_POLYGON_MODE_POINT;
        }

        LOGE("Invalid polygon mode %d!\n", polygonMode);
        return VK_POLYGON_MODE_MAX_ENUM;
    }

    VkCullModeFlags getCullMode(CullMode cullMode)
    {
        switch (cullMode) {
            case CULL_MODE_NONE:
                return VK_CULL_MODE_NONE;
            case CULL_MODE_FRONT:
                return VK_CULL_MODE_FRONT_BIT;
            case CULL_MODE_BACK:
                return VK_CULL_MODE_BACK_BIT;
            case CULL_MODE_FRONT_AND_BACK:
                return VK_CULL_MODE_FRONT_AND_BACK;
        }

        LOGE("Invalid cull mode %d!\n", cullMode);
        return VK_CULL_MODE_FLAG_BITS_MAX_ENUM;
    }

    VkFrontFace getFrontFace(FrontFace frontFace)
    {
        switch (frontFace) {
            case FRONT_FACE_COUNTER_CLOCKWISE:
                return VK_FRONT_FACE_COUNTER_CLOCKWISE;
            case FRONT_FACE_CLOCKWISE:
                return VK_FRONT_FACE_CLOCKWISE;
        }

        LOGE("Invalid front mode %d!\n", frontFace);
        return VK_FRONT_FACE_MAX_ENUM;
    }

    VkBlendOp getBlendOp(BlendOp blendOp)
    {
        switch (blendOp) {
            case BLEND_OP_NONE:
                return VK_BLEND_OP_ADD;
            case BLEND_OP_ADD:
                return VK_BLEND_OP_ADD;
            case BLEND_OP_SUBTRACT:
                return VK_BLEND_OP_SUBTRACT;
            case BLEND_OP_REVERSE_SUBTRACT:
                return VK_BLEND_OP_REVERSE_SUBTRACT;
            case BLEND_OP_MIN:
                return VK_BLEND_OP_MIN;
            case BLEND_OP_MAX:
                return VK_BLEND_OP_MAX;
        }

        LOGE("Invalid blend operator %d!\n", blendOp);
        return VK_BLEND_OP_MAX_ENUM;
    }

    VkBlendFactor getBlendFactor(BlendFactor blendFactor)
    {
        switch (blendFactor) {
            case BLEND_FACTOR_ZERO:
                return VK_BLEND_FACTOR_ZERO;
            case BLEND_FACTOR_ONE:
                return VK_BLEND_FACTOR_ONE;
            case BLEND_FACTOR_SRC_COLOR:
                return VK_BLEND_FACTOR_SRC_COLOR;
            case BLEND_FACTOR_ONE_MINUS_SRC_COLOR:
                return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
            case BLEND_FACTOR_DST_COLOR:
                return VK_BLEND_FACTOR_DST_COLOR;
            case BLEND_FACTOR_ONE_MINUS_DST_COLOR:
                return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
            case BLEND_FACTOR_SRC_ALPHA:
                return VK_BLEND_FACTOR_SRC_ALPHA;
            case BLEND_FACTOR_ONE_MINUS_SRC_ALPHA:
                return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            case BLEND_FACTOR_DST_ALPHA:
                return VK_BLEND_FACTOR_DST_ALPHA;
            case BLEND_FACTOR_ONE_MINUS_DST_ALPHA:
                return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
            case BLEND_FACTOR_CONSTANT_COLOR:
                return VK_BLEND_FACTOR_CONSTANT_COLOR;
            case BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR:
                return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
            case BLEND_FACTOR_CONSTANT_ALPHA:
                return VK_BLEND_FACTOR_CONSTANT_ALPHA;
            case BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA:
                return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
            case BLEND_FACTOR_SRC_ALPHA_SATURATE:
                return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
        }

        LOGE("Invalid blend factor %d!\n", blendFactor);
        return VK_BLEND_FACTOR_MAX_ENUM;
    }

    VkColorComponentFlags getColorComponentFlags(ColorComponentFlags colorComponentMask)
    {
        VkColorComponentFlags result = 0;
        if (isBitSet(colorComponentMask, COLOR_COMPONENT_R)) {
            result |= VK_COLOR_COMPONENT_R_BIT;
        }

        if (isBitSet(colorComponentMask, COLOR_COMPONENT_G)) {
            result |= VK_COLOR_COMPONENT_G_BIT;
        }

        if (isBitSet(colorComponentMask, COLOR_COMPONENT_B)) {
            result |= VK_COLOR_COMPONENT_B_BIT;
        }

        if (isBitSet(colorComponentMask, COLOR_COMPONENT_A)) {
            result |= VK_COLOR_COMPONENT_A_BIT;
        }

        return result;
    }

    VkDescriptorType getDescriptorType(DescriptorType type)
    {
        switch (type) {
            case DESCRIPTOR_TYPE_SAMPLER:
                return VK_DESCRIPTOR_TYPE_SAMPLER;
            case DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            case DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            case DESCRIPTOR_TYPE_STORAGE_IMAGE:
                return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            case DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
                return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
            case DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
            case DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            case DESCRIPTOR_TYPE_STORAGE_BUFFER:
                return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            case DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            case DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
            case DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
                return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        }

        LOGE("Invalid descriptor type %d!\n", type);
        return VK_DESCRIPTOR_TYPE_MAX_ENUM;
    }

    VkShaderStageFlags getShaderStageFlags(ShaderStageFlags stage)
    {
        VkShaderStageFlags result = 0;
        if (isBitSet(stage, SHADER_STAGE_VERTEX)) {
            result |= VK_SHADER_STAGE_VERTEX_BIT;
        }

        if (isBitSet(stage, SHADER_STAGE_FRAGMENT)) {
            result |= VK_SHADER_STAGE_FRAGMENT_BIT;
        }

        if (isBitSet(stage, SHADER_STAGE_TESSELLATION_CONTROL)) {
            result |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        }

        if (isBitSet(stage, SHADER_STAGE_TESSELLATION_EVALUATION)) {
            result |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        }

        return result;
    }

    const char *toString(VkPresentModeKHR presentMode)
    {
        switch (presentMode) {
            case VK_PRESENT_MODE_IMMEDIATE_KHR:
                return "VK_PRESENT_MODE_IMMEDIATE_KHR";
            case VK_PRESENT_MODE_MAILBOX_KHR:
                return "VK_PRESENT_MODE_MAILBOX_KHR";
            case VK_PRESENT_MODE_FIFO_KHR:
                return "VK_PRESENT_MODE_FIFO_KHR";
            case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
                return "VK_PRESENT_MODE_FIFO_RELAXED_KHR";
            case VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR:
                return "VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR";
            case VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR:
                return "VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR";
            case VK_PRESENT_MODE_FIFO_LATEST_READY_KHR:
                return "VK_PRESENT_MODE_FIFO_LATEST_READY_KHR";
            case VK_PRESENT_MODE_MAX_ENUM_KHR:
                return "VK_PRESENT_MODE_MAX_ENUM_KHR";
        }

        return "UNDEFINED";
    }

    void setDebugName(VkDevice device, uint64_t objectHandle, VkObjectType objectType, String name)
    {
#ifndef NDEBUG
        if (objectHandle == 0) return;

        VkDebugUtilsObjectNameInfoEXT objectNameInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
        objectNameInfo.objectHandle = objectHandle;
        objectNameInfo.objectType = objectType;
        objectNameInfo.pObjectName = name.c_str();

        vkSetDebugUtilsObjectNameEXT(device, &objectNameInfo);
#endif
    }

    void beginDebugLabel(VkCommandBuffer cmd, const char *name, float color[4])
    {
#ifndef NDEBUG
        VkDebugUtilsLabelEXT label = {VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
        label.pLabelName = name;
        label.color[0] = color[0];
        label.color[1] = color[1];
        label.color[2] = color[2];
        label.color[3] = color[3];

        vkCmdBeginDebugUtilsLabelEXT(cmd, &label);
#endif
    }

    void endDebugLabel(VkCommandBuffer cmd)
    {
#ifndef NDEBUG
        vkCmdEndDebugUtilsLabelEXT(cmd);
#endif
    }
} // namespace vulkan