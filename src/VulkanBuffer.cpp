#include "VulkanBuffer.h"
#include <iostream>
#include <cassert>
#include <stdexcept>

void VulkanBase::VulkanBuffer::map(VkDeviceSize deviceSize, VkDeviceSize offset) {
    VK_CHECK_RESULT(vkMapMemory(logicalDevice, memory, offset, deviceSize, 0, &mapped));
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
    VK_CHECK_RESULT(vkFlushMappedMemoryRanges(logicalDevice, 1, &mappedRange));
}

void VulkanBase::VulkanBuffer::setDescriptor(VkDeviceSize deviceSize, VkDeviceSize offset) {
    bufferInfo.offset = offset;
    bufferInfo.range = deviceSize;
    bufferInfo.buffer = buffer;
}

void VulkanBase::VulkanBuffer::bind(VkDeviceSize offset) const {
    VK_CHECK_RESULT(vkBindBufferMemory(logicalDevice, buffer, memory, offset));
}

void VulkanBase::VulkanBuffer::cleanUp() {
    vkDestroyBuffer(logicalDevice, buffer, nullptr);
    vkFreeMemory(logicalDevice, memory, nullptr);
}

