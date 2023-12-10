#ifndef RICHELIEU_VULKANDEVICE_H
#define RICHELIEU_VULKANDEVICE_H

#include "vulkan/vulkan.h"
#include "VulkanBuffer.h"

#include <vector>

namespace VulkanBase {
    struct QueueIndices {
        uint32_t graphicsIdx;
        uint32_t presentIdx;
    };
    class VulkanDevice {
    public:
        VkPhysicalDevice physicalDevice;
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceFeatures features;
        VkPhysicalDeviceMemoryProperties memoryProperties;
        std::vector<VkExtensionProperties> extensionProperties;
        std::vector<VkQueueFamilyProperties> queueFamilyProperties;
        VkDevice logicalDevice;
        VkCommandPool commandPool;
        VkQueue graphicsQueue;
        VkQueue presentQueue;
        QueueIndices queueIndices;
        VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

        VulkanDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
        void createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkBuffer *buffer, VkDeviceMemory *memory, VkDeviceSize size, void *data = nullptr);
        void createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VulkanBuffer *pBuffer, VkDeviceSize size, void *data = nullptr);
        void copyBuffer(VulkanBuffer *src, VulkanBuffer *dest, VkQueue queue, VkBufferCopy *copyRegion);
        uint32_t getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32 *hasFound = nullptr) const;
        VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level, bool begin);
        void flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free) const;
        ~VulkanDevice();

    private:
        void createLogicalDevice();
        VkCommandPool createCommandPool(uint32_t queueFamilyIdx, VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT) const;
    };
}
#endif
