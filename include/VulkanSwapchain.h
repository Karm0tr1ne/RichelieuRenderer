#ifndef RICHELIEU_VULKANSWAPCHAIN_H
#define RICHELIEU_VULKANSWAPCHAIN_H

#include "VulkanDevice.h"
#include "VulkanTools.h"


#include <string>
#include <vector>

namespace VulkanBase {
    struct SwapchainBuffer {
        VkImage image;
        VkImageView imageView;
    };

    class VulkanSwapchain {
    private:
        VkInstance instance;
        VkDevice logicalDevice;
        VkPhysicalDevice physicalDevice;
        VkSurfaceKHR surface;

    public:
        VkFormat colorFormat;
        VkColorSpaceKHR colorSpace;
        VkSwapchainKHR swapchain = VK_NULL_HANDLE;
        uint32_t imageCount;
        std::vector<SwapchainBuffer> swapchainBuffers;
        QueueIndices queueIndices;

        void initSurface();
        void createSwapchain(uint32_t *width, uint32_t *height);
        VkResult acquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t *imageIndex);
        VkResult queuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore);
        VulkanSwapchain(QueueIndices _queueIndices, VkPhysicalDevice _physicalDevice, VkDevice _device, VkSurfaceKHR _surface);
        ~VulkanSwapchain();
    };
}
#endif
