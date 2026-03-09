#include "graphics/barrier_merger.h"

void BarrierMerger::transitionImage(VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout, VkImageAspectFlags aspect)
{
    transitionImageMipmapped2(image, currentLayout, newLayout, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, 0, 1, aspect);
}

void BarrierMerger::transitionImageMipmapped(VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout, uint32_t startMip, uint32_t endMip, VkImageAspectFlags aspect)
{
    transitionImageMipmapped2(image, currentLayout, newLayout, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, startMip, endMip, aspect);
}

void BarrierMerger::bufferBarrier(VkBuffer buffer)
{
    bufferBarrier2(buffer, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT);
}

void BarrierMerger::transitionImage2(VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout, VkPipelineStageFlags2 srcStage, VkPipelineStageFlags2 dstStage, VkImageAspectFlags aspect)
{
    transitionImageMipmapped2(image, currentLayout, newLayout, srcStage, dstStage, 0, 1, aspect);
}

void BarrierMerger::transitionImageMipmapped2(VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout, VkPipelineStageFlags2 srcStage, VkPipelineStageFlags2 dstStage, uint32_t startMip, uint32_t endMip, VkImageAspectFlags aspect)
{
    VkImageMemoryBarrier2 &barrier = imageBarriers.emplace_back();
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.image = image;
    barrier.oldLayout = currentLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange = {aspect, startMip, endMip, 0, 1};
    barrier.srcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
    barrier.srcStageMask = srcStage;
    barrier.dstStageMask = dstStage;
}

void BarrierMerger::bufferBarrier2(VkBuffer buffer, VkPipelineStageFlags2 srcStage, VkPipelineStageFlags2 dstStage)
{
    VkBufferMemoryBarrier2 &barrier = bufferBarriers.emplace_back();
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
    barrier.buffer = buffer;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    barrier.srcStageMask = srcStage;
    barrier.dstStageMask = dstStage;
    barrier.offset = 0;
    barrier.size = VK_WHOLE_SIZE;
}

void BarrierMerger::flushBarriers(VkCommandBuffer cmd)
{
    VkDependencyInfo depInfo = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    depInfo.bufferMemoryBarrierCount = bufferBarriers.size();
    depInfo.pBufferMemoryBarriers = bufferBarriers.data();
    depInfo.imageMemoryBarrierCount = imageBarriers.size();
    depInfo.pImageMemoryBarriers = imageBarriers.data();

    vkCmdPipelineBarrier2(cmd, &depInfo);

    bufferBarriers.clear();
    imageBarriers.clear();
}