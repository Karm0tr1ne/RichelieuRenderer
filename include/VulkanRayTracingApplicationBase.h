#ifndef RICHELIEURENDERER_VULKANRAYTRACINGAPPLICATIONBASE_H
#define RICHELIEURENDERER_VULKANRAYTRACINGAPPLICATIONBASE_H

#include "VulkanApplicationBase.h"

namespace VulkanBase {
    struct AccelerationStructure {
        VkAccelerationStructureKHR handle;
        uint64_t address = 0;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkBuffer buffer = VK_NULL_HANDLE;
        VulkanDevice &device;

        AccelerationStructure(VulkanDevice &device, VkAccelerationStructureTypeKHR type,
                              VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo);
        void cleanUp();
    };

    struct RayTracingScratchBuffer {
        uint64_t address = 0;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkBuffer buffer = VK_NULL_HANDLE;
        VulkanDevice &device;

        RayTracingScratchBuffer(VulkanDevice &device, VkDeviceSize deviceSize);
        void cleanUp();
    };

    class ShaderBindingTable : public VulkanBase::VulkanBuffer {
    public:
        VkStridedDeviceAddressRegionKHR stridedDeviceAddressRegion{};
    };

    class VulkanRayTracingApplicationBase : public VulkanApplicationBase {
    public:
        VulkanBase::VulkanBuffer vertexBuffer;
        VulkanBase::VulkanBuffer indexBuffer;
        VulkanBase::VulkanBuffer ubo;
        uint32_t indexCount = 0;
        VulkanBase::VulkanBuffer transformBuffer;
        std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups{};
        VulkanBase::VulkanBuffer raygenShaderBindingTable;
        VulkanBase::VulkanBuffer missShaderBindingTable;
        VulkanBase::VulkanBuffer hitShaderBindingTable;

        AccelerationStructure bottomLevelAS;
        AccelerationStructure topLevelAS;

        PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddress;
        PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructure;
        PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructure;
        PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizes;
        PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddress;
        PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructures;
        PFN_vkBuildAccelerationStructuresKHR vkBuildAccelerationStructures;
        PFN_vkCmdTraceRaysKHR vkCmdTraceRays;
        PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandles;
        PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelines;

        VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties{};
        VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
        VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{};
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR enabledRayTracingPipelineFeatures{};
        VkPhysicalDeviceAccelerationStructureFeaturesKHR enabledAccelerationStructureFeatures{};

        struct StorageImage {
            VkDeviceMemory memory = VK_NULL_HANDLE;
            VkImage image = VK_NULL_HANDLE;
            VkImageView view = VK_NULL_HANDLE;
            VkFormat format;
        } storageImage;

        bool rayQueryOnly = false;

        void enableExtensions();
        uint64_t getBufferDeviceAddress(VkBuffer buffer);
        void createStorageImage(VkFormat format, VkExtent3D extent);
        void cleanUpStorageImage();
        void createShaderBindingTable(ShaderBindingTable& bindingTable, uint32_t handleCount);
        VkStridedDeviceAddressRegionKHR getSbtEntryStridedDeviceAddressRegion(VkBuffer buffer, uint32_t handleCount);
        virtual void prepare();

    protected:
        virtual void setupRenderPass();
        virtual void setupFrameBuffer();
    };
}
#endif
