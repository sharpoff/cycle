#pragma once

// taken from here: https://vkguide.dev/docs/ascendant/ascendant_light/

#include "core/containers.h"
#include <volk.h>

struct BarrierMerger
{
    void TransitionImage(VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout, VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT);
    void TransitionImageMipmapped(VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout, uint32_t startMip, uint32_t endMip, VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT);
    void BufferBarrier(VkBuffer buffer);

    void TransitionImage2(VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout, VkPipelineStageFlags2 srcStage, VkPipelineStageFlags2 dstStage, VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT);
    void TransitionImageMipmapped2(VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout, VkPipelineStageFlags2 srcStage, VkPipelineStageFlags2 dstStage, uint32_t startMip, uint32_t endMip, VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT);
    void BufferBarrier2(VkBuffer buffer, VkPipelineStageFlags2 srcStage, VkPipelineStageFlags2 dstStage);

    void FlushBarriers(VkCommandBuffer cmd);

    Vector<VkImageMemoryBarrier2> imageBarriers;
    Vector<VkBufferMemoryBarrier2> bufferBarriers;
};