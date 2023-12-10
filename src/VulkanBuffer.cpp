#include "VulkanBuffer.h"

#include <stdexcept>

void VulkanBase::VulkanBuffer::map(VkDeviceSize deviceSize, VkDeviceSize offset) {
    if (vkMapMemory(logicalDevice, memory, offset, deviceSize, 0, &mapped) != VK_SUCCESS) {
        throw std::runtime_error("failed to map memory!");
    }
}

void VulkanBase::VulkanBuffer::unmap() {
    if (mapped) {
        vkUnmapMemory(logicalDevice, memory);
        mapped = nullptr;
    }
}

void VulkanBase::VulkanBuffer::flush(VkDeviceSize deviceSize, VkDeviceSize offset) const {
    VkMappedMemoryRange mappedRange{};
    mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory = memory;
    mappedRange.offset = offset;
    mappedRange.size = deviceSize;
    if (vkFlushMappedMemoryRanges(logicalDevice, 1, &mappedRange) != VK_SUCCESS) {
        throw std::runtime_error("failed to flush mapped memory ranges!");
    }
}

void VulkanBase::VulkanBuffer::setDescriptor(VkDeviceSize deviceSize, VkDeviceSize offset) {
    bufferInfo.offset = offset;
    bufferInfo.range = deviceSize;
    bufferInfo.buffer = buffer;
}

void VulkanBase::VulkanBuffer::bind(VkDeviceSize offset) const {
    if (vkBindBufferMemory(logicalDevice, buffer, memory, offset) != VK_SUCCESS) {
        throw std::runtime_error("failed to bind buffer and memory!");
    }
}

void VulkanBase::VulkanBuffer::cleanUp() {
    vkDestroyBuffer(logicalDevice, buffer, nullptr);
    vkFreeMemory(logicalDevice, memory, nullptr);
}

