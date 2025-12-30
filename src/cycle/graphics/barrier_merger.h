#pragma once

// taken from here: https://vkguide.dev/docs/ascendant/ascendant_light/

#include "cycle/containers.h"
#include <volk.h>

struct BarrierMerger
{
    void transitionImage(VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout, VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT);
    void transitionImageMipmapped(VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout, uint32_t startMip, uint32_t endMip, VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT);

    void bufferBarrier(VkBuffer buffer);

    void flushBarriers(VkCommandBuffer cmd);

    Vector<VkImageMemoryBarrier> imageBarriers;
    Vector<VkBufferMemoryBarrier> bufferBarriers;
};