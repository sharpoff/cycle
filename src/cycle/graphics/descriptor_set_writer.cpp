#include "cycle//graphics/descriptor_set_writer.h"

void DescriptorSetWriter::write(uint32_t binding, VkBuffer &buffer, uint32_t size, VkDescriptorType type, uint32_t dstArrayElement)
{
    if (size <= 0)
        return;

    VkDescriptorBufferInfo &bufferInfo = bufferInfos.emplace_back(VkDescriptorBufferInfo{.buffer = buffer, .offset = 0, .range = size});

    VkWriteDescriptorSet write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstBinding = binding;
    write.dstSet = VK_NULL_HANDLE; // will be added on update
    write.descriptorCount = 1;
    write.dstArrayElement = dstArrayElement;
    write.descriptorType = type;
    write.pBufferInfo = &bufferInfo;

    writes.push_back(write);
}

void DescriptorSetWriter::write(uint32_t binding, VkImageView &imageView, VkSampler &sampler, VkImageLayout layout, VkDescriptorType type, uint32_t dstArrayElement)
{
    VkDescriptorImageInfo &imageInfo = imageInfos.emplace_back(VkDescriptorImageInfo{.sampler = sampler, .imageView = imageView, .imageLayout = layout});

    VkWriteDescriptorSet write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstBinding = binding;
    write.dstSet = VK_NULL_HANDLE; // will be added on update
    write.dstArrayElement = dstArrayElement;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pImageInfo = &imageInfo;

    writes.push_back(write);
}

void DescriptorSetWriter::write(uint32_t binding, VkImageView &imageView, VkImageLayout layout, VkDescriptorType type, uint32_t dstArrayElement)
{
    VkDescriptorImageInfo &imageInfo = imageInfos.emplace_back(VkDescriptorImageInfo{.imageView = imageView, .imageLayout = layout});

    VkWriteDescriptorSet write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstBinding = binding;
    write.dstSet = VK_NULL_HANDLE; // will be added on update
    write.dstArrayElement = dstArrayElement;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pImageInfo = &imageInfo;

    writes.push_back(write);
}

void DescriptorSetWriter::write(uint32_t binding, VkSampler &sampler, VkDescriptorType type, uint32_t dstArrayElement)
{
    VkDescriptorImageInfo &imageInfo = imageInfos.emplace_back(VkDescriptorImageInfo{.sampler = sampler});

    VkWriteDescriptorSet write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstBinding = binding;
    write.dstSet = VK_NULL_HANDLE; // will be added on update
    write.dstArrayElement = dstArrayElement;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pImageInfo = &imageInfo;

    writes.push_back(write);
}

void DescriptorSetWriter::update(VkDevice device, VkDescriptorSet set)
{
    for (VkWriteDescriptorSet &write : writes) {
        write.dstSet = set;
    }

    vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
}

void DescriptorSetWriter::clear()
{
    imageInfos.clear();
    bufferInfos.clear();
    writes.clear();
}