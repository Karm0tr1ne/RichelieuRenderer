#include "VulkanRayTracingApplicationBase.h"

#include <iostream>
#include <array>

VulkanBase::RayTracingScratchBuffer::RayTracingScratchBuffer(VulkanDevice &device, VkDeviceSize deviceSize) : device(device) {
    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = deviceSize;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    VK_CHECK_RESULT(vkCreateBuffer(device.logicalDevice, &bufferCreateInfo, nullptr, &buffer));

    VkMemoryRequirements memoryRequirements{};
    vkGetBufferMemoryRequirements(device.logicalDevice, buffer, &memoryRequirements);
    VkMemoryAllocateFlagsInfo flagsInfo{};
    flagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    flagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.memoryTypeIndex = device.getMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    allocateInfo.allocationSize = memoryRequirements.size;
    allocateInfo.pNext = &flagsInfo;
    VK_CHECK_RESULT(vkAllocateMemory(device.logicalDevice, &allocateInfo, nullptr, &memory));
    VK_CHECK_RESULT(vkBindBufferMemory(device.logicalDevice, buffer, memory, 0));

    VkBufferDeviceAddressInfoKHR addressInfo{};
    addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    addressInfo.buffer = buffer;
    address = vkGetBufferDeviceAddressKHR(device.logicalDevice, &addressInfo);
}

void VulkanBase::RayTracingScratchBuffer::cleanUp() {
    if (memory != VK_NULL_HANDLE) {
        vkFreeMemory(device.logicalDevice, memory, nullptr);
    }
    if (buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device.logicalDevice, buffer, nullptr);
    }
}

VulkanBase::AccelerationStructure::AccelerationStructure(VulkanDevice &device, VkAccelerationStructureTypeKHR type,
                                             VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo) : device(device) {
    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = buildSizesInfo.accelerationStructureSize;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    VK_CHECK_RESULT(vkCreateBuffer(device.logicalDevice, &bufferCreateInfo, nullptr, &buffer));

    VkMemoryRequirements memoryRequirements{};
    vkGetBufferMemoryRequirements(device.logicalDevice, buffer, &memoryRequirements);

    VkMemoryAllocateFlagsInfo allocateFlagsInfo{};
    allocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    allocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.pNext = &allocateFlagsInfo;
    allocateInfo.allocationSize = memoryRequirements.size;
    allocateInfo.memoryTypeIndex = device.getMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK_RESULT(vkAllocateMemory(device.logicalDevice, &allocateInfo, nullptr, &memory));
    VK_CHECK_RESULT(vkBindBufferMemory(device.logicalDevice, buffer, memory, 0));

    VkAccelerationStructureCreateInfoKHR structureCreateInfo{};
    structureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    structureCreateInfo.buffer = buffer;
    structureCreateInfo.size = buildSizesInfo.accelerationStructureSize;
    structureCreateInfo.type = type;
    VK_CHECK_RESULT(vkCreateAccelerationStructureKHR(device.logicalDevice, &structureCreateInfo, nullptr, &handle));

    VkAccelerationStructureDeviceAddressInfoKHR deviceAddressInfo{};
    deviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    deviceAddressInfo.accelerationStructure = handle;
    address = vkGetAccelerationStructureDeviceAddressKHR(device.logicalDevice, &deviceAddressInfo);
}

void VulkanBase::AccelerationStructure::cleanUp() {
    vkFreeMemory(device.logicalDevice, memory, nullptr);
    vkDestroyBuffer(device.logicalDevice, buffer, nullptr);
    vkDestroyAccelerationStructureKHR(device.logicalDevice, handle, nullptr);
}

void VulkanBase::VulkanRayTracingApplicationBase::createShaderBindingTable(VulkanBase::ShaderBindingTable &bindingTable,
                                                                           uint32_t handleCount) {
    vulkanDevice->createBuffer(VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &bindingTable,
            rayTracingPipelineProperties.shaderGroupHandleSize * handleCount);
    bindingTable.stridedDeviceAddressRegion = getSbtEntryStridedDeviceAddressRegion(bindingTable.buffer, handleCount);
    bindingTable.map();
}

void VulkanBase::VulkanRayTracingApplicationBase::enableExtensions() {
    apiVersion = VK_API_VERSION_1_2;
    deviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
    if (!rayQueryOnly) {
        deviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
    }
    // VK_KHR_ACCELERATION_STRUCTURE
    deviceExtensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
    deviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
    deviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
    // VK_KHR_RAY_TRACING_PIPELINE
    deviceExtensions.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
    // VK_KHR_SPIRV_1_4
    deviceExtensions.push_back(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
}

void VulkanBase::VulkanRayTracingApplicationBase::createStorageImage(VkFormat format, VkExtent3D extent) {
    if (storageImage.image != VK_NULL_HANDLE) {
        vkDestroyImageView(vulkanDevice->logicalDevice, storageImage.view, nullptr);
        vkDestroyImage(vulkanDevice->logicalDevice, storageImage.image, nullptr);
        vkFreeMemory(vulkanDevice->logicalDevice, storageImage.memory, nullptr);
        storageImage = {};
    }
    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = format;
    imageCreateInfo.extent = extent;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VK_CHECK_RESULT(vkCreateImage(vulkanDevice->logicalDevice, &imageCreateInfo, nullptr, &storageImage.image));

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(vulkanDevice->logicalDevice, storageImage.image, &memReqs);
    VkMemoryAllocateInfo memoryAllocateInfo{};
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.allocationSize = memReqs.size;
    memoryAllocateInfo.memoryTypeIndex = vulkanDevice->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK_RESULT(vkAllocateMemory(vulkanDevice->logicalDevice, &memoryAllocateInfo, nullptr, &storageImage.memory));
    VK_CHECK_RESULT(vkBindImageMemory(vulkanDevice->logicalDevice, storageImage.image, storageImage.memory, 0));

    VkImageViewCreateInfo colorImageView{};
    colorImageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
    colorImageView.format = format;
    colorImageView.subresourceRange = {};
    colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    colorImageView.subresourceRange.baseMipLevel = 0;
    colorImageView.subresourceRange.levelCount = 1;
    colorImageView.subresourceRange.baseArrayLayer = 0;
    colorImageView.subresourceRange.layerCount = 1;
    colorImageView.image = storageImage.image;
    VK_CHECK_RESULT(vkCreateImageView(vulkanDevice->logicalDevice, &colorImageView, nullptr, &storageImage.view));

    VkCommandBuffer cmdBuffer = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
    VulkanBase::Tools::setImageLayout(cmdBuffer, storageImage.image,
                               VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_GENERAL,
                               { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
    vulkanDevice->flushCommandBuffer(cmdBuffer, vulkanDevice->graphicsQueue);
}

void VulkanBase::VulkanRayTracingApplicationBase::cleanUpStorageImage() {
    vkDestroyImageView(vulkanDevice->logicalDevice, storageImage.view, nullptr);
    vkDestroyImage(vulkanDevice->logicalDevice, storageImage.image, nullptr);
    vkFreeMemory(vulkanDevice->logicalDevice, storageImage.memory, nullptr);
}

uint64_t VulkanBase::VulkanRayTracingApplicationBase::getBufferDeviceAddress(VkBuffer buffer) {
    VkBufferDeviceAddressInfoKHR addressInfo{};
    addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    addressInfo.buffer = buffer;
    return vkGetBufferDeviceAddressKHR(vulkanDevice->logicalDevice, &addressInfo);
}

VkStridedDeviceAddressRegionKHR VulkanBase::VulkanRayTracingApplicationBase::getSbtEntryStridedDeviceAddressRegion(VkBuffer buffer,
                                                                                   uint32_t handleCount) {
    const uint32_t handledSizeAligned = VulkanBase::Tools::alignedSize(rayTracingPipelineProperties.shaderGroupHandleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);
    VkStridedDeviceAddressRegionKHR addressRegion{};
    addressRegion.deviceAddress = getBufferDeviceAddress(buffer);
    addressRegion.size = handleCount * handledSizeAligned;
    addressRegion.stride = handledSizeAligned;
    return addressRegion;
}

void VulkanBase::VulkanRayTracingApplicationBase::prepare() {
    VulkanApplicationBase::prepare();
    rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
    VkPhysicalDeviceProperties2 deviceProperties2{};
    deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    deviceProperties2.pNext = &rayTracingPipelineProperties;
    vkGetPhysicalDeviceProperties2(vulkanDevice->physicalDevice, &deviceProperties2);

    accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    VkPhysicalDeviceFeatures2 deviceFeatures2{};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.pNext = &accelerationStructureFeatures;
    vkGetPhysicalDeviceFeatures2(vulkanDevice->physicalDevice, &deviceFeatures2);

    vkGetBufferDeviceAddress = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(vulkanDevice->logicalDevice, "vkGetBufferDeviceAddressKHR"));
    vkCmdBuildAccelerationStructures = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(vulkanDevice->logicalDevice, "vkCmdBuildAccelerationStructuresKHR"));
    vkBuildAccelerationStructures = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(vulkanDevice->logicalDevice, "vkBuildAccelerationStructuresKHR"));
    vkCreateAccelerationStructure = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(vulkanDevice->logicalDevice, "vkCreateAccelerationStructureKHR"));
    vkDestroyAccelerationStructure = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(vulkanDevice->logicalDevice, "vkDestroyAccelerationStructureKHR"));
    vkGetAccelerationStructureBuildSizes = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(vulkanDevice->logicalDevice, "vkGetAccelerationStructureBuildSizesKHR"));
    vkCmdTraceRays = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(vulkanDevice->logicalDevice, "vkCmdTraceRaysKHR"));
    vkGetRayTracingShaderGroupHandles = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(vulkanDevice->logicalDevice, "vkGetRayTracingShaderGroupHandlesKHR"));
    vkCreateRayTracingPipelines = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(vulkanDevice->logicalDevice, "vkCreateRayTracingPipelinesKHR"));
}

void VulkanBase::VulkanRayTracingApplicationBase::setupRenderPass() {
    vkDestroyRenderPass(vulkanDevice->logicalDevice, renderPass, nullptr);

    std::array<VkAttachmentDescription, 2> attachments = {};
    attachments[0].format = swapchain->colorFormat;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = rayQueryOnly? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = rayQueryOnly? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    attachments[1].format = depthFormat;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorReference{};
    colorReference.attachment = 0;
    colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthReference{};
    depthReference.attachment = 1;
    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription{};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorReference;
    subpassDescription.pDepthStencilAttachment = &depthReference;
    subpassDescription.inputAttachmentCount = 0;
    subpassDescription.pInputAttachments = nullptr;
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments = nullptr;
    subpassDescription.pResolveAttachments = nullptr;

    std::array<VkSubpassDependency, 2> dependencies{};

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_NONE_KHR;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpassDescription;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();
    VK_CHECK_RESULT(vkCreateRenderPass(vulkanDevice->logicalDevice, &renderPassInfo, nullptr, &renderPass));
}

void VulkanBase::VulkanRayTracingApplicationBase::setupFrameBuffer() {
    VkImageView attachments[2];
    attachments[1] = depthStencil.imageView;

    VkFramebufferCreateInfo frameBufferCreateInfo{};
    frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCreateInfo.pNext = VK_NULL_HANDLE;
    frameBufferCreateInfo.renderPass = renderPass;
    frameBufferCreateInfo.attachmentCount = 2;
    frameBufferCreateInfo.pAttachments = attachments;
    frameBufferCreateInfo.width = width;
    frameBufferCreateInfo.height = height;
    frameBufferCreateInfo.layers = 1;

    frameBuffers.resize(swapchain->imageCount);
    for (uint32_t i = 0; i < frameBuffers.size(); i++)
    {
        attachments[0] = swapchain->swapchainBuffers[i].imageView;
        VK_CHECK_RESULT(vkCreateFramebuffer(vulkanDevice->logicalDevice, &frameBufferCreateInfo, nullptr, &frameBuffers[i]));
    }
}

