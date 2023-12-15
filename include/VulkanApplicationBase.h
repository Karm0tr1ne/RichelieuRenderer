#ifndef RICHELIEU_VULKANAPPLICATIONBASE_H
#define RICHELIEU_VULKANAPPLICATIONBASE_H

#include <vector>
#include <cstring>
#include <optional>

#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "VulkanTools.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "GUI.h"
#include "Camera.hpp"

#ifdef NDEBUG
const bool enableValidation = false;
#else
const bool enableValidation = true;
#endif

const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions={
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

class VulkanApplicationBase {
public:
    uint32_t width = 1280;
    uint32_t height = 720;
    bool framebufferResized = false;
    std::vector<const char*> requiredExtensions;
    GUI gui;
    VkClearColorValue defaultClearColor = {{0.025f, 0.025f, 0.025f, 1.0f}};
    Camera camera;
    std::string title = "Richelieu Renderer";
    std::string name = "RichelieuRenderer";
    uint32_t apiVersion = VK_API_VERSION_1_0;
    struct {
        VkImage image;
        VkDeviceMemory memory;
        VkImageView imageView;
    } depthStencil;

    struct {
        VkImage image;
        VkDeviceMemory memory;
        VkImageView imageView;
    } colorResources;

    VulkanApplicationBase();
    virtual ~VulkanApplicationBase();
    void setupWindow();
    void initVulkan();
    VkPipelineShaderStageCreateInfo createShader(const std::string& filePath, VkShaderStageFlagBits stages);
    virtual void prepare();
    void renderLoop();

    static VkResult createDebugUtilsMessengerExt(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
    static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

    void createInstance();
protected:
    GLFWwindow* window;
    VkInstance instance;
    VulkanBase::VulkanDevice *vulkanDevice;
    VkSurfaceKHR surface;
    VulkanBase::VulkanSwapchain *swapchain;
    std::vector<VkFramebuffer> frameBuffers;
    std::vector<VkCommandBuffer> drawCommandBuffers;
    std::vector<VkShaderModule> shaderModules;
    VkCommandPool cmdPool;
    VkRenderPass renderPass;
    VkPipelineCache pipelineCache;
    uint32_t currentBuffer = 0;
    VkSubmitInfo submitInfo;
    VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    struct {
        VkSemaphore presentCompleteSemaphore;
        VkSemaphore renderCompleteSemaphore;
    } semaphores;
    std::vector<VkFence> inFlightFences;
    VkFormat depthFormat;

    virtual int getDeviceScore(VkPhysicalDevice physicalDevice);
    virtual void buildCommandBuffers() = 0;
    void prepareFrame();
    void submitFrame();
    virtual void drawFrame();

private:
    VkDebugUtilsMessengerEXT debugMessenger;

    std::vector<const char *> getRequiredExtensions();
    VkPhysicalDevice pickPhysicalDevice();
    void setupDepthStencil();
    void createFrameBuffer();
    void createRenderPass();
    void windowResize();
    void createCommandPool();
    void createSyncPrimitives();
    void createCommandBuffers();
    void setupColorResources();
    void createPipelineCache();
};
#endif
