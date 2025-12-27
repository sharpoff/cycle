#pragma once

#include "cycle/graphics/graphics_types.h"

struct CommandEncoder
{
    void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
    void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance);
    void bindPipeline(RenderPipeline &pipeline);
    void bindPipeline(ComputePipeline &pipeline);
    void bindVertexBuffer(Buffer &vertexBuffer);
    void bindIndexBuffer(Buffer &indexBuffer);
    void beginRendering(const RenderingInfo &renderInfo);
    void endRendering();
    void setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
    void setScissor(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
    void pushConstants(PipelineLayout &pipelineLayout, ShaderStageFlags shaderStage, void *data, uint32_t size, uint32_t offset = 0);

    void beginImGuiFrame();
    void endImGuiFrame();

    VkCommandBuffer cmd;
    Queue<RenderingInfo> renderInfos;
};