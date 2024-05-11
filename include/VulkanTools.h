#ifndef RICHELIEU_VULKANTOOLS_H
#define RICHELIEU_VULKANTOOLS_H

#include <string>
#include <vulkan/vulkan.h>

#define VK_CHECK_RESULT(f)																				\
{																										\
	VkResult res = (f);																					\
	if (res != VK_SUCCESS)																				\
	{																									\
		std::cout << "Fatal : VkResult is \"" << VulkanBase::Tools::errorString(res) << "\" in " << __FILE__ << " at line " << __LINE__ << "\n"; \
		assert(res == VK_SUCCESS);																		\
	}																									\
}

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
        std::string errorString(VkResult res);
        uint32_t alignedSize(uint32_t val, uint32_t aligned);
    }
}
#endif
