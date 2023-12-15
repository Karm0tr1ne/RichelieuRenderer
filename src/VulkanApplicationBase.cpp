#include "VulkanApplicationBase.h"
#include "VulkanTools.h"
#include <iostream>
#include <stdexcept>
#include <map>
#include <array>

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugMessage(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                         VkDebugUtilsMessageTypeFlagsEXT type,
                                                         const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                         void *pUserData) {
    std::cout << "Validation Layer Message: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

VulkanApplicationBase::VulkanApplicationBase() {

}

static void framebufferResizedCallback(GLFWwindow *window, int width, int height) {
    auto app = reinterpret_cast<VulkanApplicationBase *>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

void VulkanApplicationBase::initVulkan() {
    createInstance();
    VkPhysicalDevice physicalDevice = pickPhysicalDevice();
    vulkanDevice = new VulkanBase::VulkanDevice(physicalDevice, surface);
    swapchain = new VulkanBase::VulkanSwapchain(vulkanDevice->queueIndices, vulkanDevice->physicalDevice, vulkanDevice->logicalDevice, surface);
}

void VulkanApplicationBase::setupWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    const char *windowTitle = title.c_str();
    window = glfwCreateWindow(width, height, windowTitle, nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizedCallback);
}

void VulkanApplicationBase::prepare() {
    createCommandPool();
    swapchain->createSwapchain(&width, &height);
    createCommandBuffers();
    createSyncPrimitives();
    setupDepthStencil();
    setupColorResources();
    createRenderPass();
    createPipelineCache();
    createFrameBuffer();
}

void VulkanApplicationBase::setupDepthStencil() {
    depthFormat = VulkanBase::Tools::findSupportDepthFormat(vulkanDevice->physicalDevice);
    VkImageCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.pNext = nullptr;
    createInfo.format = depthFormat;
    createInfo.extent = {width, height, 1};
    createInfo.mipLevels = 1;
    createInfo.arrayLayers = 1;
    createInfo.samples = vulkanDevice->msaaSamples;
    createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    createInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VK_CHECK_RESULT(vkCreateImage(vulkanDevice->logicalDevice, &createInfo, nullptr, &depthStencil.image));

    VkMemoryRequirements memoryRequirements{};
    vkGetImageMemoryRequirements(vulkanDevice->logicalDevice, depthStencil.image, &memoryRequirements);

    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = memoryRequirements.size;
    allocateInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK_RESULT(vkAllocateMemory(vulkanDevice->logicalDevice, &allocateInfo, nullptr, &depthStencil.memory));
    VK_CHECK_RESULT(vkBindImageMemory(vulkanDevice->logicalDevice, depthStencil.image, depthStencil.memory, 0));

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = depthStencil.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = depthFormat;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    VK_CHECK_RESULT(vkCreateImageView(vulkanDevice->logicalDevice, &viewInfo, nullptr, &depthStencil.imageView));
}

void VulkanApplicationBase::setupColorResources() {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = {width, height, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = swapchain->colorFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    imageInfo.samples = vulkanDevice->msaaSamples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_CHECK_RESULT(vkCreateImage(vulkanDevice->logicalDevice, &imageInfo, nullptr, &colorResources.image));

    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(vulkanDevice->logicalDevice, colorResources.image, &memoryRequirements);
    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = memoryRequirements.size;
    allocateInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK_RESULT(vkAllocateMemory(vulkanDevice->logicalDevice, &allocateInfo, nullptr, &colorResources.memory));
    VK_CHECK_RESULT(vkBindImageMemory(vulkanDevice->logicalDevice, colorResources.image, colorResources.memory, 0));

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = colorResources.image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = swapchain->colorFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VK_CHECK_RESULT(vkCreateImageView(vulkanDevice->logicalDevice, &viewInfo, nullptr, &colorResources.imageView));
}

void VulkanApplicationBase::createFrameBuffer() {
    std::array<VkImageView, 3> attachments = {};

    attachments[0] = colorResources.imageView;
    attachments[1] = depthStencil.imageView;

    VkFramebufferCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    createInfo.width = width;
    createInfo.height = height;
    createInfo.pNext = nullptr;
    createInfo.renderPass = renderPass;
    createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    createInfo.pAttachments = attachments.data();
    createInfo.layers = 1;

    frameBuffers.resize(swapchain->imageCount);
    for (uint32_t i = 0; i < frameBuffers.size(); i++) {
        attachments[2] = swapchain->swapchainBuffers[i].imageView;
        VK_CHECK_RESULT(vkCreateFramebuffer(vulkanDevice->logicalDevice, &createInfo, nullptr, &frameBuffers[i]));
    }
}

void VulkanApplicationBase::createRenderPass() {
    std::array<VkAttachmentDescription, 3> attachments = {};
    // Color attachment
    attachments[0].format = swapchain->colorFormat;
    attachments[0].samples = vulkanDevice->msaaSamples;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // Depth attachment
    attachments[1].format = depthFormat;
    attachments[1].samples = vulkanDevice->msaaSamples;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    // Resolve attachment
    attachments[2].format = swapchain->colorFormat;
    attachments[2].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[2].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Attachment Reference
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentResolveRef{};
    colorAttachmentResolveRef.attachment = 2;
    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // SubPass Description
    VkSubpassDescription description{};
    description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    description.colorAttachmentCount = 1;
    description.pColorAttachments = &colorAttachmentRef;
    description.pDepthStencilAttachment = &depthAttachmentRef;
    description.pResolveAttachments = &colorAttachmentResolveRef;

    // SubPass Dependency
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    // Render Pass
    VkRenderPassCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    createInfo.pAttachments = attachments.data();
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &description;
    createInfo.dependencyCount = 1;
    createInfo.pDependencies = &dependency;
    VK_CHECK_RESULT(vkCreateRenderPass(vulkanDevice->logicalDevice, &createInfo, nullptr, &renderPass));
}

void VulkanApplicationBase::prepareFrame() {
    VkResult result = swapchain->acquireNextImage(semaphores.presentCompleteSemaphore, &currentBuffer);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        windowResize();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swapchain image!");
    }
}

void VulkanApplicationBase::submitFrame() {
    VkResult result = swapchain->queuePresent(vulkanDevice->presentQueue, currentBuffer, semaphores.renderCompleteSemaphore);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        windowResize();
        return;
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swapchain images!");
    }
    vkQueueWaitIdle(vulkanDevice->presentQueue);
}

int VulkanApplicationBase::getDeviceScore(const VkPhysicalDevice physicalDevice) {
    int score = 1;
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    std::cout << "New Physical Device Found : " << properties.deviceName << std::endl;
    if (properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        std::cout << properties.deviceName << "is not drawing GPUs." << std::endl;
        return -1;
    }
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(physicalDevice, &features);
    if (!features.samplerAnisotropy) {
        std::cout << properties.deviceName << "doesn't support anisotropy filtering." << std::endl;
        return -1;
    }
    return score;
}

VkPhysicalDevice VulkanApplicationBase::pickPhysicalDevice() {
    VkPhysicalDevice physicalDevice;
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0)
        throw std::runtime_error("failed to find GPU's with Vulkan support!");

    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());
    std::multimap<int, VkPhysicalDevice> candidates;
    for (const auto &candidate: physicalDevices) {
        int score = getDeviceScore(candidate);
        candidates.insert(std::make_pair(score, candidate));
    }
    if (candidates.rbegin()->first > 0) {
        physicalDevice = candidates.rbegin()->second;
    } else {
        throw std::runtime_error("failed to find a suitable GPU!");
    }

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    std::cout << " Device: " << deviceProperties.deviceName << std::endl;
    std::cout << " Type: " << VulkanBase::Tools::physicalDeviceTypeString(deviceProperties.deviceType) << std::endl;
    std::cout << " API: " << (deviceProperties.apiVersion >> 22) << "." << ((deviceProperties.apiVersion >> 12) & 0x3ff) << "." << (deviceProperties.apiVersion & 0xfff) << std::endl;

    return physicalDevice;
}

std::vector<const char *> VulkanApplicationBase::getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    return extensions;
}

void VulkanApplicationBase::createInstance() {
    requiredExtensions = getRequiredExtensions();

    if (enableValidation) {
        requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char *layerName: validationLayers) {
            bool is_found = false;
            for (const auto &layerProperty: availableLayers) {
                if (0 == strcmp(layerName, layerProperty.layerName)) {
                    is_found = true;
                    break;
                }
            }
            if (!is_found) {
                std::string message(layerName);
                throw std::runtime_error("validation layer : " + message + "requested but not available!");
            }
        }
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = nullptr;
    appInfo.pApplicationName = "VulkanRenderer";
    appInfo.pEngineName = "Richelieu";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();
    if (enableValidation) {
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
        debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugCreateInfo.pfnUserCallback = VulkanDebugMessage;
        debugCreateInfo.pUserData = nullptr;
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = nullptr;
        createInfo.pNext = nullptr;
    }

    VK_CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &instance));

    if (enableValidation) {
        VkDebugUtilsMessengerCreateInfoEXT createInfoExt = {};
        createInfoExt.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfoExt.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
        createInfoExt.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfoExt.pfnUserCallback = VulkanDebugMessage;
        createInfoExt.pUserData = nullptr;
        if (createDebugUtilsMessengerExt(instance, &createInfoExt, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create surface!");
    }
}

void VulkanApplicationBase::renderLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
    }
    vkDeviceWaitIdle(vulkanDevice->logicalDevice);
}

void VulkanApplicationBase::drawFrame() {}

void VulkanApplicationBase::createCommandBuffers() {
    drawCommandBuffers.resize(swapchain->imageCount);
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = cmdPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = static_cast<uint32_t>(drawCommandBuffers.size());
    VK_CHECK_RESULT(vkAllocateCommandBuffers(vulkanDevice->logicalDevice, &allocateInfo, drawCommandBuffers.data()));
}

void VulkanApplicationBase::createSyncPrimitives() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VK_CHECK_RESULT(vkCreateSemaphore(vulkanDevice->logicalDevice, &semaphoreInfo, nullptr, &semaphores.renderCompleteSemaphore));
    VK_CHECK_RESULT(vkCreateSemaphore(vulkanDevice->logicalDevice, &semaphoreInfo, nullptr, &semaphores.presentCompleteSemaphore));

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    inFlightFences.resize(drawCommandBuffers.size());
    for (auto& fence : inFlightFences) {
        VK_CHECK_RESULT(vkCreateFence(vulkanDevice->logicalDevice, &fenceInfo, nullptr, &fence));
    }
    submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pWaitDstStageMask = &waitStages;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &semaphores.presentCompleteSemaphore;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &semaphores.renderCompleteSemaphore;
}

void VulkanApplicationBase::createCommandPool() {
    VkCommandPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.queueFamilyIndex = swapchain->queueIndices.graphicsIdx;
    createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK_RESULT(vkCreateCommandPool(vulkanDevice->logicalDevice, &createInfo, nullptr, &cmdPool));
}

void VulkanApplicationBase::windowResize() {
    int destWidth = 0;
    int destHeight = 0;
    glfwGetFramebufferSize(window, &destWidth, &destHeight);
    while (destWidth == 0 || destHeight == 0) {
        glfwGetFramebufferSize(window, &destWidth, &destHeight);
        glfwWaitEvents();
    }
    vkDeviceWaitIdle(vulkanDevice->logicalDevice);

    width = destWidth;
    height = destHeight;
    swapchain->createSwapchain(&width, &height);

    vkDestroyImageView(vulkanDevice->logicalDevice, depthStencil.imageView, nullptr);
    vkDestroyImage(vulkanDevice->logicalDevice, depthStencil.image, nullptr);
    vkFreeMemory(vulkanDevice->logicalDevice, depthStencil.memory, nullptr);
    setupDepthStencil();
    for (auto& frameBuffer : frameBuffers) {
        vkDestroyFramebuffer(vulkanDevice->logicalDevice, frameBuffer, nullptr);
    }
    createFrameBuffer();
    vkFreeCommandBuffers(vulkanDevice->logicalDevice, cmdPool, static_cast<uint32_t>(drawCommandBuffers.size()), drawCommandBuffers.data());
    createCommandBuffers();
    buildCommandBuffers();

    for (auto& fence : inFlightFences) {
        vkDestroyFence(vulkanDevice->logicalDevice, fence, nullptr);
    }
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    inFlightFences.resize(drawCommandBuffers.size());
    for (auto& fence : inFlightFences) {
        VK_CHECK_RESULT(vkCreateFence(vulkanDevice->logicalDevice, &fenceInfo, nullptr, &fence));
    }

    vkDeviceWaitIdle(vulkanDevice->logicalDevice);
}

void VulkanApplicationBase::buildCommandBuffers() {}

VkResult VulkanApplicationBase::createDebugUtilsMessengerExt(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                                             const VkAllocationCallbacks *pAllocator,
                                                             VkDebugUtilsMessengerEXT *pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void VulkanApplicationBase::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                                const VkAllocationCallbacks *pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,
                                                                            "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

VulkanApplicationBase::~VulkanApplicationBase() {
    delete(swapchain);
    vkFreeCommandBuffers(vulkanDevice->logicalDevice, cmdPool, static_cast<uint32_t>(drawCommandBuffers.size()), drawCommandBuffers.data());
    vkDestroyRenderPass(vulkanDevice->logicalDevice, renderPass, nullptr);
    for (auto& frameBuffer : frameBuffers) {
        vkDestroyFramebuffer(vulkanDevice->logicalDevice, frameBuffer, nullptr);
    }
    for (auto& shaderModule : shaderModules) {
        vkDestroyShaderModule(vulkanDevice->logicalDevice, shaderModule, nullptr);
    }
    vkDestroyImageView(vulkanDevice->logicalDevice, colorResources.imageView, nullptr);
    vkDestroyImage(vulkanDevice->logicalDevice, colorResources.image, nullptr);
    vkFreeMemory(vulkanDevice->logicalDevice, colorResources.memory, nullptr);
    vkDestroyImageView(vulkanDevice->logicalDevice, depthStencil.imageView, nullptr);
    vkDestroyImage(vulkanDevice->logicalDevice, depthStencil.image, nullptr);
    vkFreeMemory(vulkanDevice->logicalDevice, depthStencil.memory, nullptr);

    vkDestroyPipelineCache(vulkanDevice->logicalDevice, pipelineCache, nullptr);

    vkDestroyCommandPool(vulkanDevice->logicalDevice, cmdPool, nullptr);
    vkDestroySemaphore(vulkanDevice->logicalDevice, semaphores.renderCompleteSemaphore, nullptr);
    vkDestroySemaphore(vulkanDevice->logicalDevice, semaphores.presentCompleteSemaphore, nullptr);
    for (auto& fence : inFlightFences) vkDestroyFence(vulkanDevice->logicalDevice, fence, nullptr);
    delete(vulkanDevice);
    if (enableValidation) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();
}

void VulkanApplicationBase::createPipelineCache() {
    VkPipelineCacheCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    VK_CHECK_RESULT(vkCreatePipelineCache(vulkanDevice->logicalDevice, &createInfo, nullptr, &pipelineCache));
}

VkPipelineShaderStageCreateInfo VulkanApplicationBase::createShader(const std::string& filePath, VkShaderStageFlagBits stages) {
    VkPipelineShaderStageCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    createInfo.stage = stages;
    createInfo.module = VulkanBase::Tools::loadShader(filePath, vulkanDevice->logicalDevice);
    createInfo.pName = "main";
    shaderModules.push_back(createInfo.module);
    return createInfo;
}
