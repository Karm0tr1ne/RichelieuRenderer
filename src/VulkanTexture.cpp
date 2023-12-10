#include "VulkanTexture.h"
#include "VulkanTools.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <string>

namespace VulkanBase {
    VkDescriptorImageInfo Texture::getImageInfo() {
        VkDescriptorImageInfo imageInfo;
        imageInfo.sampler = sampler;
        imageInfo.imageLayout = imageLayout;
        imageInfo.imageView = imageView;
        return imageInfo;
    }

    void Texture::cleanUp() {
        vkDestroyImageView(pDevice->logicalDevice, imageView, nullptr);
        vkDestroyImage(pDevice->logicalDevice, image, nullptr);
        if (sampler != nullptr) {
            vkDestroySampler(pDevice->logicalDevice, sampler, nullptr);
        }
        vkFreeMemory(pDevice->logicalDevice, deviceMemory, nullptr);
    }

    void Texture2D::loadFromFile(const std::string& filePath, VkFormat format, VulkanBase::VulkanDevice *device,
                                 VkImageUsageFlags imageUsageFlags, VkImageLayout imageLayout) {
        this->pDevice = device;

        std::cout << "Loading New Image: " << filePath << std::endl;
        int texWidth, texHeight, channels;
        const char* path = filePath.c_str();
        stbi_uc* pixels = stbi_load(path, &texWidth, &texHeight, &channels, STBI_rgb_alpha);
        if (!pixels) {
            throw std::runtime_error("failed to load image: " + filePath);
        }
        width = static_cast<uint32_t>(texWidth);
        height = static_cast<uint32_t>(texHeight);
        VkDeviceSize imageSize = width * height * 4;

        VkCommandBuffer copyCommand = pDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

        VkBuffer stagingBuffer;
        VkMemoryRequirements memoryRequirements;
        VkMemoryAllocateInfo allocateInfo{};
        VkDeviceMemory stagingMemory;

        VkBufferCreateInfo bufferCreateInfo{};
        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.size = imageSize;
        bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (vkCreateBuffer(pDevice->logicalDevice, &bufferCreateInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create staging buffer!");
        }
        vkGetBufferMemoryRequirements(pDevice->logicalDevice, stagingBuffer, &memoryRequirements);

        allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocateInfo.allocationSize = memoryRequirements.size;
        allocateInfo.memoryTypeIndex = pDevice->getMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (vkAllocateMemory(pDevice->logicalDevice, &allocateInfo, nullptr, &stagingMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate memory for staging buffer");
        }
        if (vkBindBufferMemory(pDevice->logicalDevice, stagingBuffer, stagingMemory, 0) != VK_SUCCESS) {
            throw std::runtime_error("failed to bind staging buffer with memory!");
        }

        void *data;
        vkMapMemory(pDevice->logicalDevice, stagingMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<uint32_t>(imageSize));
        vkUnmapMemory(pDevice->logicalDevice, stagingMemory);

        stbi_image_free(pixels);

        VkImageCreateInfo imageCreateInfo{};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format = format;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.mipLevels = 1;
        imageCreateInfo.usage = imageUsageFlags;
        if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT))
        {
            imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateInfo.extent = {width, height, 1};
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        if (vkCreateImage(pDevice->logicalDevice, &imageCreateInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }
        vkGetImageMemoryRequirements(pDevice->logicalDevice, image, &memoryRequirements);
        allocateInfo.allocationSize = memoryRequirements.size;
        allocateInfo.memoryTypeIndex = pDevice->getMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (vkAllocateMemory(pDevice->logicalDevice, &allocateInfo, nullptr, &deviceMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate memory!");
        }
        if (vkBindImageMemory(pDevice->logicalDevice, image, deviceMemory, 0) != VK_SUCCESS) {
            throw std::runtime_error("failed to bind image and memory!");
        }

        VkImageSubresourceRange subresourceRange{};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.layerCount = 1;

        VulkanBase::Tools::setImageLayout(copyCommand, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
        region.imageOffset = {0, 0, 0};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageSubresource.mipLevel = 0;

        vkCmdCopyBufferToImage(copyCommand, stagingBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,&region);
        this->imageLayout = imageLayout;
        VulkanBase::Tools::setImageLayout(copyCommand, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imageLayout, subresourceRange);

        pDevice->flushCommandBuffer(copyCommand, pDevice->graphicsQueue, true);

        vkDestroyBuffer(pDevice->logicalDevice, stagingBuffer, nullptr);
        vkFreeMemory(pDevice->logicalDevice, stagingMemory, nullptr);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;
        samplerInfo.anisotropyEnable = pDevice->features.samplerAnisotropy;
        samplerInfo.maxAnisotropy = pDevice->features.samplerAnisotropy ? pDevice->properties.limits.maxSamplerAnisotropy : 1.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        if (vkCreateSampler(pDevice->logicalDevice, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create sampler!");
        }

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        viewInfo.image = image;
        if (vkCreateImageView(pDevice->logicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image view!");
        }
    }
}