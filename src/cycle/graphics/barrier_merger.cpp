#include "cycle/graphics/barrier_merger.h"

void BarrierMerger::transitionImage(VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout, VkImageAspectFlags aspect)
{
    VkImageMemoryBarrier &barrier = imageBarriers.emplace_back();
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.oldLayout = currentLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange = {aspect, 0, 1, 0, 1};
    barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
}

void BarrierMerger::transitionImageMipmapped(VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout, uint32_t startMip, uint32_t endMip, VkImageAspectFlags aspect)
{
    VkImageMemoryBarrier &barrier = imageBarriers.emplace_back();
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.oldLayout = currentLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange = {aspect, startMip, endMip, 0, 1};
    barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
}

void BarrierMerger::bufferBarrier(VkBuffer buffer)
{
    VkBufferMemoryBarrier &barrier = bufferBarriers.emplace_back();
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.buffer = buffer;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    barrier.offset = 0;
    barrier.size = VK_WHOLE_SIZE;
}

void BarrierMerger::flushBarriers(VkCommandBuffer cmd)
{
    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0,
        0, nullptr,
        bufferBarriers.size(), bufferBarriers.data(),
        imageBarriers.size(), imageBarriers.data()
    );

    bufferBarriers.clear();
    imageBarriers.clear();
}