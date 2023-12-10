#ifndef RICHELIEU_VULKANTOOLS_H
#define RICHELIEU_VULKANTOOLS_H

#include <string>
#include <vulkan/vulkan.h>
namespace VulkanBase {
    namespace Tools {
        std::string getAssetPath();
        std::string getShaderPath();
        VkShaderModule loadShader(const std::string &filePath, VkDevice logicalDevice);
        std::string physicalDeviceTypeString(VkPhysicalDeviceType type);
        VkFormat findSupportDepthFormat(VkPhysicalDevice physicalDevice);
        void setImageLayout(VkCommandBuffer commandBuffer,
                            VkImage image, VkImageLayout oldLayout,
                            VkImageLayout newLayout,
                            VkImageSubresourceRange subresourceRange,
                            VkPipelineStageFlags srcStageFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                            VkPipelineStageFlags destStageFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
    }
}
#endif
