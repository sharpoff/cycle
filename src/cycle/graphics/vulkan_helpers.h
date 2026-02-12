#pragma once

#include "cycle/containers.h"
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
    VkRenderingAttachmentInfo createAttachmentInfo(VkImageView imageView, VkImageLayout layout, bool load, bool store, VkImageView resolveImageView = VK_NULL_HANDLE, VkImageLayout resolveLayout = VK_IMAGE_LAYOUT_UNDEFINED);
    VkRenderingInfo createRenderingInfo(const VkExtent2D &renderArea, const Vector<VkRenderingAttachmentInfo> &colorAttachments, VkRenderingAttachmentInfo *depthAttachment = nullptr);

    const char *toString(VkPresentModeKHR presentMode);
    const char *toString(VkFormat format);

    void setDebugName(VkDevice device, uint64_t objectHandle, VkObjectType objectType, const char *name);
    void beginDebugLabel(VkCommandBuffer cmd, const char *name);
    void endDebugLabel(VkCommandBuffer cmd);
} // namespace vulkan