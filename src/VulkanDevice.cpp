#include "VulkanDevice.h"
#include "VulkanApplicationBase.h"

#include <iostream>

namespace VulkanBase {
    VulkanDevice::VulkanDevice(VkPhysicalDevice physDevice, VkSurfaceKHR surface) : physicalDevice(physDevice){
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        VkSampleCountFlags counts = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;
        if (counts & VK_SAMPLE_COUNT_64_BIT) msaaSamples = VK_SAMPLE_COUNT_64_BIT;
        if (counts & VK_SAMPLE_COUNT_32_BIT) msaaSamples = VK_SAMPLE_COUNT_32_BIT;
        if (counts & VK_SAMPLE_COUNT_16_BIT) msaaSamples = VK_SAMPLE_COUNT_16_BIT;
        if (counts & VK_SAMPLE_COUNT_8_BIT) msaaSamples = VK_SAMPLE_COUNT_8_BIT;
        if (counts & VK_SAMPLE_COUNT_4_BIT) msaaSamples = VK_SAMPLE_COUNT_4_BIT;
        if (counts & VK_SAMPLE_COUNT_2_BIT) msaaSamples = VK_SAMPLE_COUNT_2_BIT;
        vkGetPhysicalDeviceFeatures(physicalDevice, &features);
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
        extensionProperties.resize(extensionCount);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensionProperties.data());

        queueIndices.graphicsIdx = -1;
        queueIndices.presentIdx = -1;
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
        queueFamilyProperties.resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

        int i = 0;
        for (const auto &queueFamily: queueFamilyProperties) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                queueIndices.graphicsIdx = i;
            }
            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
            if (presentSupport == VK_TRUE) {
                queueIndices.presentIdx = i;
                break;
            }
            i++;
        }

        createLogicalDevice();
        vkGetDeviceQueue(logicalDevice, queueIndices.graphicsIdx, 0, &graphicsQueue);
        vkGetDeviceQueue(logicalDevice, queueIndices.presentIdx, 0, &presentQueue);
    }

    void VulkanDevice::createLogicalDevice() {
        float queuePriority = 1.0f;
        VkDeviceQueueCreateInfo queueCreateInfos[2] = {};
        queueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[0].queueFamilyIndex = queueIndices.graphicsIdx;
        queueCreateInfos[0].queueCount = 1;
        queueCreateInfos[0].pQueuePriorities = &queuePriority;
        queueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[1].queueFamilyIndex = queueIndices.presentIdx;
        queueCreateInfos[1].queueCount = 1;
        queueCreateInfos[1].pQueuePriorities = &queuePriority;

        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = queueCreateInfos;
        createInfo.queueCreateInfoCount = queueIndices.graphicsIdx != queueIndices.presentIdx ? 2 : 1;
        features.samplerAnisotropy = VK_TRUE;
        createInfo.pEnabledFeatures = &features;
        createInfo.enabledLayerCount = enableValidation ? static_cast<uint32_t>(validationLayers.size()) : 0;
        createInfo.ppEnabledLayerNames = enableValidation ? validationLayers.data() : nullptr;
        createInfo.enabledExtensionCount = deviceExtensions.size();
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();
        VK_CHECK_RESULT(vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice));

        commandPool = createCommandPool(queueIndices.graphicsIdx);
    }

    uint32_t VulkanDevice::getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties,
                                         VkBool32 *hasFound) const {
        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
            if ((typeBits & 1) == 1) {
                if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                    if (hasFound) {
                        *hasFound = true;
                    }
                    return i;
                }
            }
            typeBits >>= 1;
        }
        if (hasFound) {
            *hasFound = false;
            return 0;
        } else {
            throw std::runtime_error("Unable to find a matching memory type!");
        }
    }

    VulkanDevice::~VulkanDevice() {
        vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
        vkDestroyDevice(logicalDevice, nullptr);
    }

    VkCommandPool VulkanDevice::createCommandPool(uint32_t queueFamilyIdx, VkCommandPoolCreateFlags createFlags) const {
        VkCommandPoolCreateInfo createInfo{};
        VkCommandPool cmdPool;
        createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        createInfo.flags = createFlags;
        createInfo.queueFamilyIndex = queueFamilyIdx;
        VK_CHECK_RESULT(vkCreateCommandPool(logicalDevice, &createInfo, nullptr, &cmdPool));
        return cmdPool;
    }

    void VulkanDevice::createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VulkanBuffer *pBuffer,
                               VkDeviceSize size, void *data) {
        pBuffer->logicalDevice = logicalDevice;

        VkBufferCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        createInfo.size = size;
        createInfo.usage = usageFlags;

        VK_CHECK_RESULT(vkCreateBuffer(logicalDevice, &createInfo, nullptr, &pBuffer->buffer));

        VkMemoryRequirements memoryRequirements;
        VkMemoryAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        vkGetBufferMemoryRequirements(logicalDevice, pBuffer->buffer, &memoryRequirements);
        allocateInfo.allocationSize = memoryRequirements.size;
        allocateInfo.memoryTypeIndex = getMemoryType(memoryRequirements.memoryTypeBits, propertyFlags);

        VkMemoryAllocateFlagsInfoKHR flagsInfo{};
        if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
            flagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
            flagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
            allocateInfo.pNext = &flagsInfo;
        }
        VK_CHECK_RESULT(vkAllocateMemory(logicalDevice, &allocateInfo, nullptr, &pBuffer->memory));

        pBuffer->alignment = memoryRequirements.alignment;
        pBuffer->size = size;
        pBuffer->usageFlags = usageFlags;
        pBuffer->propertyFlags = propertyFlags;

        if (data != nullptr) {
            pBuffer->map();
            memcpy(pBuffer->mapped, data, size);
            if ((propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) {
                pBuffer->flush();
            }
            pBuffer->unmap();
        }
        pBuffer->setDescriptor();
        pBuffer->bind();
    }

    void VulkanDevice::copyBuffer(VulkanBuffer *src, VulkanBuffer *dest, VkQueue queue, VkBufferCopy *copyRegion) {
        if (dest->size > src->size || src->buffer) {
            throw std::runtime_error("Dest buffer has no capability to be copied!");
        }

        VkCommandBuffer copyCommand = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
        VkBufferCopy bufferCopy{};
        if (copyRegion != nullptr) {
            bufferCopy.size = src->size;
        } else {
            bufferCopy = *copyRegion;
        }
        vkCmdCopyBuffer(copyCommand, src->buffer, dest->buffer, 1, &bufferCopy);

        flushCommandBuffer(copyCommand, queue, true);
    }

    VkCommandBuffer VulkanDevice::createCommandBuffer(VkCommandBufferLevel level, bool begin) {
        VkCommandBufferAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.commandPool = commandPool;
        allocateInfo.level = level;
        allocateInfo.commandBufferCount = 1;
        VkCommandBuffer copyCommand;
        VK_CHECK_RESULT(vkAllocateCommandBuffers(logicalDevice, &allocateInfo, &copyCommand));
        if (begin) {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            VK_CHECK_RESULT(vkBeginCommandBuffer(copyCommand, &beginInfo));
        }
        return copyCommand;
    }

    void VulkanDevice::flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free) const {
        VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        // ensure that the command buffer has finished executing
        VkFence fence;
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = 0;
        VK_CHECK_RESULT(vkCreateFence(logicalDevice, &fenceInfo, nullptr, &fence));
        VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
        VK_CHECK_RESULT(vkWaitForFences(logicalDevice, 1, &fence, VK_TRUE, UINT64_MAX));
        vkDestroyFence(logicalDevice, fence, nullptr);
        if (free) {
            vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
        }
    }

    void VulkanDevice::createBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkBuffer *buffer, VkDeviceMemory *memory, VkDeviceSize size, void *data) {
        VkBufferCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        createInfo.size = size;
        createInfo.usage = usageFlags;

        VK_CHECK_RESULT(vkCreateBuffer(logicalDevice, &createInfo, nullptr, buffer));

        VkMemoryRequirements memoryRequirements;
        VkMemoryAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        vkGetBufferMemoryRequirements(logicalDevice, *buffer, &memoryRequirements);
        allocateInfo.allocationSize = memoryRequirements.size;
        allocateInfo.memoryTypeIndex = getMemoryType(memoryRequirements.memoryTypeBits, propertyFlags);

        VkMemoryAllocateFlagsInfoKHR flagsInfo{};
        if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
            flagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
            flagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
            allocateInfo.pNext = &flagsInfo;
        }
        VK_CHECK_RESULT(vkAllocateMemory(logicalDevice, &allocateInfo, nullptr, memory));

        if (data != nullptr) {
            void *mapped;
            VK_CHECK_RESULT(vkMapMemory(logicalDevice, *memory, 0, size, 0, &mapped));
            memcpy(mapped, data, size);
            // If host coherency hasn't been requested, do a manual flush to make writes visible
            if ((propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
            {
                VkMappedMemoryRange mappedRange{};
                mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
                mappedRange.memory = *memory;
                mappedRange.offset = 0;
                mappedRange.size = size;
                vkFlushMappedMemoryRanges(logicalDevice, 1, &mappedRange);
            }
            vkUnmapMemory(logicalDevice, *memory);
        }
        VK_CHECK_RESULT(vkBindBufferMemory(logicalDevice, *buffer, *memory, 0));
    }
}
