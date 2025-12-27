#pragma once

#include "cycle/containers.h"

#include <volk.h>

class DescriptorSetWriter
{
public:
    void write(uint32_t binding, VkBuffer &buffer, uint32_t size, VkDescriptorType type, uint32_t dstArrayElement);
    void write(uint32_t binding, VkImageView &imageView, VkSampler &sampler, VkImageLayout layout, VkDescriptorType type, uint32_t dstArrayElement);

    void update(VkDevice device, VkDescriptorSet set);
    void clear();

private:
    Deque<VkDescriptorImageInfo>  imageInfos;
    Deque<VkDescriptorBufferInfo> bufferInfos;

    Vector<VkWriteDescriptorSet> writes;
};