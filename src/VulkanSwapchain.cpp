#include <stdexcept>
#include <iostream>
#include <cassert>
#include "VulkanSwapchain.h"
#include "VulkanTools.h"

namespace VulkanBase {
    VulkanSwapchain::VulkanSwapchain(QueueIndices _queueIndices, VkPhysicalDevice _physicalDevice, VkDevice _device, VkSurfaceKHR _surface) : queueIndices(_queueIndices), physicalDevice(_physicalDevice), logicalDevice(_device), surface(_surface) {
        initSurface();

    }

    void VulkanSwapchain::initSurface() {
        uint32_t formatsCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatsCount, nullptr);
        if (formatsCount == 0) {
            throw std::runtime_error("unable to find any surface format!");
        }
        std::vector<VkSurfaceFormatKHR> surfaceFormats(formatsCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatsCount, surfaceFormats.data());
        VkSurfaceFormatKHR selectedFormat = surfaceFormats[0];
        for (const auto &surfaceFormat: surfaceFormats) {
            if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                selectedFormat = surfaceFormat;
                break;
            }
        }
        colorFormat = selectedFormat.format;
        colorSpace = selectedFormat.colorSpace;
        }

    void VulkanSwapchain::createSwapchain(uint32_t *width, uint32_t *height) {
        VkSwapchainKHR oldSwapchain = swapchain;
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
        if (presentModeCount == 0) {
            throw std::runtime_error("unable to find any present mode!");
        }
        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());
        VkPresentModeKHR selectedPresentMode = VK_PRESENT_MODE_FIFO_KHR;
        for (const auto &presentMode: presentModes) {
            if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                selectedPresentMode = presentMode;
                break;
            }
            if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                selectedPresentMode = presentMode;
            }
        }

        VkExtent2D swapchainExtent{};
        if (surfaceCapabilities.currentExtent.width == (uint32_t)-1)
        {
            swapchainExtent.width = *width;
            swapchainExtent.height = *height;
        }
        else
        {
            swapchainExtent = surfaceCapabilities.currentExtent;
            *width = surfaceCapabilities.currentExtent.width;
            *height = surfaceCapabilities.currentExtent.height;
        }

        uint32_t swapchainImages = surfaceCapabilities.minImageCount + 1;
        // which means minImageCount + 1 is bigger than maxImageCount
        if (surfaceCapabilities.maxImageCount > 0 && swapchainImages > surfaceCapabilities.maxImageCount) {
            swapchainImages = surfaceCapabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = swapchainImages;
        createInfo.imageFormat = colorFormat;
        createInfo.imageColorSpace = colorSpace;
        createInfo.imageExtent = {swapchainExtent.width, swapchainExtent.height};
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if (queueIndices.graphicsIdx != queueIndices.presentIdx) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            uint32_t queueFamilyIndices[] = {
                    queueIndices.graphicsIdx, queueIndices.presentIdx
            };
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }
        createInfo.preTransform = surfaceCapabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = selectedPresentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = oldSwapchain;
        VK_CHECK_RESULT(vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapchain));

        if (oldSwapchain != VK_NULL_HANDLE) {
            for (auto &swapchainBuffer: swapchainBuffers) {
                vkDestroyImageView(logicalDevice, swapchainBuffer.imageView, nullptr);
            }
            vkDestroySwapchainKHR(logicalDevice, oldSwapchain, nullptr);
        }

        vkGetSwapchainImagesKHR(logicalDevice, swapchain, &imageCount, nullptr);
        std::vector<VkImage> images(imageCount);
        vkGetSwapchainImagesKHR(logicalDevice, swapchain, &imageCount, images.data());

        swapchainBuffers.resize(imageCount);
        for (uint32_t i = 0; i < imageCount; i++) {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.format = colorFormat;
            viewInfo.pNext = nullptr;
            viewInfo.components = {
                    VK_COMPONENT_SWIZZLE_R,
                    VK_COMPONENT_SWIZZLE_G,
                    VK_COMPONENT_SWIZZLE_B,
                    VK_COMPONENT_SWIZZLE_A,
                    };
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.layerCount = 1;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.flags = 0;
            swapchainBuffers[i].image = images[i];
            viewInfo.image = swapchainBuffers[i].image;

            VK_CHECK_RESULT(vkCreateImageView(logicalDevice, &viewInfo, nullptr, &swapchainBuffers[i].imageView));
        }
    }

    VkResult VulkanSwapchain::acquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t *imageIndex) {
        return vkAcquireNextImageKHR(logicalDevice, swapchain,UINT64_MAX, presentCompleteSemaphore, VK_NULL_HANDLE, imageIndex);
    }

    VkResult VulkanSwapchain::queuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore) {
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.pNext = nullptr;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.pImageIndices = &imageIndex;
        if (waitSemaphore != VK_NULL_HANDLE) {
            presentInfo.pWaitSemaphores = &waitSemaphore;
            presentInfo.waitSemaphoreCount = 1;
        }
        return vkQueuePresentKHR(queue, &presentInfo);
    }

    VulkanSwapchain::~VulkanSwapchain() {
        for (auto& buffer : swapchainBuffers) {
            vkDestroyImageView(logicalDevice, buffer.imageView, nullptr);
        }
        vkDestroySwapchainKHR(logicalDevice, swapchain, nullptr);
    }
}