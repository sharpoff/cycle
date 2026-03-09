#include "graphics/vulkan_helpers.h"

namespace vulkan
{
    VkRenderingAttachmentInfo createAttachmentInfo(VkImageView imageView, VkImageLayout layout, bool load, bool store, VkImageView resolveImageView, VkImageLayout resolveLayout)
    {
        VkRenderingAttachmentInfo attachmentInfo = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        attachmentInfo.clearValue.color = {0.0f, 0.0f, 0.0f, 1.0f};
        attachmentInfo.clearValue.depthStencil = {0.0f, 0};
        attachmentInfo.imageView = imageView;
        attachmentInfo.imageLayout = layout;
        if (resolveImageView != VK_NULL_HANDLE) {
            attachmentInfo.resolveImageView = resolveImageView;
            attachmentInfo.resolveImageLayout = resolveLayout;
            attachmentInfo.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
        }
        attachmentInfo.loadOp = load ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachmentInfo.storeOp = store ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
        return attachmentInfo;
    }

    VkRenderingInfo createRenderingInfo(const VkExtent2D &renderArea, const Vector<VkRenderingAttachmentInfo> &colorAttachments, VkRenderingAttachmentInfo *depthAttachment)
    {
        VkRenderingInfo renderingInfo = {VK_STRUCTURE_TYPE_RENDERING_INFO};
        renderingInfo.renderArea.extent = renderArea;
        renderingInfo.colorAttachmentCount = colorAttachments.size();
        renderingInfo.pColorAttachments = colorAttachments.data();
        renderingInfo.pDepthAttachment = depthAttachment;
        renderingInfo.layerCount = 1;
        return renderingInfo;
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

    void setDebugName(VkDevice device, uint64_t objectHandle, VkObjectType objectType, const char *name)
    {
#ifndef NDEBUG
        if (objectHandle == 0 || !name) return;

        VkDebugUtilsObjectNameInfoEXT objectNameInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
        objectNameInfo.objectHandle = objectHandle;
        objectNameInfo.objectType = objectType;
        objectNameInfo.pObjectName = name;

        vkSetDebugUtilsObjectNameEXT(device, &objectNameInfo);
#endif
    }

    void beginDebugLabel(VkCommandBuffer cmd, const char *name)
    {
#ifndef NDEBUG
        VkDebugUtilsLabelEXT label = {VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
        label.pLabelName = name;
        label.color[0] = 0.0f;
        label.color[1] = 0.0f;
        label.color[2] = 0.0f;
        label.color[3] = 1.0f;

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