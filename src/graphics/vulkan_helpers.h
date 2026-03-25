#pragma once

#include "core/containers.h"
#include <assert.h>

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
    VkRenderingAttachmentInfo CreateAttachmentInfo(VkImageView imageView, VkImageLayout layout, bool load, bool store, VkImageView resolveImageView = VK_NULL_HANDLE, VkImageLayout resolveLayout = VK_IMAGE_LAYOUT_UNDEFINED);
    VkRenderingInfo CreateRenderingInfo(const VkExtent2D &renderArea, const Vector<VkRenderingAttachmentInfo> &colorAttachments, VkRenderingAttachmentInfo *depthAttachment = nullptr);

    const char *ToString(VkPresentModeKHR presentMode);
    const char *ToString(VkFormat format);

    void SetDebugName(VkDevice device, uint64_t objectHandle, VkObjectType objectType, const char *name);
    void BeginDebugLabel(VkCommandBuffer cmd, const char *name);
    void EndDebugLabel(VkCommandBuffer cmd);
} // namespace vulkan