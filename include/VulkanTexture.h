#ifndef RICHELIEU_VULKANTEXTURE_H
#define RICHELIEU_VULKANTEXTURE_H

#include <string>
#include <vector>
#include <stdlib.h>
#include <array>

#include "vulkan/vulkan.h"

#include "VulkanBuffer.h"
#include "VulkanDevice.h"

namespace VulkanBase {
    class Texture {
    public:
        VulkanDevice *pDevice;
        VkImage image;
        VkImageLayout imageLayout;
        VkDescriptorImageInfo imageInfo;
        VkDeviceMemory deviceMemory;
        VkImageView imageView;
        uint32_t width;
        uint32_t height;
        uint32_t mipLevels;
        uint32_t layerCount;
        VkSampler sampler;

        void cleanUp();
    };

    class Texture2D : public Texture {
    public:
        void loadFromFile(const std::string& filePath, VkFormat format, VulkanDevice *device, VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        void loadFromBuffer(void* buffer, VkDeviceSize bufferSize, VkFormat format, uint32_t texWidth, uint32_t texHeight, VulkanDevice *device, VkFilter filter = VK_FILTER_LINEAR, VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    };

    class Texture2DArray : public Texture {
    public:
        void loadFromFile(std::string filename, VkFormat format, VulkanDevice *device, VkQueue copyQueue, VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    };

    class TextureCubeMap : public Texture {
    public:
        void loadFromFiles(const std::array<std::string, 6> &filePaths, VkFormat format, VulkanDevice *device, VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT, VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    };
}
#endif
