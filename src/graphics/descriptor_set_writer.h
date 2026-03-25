#pragma once

#include "core/containers.h"

#include <volk.h>

class DescriptorSetWriter
{
public:
    void Write(uint32_t binding, VkBuffer &buffer, uint32_t size, VkDescriptorType type, uint32_t dstArrayElement); // STORAGE/UNIFORM
    void Write(uint32_t binding, VkImageView &imageView, VkSampler &sampler, VkImageLayout layout, VkDescriptorType type, uint32_t dstArrayElement); // COMBINED_IMAGE_SAMPLER
    void Write(uint32_t binding, VkImageView &imageView, VkImageLayout layout, VkDescriptorType type, uint32_t dstArrayElement); // SAMPLED_IMAGE
    void Write(uint32_t binding, VkSampler &sampler, VkDescriptorType type, uint32_t dstArrayElement); // SAMPLER

    void Update(VkDevice device, VkDescriptorSet set);
    void Clear();

private:
    Deque<VkDescriptorImageInfo>  imageInfos;
    Deque<VkDescriptorBufferInfo> bufferInfos;

    Vector<VkWriteDescriptorSet> writes;
};