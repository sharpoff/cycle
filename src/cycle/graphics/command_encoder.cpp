#include "cycle/graphics/command_encoder.h"

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
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout->layout, 0, 1, &pipeline.layout->descriptorSet, 0, nullptr);
}

void CommandEncoder::bindPipeline(ComputePipeline &pipeline)
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.layout->layout, 0, 1, &pipeline.layout->descriptorSet, 0, nullptr);
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

void CommandEncoder::beginRendering(const VkRenderingInfo &renderInfo)
{
    vkCmdBeginRendering(cmd, &renderInfo);

    setViewport(0.0f, 0.0f, renderInfo.renderArea.extent.width, renderInfo.renderArea.extent.height);
    setScissor(0.0f, 0.0f, renderInfo.renderArea.extent.width, renderInfo.renderArea.extent.height);
}

void CommandEncoder::endRendering()
{
    vkCmdEndRendering(cmd);
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

void CommandEncoder::pushConstants(PipelineLayout &pipelineLayout, VkShaderStageFlags shaderStage, void *data, uint32_t size, uint32_t offset)
{
    vkCmdPushConstants(cmd, pipelineLayout.layout, shaderStage, offset, size, data);
}