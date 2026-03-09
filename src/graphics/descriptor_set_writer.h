#pragma once

#include "core/containers.h"

#include <volk.h>

class DescriptorSetWriter
{
public:
    void write(uint32_t binding, VkBuffer &buffer, uint32_t size, VkDescriptorType type, uint32_t dstArrayElement); // STORAGE/UNIFORM
    void write(uint32_t binding, VkImageView &imageView, VkSampler &sampler, VkImageLayout layout, VkDescriptorType type, uint32_t dstArrayElement); // COMBINED_IMAGE_SAMPLER
    void write(uint32_t binding, VkImageView &imageView, VkImageLayout layout, VkDescriptorType type, uint32_t dstArrayElement); // SAMPLED_IMAGE
    void write(uint32_t binding, VkSampler &sampler, VkDescriptorType type, uint32_t dstArrayElement); // SAMPLER

    void update(VkDevice device, VkDescriptorSet set);
    void clear();

private:
    Deque<VkDescriptorImageInfo>  imageInfos;
    Deque<VkDescriptorBufferInfo> bufferInfos;

    Vector<VkWriteDescriptorSet> writes;
};