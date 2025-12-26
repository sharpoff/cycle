#include "cycle/graphics/command_encoder.h"

#include "cycle/graphics/vulkan_helpers.h"
#include "cycle/math.h"

#include <assert.h>

void CommandEncoder::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    vkCmdDraw(cmd, vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandEncoder::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
    vkCmdDrawIndexed(cmd, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void CommandEncoder::bindPipeline(RenderPipeline &pipeline)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
    if (!pipeline.layout->descriptorSets.empty())
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout->layout, 0, pipeline.layout->descriptorSets.size(), pipeline.layout->descriptorSets.data(), 0, nullptr);
}

void CommandEncoder::bindPipeline(ComputePipeline &pipeline)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);
    if (!pipeline.layout->descriptorSets.empty())
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.layout->layout, 0, pipeline.layout->descriptorSets.size(), pipeline.layout->descriptorSets.data(), 0, nullptr);
}

void CommandEncoder::bindVertexBuffer(Buffer &vertexBuffer)
{
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer.buffer, &offset);
}

void CommandEncoder::bindIndexBuffer(Buffer &indexBuffer)
{
    vkCmdBindIndexBuffer(cmd, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
}

void CommandEncoder::beginRendering(const RenderingInfo &renderInfo)
{
    VkRenderingAttachmentInfo depthInfo = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};

    Vector<VkImageMemoryBarrier> imageMemoryBarriers;
    VkPipelineStageFlags srcPipelineStage = 0;
    VkPipelineStageFlags dstPipelineStage = 0;

    if (renderInfo.depthAttachment) {
        const AttachmentInfo &attachment = *renderInfo.depthAttachment;

        depthInfo.clearValue.depthStencil = {0.0f, 0};
        depthInfo.loadOp = attachment.load ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthInfo.storeOp = attachment.store ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthInfo.imageView = attachment.image->view;
        depthInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;

        VkImageMemoryBarrier &barrier = imageMemoryBarriers.emplace_back();
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = attachment.image->image;
        barrier.subresourceRange = vulkan::getImageSubresourceRange(*attachment.image);
        srcPipelineStage |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dstPipelineStage |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    }

    Vector<VkRenderingAttachmentInfo> colorAttachments;
    for (size_t i = 0; i < renderInfo.colorAttachments.size(); i++) {
        const AttachmentInfo &attachment = renderInfo.colorAttachments[i];

        VkRenderingAttachmentInfo &info = colorAttachments.emplace_back();
        info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        info.clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        info.loadOp = attachment.load ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
        info.storeOp = attachment.store ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
        info.imageView = attachment.image->view;
        info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkImageMemoryBarrier &barrier = imageMemoryBarriers.emplace_back();
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = attachment.image->image;
        barrier.subresourceRange = vulkan::getImageSubresourceRange(*attachment.image);
        srcPipelineStage |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dstPipelineStage |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }

    // insert barriers
    vkCmdPipelineBarrier(cmd,
        srcPipelineStage, dstPipelineStage, // stages
        0,
        0, nullptr, // memory barriers
        0, nullptr, // buffer memory barriers
        imageMemoryBarriers.size(), imageMemoryBarriers.data() // image memory barriers
    );

    uint32_t width = renderInfo.renderAreaExtent.x;
    uint32_t height = renderInfo.renderAreaExtent.y;

    // begin rendering
    VkRenderingInfo vulkanRenderingInfo = {VK_STRUCTURE_TYPE_RENDERING_INFO};
    vulkanRenderingInfo.renderArea.offset = {};
    vulkanRenderingInfo.renderArea.extent = {width, height};
    vulkanRenderingInfo.pDepthAttachment = &depthInfo;
    vulkanRenderingInfo.pColorAttachments = colorAttachments.data();
    vulkanRenderingInfo.colorAttachmentCount = colorAttachments.size();
    vulkanRenderingInfo.layerCount = 1;

    vkCmdBeginRendering(cmd, &vulkanRenderingInfo);

    setViewport(0.0f, 0.0f, width, height);
    setScissor(0.0f, 0.0f, width, height);

    renderInfos.push(renderInfo);
}

void CommandEncoder::endRendering()
{
    vkCmdEndRendering(cmd);

    auto &colorAttachments = renderInfos.front().colorAttachments;
    for (auto &attachment : colorAttachments) {
        if (!attachment.image->isSwapchain) { // find swapchain image
            continue;
        }

        VkImageMemoryBarrier swapchainBarrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        swapchainBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        swapchainBarrier.dstAccessMask = 0;
        swapchainBarrier.oldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        swapchainBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        swapchainBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        swapchainBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        swapchainBarrier.image = attachment.image->image;
        swapchainBarrier.subresourceRange.levelCount = 1;
        swapchainBarrier.subresourceRange.baseMipLevel = 0;
        swapchainBarrier.subresourceRange.baseArrayLayer = 0;
        swapchainBarrier.subresourceRange.layerCount = 1;
        swapchainBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_NONE, // stages
            0,
            0, nullptr, // memory barriers
            0, nullptr, // buffer memory barriers
            1, &swapchainBarrier // image memory barriers
        );

        break;
    }

    renderInfos.pop();
}

void CommandEncoder::setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    VkViewport viewport = {};
    viewport.x = x;
    viewport.y = y;
    viewport.width = width;
    viewport.height = height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);
}

void CommandEncoder::setScissor(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    VkRect2D scissor = {};
    scissor.offset.x = x;
    scissor.offset.y = y;
    scissor.extent.width = width;
    scissor.extent.height = height;
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

void CommandEncoder::pushConstants(PipelineLayout &pipelineLayout, ShaderStageFlags shaderStage, void *data, uint32_t size, uint32_t offset)
{
    vkCmdPushConstants(cmd, pipelineLayout.layout, vulkan::getShaderStageFlags(shaderStage), 0, size, &data);
}