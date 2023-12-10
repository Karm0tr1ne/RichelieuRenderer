#ifndef RICHELIEU_VULKANBUFFER_H
#define RICHELIEU_VULKANBUFFER_H

#include <vector>

#include <vulkan/vulkan.h>
#include "VulkanTools.h"

namespace VulkanBase {
    class VulkanBuffer {
    public:
        VkDevice logicalDevice;
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkDescriptorBufferInfo bufferInfo{};
        VkDeviceSize size = 0;
        VkDeviceSize alignment = 0;
        void* mapped = nullptr;

        VkBufferUsageFlags usageFlags;
        VkMemoryPropertyFlags propertyFlags;

        void map(VkDeviceSize deviceSize = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        void unmap();
        void flush(VkDeviceSize deviceSize = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;
        void setDescriptor(VkDeviceSize deviceSize = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
        void bind(VkDeviceSize offset = 0) const;
        void cleanUp();
    };
}

#endif
